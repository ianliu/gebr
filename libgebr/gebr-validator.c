#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <geoxml/geoxml.h>
#include <glib/gprintf.h>

#include "gebr-validator.h"
#include "utils.h"
#include "gebr-iexpr.h"
#include "gebr-arith-expr.h"
#include "gebr-string-expr.h"
#include "gebr-expr.h"

/* Structures {{{1 */
struct _GebrValidator
{
	GebrArithExpr *arith_expr;
	GebrStringExpr *string_expr;

	GebrGeoXmlDocument **docs[3];

	GHashTable *vars;
};

typedef struct {
	gchar *name;
	GebrGeoXmlParameter *param[4];
	gdouble weight[4];
	GList *dep[4];
	GError *error[4];
} HashData;


#define ITER_INI_EXPR ";iter=bc_reset(0);"
#define ITER_END_EXPR ";iter=bc_reset(1);"

#define GET_VAR_NAME(p) (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p)))
#define GET_VAR_VALUE(p) (gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE))
#define SET_VAR_VALUE(p,v) (gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE, (v)))
#define SET_VAR_NAME(p,n) (gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p), (n)))

/* Prototypes {{{1 */
static void		set_error		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope,
						 GError                *error);

static gboolean		get_error_indirect	(GebrValidator          *self,
               		                  	 GList                  *var_names,
               		                  	 const gchar		*name,
               		                  	 GebrGeoXmlParameterType my_type,
               		                  	 GebrGeoXmlDocumentType scope,
               		                  	 GError                **err);

static GebrGeoXmlParameter * get_param		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope);

static GebrGeoXmlParameter * get_dep_param      (GebrValidator *self,
                                                 const gchar *my_name,
                                                 GebrGeoXmlDocumentType my_scope,
                                                 const gchar *dep_name);

static gdouble 		get_weight		(GebrValidator *self,
               		          		 GebrGeoXmlParameter *param);

/* NodeData functions {{{1 */
static HashData *
hash_data_new_from_xml(GebrGeoXmlParameter *param)
{
	HashData *n = g_new0(HashData, 1);
	GebrGeoXmlDocumentType type;
	type = gebr_geoxml_parameter_get_scope(param);
	n->param[type] = param;
	n->name = g_strdup(GET_VAR_NAME(param));
	n->weight[type] = G_MAXDOUBLE;
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

	data = g_hash_table_lookup(self->vars, name);
	g_return_val_if_fail(data != NULL, FALSE);

	if (!data->param[scope]) {
		return FALSE;
	}

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
		return GEBR_IEXPR(self->string_expr);
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
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
			            _("Variable %s is not defined"),
			            dep_name);
			return FALSE;
		}

		dep_param = get_dep_param(self, my_name, my_scope, dep_name);
		if (!dep_param) {
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

		if (!dep_data->error[dep_scope]) {
			get_error_indirect(self, dep_data->dep[dep_scope], dep_name, dep_type, dep_scope, &error);
		}

		if (dep_data->error[dep_scope] || error) {
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

/* Validate @expression and extract vars on @deps with @error */
static gboolean
validate_and_extract_vars(GebrValidator  *self,
                          const gchar 		*expression,
                          GebrGeoXmlParameterType type,
                          GList 		 **deps,
                          GError                **error)
{
	GebrIExpr *iexpr;
	GError *err = NULL;

	iexpr = get_validator_by_type(self, type);

	if (!iexpr || !*expression) {
		*deps = NULL;
		return TRUE;
	}
	if (!gebr_iexpr_is_valid(iexpr, expression, &err)
	    && err->code != GEBR_IEXPR_ERROR_UNDEF_VAR) {
		g_propagate_error(error, err);
		return FALSE;
	}
	if (err)
		g_clear_error(&err);

	*deps = gebr_iexpr_extract_vars(iexpr, expression);

	return TRUE;
}

static gboolean
gebr_validator_change_value_by_name(GebrValidator          *self,
				    const gchar            *name,
				    GebrGeoXmlDocumentType  scope,
				    GebrGeoXmlParameterType type,
				    const gchar            *new_value,
				    GList                 **affected,
				    GError                **error)
{
	HashData *data;
	GError *err = NULL;
	gboolean valid;

	if (affected)
		*affected = NULL;

	data = g_hash_table_lookup(self->vars, name);

	g_return_val_if_fail(data != NULL, FALSE);

	valid = validate_and_extract_vars(self, new_value, type, &data->dep[scope], &err);

	if (valid) {
		set_error(self, name, scope, NULL);

		valid = get_error_indirect(self, data->dep[scope], name, type, scope, error);
	} else {
		set_error(self, name, scope, err);
		g_propagate_error(error, err);
	}
	return valid;
}

static void
get_iter_bounds(GebrGeoXmlParameter *iter_param, const gchar **ini, const gchar **step, const gchar **n)
{
	GebrGeoXmlSequence *seq;

	gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(iter_param), FALSE, &seq, 1);
	*ini = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));

	gebr_geoxml_sequence_next(&seq);
	*step = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));

	gebr_geoxml_sequence_next(&seq);
	*n = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
}

