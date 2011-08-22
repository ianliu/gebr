#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <geoxml/geoxml.h>
#include <glib/gprintf.h>
#include <geoxml/geoxml.h>

#include "gebr-validator.h"
#include "utils.h"
#include "gebr-arith-expr.h"

/* Structures {{{1 */
struct _GebrValidator
{
	GebrArithExpr *arith_expr;
	GebrGeoXmlDocument **docs[3];
	GHashTable *vars;
	// Scope of the last sync with BC
	GebrGeoXmlDocumentType cached_scope;
};

typedef struct {
	gchar *name;
	GebrGeoXmlParameter *param[3];
	gdouble weight[3];
	GList *dep[3];
	GError *error[3];
} HashData;

static GebrGeoXmlDocument *cache_docs[] = { NULL, NULL, NULL};

#define MAX_RESULT_LENGTH 68
#define ITER_INI_EXPR ";iter=bc_reset(0);"
#define ITER_END_EXPR ";iter=bc_reset(1);"

#define GET_VAR_NAME(p) (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p)))
#define GET_VAR_VALUE(p) (gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE))
#define SET_VAR_VALUE(p,v) (gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE, (v)))
#define SET_VAR_NAME(p,n) (gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p), (n)))

/* Prototypes {{{1 */
static GebrGeoXmlParameter * get_param(GebrValidator         *self,
                                       const gchar           *name,
                                       GebrGeoXmlDocumentType scope);

static GebrGeoXmlParameter * get_dep_param(GebrValidator *self,
                                           const gchar *my_name,
                                           GebrGeoXmlDocumentType my_scope,
                                           const gchar *dep_name);

static gboolean gebr_validator_validate_iter(GebrValidator *self,
                                             GebrGeoXmlParameter *param,
                                             GError **error);

static gboolean gebr_validator_evaluate_internal(GebrValidator *self,
                                                 const gchar *name,
                                                 const gchar *expr,
                                                 GebrGeoXmlParameterType type,
                                                 gchar **value,
                                                 GebrGeoXmlDocumentType scope,
                                                 gboolean show_interval,
                                                 GError **error);

/* NodeData functions {{{1 */
static HashData *
hash_data_new_from_xml(GebrGeoXmlParameter *param)
{
	HashData *n = g_new0(HashData, 1);
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	n->param[scope] = param;
	n->name = GET_VAR_NAME(param);
	n->weight[scope] = G_MAXDOUBLE;
	return n;
}

static void
hash_data_free(gpointer p)
{
	HashData *n = p;
	for (int i = 0; i < 3; i++) {
		g_list_foreach(n->dep[i], (GFunc) g_free, NULL);
		g_list_free(n->dep[i]);
		if (n->error[i])
			g_error_free(n->error[i]);
		if (n->param[i])
			gebr_geoxml_object_unref(n->param[i]);
	}
	g_free(n->name);
	g_free(n);
}

static gboolean
hash_data_remove(GebrValidator *self,
		const gchar *name,
		GebrGeoXmlDocumentType scope)
{
	HashData *data;

	g_return_val_if_fail(scope < GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN, FALSE);
	data = g_hash_table_lookup(self->vars, name);
	g_return_val_if_fail(data != NULL, FALSE);

	if (!data->param[scope]) {
		return FALSE;
	}

	gebr_geoxml_object_unref(data->param[scope]);
	data->param[scope] = NULL;
	data->weight[scope] = G_MAXDOUBLE;
	g_list_foreach(data->dep[scope], (GFunc) g_free, NULL);
	g_list_free(data->dep[scope]);
	data->dep[scope] = NULL;
	g_clear_error(&data->error[scope]);

	if (!get_param(self, name, GEBR_GEOXML_DOCUMENT_TYPE_FLOW))
		g_hash_table_remove(self->vars, name);

	return TRUE;
}

/* Private functions {{{1 */
static GebrIExpr *
get_validator_by_type(GebrValidator *self,
		      GebrGeoXmlParameterType type)
{
	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		// FIXME
		return NULL;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		return GEBR_IEXPR(self->arith_expr);
	default:
		return NULL;
	}
}

static gboolean
get_error_indirect(GebrValidator *self,
                   GList *dep_names,
                   const gchar *my_name,
                   GebrGeoXmlParameterType my_type,
                   GebrGeoXmlDocumentType my_scope,
                   GError **err)
{
	HashData *dep_data;
	GError *error = NULL;
	GebrGeoXmlParameterType dep_type;
	GebrGeoXmlDocumentType dep_scope;

	for (GList *i = dep_names; i; i = i->next) {
		gchar *dep_name = i->data;
		GebrGeoXmlParameter *dep_param;

		if (g_strcmp0(my_name, dep_name) == 0) {
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_BAD_REFERENCE,
			            _("Variable %s can not reference itself"),
			            dep_name);
			return FALSE;
		}

		dep_data = g_hash_table_lookup(self->vars, dep_name);
		if (!dep_data) {
			if (!g_strcmp0(dep_name, "iter"))
				g_set_error(err, GEBR_IEXPR_ERROR,
				            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
				            my_scope == GEBR_GEOXML_DOCUMENT_TYPE_FLOW ?
				            _("Insert program Loop to use variable iter"):
				            _("Iter can be used only on flow variables"));
			else
				g_set_error(err, GEBR_IEXPR_ERROR,
				            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
				            _("Variable %s is not defined"),
				            dep_name);
			return FALSE;
		}

		dep_param = get_dep_param(self, my_name, my_scope, dep_name);
		if (!dep_param) {
			if (!g_strcmp0(dep_name, "iter"))
				g_set_error(err, GEBR_IEXPR_ERROR,
				            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
				            _("Iter can be used only on flow variables"));
			else
				g_set_error(err, GEBR_IEXPR_ERROR,
				            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
				            _("Variable %s is not yet defined"),
				            dep_name);
			return FALSE;
		}

		dep_type = gebr_geoxml_parameter_get_type(dep_param);
		dep_scope = gebr_geoxml_parameter_get_scope(dep_param);

		if ((my_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
		     my_type == GEBR_GEOXML_PARAMETER_TYPE_INT)
		    && dep_type == GEBR_GEOXML_PARAMETER_TYPE_STRING) {
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_TYPE_MISMATCH,
			            _("Variable %s is String"),
			            dep_name);
			return FALSE;
		}

		if (!dep_data->error[dep_scope])
			get_error_indirect(self, dep_data->dep[dep_scope], dep_name, dep_type, dep_scope, &error);

		if (dep_data->error[dep_scope] || error) {
			if (error)
				g_clear_error(&error);

			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_BAD_REFERENCE,
			            _("Variable %s is not well defined"),
			            dep_name);
			return FALSE;
		}
	}

	return TRUE;
}

