#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <geoxml/geoxml.h>

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
	GList *var_order[3];
};

typedef struct {
	GebrGeoXmlParameter *param[4];
	gdouble weight[4];
	gchar *name;
	GList *dep[4];
	GList *antidep;
	GError *error[4];
} HashData;

#define GET_VAR_NAME(p) (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p)))
#define GET_VAR_VALUE(p) (gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE))
#define SET_VAR_VALUE(p,v) (gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE, (v)))
#define SET_VAR_NAME(p,n) (gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p), (n)))

#define GET_VAR_DATA(param, data) \
	G_STMT_START { \
		data = g_hash_table_lookup(self->vars, GET_VAR_NAME(param)); \
		g_return_val_if_fail(data != NULL, FALSE); \
	} G_STMT_END


/* Prototypes {{{1 */
static void		set_error		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope,
						 GError                *error);

static gboolean		get_error_indirect	(GebrValidator          *self,
               		                  	 GList                  *var_names,
               		                  	 GebrGeoXmlParameterType my_type,
               		                  	 GebrGeoXmlDocumentType scope,
               		                  	 GError                **err);

static GebrGeoXmlParameter * get_param		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope);

static GebrGeoXmlDocumentType get_scope		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope);

static const gchar *	get_value		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope);

static gboolean         detect_cicle	        (GebrValidator *self,
                                    	         const gchar *name,
                                    	         GebrGeoXmlDocumentType scope);

static gdouble 		get_weigth		(GebrValidator *self,
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
	return n;
}

