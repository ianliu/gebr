/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "gebr-expr.h"

#define EVAL_COOKIE "GEBR-EVAL-COOKIE\n"

static gboolean gebr_expr_eval_internal (GebrExpr *self, const gchar *expr, gdouble *result, GError **err);

/*
 * configure_channel:
 * @self:
 * @channel:
 * @err:
 *
 * Set @channel to non-blocking and @err if an error occured.
 *
 * Returns: %TRUE if no error ocurred
 */
static gboolean
configure_channel (GebrExpr *self, GIOChannel *channel, GError **err)
{
	GError *error = NULL;

	g_io_channel_set_close_on_unref (channel, TRUE);
	g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &error);

	if (error) {
		g_propagate_error (err, error);
		return FALSE;
	}

	return TRUE;
}

/*
 * @vars: The hash table holding VarName -> Value
 * @in_ch: Input channel for sending messages to 'bc'
 * @out_ch: Output channel for receiving messages from 'bc'
 * @out_ch: Error channel for receiving errors from 'bc'
 */
struct _GebrExpr
{
	GHashTable *vars;
	GIOChannel *in_ch;
	GIOChannel *out_ch;
	GIOChannel *err_ch;
};

/*
 * The VarName structure is the 'key' for the hash table of
 * variables. Its purpose is to maintain a unique id for that
 * variable, enabling us to set any name we want.
 */
typedef struct {
	gint id;
	gchar *name;
	gchar *rname;
} VarName;

GQuark gebr_expr_error_quark (void)
{
	return g_quark_from_static_string ("gebr-expr-error-quark");
}


gboolean
gebr_expr_is_reserved_word (const gchar *name)
{
	gint i = 0;
	const gchar *reserved_words[] = {
		// Reserved variables
		"ibase",
		"last",
		"obase",
		"scale",
		// Reserved function
		"length",
		"read", 
		"sqrt", 
		// Control statements
		"break",
		"continue",
		"else",
		"for",
		"halt",
		"if",
		"return",
		"while",
		// Pseudo statements
		"auto",
		"define",
		"limits",
		"print",
		"quit",
		"void",
		"warranty",
		NULL
	};

	while (reserved_words[i])
		if (g_strcmp0(name, reserved_words[i++]) == 0)
			return TRUE;

	return FALSE;
}

gboolean
gebr_expr_is_name_valid (const gchar *name)
{
	gsize i = 0;

	if (!g_ascii_isalpha (name[i]) || !g_ascii_islower(name[i]))
		return FALSE;

	while (name[++i])
	{
		if (g_ascii_isdigit (name[i]) || name[i] == '_')
			continue;

		if (!g_ascii_isalpha (name[i]) || !g_ascii_islower(name[i]))
			return FALSE;
	}
	return !gebr_expr_is_reserved_word (name);
}

static VarName *
var_name_new (const gchar *name)
{
	static guint i = 0;

	VarName *var = g_new (VarName, 1);
	var->id = i++;
	var->name = g_strdup (name);
	var->rname = g_strdup_printf ("var%d", var->id);
	return var;
}

static guint
var_name_hash (gconstpointer key)
{
	const VarName *var = key;
	return g_str_hash (var->name);
}

static gboolean
var_name_equal (gconstpointer p1,
		gconstpointer p2)
{
	const VarName *var1 = p1;
	const VarName *var2 = p2;
	return g_str_equal (var1->name, var2->name);
}

static void
var_name_free (gpointer data)
{
	VarName *var = data;
	g_free (var->name);
	g_free (var->rname);
	g_free (var);
}

GebrExpr *
gebr_expr_new (GError **err)
{
	const gchar *home_dir	= g_get_home_dir ();
	gchar *argv[]		= {"bc", "-l", NULL};
	GError *error		= NULL;
	GebrExpr *self		= NULL;
	gint in_fd, out_fd, err_fd;

	g_spawn_async_with_pipes (home_dir, argv, NULL,
				  G_SPAWN_SEARCH_PATH,
				  NULL, NULL, NULL,
				  &in_fd, &out_fd, &err_fd,
				  &error);

	if (error) {
		g_propagate_error (err, error);
		return NULL;
	}

	self = g_new (GebrExpr, 1);
	self->in_ch = g_io_channel_unix_new (in_fd);
	self->out_ch = g_io_channel_unix_new (out_fd);
	self->err_ch = g_io_channel_unix_new (err_fd);

	if (!configure_channel (self, self->in_ch, err) ||
	    !configure_channel (self, self->out_ch, err) ||
	    !configure_channel (self, self->err_ch, err))
	{
		g_io_channel_unref (self->in_ch);
		g_io_channel_unref (self->out_ch);
		g_io_channel_unref (self->err_ch);
		g_free (self);
		return NULL;
	}

	self->vars = g_hash_table_new_full (var_name_hash,
					    var_name_equal,
					    var_name_free,
					    g_free);

	return self;
}

