/*   libgebr - GeBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <glib/gi18n.h>

#include "gebr-arith-expr.h"
#include "gebr-iexpr.h"

#define EVAL_COOKIE "GEBR-EVAL-COOKIE\n"

/* Prototypes & Private {{{1 */

/*
 * @vars: The hash table holding VarName -> Value
 * @in_ch: Input channel for sending messages to 'bc'
 * @out_ch: Output channel for receiving messages from 'bc'
 * @out_ch: Error channel for receiving errors from 'bc'
 */
struct _GebrArithExprPriv {
	GHashTable *vars;
	GIOChannel *in_ch;
	GIOChannel *out_ch;
	GIOChannel *err_ch;
	gboolean initialized;
};

/* Prototypes {{{2 */
static gboolean configure_channel(GIOChannel *channel, GError **err);

static void gebr_arith_expr_interface_init(GebrIExprInterface *iface);

static void gebr_arith_expr_finalize(GObject *object);

static gboolean gebr_arith_expr_eval_internal(GebrArithExpr *self,
					      const gchar   *expr,
					      gdouble       *result,
					      GError       **err);

G_DEFINE_TYPE_WITH_CODE(GebrArithExpr, gebr_arith_expr, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_IEXPR,
					      gebr_arith_expr_interface_init));

/* Class & Instance initialize/finalize {{{1 */
static void gebr_arith_expr_class_init(GebrArithExprClass *klass)
{
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = gebr_arith_expr_finalize;

	g_type_class_add_private(klass, sizeof(GebrArithExprPriv));
}

static void gebr_arith_expr_init(GebrArithExpr *self)
{
	const gchar *home_dir	= g_get_home_dir ();
	gchar *argv[]		= {"bc", "-l", NULL};
	GError *error		= NULL;
	gint in_fd, out_fd, err_fd;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_ARITH_EXPR,
						 GebrArithExprPriv);

	g_spawn_async_with_pipes (home_dir, argv, NULL,
				  G_SPAWN_SEARCH_PATH,
				  NULL, NULL, NULL,
				  &in_fd, &out_fd, &err_fd,
				  &error);

	if (error) {
		g_warning("Could not execute `bc': %s",
			  error->message);
		g_clear_error(&error);
		self->priv->initialized = FALSE;
		return;
	}

	self->priv->in_ch = g_io_channel_unix_new (in_fd);
	self->priv->out_ch = g_io_channel_unix_new (out_fd);
	self->priv->err_ch = g_io_channel_unix_new (err_fd);

	if (!configure_channel (self->priv->in_ch, &error) ||
	    !configure_channel (self->priv->out_ch, &error) ||
	    !configure_channel (self->priv->err_ch, &error))
	{
		g_io_channel_unref (self->priv->in_ch);
		g_io_channel_unref (self->priv->out_ch);
		g_io_channel_unref (self->priv->err_ch);

		g_warning("Could not create channels to listen `bc': %s",
			  error->message);
		g_clear_error(&error);
		self->priv->initialized = FALSE;
		return;
	}

	self->priv->vars = g_hash_table_new_full(g_str_hash,
						 g_str_equal,
						 g_free,
						 NULL);

	self->priv->initialized = TRUE;
}

static void gebr_arith_expr_finalize(GObject *object)
{
	GebrArithExpr *self = GEBR_ARITH_EXPR(object);

	g_hash_table_unref(self->priv->vars);
}