static void
set_error(GebrValidator *self,
	  const gchar *name,
	  GebrGeoXmlDocumentType scope,
	  GError *error)
{
	if (!name) return;
	HashData *data = g_hash_table_lookup(self->vars, name);
	g_return_if_fail(data != NULL);
	if (error) {
		data->error[scope] = g_error_copy(error);
	} else if (data->error[scope])
		g_clear_error(&data->error[scope]);
}

/*
 * Get the first non-NULL parameter from the Hash Data.
 * This respects scope priority, ie it returns the value
 * of Flow, Line and them Project.
 */
static GebrGeoXmlParameter *
get_param(GebrValidator *self,
	  const gchar *name,
	  GebrGeoXmlDocumentType scope)
{
	HashData *data;

	data = g_hash_table_lookup(self->vars, name);

	g_return_val_if_fail(data != NULL, NULL);

	for (; scope < 3; scope++) {
		if (!data->param[scope])
			continue;
		return data->param[scope];
	}
	return NULL;
}

static GebrGeoXmlParameter *
get_dep_param(GebrValidator *self,
              const gchar *my_name,
              GebrGeoXmlDocumentType my_scope,
              const gchar *dep_name)
{
	HashData *my_data, *dep_data;
	GebrGeoXmlParameter *dep_param;
	GebrGeoXmlDocumentType dep_scope;

	dep_data = g_hash_table_lookup(self->vars, dep_name);

	dep_param = get_param(self, dep_name, my_scope);
	if (!dep_param || !my_name)
		return dep_param;

	my_data = g_hash_table_lookup(self->vars, my_name);
	g_return_val_if_fail(my_data != NULL, NULL);

	dep_scope = gebr_geoxml_parameter_get_scope(dep_param);
	if (my_scope == dep_scope && dep_data->weight[dep_scope] > my_data->weight[my_scope])
		return get_param(self, dep_name, my_scope+1);

	return dep_param;
}

static gboolean
translate_string_expr(GebrValidator *self,
                      const gchar *expr,
                      const gchar *my_name,
                      GebrGeoXmlDocumentType my_scope,
                      gchar **translated,
                      GList **deps,
                      GError **error)
{
	GString *str_expr =  g_string_sized_new(128);
	enum {
		INIT,
		START,
		TEXT,
		VAR
	} state = INIT;

	while (*expr) {
		switch (state) {
		case INIT:
			g_string_append(str_expr, "print ");
		case START:
			if (*expr == '[' && expr[1] != '[') {
				state = VAR;
				break;
			}
			g_string_append_c(str_expr, '"');
			state = TEXT;
			continue;
		case TEXT:
			if (*expr == '"') {
				g_string_append(str_expr, "\\q");
				break;
			}
			if (*expr == '\\') {
				g_string_append(str_expr, "\\\\");
				break;
			}
			if (*expr == '[') {
				if (expr[1] != '[') {
					g_string_append(str_expr, "\",");
					state = VAR;
				} else {
					g_string_append_c(str_expr, '[');
					expr++; // discards one '['
				}
				break;
			}
			if (*expr == ']') {
				if (expr[1] != ']') {
					goto unmatched_right;
				}
				expr++; // discards one ']'
			}
			g_string_append_c(str_expr, *expr);
			break;
		case VAR: {
			int size = 0;
			while (expr[size] != ']') {
				if (!expr[size]) {
					goto unmatched_right;
				}
				if (expr[size] == '[') {
					goto unmatched_left;
				}
				size++;
			}
			gchar *var_name = g_strndup(expr, size);
			if (translated) {
				GebrGeoXmlParameter *var_param = get_dep_param(self, my_name, my_scope, var_name);
				GebrGeoXmlDocumentType var_scope = gebr_geoxml_parameter_get_scope(var_param);
				GebrGeoXmlParameterType var_type = gebr_geoxml_parameter_get_type(var_param);
				g_string_append_printf(str_expr, var_type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "str(%s[%d]),\"\\b\"," : "%s[%d],", var_name, var_scope);
			}
			if (deps) {
				*deps = g_list_prepend(*deps, var_name);
			} else {
				g_free(var_name);
			}
			state = START;
			expr += size;
			break;
		} default:
			g_warn_if_reached();
		}
		expr++;
	}
	switch (state) {
	case TEXT:
		g_string_append(str_expr, "\",");
		break;
	case VAR:
		goto unmatched_left;
	default:
		break;
	}

	g_string_append(str_expr, "\"\\n\"");

	if (deps) {
		*deps = g_list_reverse(*deps);
	}

	if (translated) {
		*translated = str_expr->str;
		g_string_free(str_expr, FALSE);
	} else {
		g_string_free(str_expr, TRUE);
	}
	return TRUE;

	unmatched_right:
	g_set_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX,
	            _("Syntax error: unmatched right bracket"));
	goto exception;

	unmatched_left:
	g_set_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX,
	            _("Syntax error: unmatched left bracket"));

	exception:
	if (deps) {
		g_list_foreach(*deps, (GFunc)g_free, NULL);
		g_list_free(*deps);
		*deps = NULL;
	}
	if (translated)
		g_free(*translated);
	g_string_free(str_expr, TRUE);
	return FALSE;
}