/* Public functions {{{1 */
GebrValidator *
gebr_validator_new(GebrGeoXmlDocument **flow,
		   GebrGeoXmlDocument **line,
		   GebrGeoXmlDocument **proj)
{
	GebrValidator *self = g_new(GebrValidator, 1);
	self->arith_expr = gebr_arith_expr_new();
	self->string_expr = gebr_string_expr_new();
	self->docs[0] = flow;
	self->docs[1] = line;
	self->docs[2] = proj;

	self->vars = g_hash_table_new_full(g_str_hash,
					   g_str_equal,
					   g_free,
					   hash_data_free);

	return self;
}

static gdouble
get_weight(GebrValidator *self,
           GebrGeoXmlParameter *param)
{
	HashData *data;
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);

	data = g_hash_table_lookup(self->vars, GET_VAR_NAME(param));

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
	const gchar *name;
	GebrGeoXmlSequence *prev_param;
	GebrGeoXmlSequence *next_param;
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	HashData *data;

	name = GET_VAR_NAME(param);
	g_return_val_if_fail(name != NULL && strlen(name), FALSE);
	data = g_hash_table_lookup(self->vars, name);

	if (!data) {
		data = hash_data_new_from_xml(param);
		g_hash_table_insert(self->vars, g_strdup(name), data);
	} else
		if (!data->param[scope])
			data->param[scope] = param;

	prev_param = GEBR_GEOXML_SEQUENCE(param);
	next_param = GEBR_GEOXML_SEQUENCE(param);
	gebr_geoxml_sequence_previous(&prev_param);
	gebr_geoxml_sequence_next(&next_param);
	data->weight[scope] = compute_weight(self,
					     GEBR_GEOXML_PARAMETER(prev_param),
					     GEBR_GEOXML_PARAMETER(next_param));

	return gebr_validator_change_value(self, param, GET_VAR_VALUE(param), affected, error);
}

gboolean
gebr_validator_remove(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError		 **error)
{
	const gchar *name;
	GebrGeoXmlDocumentType scope;
	gboolean removed;

	name = GET_VAR_NAME(param);
	scope = gebr_geoxml_parameter_get_scope (param);

	removed = hash_data_remove(self, name, scope);
	if (removed)
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(param));

	return removed;
}