/* GebrIExpr interface methods implementation {{{1 */
static gboolean gebr_arith_expr_set_var(GebrIExpr              *iface,
					const gchar            *name,
					GebrGeoXmlParameterType type,
					const gchar            *value,
					GError                **err)
{
	GList *vars;
	gchar *evaluate;
	gdouble result;
	gboolean retval = TRUE;
	const gchar *undef_var = NULL;
	const gchar *invalid_var = NULL;
	GebrArithExpr *self = GEBR_ARITH_EXPR(iface);

	if (!self->priv->initialized) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INITIALIZE,
			    _("Error while initializing validator,"
			      " please contact support"));
		return FALSE;
	}

	switch (type)
	{
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		break;
	default:
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_TYPE,
			    _("Invalid variable type"));
		return FALSE;
	}

	// Checks if the value uses any undefined variable
	vars = gebr_iexpr_extract_vars(iface, value);

	GRegex *regex;
	GMatchInfo *info;

	regex = g_regex_new ("^[a-z][a-z0-9_]*$", 0, 0, NULL);

	for (GList *i = vars; i; i = i->next) {
		const gchar *name = i->data;
		g_regex_match (regex, name, 0, &info);
		if (!g_match_info_matches (info))
		{
			invalid_var = name;
			g_match_info_free (info);
			break;
		}
		g_match_info_free (info);

		if (!g_hash_table_lookup(self->priv->vars, name)) {
			undef_var = name;
			break;
		}
	}
	g_regex_unref (regex);


	if (invalid_var) {
		retval = FALSE;
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_VAR,
			    _("Invalid name for variable %s"),
			    invalid_var);
		goto exception;
	}

	if (undef_var) {
		retval = FALSE;
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_UNDEF_VAR,
			    _("Variable %s is not yet defined"),
			    undef_var);
		goto exception;
	}

	// Checks if there is syntax errors
	evaluate = g_strconcat(name, "=", value, ";0", NULL);
	if (!gebr_arith_expr_eval_internal(self, evaluate, &result, err)) {
		retval = FALSE;
		g_free(evaluate);
		goto exception;
	}

	if (result != 0) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_SYNTAX,
			    _("Syntax error when assigning `%s'"),
			    evaluate);
		g_free(evaluate);
		goto exception;
	}
	g_free(evaluate);

	// Finally, insert variable into local hash table
	g_hash_table_insert(self->priv->vars,
			    g_strdup(name),
			    GINT_TO_POINTER(1));

exception:
	g_list_foreach(vars, (GFunc) g_free, NULL);
	g_list_free(vars);

	return retval;
}

static gboolean gebr_arith_expr_is_valid(GebrIExpr   *iface,
					 const gchar *expr,
					 GError     **err)
{
	GList *vars = NULL;
	const gchar *invalid_var = NULL;
	const gchar *undef_var = NULL;
	GebrArithExpr *self = GEBR_ARITH_EXPR(iface);

	// Checks if the value uses any undefined variable
	vars = gebr_iexpr_extract_vars(iface, expr);
	GRegex *regex;
	GMatchInfo *info;

	regex = g_regex_new ("^[a-z][a-z0-9_]*$", 0, 0, NULL);

	for (GList *i = vars; i; i = i->next) {
		const gchar *name = i->data;
		g_regex_match (regex, name, 0, &info);
		if (!g_match_info_matches (info))
		{
			invalid_var = name;
			g_match_info_free (info);
			break;
		}
		g_match_info_free (info);

		if (!g_hash_table_lookup(self->priv->vars, name)) {
			undef_var = name;
			break;
		}
	}
	g_regex_unref (regex);


	if (invalid_var) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_VAR,
			    _("Invalid name for variable %s"),
			    invalid_var);
		return FALSE;
	}


	if (undef_var) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_UNDEF_VAR,
			    _("Variable %s is not yet defined"),
			    undef_var);
		return FALSE;
	}

	return gebr_arith_expr_eval(self, expr, NULL, err);
}

static void gebr_arith_expr_reset(GebrIExpr *iface)
{
	GebrArithExpr *self = GEBR_ARITH_EXPR(iface);
	g_hash_table_remove_all(self->priv->vars);
}