/* Validate @expression and extract vars on @deps with @error */
static gboolean
define_validate_and_extract_vars(GebrValidator  *self,
                                 const gchar *name,
                                 const gchar *expression,
                                 GebrGeoXmlParameterType type,
                                 GebrGeoXmlDocumentType scope,
                                 GList **deps,
                                 GError **error)
{
	GList *vars = NULL;
	gboolean valid = FALSE;
	if (deps) {
		g_list_foreach(*deps, (GFunc) g_free, NULL);
		g_list_free(*deps);
		*deps = NULL;
	}

	if (!*expression) {
		return TRUE;
	}

	gchar *striped = g_strstrip(g_strdup(expression));
	gboolean empty = !*striped;
	g_free(striped);

	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		if (empty) return TRUE;
		valid = translate_string_expr(self, expression, NULL, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, NULL, &vars, error);
		if (valid)
			valid = get_error_indirect(self, vars, name, type, scope, error);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: {
		if (empty) {
			g_set_error (error,
			             GEBR_IEXPR_ERROR,
			             GEBR_IEXPR_ERROR_EMPTY_EXPR,
			             _("Expression does not evaluate to a value"));
			return FALSE;
		}
		vars = gebr_iexpr_extract_vars(GEBR_IEXPR(self->arith_expr), expression);
		valid = get_error_indirect(self, vars, name, type, scope, error);
		if (valid) {
			valid = FALSE;
			if (strchr(expression, ';')) {
				g_set_error (error,
				             GEBR_IEXPR_ERROR,
				             GEBR_IEXPR_ERROR_SYNTAX,
				             _("Invalid syntax: ';'"));
				break;
			}
			if (strchr(expression, '"')) {
				g_set_error (error,
				             GEBR_IEXPR_ERROR,
				             GEBR_IEXPR_ERROR_SYNTAX,
				             _("Invalid syntax: '\"'"));
				break;
			}
			gchar *define;
			if (name) {
				define = g_strconcat(name, "=(", expression, ");", name, NULL);
				self->cached_scope = GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;
			} else {
				define = g_strdup(expression);
			}
			gchar *result = NULL;
			valid = gebr_arith_expr_eval_internal(self->arith_expr, define, &result, error);
			if (valid && strlen(result) > MAX_RESULT_LENGTH) {
				valid = FALSE;
				g_set_error(error, GEBR_IEXPR_ERROR,
					    GEBR_IEXPR_ERROR_TOOBIG,
					    _("Expression result is too big"));
			}
			g_free(define);
			g_free(result);
		}
		break;
	} default:
		return TRUE;
	}
	if (deps) {
		*deps = vars;
	} else {
		g_list_foreach(vars, (GFunc) g_free, NULL);
		g_list_free(vars);
	}
	return valid;
}

/* Validate @expression and extract vars on @deps with @error */
static gboolean
validate_and_extract_param(GebrValidator  *self,
                           GebrGeoXmlParameter *param,
                           GList **deps,
                           GError **error)
{
	GError *err = NULL;
	gchar *name = GET_VAR_NAME(param);
	gchar *value = GET_VAR_VALUE(param);
	GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(param);
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param)) && !*value) {
		g_set_error(&err,
		            GEBR_IEXPR_ERROR,
		            GEBR_IEXPR_ERROR_EMPTY_EXPR,
		            _("This parameter is required"));
		set_error(self, name, scope, err);
		g_propagate_error(error, err);
		g_free(name);
		g_free(value);
		return FALSE;
	}
	if (define_validate_and_extract_vars(self, name, value, type, scope, deps, &err)) {
		set_error(self, name, scope, NULL);
		g_free(name);
		g_free(value);
		return TRUE;
	}
	if (err->code >= GEBR_IEXPR_ERROR_EMPTY_EXPR && err->code <= GEBR_IEXPR_ERROR_SYNTAX) {
		set_error(self, name, scope, err);
	} else {
		set_error(self, name, scope, NULL);
	}
	g_propagate_error(error, err);
	g_free(name);
	g_free(value);
	return FALSE;
}

