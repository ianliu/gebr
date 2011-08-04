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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "utils.h"
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
	gboolean initialized;
};

/* Prototypes {{{2 */
static gboolean configure_channel(GIOChannel *channel, GError **err);

static void gebr_arith_expr_interface_init(GebrIExprInterface *iface);

static void gebr_arith_expr_finalize(GObject *object);

static gboolean
gebr_arith_expr_eval_impl (GebrIExpr   *self,
			   const gchar *expr,
			   gchar      **result,
			   GError     **err);

gboolean arith_spawn_bc(int *in_fd, int *out_fd);

G_DEFINE_TYPE_WITH_CODE(GebrArithExpr, gebr_arith_expr, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_IEXPR,
					      gebr_arith_expr_interface_init));

/* Class & Instance initialize/finalize {{{1 */
static void gebr_arith_expr_class_init(GebrArithExprClass *klass)
{
	GObjectClass *g_class;

	signal(SIGCHLD, SIG_IGN);

	g_class = G_OBJECT_CLASS(klass);
	g_class->finalize = gebr_arith_expr_finalize;

	g_type_class_add_private(klass, sizeof(GebrArithExprPriv));
}

static void gebr_arith_expr_init(GebrArithExpr *self)
{
	gint in_fd, out_fd;
	GError *error = NULL;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_ARITH_EXPR,
						 GebrArithExprPriv);

	if (!arith_spawn_bc (&in_fd, &out_fd)) {
		g_warning("Could not execute `bc'");
		self->priv->initialized = FALSE;
		return;
	}

	self->priv->in_ch = g_io_channel_unix_new (in_fd);
	self->priv->out_ch = g_io_channel_unix_new (out_fd);

	if (!configure_channel (self->priv->in_ch, &error) ||
	    !configure_channel (self->priv->out_ch, &error))
	{
		g_io_channel_unref (self->priv->in_ch);
		g_io_channel_unref (self->priv->out_ch);

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

	G_OBJECT_CLASS(gebr_arith_expr_parent_class)->finalize(object);
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
	gchar *result;
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

	if (g_strcmp0(result,"0") != 0) {
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
			invalid_var = g_strdup(name);
			g_match_info_free (info);
			break;
		}
		g_match_info_free (info);

		if (!g_hash_table_lookup(self->priv->vars, name)) {
			undef_var = g_strdup(name);
			break;
		}
	}
	g_list_foreach(vars, (GFunc)g_free, NULL);
	g_list_free(vars);
	g_regex_unref (regex);


	if (invalid_var) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INVAL_VAR,
			    _("Invalid name for variable %s"),
			    invalid_var);
		return FALSE;
	}

	if (!gebr_arith_expr_eval(self, expr, NULL, err))
		return FALSE;

	if (undef_var) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_UNDEF_VAR,
			    _("Variable %s is not yet defined"),
			    undef_var);
		return FALSE;
	}

	return TRUE;
}

static void gebr_arith_expr_reset(GebrIExpr *iface)
{
	GebrArithExpr *self = GEBR_ARITH_EXPR(iface);
	g_hash_table_remove_all(self->priv->vars);
}

GList *
gebr_arith_expr_extract_vars(GebrIExpr   *iface,
			     const gchar *expr)
{
	GList *vars = NULL;
	GRegex *regex;
	GMatchInfo *info;
	GHashTable *set;

	set = g_hash_table_new(g_str_hash, g_str_equal);
	regex = g_regex_new ("([a-z][0-9a-z_]*)\\s*(.?)", G_REGEX_CASELESS, 0, NULL);
	g_regex_match (regex, expr, 0, &info);

	while (g_match_info_matches (info))
	{
		gchar *word = g_match_info_fetch (info, 1);
		gchar *next_char = g_match_info_fetch(info, 2);
		if (next_char[0] != '(' && !g_hash_table_lookup(set, word)) {
			vars = g_list_prepend (vars, word);
			g_hash_table_insert(set, word, GUINT_TO_POINTER(1));
		} else
			g_free(word);
		g_match_info_next (info, NULL);
		g_free(next_char);
	}

	g_hash_table_unref(set);
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
	iface->eval = gebr_arith_expr_eval_impl;
}

/* Private functions {{{1 */
/*
 */
gboolean
arith_spawn_bc(int *infd, int *outfd)
{
	const gchar *home_dir	= g_get_home_dir ();
	gchar *argv[]		= {"bc", "-l", NULL};

	gint in_fd[2];
	gint out_fd[2];

	if (pipe(in_fd) < 0 || pipe(out_fd) < 0)
		return FALSE;

	GPid child;
	child = fork();

	if (child < 0)
		return FALSE;

	if (child == 0) {
		if (dup2(in_fd[0], 0) < 0 ||
		    dup2(out_fd[1], 1) < 0 ||
		    dup2(out_fd[1], 2) < 0)
			_exit(1);

		if (close(in_fd[1]) < 0 ||
		    close(out_fd[0]) < 0)
			_exit(1);

		if (g_chdir(home_dir) < 0)
			_exit(1);

		setsid();
		execvp(argv[0], argv);
	}

	close(in_fd[0]);
	close(out_fd[1]);

	*infd = in_fd[1];
	*outfd = out_fd[0];

	return TRUE;
}

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