gboolean
gebr_validator_rename(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      const gchar         *new_name,
		      GList              **affected,
		      GError             **error)
{
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
	gboolean retval;
	const gchar *name;
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
		return FALSE;
	}

	retval = gebr_validator_change_value_by_name(self, name, scope, type,
						     new_value, affected, error);

	SET_VAR_VALUE(param, new_value);

	return retval;
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
	HashData *data;
	GebrGeoXmlParameter *new_param;
	GebrGeoXmlParameterType type;
	GebrGeoXmlDocumentType t1, t2;

	name = GET_VAR_NAME(source);
	data = g_hash_table_lookup(self->vars, name);

	g_return_val_if_fail(data != NULL, FALSE);

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
			gebr_validator_insert(self, new_param, NULL, &err1);
		}
	} else
		new_param = source;

	hash_data_remove(self, name, t1);

	if (t1 != t2)
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(source));

	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(new_param),
					GEBR_GEOXML_SEQUENCE(pivot));

	gebr_validator_insert(self, new_param, NULL, error);

	*copy = new_param;

	if (*error)
		return FALSE;
	return TRUE;
}

gboolean
gebr_validator_check_using_var(GebrValidator *self,
                               const gchar   *source,
			       GebrGeoXmlDocumentType scope,
                               const gchar   *var)
{
	HashData *data;

	if (g_strcmp0(source, var) == 0)
		return TRUE;

	data = g_hash_table_lookup(self->vars, source);

	g_return_val_if_fail(data != NULL, FALSE);

	for (GList *i = data->dep[scope]; i; i = i->next) {
		if (g_strcmp0(i->data, var) == 0)
			return TRUE;

		if (gebr_validator_check_using_var(self, i->data, scope, var))
			return TRUE;
	}

	return FALSE;
}

/* Works only for strings until now */
gboolean
gebr_validator_expression_check_using_var(GebrValidator *self,
                               const gchar   *expr,
			       GebrGeoXmlDocumentType scope,
                               const gchar   *var)
{
	g_return_val_if_fail(scope == GEBR_GEOXML_DOCUMENT_TYPE_FLOW
			     || scope == GEBR_GEOXML_DOCUMENT_TYPE_LINE
			     || scope == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT,
			     FALSE);

	g_return_val_if_fail(var != NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);

	g_return_val_if_fail(gebr_validator_validate_expr(self,
							  expr,
							  GEBR_GEOXML_PARAMETER_TYPE_STRING,
							  NULL) == TRUE,
			     FALSE);

	if (expr == NULL)
		return FALSE;

	GList *vars = NULL;
	GebrIExpr *iexpr;
	gboolean retval = FALSE;

	iexpr = get_validator_by_type(self, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	vars = gebr_iexpr_extract_vars(iexpr, expr);

	for (GList * i = vars; i; i = i->next)
	{
		retval = gebr_validator_check_using_var(self, i->data, scope, var);
		if(retval)
			break;
	}

	if(vars)
	{
		g_list_foreach(vars, (GFunc)g_free, NULL);
		g_list_free(vars);
	}

	return retval;
}

gboolean
gebr_validator_validate_param(GebrValidator       *self,
			      GebrGeoXmlParameter *param,
			      gchar              **validated,
			      GError             **err)
{
	const gchar *value;
	GebrGeoXmlParameterType type;
	GebrGeoXmlSequence *seq;
	GError *error = NULL;

	// Checks if the parameter is required or not
	if (!gebr_geoxml_program_parameter_has_value(GEBR_GEOXML_PROGRAM_PARAMETER(param))) {
		if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param))) {
			g_set_error(&error,
			            GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_EMPTY_EXPR,
			            _("This parameter is required"));
			if (gebr_geoxml_parameter_is_dict_param(param))
				set_error(self, GET_VAR_NAME(param), gebr_geoxml_parameter_get_scope(param), error);
			g_propagate_error(err, error);
			return FALSE;
		}
		return TRUE;
	}

	type = gebr_geoxml_parameter_get_type(param);

	// For dictionary we have cached errors, not cached yet for programs
	if (gebr_geoxml_parameter_is_dict_param(param)) {
		HashData *data;
		const gchar *name;
		GebrGeoXmlDocumentType scope;

		scope = gebr_geoxml_parameter_get_scope(param);
		name = GET_VAR_NAME(param);
		data = g_hash_table_lookup(self->vars, name);

		g_return_val_if_fail(data != NULL, FALSE);

		if (data->error[scope]) {
			g_propagate_error(err, g_error_copy(data->error[scope]));
			return FALSE;
		} else
			return get_error_indirect(self, data->dep[scope], name, type, scope, err);

		return TRUE;
	}

	gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq))
	{
		value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
		if (gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(param)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			if (!gebr_validator_validate_expr_on_scope(self, value, type, GEBR_GEOXML_DOCUMENT_TYPE_LINE, err))
				return FALSE;
		} else if (!gebr_validator_validate_expr(self, value, type, err))
			return FALSE;
	}

	if (validated)
		*validated = g_strdup(value);

	return TRUE;
}