gboolean
gebr_validator_update_vars(GebrValidator *self,
                           GebrGeoXmlDocumentType param_scope,
                           GError **error)
{
	if (self->cached_scope == param_scope)
		return TRUE;

	int nth = 0;
	gchar* name = NULL;
	GString *bc_vars =  g_string_sized_new(1024);
	GString *bc_strings =  g_string_sized_new(2*1024);

	g_string_append(bc_strings, "define str(n) { if (n==0) \"\"");

	/* bc_reset parameter name *MUST* be 'iter' so we don't need
	 * to create another reserved word in GeBR's dictionary.
	 *  bc_reset(0) = initial iterator value
	 *  bc_reset(1) = final iterator value
	 */
	g_string_append(bc_vars, "define bc_reset(iter) {\n");

	gboolean has_iter = FALSE;
	// Validate iter final value
	int scope = GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	if (param_scope == scope && self->docs[scope]) {
		GebrGeoXmlSequence *param = gebr_geoxml_document_get_dict_parameter(*self->docs[scope]);
		g_free(name);
		name = GET_VAR_NAME(param);
		if (g_strcmp0(name, "iter") == 0) {
			has_iter = TRUE;
			if (gebr_validator_validate_iter(self, GEBR_GEOXML_PARAMETER(param), NULL)
			    && validate_and_extract_param(self, GEBR_GEOXML_PARAMETER(param), NULL, NULL)) {
				gebr_geoxml_sequence_next(&param);
				for (; param; gebr_geoxml_sequence_next(&param)) {
					GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));
					if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING)
						continue;
					validate_and_extract_param(self, GEBR_GEOXML_PARAMETER(param), NULL, NULL);
				}
			}
		}
		gebr_geoxml_object_unref(param);
	}
	for (int scope = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; scope >= (int) param_scope; scope--) {
		if (!self->docs[scope] || !*(self->docs[scope]))
			continue;
		GebrGeoXmlSequence *param = gebr_geoxml_document_get_dict_parameter(*self->docs[scope]);
		gchar* value = NULL;
		for (; param; gebr_geoxml_sequence_next(&param)) {
			g_free(name);
			name = GET_VAR_NAME(param);
			GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));

			HashData *data = g_hash_table_lookup(self->vars, name);
			if (!data || (has_iter && data->error[scope]) || !get_error_indirect(self, data->dep[scope], name, type, scope, NULL))
				continue;

			g_free(value);
			value = GET_VAR_VALUE(param);

			if (type != GEBR_GEOXML_PARAMETER_TYPE_STRING && g_strcmp0(name, "iter") == 0) {
				g_string_append_printf(bc_vars, "%1$s=%1$s[%2$d]=%3$s*iter\n", name, scope, value);
				gchar *expr = g_strconcat(value, "*0", NULL);
				// Defines 'iter' with its initial value in bc
				define_validate_and_extract_vars(self, name, expr, type, scope, NULL, NULL);
				g_free(expr);
				continue;
			}

			// Revalidate numbers
			if (type != GEBR_GEOXML_PARAMETER_TYPE_STRING
					&& validate_and_extract_param(self, GEBR_GEOXML_PARAMETER(param), NULL, NULL)) {
				g_string_append_printf(bc_vars, "%1$s=%1$s[%2$d]=(%3$s)\n", name, scope, value);
			}

			if (data->error[scope] || !get_error_indirect(self, data->dep[scope], name, type, scope, NULL))
				continue;

			if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING) {
				gchar *translated = NULL;
				translate_string_expr(self, value, name, scope, &translated, NULL, NULL);
				translated[strlen(translated) - 5] = '\0'; // removes the new line sequence
				g_string_append_printf(bc_vars, "%1$s=%1$s[%2$d]=%3$d\n", name, scope, ++nth);
				g_string_append_printf(bc_strings, " else if (n==%d) %s", nth, translated);
				g_free(translated);
				continue;
			}
		}
		g_free(name);
		name = NULL;
		g_free(value);
	}
	g_string_append(bc_strings, " }\n");
	g_string_append(bc_strings, bc_vars->str);
	g_string_append(bc_strings, "return iter };0\n" ITER_INI_EXPR);

	if (error)
		g_assert_no_error(*error);

	gboolean ok = gebr_arith_expr_eval_internal(self->arith_expr, bc_strings->str, NULL, error);
	self->cached_scope = param_scope;

	if (error)
		g_assert_no_error(*error);

	g_string_free(bc_vars, TRUE);
	g_string_free(bc_strings, TRUE);
	return ok;
}

/* Public functions {{{1 */
GebrValidator *
gebr_validator_new(GebrGeoXmlDocument **flow,
		   GebrGeoXmlDocument **line,
		   GebrGeoXmlDocument **proj)
{
	GebrValidator *self = g_new(GebrValidator, 1);
	self->arith_expr = gebr_arith_expr_new();
	self->docs[0] = flow;
	self->docs[1] = line;
	self->docs[2] = proj;

	self->vars = g_hash_table_new_full(g_str_hash,
					   g_str_equal,
					   g_free,
					   hash_data_free);

	gebr_arith_expr_eval_internal(self->arith_expr, "scale=5", NULL, NULL);
	self->cached_scope = GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;
	gebr_validator_update(self);
	return self;
}

static gdouble
get_weight(GebrValidator *self,
           GebrGeoXmlParameter *param)
{
	HashData *data;
	gchar *name = GET_VAR_NAME(param);
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);

	data = g_hash_table_lookup(self->vars, name);
	g_free(name);

	if (data && data->param[scope])
		return data->weight[scope];
	return -1;
}

static gdouble
compute_weight(GebrValidator *self,
               GebrGeoXmlParameter *before,
               GebrGeoXmlParameter *after)
{
	gdouble b;
	gdouble a;

	b = !before? 0: get_weight(self, before);

	if (b == -1)
		return 1;

	if (!after)
		return b + 1;

	a = get_weight(self, after);

	if (a == -1)
		return b + 1;

	return (a + b)/2;
}