static GList *
gebr_arith_expr_extract_vars(GebrIExpr   *iface,
			     const gchar *expr)
{
	GList *vars = NULL;
	GRegex *regex;
	GMatchInfo *info;

	regex = g_regex_new ("[a-z][0-9a-z_]*", G_REGEX_CASELESS, 0, NULL);
	g_regex_match (regex, expr, 0, &info);

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

static void gebr_arith_expr_interface_init(GebrIExprInterface *iface)
{
	iface->set_var = gebr_arith_expr_set_var;
	iface->is_valid = gebr_arith_expr_is_valid;
	iface->reset = gebr_arith_expr_reset;
	iface->extract_vars = gebr_arith_expr_extract_vars;
}

/* Private functions {{{1 */
/*
 * configure_channel:
 *
 * Set @channel to non-blocking and @err if an error occured.
 *
 * Returns: %TRUE if no error ocurred
 */
static gboolean
configure_channel(GIOChannel *channel, GError **err)
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
 * gebr_arith_expr_eval_internal:
 */
static gboolean
gebr_arith_expr_eval_internal(GebrArithExpr *self,
			      const gchar   *expr,
			      gdouble       *result,
			      GError       **err)
{
	gchar *line;
	GIOStatus status;
	gchar *result_str = NULL;
	GError *error = NULL;
	gboolean finished = FALSE;

	enum {
		READ_RESULT,
		READ_STAMP,
		ERROR,
		SET_RESULT,
	} state = READ_RESULT;

	if (!expr || !*expr)
		return TRUE;

	line = g_strdup_printf ("%s ; \"%s\"\n", expr, EVAL_COOKIE);
	status = g_io_channel_write_chars (self->priv->in_ch, line, -1, NULL, &error);
	g_free (line);

	if (error)
	{
		g_propagate_error (err, error);
		g_warning("Error writing chars to `bc' input channel: %s",
			  error->message);
		return FALSE;
	}

	g_io_channel_flush (self->priv->in_ch, &error);
	if (error)
	{
		g_propagate_error (err, error);
		g_warning("Error flushing `bc' input channel: %s",
			  error->message);
		return FALSE;
	}

	while (!finished)
	{
		status = g_io_channel_read_line (self->priv->err_ch, &line, NULL, NULL, &error);

		if (status == G_IO_STATUS_NORMAL) {
			g_set_error(err,
				    GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_SYNTAX,
				    _("Invalid expression"));
			g_free (line);
			return FALSE;
		} else if (error) {
			g_warning("Error while reading `bc' error channel: %s",
				  error->message);
			g_clear_error(&error);
		}

		switch (state)
		{
		case READ_RESULT:
			status = g_io_channel_read_line(self->priv->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0(line, EVAL_COOKIE) == 0) {
					g_set_error(err,
						    GEBR_IEXPR_ERROR,
						    GEBR_IEXPR_ERROR_SYNTAX,
						    _("Expression does not evaluate to a value"));
					g_free (line);
					return FALSE;
				} else {
					result_str = g_strdup(line);
					state = READ_STAMP;
				}
				g_free (line);
			} else if (error) {
				g_warning("Error while reading `bc' output channel: %s",
					  error->message);
				g_clear_error(&error);
			}
			break;
		case READ_STAMP:
			status = g_io_channel_read_line(self->priv->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0(line, EVAL_COOKIE) == 0) {
					state = SET_RESULT;
				} else {
					state = ERROR;
				}
				g_free (line);
			} else if (error) {
				g_warning("Error while reading `bc' output channel: %s",
					  error->message);
				g_clear_error(&error);
			}
			break;
		case ERROR:
			// TODO: Flush output and error
			status = g_io_channel_read_line(self->priv->out_ch, &line, NULL, NULL, &error);
			if (status == G_IO_STATUS_NORMAL) {
				if (g_strcmp0(line, EVAL_COOKIE) == 0) {
					g_free(result_str);
					g_free(line);
					g_set_error(err,
						    GEBR_IEXPR_ERROR,
						    GEBR_IEXPR_ERROR_SYNTAX,
						    _("Expression returned multiple results"));
					return FALSE;
				} else if (error) {
					g_warning("Error while reading `bc' output channel: %s",
						  error->message);
					g_clear_error(&error);
				}
				g_free (line);
			}
			break;

		case SET_RESULT:
			if (result != NULL)
				*result = g_ascii_strtod(result_str, NULL);
			g_free (result_str);
			finished = TRUE;
			break;
		}
	}

	return TRUE;
}

/* Public functions {{{1 */
GebrArithExpr *gebr_arith_expr_new(void)
{
	return g_object_new(GEBR_TYPE_ARITH_EXPR, NULL);
}

gboolean gebr_arith_expr_eval(GebrArithExpr *self,
			      const gchar   *expr,
			      gdouble       *result,
			      GError       **err)
{
	if (!self->priv->initialized) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INITIALIZE,
			    _("Error while initializing validator,"
			      " please contact support"));
		return FALSE;
	}

	if (strchr(expr, '=')) {
		g_set_error (err,
			     GEBR_IEXPR_ERROR,
			     GEBR_IEXPR_ERROR_SYNTAX,
			     _("Invalid assignment '='"));
		return FALSE;
	}

	return gebr_arith_expr_eval_internal(self, expr, result, err);

}