gboolean
gebr_validator_validate_expr_on_scope(GebrValidator          *self,
                                      const gchar            *str,
                                      GebrGeoXmlParameterType type,
                                      GebrGeoXmlDocumentType scope,
                                      GError                **err)
{
	GList *vars = NULL;
	gboolean valid = FALSE;

	valid = validate_and_extract_vars(self, str, type, &vars, err);

	if (valid) {
		valid = get_error_indirect(self, vars, NULL, type, scope, err);

		g_list_foreach(vars, (GFunc)g_free, NULL);
		g_list_free(vars);
	}

	return valid;
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
	if (self->docs[0])
		*flow = *self->docs[0];
	else
		*flow = NULL;
		
	if (self->docs[1])
		*line = *self->docs[1];
	else
		*line = NULL;
		
	if (self->docs[2])
		*proj = *self->docs[2];
	else
		*proj = NULL;

}

void gebr_validator_update(GebrValidator *self)
{
	static GebrGeoXmlDocument *last[] = { NULL, NULL, NULL};
	GebrGeoXmlSequence *seq;

	for (int i = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
		if (!self->docs[i] || !*(self->docs[i]) || *(self->docs[i]) == last[i])
			continue;

		if (i == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) {
			g_hash_table_remove_all(self->vars);
			last[GEBR_GEOXML_DOCUMENT_TYPE_LINE] = NULL;
			last[GEBR_GEOXML_DOCUMENT_TYPE_FLOW] = NULL;
		} else if (last[i]) {
			seq = gebr_geoxml_document_get_dict_parameter(last[i]);
			while (seq) {
				hash_data_remove(self, GET_VAR_NAME(GEBR_GEOXML_PARAMETER(seq)), i);
				gebr_geoxml_sequence_next(&seq);
			}
		}

		seq = gebr_geoxml_document_get_dict_parameter(*(self->docs[i]));
		while (seq) {
			gebr_validator_insert(self, GEBR_GEOXML_PARAMETER(seq), NULL, NULL);
			gebr_geoxml_sequence_next(&seq);
		}
		last[i] = *(self->docs[i]);
	}
}

void gebr_validator_free(GebrValidator *self)
{
	g_hash_table_unref(self->vars);
	g_object_unref(self->arith_expr);
	g_object_unref(self->string_expr);
	g_free(self);
}