gboolean
gebr_validator_insert(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError             **error)
{
	gchar *name, *value;
	GebrGeoXmlSequence *prev_param;
	GebrGeoXmlSequence *next_param;
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	HashData *data;

	name = GET_VAR_NAME(param);
	g_return_val_if_fail(name != NULL && strlen(name), FALSE);
	data = g_hash_table_lookup(self->vars, name);

	if (!data) {
		data = hash_data_new_from_xml(param);
		gebr_geoxml_object_ref(param);
		g_hash_table_insert(self->vars, name, data);
	} else {
		if (!data->param[scope]) {
			data->param[scope] = param;
			gebr_geoxml_object_ref(param);
		}
		g_free(name);
	}
	prev_param = GEBR_GEOXML_SEQUENCE(param);
	next_param = GEBR_GEOXML_SEQUENCE(param);

	gebr_geoxml_object_ref(prev_param);
	gebr_geoxml_sequence_previous(&prev_param);

	gebr_geoxml_object_ref(next_param);
	gebr_geoxml_sequence_next(&next_param);
	data->weight[scope] = compute_weight(self,
					     GEBR_GEOXML_PARAMETER(prev_param),
					     GEBR_GEOXML_PARAMETER(next_param));
	gebr_geoxml_object_unref(prev_param);
	gebr_geoxml_object_unref(next_param);
	value = GET_VAR_VALUE(param);
	gboolean is_valid = gebr_validator_change_value(self, param, value, affected, error);
	g_free(value);
	return is_valid;
}

gboolean
gebr_validator_remove(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError		 **error)
{
	self->cached_scope = GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;
	gchar *name;
	GebrGeoXmlDocumentType scope;
	gboolean removed;

	name = GET_VAR_NAME(param);
	scope = gebr_geoxml_parameter_get_scope (param);

	gebr_geoxml_object_ref(param);
	removed = hash_data_remove(self, name, scope);
	if (removed)
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(param));
	else
		gebr_geoxml_object_unref(param);

	g_free(name);
	return removed;
}

gboolean
gebr_validator_rename(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      const gchar         *new_name,
		      GList              **affected,
		      GError             **error)
{
	self->cached_scope = GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;
	const gchar * name = NULL;
	HashData *data, *new_data;
	GebrGeoXmlDocumentType scope;

	name = GET_VAR_NAME(param);
	g_return_val_if_fail(g_strcmp0(name, new_name) != 0, TRUE);

	scope = gebr_geoxml_parameter_get_scope(param);

	data = g_hash_table_lookup(self->vars, name);
	g_return_val_if_fail(data != NULL, FALSE);
	SET_VAR_NAME(param, new_name);

	new_data = g_hash_table_lookup(self->vars, new_name);
	if (!new_data) {
		new_data = hash_data_new_from_xml(param);
		g_hash_table_insert(self->vars, g_strdup(new_name), new_data);
	} else {
		g_return_val_if_fail(new_data->param[scope] == NULL, FALSE);
		new_data->param[scope] = data->param[scope];
	}
	data->param[scope] = NULL;

	new_data->weight[scope] = data->weight[scope];
	data->weight[scope] = G_MAXDOUBLE;

	new_data->dep[scope] = data->dep[scope];
	data->dep[scope] = NULL;

	new_data->error[scope] = data->error[scope];
	data->error[scope] = NULL;

	return TRUE;
}

gboolean
gebr_validator_change_value(GebrValidator       *self,
			    GebrGeoXmlParameter *param,
			    const gchar         *new_value,
			    GList              **affected,
			    GError             **error)
{
	HashData *data;
	gchar *name;
	GebrGeoXmlDocumentType scope;
	GebrGeoXmlParameterType type;
	GError *err = NULL;

	name = GET_VAR_NAME(param);
	scope = gebr_geoxml_parameter_get_scope(param);
	type = gebr_geoxml_parameter_get_type(param);

	if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param)) && !*new_value) {
		g_set_error(&err,
		            GEBR_IEXPR_ERROR,
		            GEBR_IEXPR_ERROR_EMPTY_EXPR,
		            _("This parameter is required"));
		set_error(self, name, scope, err);
		g_propagate_error(error, err);
		g_free(name);
		return FALSE;
	}

	SET_VAR_VALUE(param, new_value);
	self->cached_scope = GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;

	if (g_strcmp0(name, "iter") == 0) {
		if (!gebr_validator_validate_iter(self, param, &err)) {
			g_propagate_error(error, err);
			g_free(name);
			return FALSE;
		}
		return TRUE;
	}

	data = g_hash_table_lookup(self->vars, name);
	g_free(name);
	g_return_val_if_fail(data != NULL, FALSE);

	return validate_and_extract_param(self, param, &data->dep[scope], error);
}