gboolean
gebr_expr_set_var (GebrExpr *self,
		   const gchar *name,
		   const gchar *value,
		   GError **err)
{
	gchar *line;
	VarName *var;
	gdouble result;

	if (!gebr_expr_is_name_valid (name)) {
		g_set_error (err, gebr_expr_error_quark (),
			     GEBR_EXPR_ERROR_INVALID_NAME,
			     "Invalid variable name");
		return FALSE;
	}

	/* Insert the striped value into the hash table */
	var = var_name_new (name);
	g_hash_table_insert (self->vars, var, g_strdup (value));

	// FIXME: validate variable names?
	//rname = var_name_get_real_name (var);
	line = g_strdup_printf ("%s=%s ; 0", name, value);

	if (!gebr_expr_eval_internal (self, line, &result, err))
		return FALSE;

	if (result != 0)
		return FALSE;

	return TRUE;
}

gboolean
gebr_expr_eval (GebrExpr *self,
		const gchar *expr,
		gdouble *result,
		GError **err)
{

	if(strchr(expr, '=')){
			g_set_error (err, gebr_expr_error_quark (),
									     GEBR_EXPR_ERROR_INVALID_ASSIGNMENT,
									     "Expression might not have an assignment ('=')");
								return FALSE;
		}
	return gebr_expr_eval_internal(self, expr, result, err);
}

static gboolean
gebr_expr_eval_internal (GebrExpr *self,
		const gchar *expr,
		gdouble *result,
		GError **err)
{
	gchar *line;
	GError *error = NULL;
	GIOStatus status;

	line = g_strdup_printf ("%s ; \"%s\"\n", expr, EVAL_COOKIE);
	status = g_io_channel_write_chars (self->in_ch, line, -1, NULL, &error);
	g_free (line);

	if (error)
	{
		g_propagate_error (err, error);
		return FALSE;
	}

	g_io_channel_flush (self->in_ch, &error);
	if (error)
	{
		g_propagate_error (err, error);
		return FALSE;
	}

#define READ_RESULT 0
#define READ_STAMP 1
#define ERROR 2
#define SET_RESULT 3
#define FINISHED 4

	gint state = READ_RESULT;
	gchar *result_str = NULL;
	while (state != FINISHED)
	{
		status = g_io_channel_read_line (self->err_ch, &line, NULL, NULL, &error);

		if (status == G_IO_STATUS_NORMAL) {
			g_set_error (err, gebr_expr_error_quark(),
				     GEBR_EXPR_ERROR_SYNTAX, "%s", line);
			g_free (line);
			return FALSE;
		}

		switch (state)
		{
		case READ_RESULT:
			status = g_io_channel_read_line (self->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0 (line, EVAL_COOKIE) == 0) {
					g_set_error (err, gebr_expr_error_quark (),
						     GEBR_EXPR_ERROR_SYNTAX,
						     "Expression does not return a value");
					g_free (line);
					return FALSE;
				} else {
					result_str = g_strdup (line);
					state = READ_STAMP;
				}
				g_free (line);
			}
			break;
		case READ_STAMP:
			status = g_io_channel_read_line (self->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0 (line, EVAL_COOKIE) == 0) {
					state = SET_RESULT;
				} else {
					state = ERROR;
				}
				g_free (line);
			}
			break;
		case ERROR:
			// TODO: Flush output and error
			status = g_io_channel_read_line (self->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0 (line, EVAL_COOKIE) == 0) {
					g_free (result_str);
					g_free (line);
					g_set_error (err, gebr_expr_error_quark (),
						     GEBR_EXPR_ERROR_SYNTAX,
						     "Expression returned multiple results");
					return FALSE;
				}
				g_free (line);
			}
			break;

		case SET_RESULT:
			if (result != NULL)
				*result = g_ascii_strtod (result_str, NULL);
			g_free (result_str);
			state = FINISHED;
			break;
		}
	}

	return TRUE;
}