static HashData *
hash_data_new(const gchar *name)
{
	HashData *n = g_new0(HashData, 1);
	n->name = g_strdup(name);
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
	g_list_foreach(n->antidep, (GFunc) g_free, NULL);
	g_list_free(n->antidep);
	g_free(n->name);
	g_free(n);
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
                   GList *var_names,
                   GebrGeoXmlParameterType my_type,
                   GebrGeoXmlDocumentType scope,
                   GError **err)
{
	HashData *dep_data;
	GError *error = NULL;
	GebrGeoXmlParameterType dep_type;
	GebrGeoXmlDocumentType dep_scope;

//	if (!get_validator_by_type(self, my_type))
//		return TRUE;

	for (GList *i = var_names; i; i = i->next) {
		gchar *dep_name = i->data;

		dep_data = g_hash_table_lookup(self->vars, dep_name);

		if (!dep_data || !get_param(self, dep_name, scope)) {
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_UNDEF_REFERENCE,
			            _("Variable %s is not defined"),
			            dep_name);
			return FALSE;
		}

		if (detect_cicle(self, dep_name, scope)) {
			g_set_error(&error, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_CYCLE,
			            _("The variable %s has cyclic dependency"),
			            dep_name);
			g_propagate_error(err, error);
			return FALSE;
		}

		if (!dep_data->param[scope])
			dep_scope = get_scope(self, dep_name, scope);
		else
			dep_scope = scope;

		if (!dep_data->error[dep_scope]) {
			dep_type = gebr_geoxml_parameter_get_type(dep_data->param[dep_scope]);
			get_error_indirect(self, dep_data->dep[dep_scope], dep_type, dep_scope, &error);
		}
		else
			g_propagate_error(&error, g_error_copy(dep_data->error[dep_scope]));

		if (error) {
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_BAD_REFERENCE,
			            _("Variable %s is not well defined"),
			            dep_name);
			return FALSE;
		}
		else if (my_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT && dep_data->param[dep_scope] &&
		    gebr_geoxml_parameter_get_type(dep_data->param[dep_scope]) == GEBR_GEOXML_PARAMETER_TYPE_STRING) {
			g_set_error(err, GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_TYPE_MISMATCH,
			            _("Variable %s use different type"),
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
	} else
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

static GebrGeoXmlDocumentType
get_scope(GebrValidator *self, const gchar *name, GebrGeoXmlDocumentType scope)
{
	GebrGeoXmlParameter *param;

	param = get_param(self, name, scope);

	if (param == NULL)
		return GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN;

	return gebr_geoxml_parameter_get_scope(param);
}

static const gchar *
get_value(GebrValidator *self,
	  const gchar *name,
	  GebrGeoXmlDocumentType scope)
{
	GebrGeoXmlParameter *param;
	return (param = get_param(self, name, scope)) ? GET_VAR_VALUE(param):NULL;
}

static gboolean
detect_cicle(GebrValidator *self,
	     const gchar *name,
	     GebrGeoXmlDocumentType scope)
{
	GHashTable *visits;
	gboolean retval = FALSE;

	enum {
		WHITE,
		GREY,
		BLACK
	};

	visits = g_hash_table_new(g_str_hash, g_str_equal);

	gboolean dfs(const gchar *n)
	{
		gint c;
		const gchar *v;
		HashData *data;

		c = GPOINTER_TO_INT(g_hash_table_lookup(visits, n));

		if (c == GREY)
			return TRUE; // There is a loop

		if (c == BLACK)
			return FALSE; // Visiting someone already done

		g_hash_table_insert(visits, (gpointer)n, GUINT_TO_POINTER(GREY));
		data = g_hash_table_lookup(self->vars, n);
		for (GList *i = data->dep[scope]; i; i = i->next) {
			v = i->data;
			if (dfs(v))
				return TRUE;
		}
		g_hash_table_insert(visits, (gpointer)n, GUINT_TO_POINTER(BLACK));
		return FALSE;
	}

	retval = dfs(name);
	g_hash_table_unref(visits);
	return retval;
}

/* Remove the variable @var_name from antideps of it deps */
static gboolean
remove_from_antidep_of_deps(GebrValidator  *self,
                            GList	  **deps,
                            const gchar	   *var_name)
{
	HashData *dep;
	GList *node;

	for (; *deps; *deps = (*deps)->next) {
		gchar *dep_name = (*deps)->data;

		dep = g_hash_table_lookup(self->vars, dep_name);
		if (dep->antidep) {
			node = g_list_find_custom(dep->antidep, var_name, (GCompareFunc)g_strcmp0);
			g_free(node->data);
			dep->antidep = g_list_delete_link(dep->antidep, node);
		}
	}

	g_list_foreach(*deps, (GFunc)g_free, NULL);
	g_list_free(*deps);
	return TRUE;
}

/* Add the variable @var_name to antideps of it deps */
static void
add_to_antideps_of_deps(GebrValidator *self,
                        GList         *deps,
                        const gchar   *var_name)
{
	HashData *dep;

	for (; deps; deps = deps->next) {
		gchar *dep_name = deps->data;

		dep = g_hash_table_lookup(self->vars, dep_name);
		if (!dep) {
			dep = hash_data_new(dep_name);
			g_hash_table_insert(self->vars, g_strdup(dep_name), dep);
		}

		if (!g_list_find_custom(dep->antidep, var_name, (GCompareFunc)g_strcmp0))
			dep->antidep = g_list_prepend(dep->antidep, g_strdup(var_name));
	}
}

/* Validate @expression and extract vars on @deps with @error */
static gboolean
validate_and_extract_vars(GebrValidator  *self,
         const gchar 		*expression,
         GebrGeoXmlParameterType type,
         GList 		       **deps,
         GError                **error)
{
	GebrIExpr *iexpr;
	GError *err = NULL;

	iexpr = get_validator_by_type(self, type);

	if (!iexpr) {
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

	remove_from_antidep_of_deps(self, &data->dep[scope], name);

	valid = validate_and_extract_vars(self, new_value, type, &data->dep[scope], &err);

	add_to_antideps_of_deps (self, data->dep[scope], name);

	if (valid) {
		set_error(self, name, scope, NULL);

		valid = get_error_indirect(self, data->dep[scope], type, scope, error);
	} else {
		set_error(self, name, scope, err);
		g_propagate_error(error, err);
	}
	return valid;
}

static void
get_iter_bounds(GebrGeoXmlParameter *iter_param, gchar **_ini, gchar **_end)
{
	const gchar *ini;
	const gchar *step;
	const gchar *n;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlProgramParameter *prog;

	prog = GEBR_GEOXML_PROGRAM_PARAMETER(iter_param);

	gebr_geoxml_program_parameter_get_value(prog, FALSE, &seq, 1);
	ini = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));

	gebr_geoxml_sequence_next(&seq);
	step = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));

	gebr_geoxml_sequence_next(&seq);
	n = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));

	*_ini = g_strdup(ini);
	*_end = g_strdup_printf("%.10lf", g_strtod(ini,NULL) +
				g_strtod(step,NULL)*((gint)g_strtod(n,NULL)-1));
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

	self->var_order[0] = NULL;
	self->var_order[1] = NULL;
	self->var_order[2] = NULL;

	return self;
}