/* Validate @expression and extract vars on @deps with @error */
static GString *
translate_string_expr(GebrValidator  	*self,
                      const gchar 	*expr,
                      const gchar  	 *my_name,
                      GebrGeoXmlDocumentType my_scope)
{
	gchar *var_name = g_new(gchar, strlen(expr));;
	GString *str_expr =  g_string_sized_new(128);
	enum {
		INIT,
		START,
		TEXT,
		VAR,
	} state = INIT;

	while (*expr) {
		switch (state) {
		case INIT:
			g_string_append(str_expr, "print ");
		case START:
			if (*expr == '[') {
				state = VAR;
				break;
			}
			g_string_append_c(str_expr, '"');
			g_string_append_c(str_expr, *expr);
			state = TEXT;
			break;
		case TEXT:
			if (*expr == '[') {
				g_string_append(str_expr, "\",");
				state = VAR;
				break;
			} else if (*expr == '\\') {
				g_string_append(str_expr, "\\\\");
				break;
			}
			g_string_append_c(str_expr, *expr);
			break;
		case VAR: {
			int size = 0;
			while (*expr && *expr != ']')
				var_name[size++] = *(expr++);
			var_name[size] = '\0';
			GebrGeoXmlParameter *var_param = get_dep_param(self, my_name, my_scope, var_name);
			GebrGeoXmlDocumentType var_scope = gebr_geoxml_parameter_get_scope(var_param);
			GebrGeoXmlParameterType var_type = gebr_geoxml_parameter_get_type(var_param);
			g_string_append_printf(str_expr, var_type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "str(%s[%d]),\"\\b\"," : "%s[%d],", var_name, var_scope);
			state = START;
			break;
		} default:
			g_warn_if_reached();
		}
		expr++;
	}
	switch (state) {
	case TEXT:
		g_string_append_c(str_expr, '"');
		break;
	case START:
		g_string_set_size(str_expr, str_expr->len - 1);
		break;
	default:
		break;
	}

	return str_expr;
}

static void
gebr_validator_update_vars(GebrValidator *self,
                           GebrGeoXmlDocumentType param_scope)
{
	GString *bc_vars =  g_string_sized_new(1024);
	GString *bc_strings =  g_string_sized_new(2*1024);
	GebrIExpr *iexpr = get_validator_by_type(self, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	const gchar *ini, *step, *n = NULL;

	g_string_append(bc_strings, "define str(n) { if (n==0) \"\"");

	int nth = 0;
	g_string_append(bc_vars, "define bc_reset(iter) {\nscale=5\n");
	for (int scope = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; scope >= (int) param_scope; scope--) {
		if (!self->docs[scope] || !*(self->docs[scope]))
			continue;
		GebrGeoXmlSequence *param = gebr_geoxml_document_get_dict_parameter(*self->docs[scope]);
		for (; param; gebr_geoxml_sequence_next(&param)) {
			const gchar* name = GET_VAR_NAME(param);
			GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));

			HashData *data = g_hash_table_lookup(self->vars, name);
			if (!data || data->error[scope] || !get_error_indirect(self, data->dep[scope], name, type, scope, NULL))
				continue;

			const gchar* value = GET_VAR_VALUE(param);
			if (!strlen(value))
				continue;

			if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING) {
				GString *translate = translate_string_expr(self, value, name, scope);
				g_string_append_printf(bc_vars, "%1$s=%1$s[%2$d]=%3$d\n", name, scope, ++nth);
				g_string_append_printf(bc_strings, " else if (n==%d) %s", nth, translate->str);
				g_string_free(translate, TRUE);
				continue;
			}
			if (g_strcmp0(name, "iter") == 0) {
				get_iter_bounds(GEBR_GEOXML_PARAMETER(param), &ini, &step, &n);
				g_string_append(bc_vars, "iter=iter[0]=iter(iter)\n");
			} else {
				g_string_append_printf(bc_vars, "%1$s=%1$s[%2$d]=(%3$s)\n", name, scope, value);
			}
		}
	}
	g_string_append(bc_vars, "return iter }\n");

	g_string_append(bc_strings, " }\n");

	if (n)
		g_string_append_printf(bc_vars, "define iter(end){if(end){return iter=iter[0]=(%s)+(%s*(%s-1))}else{return iter=iter[0]=%s}}\n", ini, step, n, ini);
	else
		g_string_append(bc_vars, "define iter(end){}\n");
	g_string_append(bc_vars, ITER_INI_EXPR);

	g_string_append(bc_strings, bc_vars->str);
	printf("%s\n", bc_strings->str);
	iexpr = get_validator_by_type(self, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
	gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), bc_strings->str, NULL, NULL);

	g_string_free(bc_vars, TRUE);
	g_string_free(bc_strings, TRUE);
}