gboolean
gebr_validator_move(GebrValidator         *self,
		    GebrGeoXmlParameter   *source,
		    GebrGeoXmlParameter   *pivot,
		    GebrGeoXmlDocumentType pivot_scope,
		    GebrGeoXmlParameter  **copy,
		    GList                **affected,
		    GError               **error)
{
	const gchar *name;
	const gchar *value;
	const gchar *comment;
	HashData *data;
	GebrGeoXmlParameter *new_param;
	GebrGeoXmlParameterType type;
	GebrGeoXmlDocumentType t1, t2;

	name = GET_VAR_NAME(source);
	data = g_hash_table_lookup(self->vars, name);

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(self->docs[pivot_scope] != NULL, FALSE);

	comment = gebr_geoxml_parameter_get_label(source);
	value = GET_VAR_VALUE(source);
	t1 = gebr_geoxml_parameter_get_scope(source);
	t2 = pivot_scope;
	type = gebr_geoxml_parameter_get_type(source);

	g_assert(data->param[t1] == source);

	GError *err1 = NULL;

	if (t1 != t2) {
		if (data->param[t2]) {
			g_set_error(error, GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_BAD_MOVE,
				    _("Variable %s already exists in that scope"),
				    name);
			return FALSE;
		} else {
			GebrGeoXmlDocument *doc;
			doc = *self->docs[pivot_scope];
			new_param = gebr_geoxml_document_set_dict_keyword(doc, type, name, value);
			gebr_geoxml_parameter_set_label(new_param, comment);
			gebr_validator_insert(self, new_param, NULL, &err1);
		}
	} else {
		new_param = source;
		gebr_geoxml_object_ref(source);
	}

	hash_data_remove(self, name, t1);

	if (t1 != t2) {
		gebr_geoxml_object_ref(source);
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(source));
	}

	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(new_param),
					GEBR_GEOXML_SEQUENCE(pivot));

	gebr_validator_insert(self, new_param, NULL, error);

	*copy = new_param;

	if (*error)
		return FALSE;
	return TRUE;
}

gboolean
gebr_validator_validate_param(GebrValidator       *self,
			      GebrGeoXmlParameter *param,
			      gchar              **validated,
			      GError             **err)
{
	gchar *value;
	GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(param);
	GebrGeoXmlSequence *seq;
	GError *error = NULL;
	gchar *name = GET_VAR_NAME(param);

	// For dictionary we have cached errors, not cached yet for programs
	if (gebr_geoxml_parameter_is_dict_param(param)) {
		HashData *data = g_hash_table_lookup(self->vars, name);
		GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
		gebr_validator_update_vars(self, scope, NULL);
		if (!get_error_indirect(self, data->dep[scope], name, type, scope, err)) {
			g_free(name);
			return FALSE;
		}
		g_free(name);

		if (data->error[scope]) {
			g_propagate_error(err, g_error_copy(data->error[scope]));
			return FALSE;
		}
		return TRUE;
	}

	// Checks if the parameter is required or not
	if (!gebr_geoxml_program_parameter_has_value(GEBR_GEOXML_PROGRAM_PARAMETER(param))) {
		if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param))) {
			g_set_error(&error,
			            GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_EMPTY_EXPR,
			            _("This parameter is required"));
			g_propagate_error(err, error);
			g_free(name);
			return FALSE;
		}
		g_free(name);
		return TRUE;
	}
	GebrGeoXmlProgram *program = gebr_geoxml_parameter_get_program(param);
	if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		gebr_geoxml_object_unref(program);
		gchar *var_value = GET_VAR_VALUE(param);
		if (!gebr_validator_validate_control_parameter(self, name, var_value, err)) {
			g_free(name);
			g_free(var_value);
			return FALSE;
		}
		g_free(name);
		g_free(var_value);
		goto out;
	}
	g_free(name);
	gebr_geoxml_object_unref(program);

	gebr_validator_update_vars(self, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, NULL);

	gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
		if (!gebr_validator_validate_expr(self, value, type, err)) {
			gebr_geoxml_object_unref(seq);
			g_free(value);
			return FALSE;
		}
		g_free(value);
	}
out:
	if (validated)
		*validated = NULL;

	return TRUE;
}

gboolean
gebr_validator_validate_expr_on_scope(GebrValidator          *self,
                                      const gchar            *expression,
                                      GebrGeoXmlParameterType type,
                                      GebrGeoXmlDocumentType scope,
                                      GError                **err)
{
	return define_validate_and_extract_vars(self, NULL, expression, type, scope, NULL, err);
}

gboolean
gebr_validator_validate_expr(GebrValidator          *self,
			     const gchar            *str,
			     GebrGeoXmlParameterType type,
			     GError                **err)
{
	return gebr_validator_validate_expr_on_scope(self, str, type, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, err);
}

void gebr_validator_get_documents(GebrValidator *self,
				  GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj)
{
	if (flow)
		*flow = self->docs[0] ? *self->docs[0] : NULL;

	if (line)
		*line = self->docs[1] ? *self->docs[1] : NULL;

	if (proj)
		*proj = self->docs[2] ? *self->docs[2] : NULL;
}

static void
gebr_validator_clean_cache(GebrValidator *self)
{
	GebrGeoXmlSequence *seq;

	for (int i = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
		if (!cache_docs[i])
			continue;

		// Checks if cache and current doc are equal
		if (self->docs[i] && (cache_docs[i] == *self->docs[i]))
			continue;

		if (i == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) {
			g_hash_table_remove_all(self->vars);

			gebr_geoxml_document_unref(cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_PROJECT]);
			cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_PROJECT] = NULL;

			gebr_geoxml_document_unref(cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_LINE]);
			cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_LINE] = NULL;

			gebr_geoxml_document_unref(cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_FLOW]);
			cache_docs[GEBR_GEOXML_DOCUMENT_TYPE_FLOW] = NULL;
			break;
		} else {
			seq = gebr_geoxml_document_get_dict_parameter(cache_docs[i]);
			while (seq) {
				hash_data_remove(self, GET_VAR_NAME(GEBR_GEOXML_PARAMETER(seq)), i);
				gebr_geoxml_sequence_next(&seq);
			}
			gebr_geoxml_document_unref(cache_docs[i]);
			cache_docs[i] = NULL;
		}
	}
}