static GList*
get_prev_param(GebrValidator *self,
		GebrGeoXmlParameter *param)
{
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	GList *myself = g_list_find(self->var_order[scope], param);

	if (myself && myself->prev)
		return myself->prev;

//	if (scope != GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
//		scope++;
//
//	for (; scope < GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN; scope++) {
//		if (self->var_order[scope])
//			return g_list_last(self->var_order[scope]);
//	}
	return NULL;
}

static GList*
get_next_param(GebrValidator *self,
		GebrGeoXmlParameter *param)
{
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	GList *myself = g_list_find(self->var_order[scope], param);

	if (myself && myself->next)
		return myself->next;

//	if (scope != GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
//		scope--;
//
//	for (int i = scope; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
//		if (self->var_order[scope])
//			return g_list_first(self->var_order[scope]);
//	}
	return NULL;
}

static gdouble
get_weigth(GebrValidator *self,
           GebrGeoXmlParameter *param)
{
	HashData *data;
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);

	data = g_hash_table_lookup(self->vars, GET_VAR_NAME(param));

	g_return_val_if_fail(data != NULL, FALSE);

	//	GET_VAR_DATA(param, data);

	return data->weight[scope];
}

static gdouble
compute_weight(GebrValidator *self,
               GList *before,
               GList *after)
{
	gdouble b = (before != NULL)? get_weigth(self, before->data) : 0;
	gdouble a = (after != NULL)? get_weigth(self, after->data) : 100;

	return (a + b)/2;
}

gboolean
gebr_validator_insert(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError             **error)
{
	const gchar *name;
	GebrGeoXmlDocumentType scope = gebr_geoxml_parameter_get_scope(param);
	HashData *data;
	GList *before, *after;

	name = GET_VAR_NAME(param);
	g_return_val_if_fail(name != NULL && strlen(name), FALSE);
	data = g_hash_table_lookup(self->vars, name);

	if (!data) {
		data = hash_data_new_from_xml(param);
		g_hash_table_insert(self->vars, g_strdup(name), data);
	} else
		if (!data->param[scope])
			data->param[scope] = param;

	self->var_order[scope] = g_list_append(self->var_order[scope], param);
	before = get_prev_param(self, param);
	after = get_next_param(self, param);
	data->weight[scope] = compute_weight(self, before, after);

	g_message("%s : %f\n", name, data->weight[scope]);

	return gebr_validator_change_value(self, param, GET_VAR_VALUE(param), affected, error);
}

gboolean
gebr_validator_remove(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError		 **error)
{
	const gchar *name;
	HashData *data;
	GebrGeoXmlDocumentType scope;

	name = GET_VAR_NAME(param);
	scope = gebr_geoxml_parameter_get_scope (param);
	data = g_hash_table_lookup (self->vars, name);

	g_return_val_if_fail(data != NULL, FALSE);

	self->var_order[scope] = g_list_remove(self->var_order[scope], data->param[scope]);
	data->param[scope] = NULL;

	if (!get_value(self, name, scope)) {
		remove_from_antidep_of_deps(self, &data->dep[scope], name);

		if (data->antidep == NULL) {
			g_hash_table_remove(self->vars, name);
			return TRUE;
		}
	}

	return TRUE;
}

