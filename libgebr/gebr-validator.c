#include <stdlib.h>
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
};

typedef struct {
	GebrGeoXmlParameter *param[3];
	gchar *name;
	GList *dep[3];
	GList *antidep;
	GError *error[3];
} HashData;

#define GET_VAR_NAME(p) (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p)))
#define GET_VAR_VALUE(p) (gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE))
#define SET_VAR_VALUE(p,v) (gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE, (v)))
#define SET_VAR_NAME(p,n) (gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p), (n)))

/* Prototypes {{{1 */
static void		set_error		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope,
						 GError                *error);

static void		update_error		(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope);

static GError *		get_error		(GebrValidator 	       *self,
						 const gchar   	       *name);

static gboolean		is_variable_valid	(GebrValidator         *self,
						 const gchar           *name,
						 GebrGeoXmlDocumentType scope,
						 const gchar 	      **cause);

static GebrGeoXmlParameter * get_param		(GebrValidator         *self,
						 const gchar           *name);

static const gchar *	get_value		(GebrValidator         *self,
						 const gchar           *name);

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
		g_return_val_if_reached(GEBR_IEXPR(self->string_expr));
	}
}

static gboolean
is_variable_valid(GebrValidator *self,
		  const gchar *name,
		  GebrGeoXmlDocumentType scope,
		  const gchar **cause)
{
	HashData *data;
	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return FALSE;

	for (GList *i = data->dep[scope]; i; i = i->next) {
		gchar *v = i->data;
		if (get_error(self, v)) {
			*cause = v;
			return FALSE;
		}
	}

	return TRUE;
}

static void
set_error_full(GebrValidator *self,
	       const gchar *name,
	       GebrGeoXmlDocumentType scope,
	       GError *error,
	       gboolean update_self)
{
	HashData *data;
	gboolean self_has_error;

	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return;

	if (update_self) {
		if (data->error[scope])
			g_clear_error(&data->error[scope]);
		if (error)
			data->error[scope] = g_error_copy(error);
		self_has_error = (error != NULL);
	} else
		self_has_error = (data->error[scope] != NULL);

	for (GList *i = data->antidep; i; i = i->next) {
		HashData *d;
		gchar *v = i->data;
		const gchar *cause;

		d = g_hash_table_lookup(self->vars, v);

		if (!d) {
			g_warn_if_reached();
			continue;
		}

		for (int i = 0; i < 3; i++) {
			if (!d->param[i])
				continue;
			if (d->error[i]->code == GEBR_IEXPR_ERROR_CYCLE)
				continue;
			if (!is_variable_valid(self, v, i, &cause)) {
				GError *e = NULL;
				g_set_error(&e, GEBR_IEXPR_ERROR,
					    GEBR_IEXPR_ERROR_UNDEF_VAR,
					    _("%s is undefined because of %s"),
					    v, self_has_error? name:cause);
				set_error(self, v, scope, e);
				g_error_free(e);
			} else if (d->error[i])
				g_clear_error(&d->error[i]);
		}
	}
}

static void
set_error(GebrValidator *self,
	  const gchar *name,
	  GebrGeoXmlDocumentType scope,
	  GError *error)
{
	set_error_full(self, name, scope, error, TRUE);
}

static void
update_error(GebrValidator *self,
	     const gchar *name,
	     GebrGeoXmlDocumentType scope)
{
	set_error_full(self, name, scope, NULL, FALSE);
}

static GError *
get_error(GebrValidator *self,
	  const gchar *name)
{
	HashData *data;
	GError *err = NULL;

	data = g_hash_table_lookup(self->vars, name);

	if (!data) {
		g_set_error(&err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_UNDEF_VAR,
			    _("The variable does not exists"));
		return err;
	}

	for (int i = 0; i < 3; i++)
		if (data->param[i] && data->error[i])
			return g_error_copy(data->error[i]);

	return NULL;
}

/*
 * Get the first non-NULL parameter from the Hash Data.
 * This respects scope priority, ie it returns the value
 * of Flow, Line and them Project.
 */
static GebrGeoXmlParameter *
get_param(GebrValidator *self,
	  const gchar *name)
{
	HashData *data;
	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return NULL;
	for (int i = 0; i < 3; i++) {
		if (!data->param[i])
			continue;
		return data->param[i];
	}
	return NULL;
}