static void
clean_string(gchar **str)
{
	int i, b = 0;
	if (!*str) {
		return;
	}
	for (i = 0; (*str)[i+b]; i++) {
		if ((*str)[i+b] == '\b')
			b++;
		(*str)[i-b] = (*str)[i+b];
	}
	if ((*str)[i] == '\b')
		b+=1;
	(*str)[i-b] = '\0';
}

gboolean gebr_validator_use_iter(GebrValidator *self, const gchar *expr, GebrGeoXmlParameterType type, GebrGeoXmlDocumentType scope) {
	GList * deps;
	GebrGeoXmlParameter *iter;
	HashData *data = g_hash_table_lookup(self->vars, "iter");
	if (scope > GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		return FALSE;
	if (!data || !(iter = data->param[0]))
		return FALSE;
	data->param[0] = NULL;
	deps = gebr_iexpr_extract_vars(get_validator_by_type(self, type), expr);
	gboolean valid = get_error_indirect(self, deps, NULL, type, scope, NULL);
	data->param[0] = iter;
	g_list_foreach(deps, (GFunc) g_free, NULL);
	g_list_free(deps);
	return !valid;
}

gboolean gebr_validator_evaluate_scope(GebrValidator *self,
                                       const gchar * expr,
                                       GebrGeoXmlParameterType type,
                                       GebrGeoXmlDocumentType scope,
                                       gchar **value,
                                       GError **error)
{
	GebrIExpr *iexpr;
	GError *err = NULL;

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
	                     || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
	                     || type == GEBR_GEOXML_PARAMETER_TYPE_FILE
	                     || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE
	                     || type == GEBR_GEOXML_PARAMETER_TYPE_STRING,
	                     FALSE);

	g_return_val_if_fail(expr != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);


	if (!gebr_validator_validate_expr_on_scope(self, expr, type, scope, error))
		return FALSE;

	if (!strlen(expr)) {
		*value = g_strdup("");
		return TRUE;
	}

	gebr_validator_update_vars(self, scope);
	iexpr = get_validator_by_type(self, type);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING || type == GEBR_GEOXML_PARAMETER_TYPE_FILE) {

		GString *expression = translate_string_expr(self, expr, NULL, scope);
		g_string_append(expression, ",\"\\n\"");

		iexpr = get_validator_by_type(self, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);

		gchar *ini_value = NULL;
		gchar *end_value = NULL;

		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), expression->str, &ini_value, &err);
		clean_string(&ini_value);

		if (!gebr_validator_use_iter(self, expr, type, scope)) {
			*value = ini_value;
			g_string_free(expression, TRUE);

			if (err) g_propagate_error(error, err);
			return !err;
		}

		gchar *buf = g_strconcat(ITER_END_EXPR, expression->str, ITER_INI_EXPR, NULL);
		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), buf, &end_value, &err);
		clean_string(&end_value);
		*value = g_strdup_printf("[\"%s\", ..., \"%s\"]", ini_value, end_value);
		g_free(buf);
		g_free(ini_value);
		g_free(end_value);
		g_string_free(expression, TRUE);
	} else {
		gchar *ini_value = NULL;
		gchar *end_value = NULL;

		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), expr, &ini_value, &err);

		if (!gebr_validator_use_iter(self, expr, type, scope)) {
			*value = ini_value;
			if (err) g_propagate_error(error, err);
			return !err;
		}
		gchar *buf = g_strconcat(ITER_END_EXPR, expr, ITER_INI_EXPR, NULL);
		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), buf, &end_value, &err);
		*value = g_strdup_printf("[%s, ..., %s]", ini_value, end_value);
		g_free(buf);
		g_free(ini_value);
		g_free(end_value);
	}

	if (err) g_propagate_error(error, err);
	return !err;
}