void gebr_validator_update(GebrValidator *self)
{
	GebrGeoXmlSequence *seq;

	gebr_validator_clean_cache(self);

	for (int i = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
		if (!self->docs[i] || !*(self->docs[i]) || *(self->docs[i]) == cache_docs[i])
			continue;

		seq = gebr_geoxml_document_get_dict_parameter(*(self->docs[i]));
		while (seq) {
			gebr_validator_insert(self, GEBR_GEOXML_PARAMETER(seq), NULL, NULL);
			gebr_geoxml_sequence_next(&seq);
		}
		cache_docs[i] = *(self->docs[i]);
		gebr_geoxml_document_ref(cache_docs[i]);
	}
}

void gebr_validator_force_update(GebrValidator *self)
{
	g_hash_table_remove_all(self->vars);

	for (int i = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
		GebrGeoXmlSequence *seq;

		if (!self->docs[i] || !*(self->docs[i]))
			continue;

		seq = gebr_geoxml_document_get_dict_parameter(*(self->docs[i]));
		while (seq) {
			gebr_validator_insert(self, GEBR_GEOXML_PARAMETER(seq), NULL, NULL);
			gebr_geoxml_sequence_next(&seq);
		}
		cache_docs[i] = *(self->docs[i]);
	}
}

void gebr_validator_free(GebrValidator *self)
{
	g_hash_table_unref(self->vars);
	g_object_unref(self->arith_expr);
	g_free(self);
}

static void
clean_string(gchar **str)
{
	int i, b = 0;
	if (!str || !*str) {
		return;
	}
	for (i = 0; i < strlen(*str); i++) {
		if ((*str)[i+b] == '\b')
			b++;
		(*str)[i-b] = (*str)[i+b];
	}
	(*str)[i-b] = '\0';
}

gboolean
gebr_validator_use_iter(GebrValidator *self,
			const gchar *expr,
			GebrGeoXmlParameterType type,
			GebrGeoXmlDocumentType scope)
{
	GList * deps = NULL;
	GebrGeoXmlParameter *iter;
	if (scope > GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		return FALSE;
	HashData *data = g_hash_table_lookup(self->vars, "iter");
	if (!data || !(iter = data->param[GEBR_GEOXML_DOCUMENT_TYPE_FLOW]))
		return FALSE;
	data->param[GEBR_GEOXML_DOCUMENT_TYPE_FLOW] = NULL;
	if (get_validator_by_type(self, type) == GEBR_IEXPR(self->arith_expr))
		deps = gebr_iexpr_extract_vars(GEBR_IEXPR(self->arith_expr), expr);
	else
		translate_string_expr(self, expr, NULL, scope, NULL, &deps, NULL);
	gboolean valid = get_error_indirect(self, deps, NULL, type, scope, NULL);
	data->param[GEBR_GEOXML_DOCUMENT_TYPE_FLOW] = iter;
	g_list_foreach(deps, (GFunc) g_free, NULL);
	g_list_free(deps);
	return !valid;
}

static gboolean gebr_validator_evaluate_internal(GebrValidator *self,
                                                 const gchar *name,
                                                 const gchar *expr,
                                                 GebrGeoXmlParameterType type,
                                                 gchar **value,
                                                 GebrGeoXmlDocumentType scope,
                                                 gboolean show_interval,
                                                 GError **error)
{
	gchar *end_value = NULL;
	gchar *ini_value = NULL;
	gchar *translated = NULL;
	GError *err = NULL;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FILE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_STRING,
			     FALSE);
	gboolean is_math = get_validator_by_type(self, type) == GEBR_IEXPR(self->arith_expr);
	gboolean use_iter = show_interval && gebr_validator_use_iter(self, expr, type, scope);

	if (!is_math) {
		translate_string_expr(self, expr, NULL, scope, &translated, NULL, NULL);
		expr = translated;
	}

	if (!gebr_arith_expr_eval_internal(self->arith_expr, expr, value ? &ini_value : NULL, &err))
		goto err;

	if (use_iter) {
		gchar *buf = g_strconcat(ITER_END_EXPR, expr, ITER_INI_EXPR, NULL);
		if (!gebr_arith_expr_eval_internal(self->arith_expr, buf, value ? &end_value : NULL, &err)) {
			g_free(buf);
			goto err;
		}
		g_free(buf);
		if (value) {
			*value = g_strdup_printf(is_math ? "[%s, ..., %s]" : "[\"%s\", ..., \"%s\"]", ini_value, end_value);
			g_free(ini_value);
			g_free(end_value);
		}
	} else if (value)
		*value = ini_value;

	if (!is_math) {
		clean_string(value);
		g_free(translated);
	}

	set_error(self, name, scope, NULL);
	return TRUE;
err:
	if (err->code >= GEBR_IEXPR_ERROR_EMPTY_EXPR && err->code <= GEBR_IEXPR_ERROR_SYNTAX) {
		set_error(self, name, scope, err);
	} else {
		set_error(self, name, scope, NULL);
	}
	g_propagate_error(error, err);
	g_free(ini_value);
	g_free(end_value);
	return FALSE;
}