static const gchar *
get_value(GebrValidator *self,
	  const gchar *name)
{
	GebrGeoXmlParameter *param;
	return (param = get_param(self, name)) ? GET_VAR_VALUE(param):NULL;
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
	GebrIExpr *expr;
	gboolean had_error;

	if (affected)
		*affected = NULL;

	data = g_hash_table_lookup(self->vars, name);
	had_error = (data->error[scope] != NULL);

	g_return_val_if_fail(data != NULL, FALSE);

	for (GList *i = data->dep[scope]; i; i = i->next) {
		HashData *dep;
		gchar *dep_name = i->data;
		GList *node;

		dep = g_hash_table_lookup(self->vars, dep_name);
		node = g_list_find_custom(dep->antidep, name, (GCompareFunc)g_strcmp0);
		g_free(node->data);
		dep->antidep = g_list_delete_link(dep->antidep, node);
	}

	expr = get_validator_by_type(self, type);
	if (!gebr_iexpr_is_valid(expr, new_value, &err)
	    && err->code == GEBR_IEXPR_ERROR_SYNTAX) {
		set_error(self, name, scope, err);
		g_propagate_error(error, err);

		if (!had_error) {
			// TODO: Generate affected list
		}

		return FALSE;
	}
	if (err)
		g_clear_error(&err);

	g_list_foreach(data->dep[scope], (GFunc)g_free, NULL);
	g_list_free(data->dep[scope]);
	data->dep[scope] = gebr_iexpr_extract_vars(expr, new_value);

	for (GList *i = data->dep[scope]; i; i = i->next) {
		HashData *dep;
		gchar *dep_name = i->data;

		dep = g_hash_table_lookup(self->vars, dep_name);
		if (!dep) {
			// XXX: Do we need to set error for inexistent variables?
			dep = hash_data_new(dep_name);
			g_hash_table_insert(self->vars, g_strdup(dep_name), dep);
		}

		if (!g_list_find_custom(dep->antidep, name, (GCompareFunc)g_strcmp0))
			dep->antidep = g_list_prepend(dep->antidep, g_strdup(name));
	}

	if (detect_cicle(self, name, scope)) {
		g_set_error(&err, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_CYCLE,
			    _("The variable %s has cycle dependency"),
			    name);
		data->error[scope] = g_error_copy(err);
		update_error(self, name, scope);
		g_propagate_error(error, err);
		return FALSE;
	}

	if (type != GEBR_GEOXML_PARAMETER_TYPE_STRING
	    && type != GEBR_GEOXML_PARAMETER_TYPE_FILE)
		gebr_iexpr_set_var(GEBR_IEXPR(self->string_expr), name,
				   type, new_value, NULL);
	gebr_iexpr_set_var(expr, name, type, new_value, &err);

	if (err) {
		set_error(self, name, scope, err);
		g_propagate_error(error, err);
		return FALSE;
	} else
		set_error(self, name, scope, NULL);

	return TRUE;
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

	return self;
}

gboolean
gebr_validator_insert(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError             **error)
{
	const gchar *name;
	GebrGeoXmlParameterType type;
	HashData *data;

	name = GET_VAR_NAME(param);
	type = gebr_geoxml_parameter_get_type(param);
	data = g_hash_table_lookup(self->vars, name);

	if (data && !data->param[type]) {
		GebrGeoXmlDocumentType type;
		type = gebr_geoxml_parameter_get_scope(param);
		data->param[type] = param;
	} else if (!data) {
		data = hash_data_new_from_xml(param);
		g_hash_table_insert(self->vars, g_strdup(name), data);
	}

	return gebr_validator_change_value(self, param, GET_VAR_VALUE(param), affected, error);
}

void
gebr_validator_remove(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      GList              **affected,
		      GError		 **error)
{
	GList *tmp = NULL;
	const gchar *name;
	HashData *data;
	GebrGeoXmlDocumentType type;

	name = GET_VAR_NAME(param);
	type = gebr_geoxml_parameter_get_scope (param);
	data = g_hash_table_lookup (self->vars, name);

	if (!data)
		return;

	data->param[type] = NULL;

	if (get_value(self, name))
		update_error(self, name, type);
	else if (data->antidep == NULL)
		g_hash_table_remove(self->vars, name);
	else {
		gchar *anti_name;
		HashData *errors;
		for (GList *v = data->antidep; v; v = v->next) {
			GError *e = NULL;
			anti_name = v->data;
			errors = g_hash_table_lookup(self->vars, anti_name);
			if (errors->error[type])
				g_clear_error(&errors->error[type]);
			g_set_error(&e, GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_UNDEF_VAR,
				    _("Variable %s not defined"),
				    name);
			errors->error[type] = e;

			if (affected)
				tmp = g_list_prepend(tmp, g_strdup(anti_name));
		}
	}

	if (affected)
		*affected = g_list_reverse(tmp);
}