gboolean
gebr_validator_rename(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      const gchar         *new_name,
		      GList              **affected,
		      GError             **error)
{
	const gchar * name = NULL;
	name = GET_VAR_NAME(param);
	g_return_val_if_fail(g_strcmp0(name, new_name) != 0, TRUE);

	g_return_val_if_fail(gebr_validator_remove(self, param, NULL, error), FALSE);
	SET_VAR_NAME(param, new_name);
	gebr_validator_insert(self, param, NULL, error);

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

	name = GET_VAR_NAME(param);
	scope = gebr_geoxml_parameter_get_scope(param);
	type = gebr_geoxml_parameter_get_type(param);

	retval = gebr_validator_change_value_by_name(self, name, scope, type,
						     new_value, affected, error);

	SET_VAR_VALUE(param, new_value);

	return retval;
}

GebrGeoXmlParameter*
gebr_validator_move(GebrValidator       *self,
		    GebrGeoXmlParameter *param,
		    GebrGeoXmlParameter *pivot,
		    GList              **affected)
{
	const gchar *name;
	const gchar *value;
	HashData *data;
	GebrGeoXmlParameter *new_param;
	GebrGeoXmlParameterType type;
	GebrGeoXmlDocumentType t1, t2;

	name = GET_VAR_NAME(param);
	data = g_hash_table_lookup(self->vars, name);

	g_return_val_if_fail(data != NULL, NULL);

	value = GET_VAR_VALUE(param);
	t1 = gebr_geoxml_parameter_get_scope(param);
	t2 = gebr_geoxml_parameter_get_scope(pivot);
	type = gebr_geoxml_parameter_get_type(param);

	g_assert(data->param[t1] == param);

	GList *aff1 = NULL;
	GList *aff2 = NULL;
	GError *err1 = NULL;
	GError *err2 = NULL;

	if (t1 != t2) {
		if (data->param[t2]) {
			gebr_validator_change_value(self, data->param[t2],
						    value, &aff1, &err1);
			new_param = data->param[t2];
		} else {
			GebrGeoXmlDocument *doc;
			doc = gebr_geoxml_object_get_owner_document(GEBR_GEOXML_OBJECT(pivot));
			new_param = gebr_geoxml_document_set_dict_keyword(doc, type, name, value);
			gebr_validator_insert(self, new_param, &aff1, &err1);
		}
	} else
		new_param = param;

	gebr_validator_remove(self, param, &aff2, &err2);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(new_param),
					GEBR_GEOXML_SEQUENCE(pivot));

	return new_param;
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

	/* If this is a dictionary parameter, we can validate
	 * itself and return. Otherwise we need to validate all
	 * variables from its value.
	 */

	type = gebr_geoxml_parameter_get_type(param);

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
			return get_error_indirect(self, data->dep[scope], type, scope, err);

		return TRUE;
	}

	value = GET_VAR_VALUE(param);

	if (validated)
		*validated = g_strdup(value);

	return gebr_validator_validate_expr(self, value, type, err);
}