static gboolean
read_bc_line(GebrArithExpr *self,
	     gchar **line,
	     GError **err)
{
	GError *error = NULL;
	GIOStatus status;

	do
		status = g_io_channel_read_line(self->priv->out_ch, line, NULL, NULL, &error);
	while (status == G_IO_STATUS_AGAIN);

	if (error) {
		g_warning("Error while reading `bc' output channel: %s",
			  error->message);
		g_set_error(err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INITIALIZE,
			    _("Error while reading `bc' output channel: %s"),
			    error->message);
		g_clear_error(&error);
		return FALSE;
	}

	return TRUE;
}

/*
 * gebr_arith_expr_eval_internal:
 */
gboolean
gebr_arith_expr_eval_internal(GebrArithExpr *self,
			      const gchar   *expr,
			      gchar        **result,
			      GError       **err)
{
	gchar *line;
	GString *buffer = g_string_sized_new(70);
	GError *error = NULL;

	if (!expr || !*expr)
		return TRUE;

	line = g_strdup_printf ("%s\n\"%s\"\n", expr, EVAL_COOKIE);
	g_io_channel_write_chars (self->priv->in_ch, line, -1, NULL, &error);
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

	int results = 0;
	while (read_bc_line(self, &line, err)) {
		if (g_strcmp0(line, EVAL_COOKIE) == 0) {
			g_free(line);
			break;
		}
		if (g_str_has_prefix(line, "(standard_in) ")) {
			gchar *msg;
			msg = strchr(line, ':');
			msg[strlen(msg) - 1] = '\0';
			g_set_error(err,
				    GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_SYNTAX,
				    _("Invalid expression%s"), msg);
			goto exception_and_flush;
		}
		if (g_str_has_prefix(line, "Runtime error (func")) {
			gchar *msg;
			msg = strchr(line, ':');
			msg[strlen(msg) - 1] = '\0';
			g_set_error(err,
				    GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_RUNTIME,
				    _("Invalid expression%s"), msg);
			goto exception_and_flush;
		}
		int length = strlen(line);
		if (line[length - 2] == '\\') {
			g_set_error(err,
				    GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_TOOBIG,
				    _("Expression result is too big"));
			goto exception_and_flush;
		} else
			results++;
		g_string_append_len(buffer, line, length);
		g_free(line);
	}

	if (results == 0) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_SYNTAX,
			    _("Expression does not evaluate to a value"));
		goto exception;
	}

	if (results > 1) {
		g_set_error(err,
		            GEBR_IEXPR_ERROR,
		            GEBR_IEXPR_ERROR_SYNTAX,
		            _("Expression returned multiple results"));
		goto exception;
	}

	g_string_set_size(buffer, buffer->len - 1);
	if (result)
		*result = buffer->str;
	g_string_free(buffer, !result);
	return TRUE;

exception_and_flush:

	do {
		g_free(line);
		read_bc_line(self, &line, NULL);
	} while (g_strcmp0(line, EVAL_COOKIE) != 0);
	g_free(line);

exception:

	g_string_free(buffer, TRUE);
	return FALSE;
}

static gboolean
gebr_arith_expr_eval_impl (GebrIExpr   *self,
			   const gchar *expr,
			   gchar      **result,
			   GError     **err)
{
	gdouble res = 0;
	gboolean retval;

	retval = gebr_arith_expr_eval (GEBR_ARITH_EXPR(self), expr, &res, err);
	*result = gebr_str_remove_trailing_zeros(g_strdup_printf("%.10lf", res));

	return retval;
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
	gchar *string = NULL;

	if (!self->priv->initialized) {
		g_set_error(err,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_INITIALIZE,
			    _("Error while initializing validator,"
			      " please contact support"));
		return FALSE;
	}

	if (strchr(expr, ';')) {
		g_set_error (err,
			     GEBR_IEXPR_ERROR,
			     GEBR_IEXPR_ERROR_SYNTAX,
			     _("Invalid syntax: ';'"));
		return FALSE;
	}
	if (strchr(expr, '"')) {
		g_set_error (err,
			     GEBR_IEXPR_ERROR,
			     GEBR_IEXPR_ERROR_SYNTAX,
			     _("Invalid syntax: '\"'"));
		return FALSE;
	}

	gboolean ok = gebr_arith_expr_eval_internal(self, expr, &string, err);

	if (ok && string && result)
		*result = g_ascii_strtod(string, NULL);

	return ok;
}