gboolean
gebr_validator_rename(GebrValidator       *self,
		      GebrGeoXmlParameter *param,
		      const gchar         *new_name,
		      GList              **affected,
		      GError             **error)
{
	HashData * data = NULL;
	const gchar * name = NULL;
	GebrGeoXmlParameterType scope;
	GebrGeoXmlParameter *new_param;
	GList *a1, *a2, *aux_affec;

	name = GET_VAR_NAME(param);

	if (g_strcmp0(name,new_name) == 0)
		return TRUE;

	scope = gebr_geoxml_parameter_get_scope(param);
	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return FALSE;

	new_param = data->param[scope];
	gebr_validator_remove(self, param, &a1, error);

	SET_VAR_NAME(new_param, new_name);
	gebr_validator_insert(self, param, &a2, error);

	if (affected) {
		aux_affec = g_list_concat(a1, a2);
		aux_affec = g_list_sort(aux_affec, (GCompareFunc)g_strcmp0);

		if (aux_affec) {
			*affected = g_list_prepend(NULL, g_strdup(aux_affec->data));
			for (GList *i = aux_affec->next; i; i = i->next)
				if (g_strcmp0(i->data, i->prev->data) != 0)
					*affected = g_list_prepend(*affected, g_strdup(i->data));
			g_list_foreach(aux_affec, (GFunc)g_free, NULL);
			g_list_free(aux_affec);
		}
	} else {
		g_list_foreach(a1, (GFunc) g_free, NULL);
		g_list_foreach(a2, (GFunc) g_free, NULL);
		g_list_free(a1);
		g_list_free(a2);
	}

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
                               const gchar   *var)
{
	HashData *data;
	gboolean retval = FALSE;
	data = g_hash_table_lookup(self->vars, var);

	if (data == NULL)
		return FALSE;

	for (GList *antidep = data->antidep; antidep; antidep = antidep->next) {
		if (g_strcmp0(antidep->data, source) == 0)
			return TRUE;
		retval = gebr_validator_check_using_var(self, antidep->data, var);
		if (retval)
			break;
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
		}

		return TRUE;
	}

	value = GET_VAR_VALUE(param);
	type = gebr_geoxml_parameter_get_type(param);

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
	GList *vars;
	gboolean has_error = FALSE;
	GebrIExpr *expr;

	expr = get_validator_by_type(self, type);
	vars = gebr_iexpr_extract_vars(expr, str);

	for (GList *i = vars; i; i = i->next) {
		const gchar *name = i->data;
		GError *error = get_error(self, name);
		if (error) {
			g_propagate_error(err, g_error_copy(error));
			has_error = TRUE;
			break;
		}
	}

	g_list_foreach(vars, (GFunc)g_free, NULL);
	g_list_free(vars);

	return !has_error;
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

	gebr_iexpr_reset(GEBR_IEXPR(self->arith_expr));
	gebr_iexpr_reset(GEBR_IEXPR(self->string_expr));
	g_hash_table_remove_all(self->vars);

	for (int i = 0; i < 3; i++) {
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

		param = get_param(self, name);
		if (!param)
			return;

		scope = gebr_geoxml_parameter_get_scope(param);

		for (GList *i = deps->dep[scope]; i; i = i->next)
			declare_dep_tree(i->data);

		type = gebr_geoxml_parameter_get_type(param);

		gebr_iexpr_set_var(iexpr, name, type,
				   get_value(self, name), NULL);
	}

	iexpr = get_validator_by_type(self, type);
	vars = gebr_iexpr_extract_vars(iexpr, expr);

	for (GList * i = vars; i; i = i->next) {
		if (!use_iter && gebr_validator_check_using_var(self, i->data, "iter")) {
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
		tmp = g_strdup_printf("[%s, ..., %s]", *value, end_value);
		g_free(*value);
		g_free(end_value);
		*value = tmp;
	}

	g_free(ini);
	g_free(end);
		
	return TRUE;
}