void
gebr_expr_free (GebrExpr *self)
{
	GIOStatus status;
	do
		status = g_io_channel_write_chars (self->in_ch, "quit\n", -1, NULL, NULL);
	while (status == G_IO_STATUS_AGAIN);

	g_hash_table_unref (self->vars);
	g_io_channel_unref (self->in_ch);
	g_io_channel_unref (self->out_ch);
	g_io_channel_unref (self->err_ch);
	g_free (self);
}

GList *
gebr_expr_extract_vars (const gchar *e)
{
	GList *vars = NULL;
	GRegex *regex;
	GMatchInfo *info;

	regex = g_regex_new ("[a-z][0-9a-z_]*", 0, 0, NULL);
	g_regex_match (regex, e, 0, &info);

	while (g_match_info_matches (info))
	{
		gchar *word = g_match_info_fetch (info, 0);

		vars = g_list_prepend (vars, word);

		g_match_info_next (info, NULL);
	}

	g_match_info_free (info);
	g_regex_unref (regex);

	return g_list_reverse (vars);
}

GebrExprError gebr_str_expr_extract_vars (const gchar *str, GList ** vars)
{
	enum {
		STATE_STARTING = 0,
		STATE_CONSUMING,
		STATE_OPEN_BRACKET, 
		STATE_CLOSE_BRACKET, 
		STATE_READING_VAR,
		STATE_MATCHED,
		STATE_FINISHED,
	} status;

	gchar * word = NULL;
	status  = STATE_STARTING;
	gint i = 0;
	gint j = 0;
	GebrExprError retval = GEBR_EXPR_ERROR_NONE;

	*vars = NULL;
	word = g_new(gchar, strlen(str));

	while (status != STATE_FINISHED){
		if (str[i] || status == STATE_MATCHED){
			switch (status)
			{
			case STATE_STARTING:
				if (str[i] == '[')
					status = STATE_OPEN_BRACKET;
				else if (str[i] == ']'){
					retval = GEBR_EXPR_ERROR_SYNTAX;
					status = STATE_FINISHED;
				}
				i++;
				break;

			case STATE_OPEN_BRACKET:
					if (str[i] == '['){
						status = STATE_CONSUMING;
						i++;
					}
					else{
						status = STATE_READING_VAR;
						j = 0;
					}
				break;

			case STATE_CONSUMING:
				if (str[i] == '[')
					status = STATE_OPEN_BRACKET;
				else if (str[i] == ']')
					status = STATE_CLOSE_BRACKET;
				i++;
				break;


			case STATE_CLOSE_BRACKET:
					if (str[i] == ']'){
						if (str[i])
							status = STATE_STARTING;
						i++;
					}
					else{
						retval = GEBR_EXPR_ERROR_SYNTAX;
						status = STATE_FINISHED;
					}
				break;

			case STATE_READING_VAR:
				if (str[i] == ']'){
					word[j] = '\0';
					status = STATE_MATCHED;
				}
				else if (str[i] == '['){
					retval = GEBR_EXPR_ERROR_SYNTAX;
					status = STATE_FINISHED;
				}
				else
					word[j++] = str[i++];
				break;

			case STATE_MATCHED:
				if (strlen(word)){
					*vars = g_list_prepend (*vars, g_strdup(word));

					status = STATE_STARTING;
					i++;
				}
				else{
					retval = GEBR_EXPR_ERROR_EMPTY_VAR;
					status = STATE_FINISHED;
				}
				break;
			default:
				retval = GEBR_EXPR_ERROR_STATE_UNKNOWN;
				status = STATE_FINISHED;
				break;
			}
		}
		else{
			if (status == STATE_CONSUMING
			    || status == STATE_OPEN_BRACKET
			    || status == STATE_CLOSE_BRACKET
			    || status == STATE_READING_VAR)
				retval = GEBR_EXPR_ERROR_SYNTAX;

			status = STATE_FINISHED;
		}
	}

	g_free(word);
	*vars = g_list_reverse (*vars);
	return retval;
}
