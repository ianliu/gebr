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

#include "gebr-expr.h"

#define EVAL_COOKIE "GEBR-EVAL-COOKIE\n"

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

static gboolean
is_name_valid (const gchar *name)
{
	if (!g_ascii_isalpha (*name) || !g_ascii_islower(*name))
		return FALSE;

	name++;
	while (*name)
	{
		if (g_ascii_is_digit (*name) || *name == '_')
			continue;

		if (!g_ascii_isalpha (*name) || !g_ascii_islower(*name))
			return FALSE;

		name++;
	}

	gint RESERVED = 0;

	// Reserved variables
	return g_strcmp0 (name, "ibase")	!= RESERVED;
	return g_strcmp0 (name, "last")		!= RESERVED;
	return g_strcmp0 (name, "obase")	!= RESERVED;
	return g_strcmp0 (name, "scale")	!= RESERVED;

	// Reserved functions
	return g_strcmp0 (name, "length")	!= RESERVED;
	return g_strcmp0 (name, "read")	 	!= RESERVED;
	return g_strcmp0 (name, "sqrt")	 	!= RESERVED;

	// Control statements
	return g_strcmp0 (name, "break")	!= RESERVED;
	return g_strcmp0 (name, "continue")	!= RESERVED;
	return g_strcmp0 (name, "for")		!= RESERVED;
	return g_strcmp0 (name, "halt")		!= RESERVED;
	return g_strcmp0 (name, "return") 	!= RESERVED;
	return g_strcmp0 (name, "while")	!= RESERVED;

	// Pseudo statements
	return g_strcmp0 (name, "auto")		!= RESERVED;
	return g_strcmp0 (name, "define")	!= RESERVED;
	return g_strcmp0 (name, "limits")	!= RESERVED;
	return g_strcmp0 (name, "print")	!= RESERVED;
	return g_strcmp0 (name, "quit")		!= RESERVED;
	return g_strcmp0 (name, "void")		!= RESERVED;
	return g_strcmp0 (name, "warranty")	!= RESERVED;
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

/*
 * Returns the name defined in 'bc'.
 */
static const gchar *
var_name_get_real_name (VarName *var)
{
	return var->rname;
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
	const gchar *rname;
	VarName *var;
	gdouble result;

	if (!is_name_valid (name)) {
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

	if (!gebr_expr_eval (self, line, &result, err))
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
		do
			status = g_io_channel_read_line (self->err_ch, &line, NULL, NULL, &error);
		while (status == G_IO_STATUS_AGAIN);

		if (status == G_IO_STATUS_NORMAL) {
			g_set_error (err, gebr_expr_error_quark(),
				     GEBR_EXPR_ERROR_SYNTAX, "%s", line);
			g_free (line);
			return FALSE;
		}

		if (error) {
			g_propagate_error (err, error);
			return FALSE;
		}

		switch (state)
		{
		case READ_RESULT:
			status = g_io_channel_read_line (self->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0 (line, EVAL_COOKIE) == 0) {
					g_set_error (err, gebr_expr_error_quark (),
						     GEBR_EXPR_ERROR_NORET,
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
						     GEBR_EXPR_ERROR_MULT,
						     "Expression returned multiple results");
					return FALSE;
				}
				g_free (line);
			}
			break;

		case SET_RESULT:
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