gboolean gebr_validator_evaluate(GebrValidator *self,
                                 GebrGeoXmlParameter *my_param,
                                 const gchar * expr,
                                 GebrGeoXmlParameterType type,
                                 gchar **value,
                                 GError **error)
{
	GebrIExpr *iexpr;
	GebrGeoXmlDocumentType scope = GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	const gchar *name = NULL;
	GError *err = NULL;
	gboolean is_iter = FALSE;

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FILE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_STRING,
			     FALSE);

	g_return_val_if_fail(expr != NULL || my_param != NULL , FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);

	if (!expr) {
		expr = GET_VAR_VALUE(my_param);
		type = gebr_geoxml_parameter_get_type(my_param);
		scope = gebr_geoxml_parameter_get_scope(my_param);
		name = GET_VAR_NAME(my_param);
	}

	if (my_param) {
		if (!gebr_validator_validate_param(self, my_param, NULL, error))
			return FALSE;
	} else if (!gebr_validator_validate_expr(self, expr, type, error))
		return FALSE;

	if (!strlen(expr)) {
		*value = g_strdup("");
		return TRUE;
	}

	is_iter = gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(my_param)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR;
	if (is_iter)
		scope = GEBR_GEOXML_DOCUMENT_TYPE_LINE;
	gebr_validator_update_vars(self, scope);
	iexpr = get_validator_by_type(self, type);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING || type == GEBR_GEOXML_PARAMETER_TYPE_FILE) {

		GString *expression = translate_string_expr(self, expr, name, scope);
		g_string_append(expression, ",\"\\n\"");

		iexpr = get_validator_by_type(self, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);

		gchar *ini_value = NULL;
		gchar *end_value = NULL;

		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), expression->str, &ini_value, &err);
		clean_string(&ini_value);

		if (!gebr_validator_use_iter(self, expr, type, scope)) {
			*value = ini_value;
			g_string_free(expression, TRUE);

			if (err) g_propagate_error(error, err);
			return !err;
		}

		gchar *buf = g_strconcat(ITER_END_EXPR, expression->str, ITER_INI_EXPR, NULL);
		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), buf, &end_value, &err);
		clean_string(&end_value);
		*value = g_strdup_printf("[\"%s\", ..., \"%s\"]", ini_value, end_value);
		g_free(buf);
		g_free(ini_value);
		g_free(end_value);
		g_string_free(expression, TRUE);
	} else {
		gchar *ini_value = NULL;
		gchar *end_value = NULL;

		if (my_param && !is_iter)
			expr = GET_VAR_NAME(my_param);

		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), expr, &ini_value, &err);

		if (!gebr_validator_use_iter(self, expr, type, scope)) {
			*value = ini_value;
			if (err) g_propagate_error(error, err);
			return !err;
		}
		gchar *buf = g_strconcat(ITER_END_EXPR, expr, ITER_INI_EXPR, NULL);
		gebr_arith_expr_eval_internal(GEBR_ARITH_EXPR(iexpr), buf, &end_value, &err);
		*value = g_strdup_printf("[%s, ..., %s]", ini_value, end_value);
		g_free(buf);
		g_free(ini_value);
		g_free(end_value);
	}

	if (err) g_propagate_error(error, err);
	return !err;
}

gboolean
gebr_validator_is_var_in_scope(GebrValidator *self,
			       const gchar *name,
			       GebrGeoXmlDocumentType scope)
{
	HashData *data;

	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return FALSE;

	return data->param[scope] != NULL;
}

void
gebr_validator_set_document(GebrValidator *self,
                            GebrGeoXmlDocument **doc)
{
	GebrGeoXmlDocumentType type;
	type = gebr_geoxml_document_get_type(*doc);

	self->docs[type] = doc;

	gebr_validator_update(self);
}