gboolean
gebr_validator_validate_expr(GebrValidator          *self,
			     const gchar            *str,
			     GebrGeoXmlParameterType type,
			     GError                **err)
{
	GList *vars = NULL;
	gboolean valid = FALSE;

	valid = validate_and_extract_vars(self, str, type, &vars, err);

	if (valid) {
		valid = get_error_indirect(self, vars, type, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, err);

		g_list_foreach(vars, (GFunc)g_free, NULL);
		g_list_free(vars);
	}

	return valid;
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
	GebrGeoXmlSequence *seq;

//	TODO: Necessary?
//	gebr_iexpr_reset(GEBR_IEXPR(self->arith_expr));
//	gebr_iexpr_reset(GEBR_IEXPR(self->string_expr));
	g_hash_table_remove_all(self->vars);

	for (int j = 0; j < 3; j++) {
		g_list_free(self->var_order[j]);
		self->var_order[j] = NULL;
	}

	for (int i = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT; i >= GEBR_GEOXML_DOCUMENT_TYPE_FLOW; i--) {
		if (*(self->docs[i]) == NULL)
			continue;

		seq = gebr_geoxml_document_get_dict_parameter(*(self->docs[i]));
		while (seq) {
			gebr_validator_insert(self, GEBR_GEOXML_PARAMETER(seq), NULL, NULL);
			gebr_geoxml_sequence_next(&seq);
		}
	}
}

void gebr_validator_free(GebrValidator *self)
{
	g_hash_table_unref(self->vars);
	g_object_unref(self->arith_expr);
	g_object_unref(self->string_expr);
	g_free(self);
}

gboolean gebr_validator_evaluate(GebrValidator *self,
                                 const gchar * expr,
                                 GebrGeoXmlParameterType type,
                                 gchar **value,
                                 GError **error)
{
	gchar *ini = NULL, *end = NULL;
	GList *vars = NULL;
	GebrIExpr *iexpr;
	gboolean use_iter = FALSE;
	GebrGeoXmlParameter *iter_param;

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
			     || type == GEBR_GEOXML_PARAMETER_TYPE_FILE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE
			     || type == GEBR_GEOXML_PARAMETER_TYPE_STRING,
			     FALSE);

	g_return_val_if_fail(expr != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);

	if (!gebr_validator_validate_expr(self, expr, type, error))
		return FALSE;

	void declare_dep_tree(const gchar *name)
	{
		GebrGeoXmlParameter *param;
		GebrGeoXmlDocumentType scope;
		GebrGeoXmlParameterType type;
		HashData *deps = g_hash_table_lookup(self->vars, name);

		g_return_if_fail(deps != NULL);

		param = get_param(self, name, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		if (!param)
			return;

		scope = gebr_geoxml_parameter_get_scope(param);

		for (GList *i = deps->dep[scope]; i; i = i->next)
			declare_dep_tree(i->data);

		type = gebr_geoxml_parameter_get_type(param);

		gebr_iexpr_set_var(iexpr, name, type,
				   get_value(self, name, scope), NULL);
	}

	iexpr = get_validator_by_type(self, type);
	vars = gebr_iexpr_extract_vars(iexpr, expr);

	for (GList * i = vars; i; i = i->next) {
		GebrGeoXmlDocumentType s = get_scope(self, i->data, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		if (!use_iter && gebr_validator_check_using_var(self, i->data, s, "iter")) {
			HashData *iter_data;
			use_iter = TRUE;
			iter_data = g_hash_table_lookup(self->vars, "iter");

			if (iter_data) {
				iter_param = iter_data->param[0];
				get_iter_bounds(iter_param, &ini, &end);
				SET_VAR_VALUE(iter_param, ini);
			} else
				g_warn_if_reached();
		}
		declare_dep_tree(i->data);
	}

	gebr_iexpr_eval(iexpr, expr, value, NULL);

	if (use_iter) {
		gchar *end_value;
		gchar *tmp;

		SET_VAR_VALUE(iter_param, end);

		for (GList * i = vars; i; i = i->next)
			declare_dep_tree(i->data);

		gebr_iexpr_eval(iexpr, expr, &end_value, NULL);
		if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING)
			tmp = g_strdup_printf("[\"%s\", ..., \"%s\"]", *value, end_value);
		else
			tmp = g_strdup_printf("[%s, ..., %s]", *value, end_value);
		g_free(*value);
		g_free(end_value);
		*value = tmp;
	}

	g_free(ini);
	g_free(end);

	if(vars)
	{
		g_list_foreach(vars, (GFunc)g_free, NULL);
		g_list_free(vars);
	}
		
	return TRUE;
}