gboolean gebr_validator_evaluate_interval(GebrValidator *self,
                                          const gchar *expr,
                                          GebrGeoXmlParameterType type,
                                          GebrGeoXmlDocumentType scope,
                                          gboolean show_interval,
                                          gchar **value,
                                          GError **error)
{
	g_return_val_if_fail(expr != NULL, FALSE);

	if (!strlen(expr)) {
		*value = g_strdup("");
		return TRUE;
	}

	if(!gebr_validator_update_vars(self, scope, error))
		return FALSE;

	if (!gebr_validator_validate_expr_on_scope(self, expr, type, scope, error))
		return FALSE;

	return gebr_validator_evaluate_internal(self, NULL, expr, type, value, scope, show_interval, error);
}

gboolean gebr_validator_evaluate(GebrValidator *self,
                                 const gchar *expr,
                                 GebrGeoXmlParameterType type,
                                 GebrGeoXmlDocumentType scope,
                                 gchar **value,
                                 GError **error)
{
	return gebr_validator_evaluate_interval(self, expr, type, scope, TRUE, value, error);
}

gboolean gebr_validator_evaluate_param(GebrValidator *self,
                                       GebrGeoXmlParameter *param,
                                       gchar **value,
                                       GError **error)
{
	g_return_val_if_fail(param != NULL && gebr_geoxml_parameter_is_dict_param(param), FALSE);
	GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(param);
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	const gchar *name = GET_VAR_NAME(param);
	const gchar *expr = GET_VAR_VALUE(param);

	HashData *data = g_hash_table_lookup(self->vars, name);
	g_return_val_if_fail(data != NULL , FALSE);

	if (!get_error_indirect(self, data->dep[scope], name, type, scope, error))
		return FALSE;

	if (g_strcmp0(name, "iter") == 0 && !gebr_validator_validate_iter(self, param, error))
		return FALSE;

	if(!gebr_validator_update_vars(self, scope, error))
		return FALSE;

	if (data->error[scope]) {
		g_propagate_error(error, g_error_copy(data->error[scope]));
		return FALSE;
	}

	// Numeric expressions on dictionary must be just fetched by name
	if (get_validator_by_type(self, type) == GEBR_IEXPR(self->arith_expr))
		expr = name;

	return gebr_validator_evaluate_internal(self, name, expr, type, value, scope, TRUE, error);
}

gboolean
gebr_validator_is_var_in_scope(GebrValidator *self,
			       const gchar *name,
			       GebrGeoXmlDocumentType scope)
{
	HashData *data;
	g_return_val_if_fail(scope < GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN, FALSE);

	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return FALSE;

	return data->param[scope] != NULL;
}

void
gebr_validator_set_document(GebrValidator *self,
                            GebrGeoXmlDocument **doc,
			    GebrGeoXmlDocumentType type,
			    gboolean force)
{
	self->docs[type] = doc;
	gebr_validator_update(self);
}

gboolean
gebr_validator_validate_control_parameter(GebrValidator *self,
                                          const gchar *name,
                                          const gchar *expression,
                                          GError **error)
{
	GError *err = NULL;
	gchar *result = NULL;
	int result_int;
	gdouble result_d;

	if (!gebr_validator_evaluate(self, expression, GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                        GEBR_GEOXML_DOCUMENT_TYPE_LINE, &result, &err)) {
		g_propagate_error(error, err);
		return FALSE;
	}

	if (name && !g_strcmp0(name, "niter")) {
		if (!strlen(expression)) {
			g_set_error(error, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_EMPTY_EXPR,
			            _("Parameter required"));
			g_free(result);
			return FALSE;
		}

		result_int = atoi(result);
		result_d = g_ascii_strtod(result, NULL);
		if (result_d > result_int) {
			g_set_error(error,
			            GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_SYNTAX,
			            _("Accepts only integer values"));
			g_free(result);
			return FALSE;
		}
		if (result_int <= 0) {
			g_set_error(error,
			            GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_SYNTAX,
			            _("Accepts only positive values"));
			g_free(result);
			return FALSE;
		}
	}
	g_free(result);

	return TRUE;
}
static gboolean
gebr_validator_validate_iter(GebrValidator *self,
                             GebrGeoXmlParameter *param,
                             GError **error)
{
	GError *err = NULL;
	const gchar *labels[3] = {"Initial value", "Step", "Total number of steps"};
	int i = 0;
	const gchar *name = NULL;
	GebrGeoXmlSequence *seq;
	const gchar *value;
	HashData *data;

	data = g_hash_table_lookup(self->vars, "iter");
	g_return_val_if_fail(data != NULL, FALSE);

	if (!define_validate_and_extract_vars(self, NULL, GET_VAR_VALUE(param),
	                               GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                               GEBR_GEOXML_DOCUMENT_TYPE_LINE,
	                               &data->dep[GEBR_GEOXML_DOCUMENT_TYPE_FLOW], error))
		return FALSE;

	gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, &seq, 1);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		if (i == 2)
			name = g_strdup("niter");
		value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
		if (!gebr_validator_validate_control_parameter(self, name, value, &err)) {
			gebr_geoxml_object_unref(seq);
			break;
		}
		i++;
	}

	/* Comment for translators: 1st %s is expression error; 2nd is variable label */
	if (err) {
		gchar *old_msg = (err)->message;
		(err)->message = g_strdup_printf(_("%s on \"%s\""), old_msg, labels[i]);
		g_free(old_msg);
		if (err->code >= GEBR_IEXPR_ERROR_EMPTY_EXPR && err->code <= GEBR_IEXPR_ERROR_SYNTAX)
			set_error(self, "iter", GEBR_GEOXML_DOCUMENT_TYPE_FLOW, err);
		g_propagate_error(error, err);
		return FALSE;
	}

	return TRUE;
}
