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
 * Sets an io channel to non-blocking and
 * prints error properly.
 */
#define SET_NONBLOCKING(ch,error) \
	G_STMT_START { \
	  g_io_channel_set_flags (ch, G_IO_FLAG_NONBLOCK, &error); \
	  if (error) { \
	    g_critical ("Could not set NONBLOCK flag: %s", \
			error->message); \
	    g_clear_error (&error); \
	  } \
	} G_STMT_END

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

#if 0
/*
 * Returns: %TRUE if there was output in @channel, %FALSE otherwise
 */
static gboolean
flush_channel (GIOChannel *channel)
{
	char c;
	gboolean had_output = FALSE;
	GIOStatus status;

	do {
		status = g_io_channel_read_chars (channel, &c, 1, NULL, NULL);
		if (status == G_IO_STATUS_NORMAL)
			had_output = TRUE;
	} while (status == G_IO_STATUS_NORMAL);

	return had_output;
}
#endif

GebrExpr *
gebr_expr_new (void)
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
		g_critical ("Could not run 'bc': %s",
			    error->message);
		g_clear_error (&error);
		return NULL;
	}

	self = g_new (GebrExpr, 1);
	self->in_ch = g_io_channel_unix_new (in_fd);
	self->out_ch = g_io_channel_unix_new (out_fd);
	self->err_ch = g_io_channel_unix_new (err_fd);

	SET_NONBLOCKING (self->in_ch, error);
	SET_NONBLOCKING (self->out_ch, error);
	SET_NONBLOCKING (self->err_ch, error);

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
		status = g_io_channel_read_line (self->err_ch, &line, NULL, NULL, &error);
		if (status == G_IO_STATUS_NORMAL) {
			// g_set_error (line);
			g_free (line);
			return FALSE;
		}

		switch (state)
		{
		case READ_RESULT:
			status = g_io_channel_read_line (self->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0 (line, EVAL_COOKIE) == 0) {
					// Expression does not return a value
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
	g_hash_table_unref (self->vars);
	g_io_channel_unref (self->in_ch);
	g_io_channel_unref (self->out_ch);
	g_io_channel_unref (self->err_ch);
	g_free (self);
}
