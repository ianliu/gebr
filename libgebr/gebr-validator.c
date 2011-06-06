#include <glib/gi18n.h>
#include <geoxml/geoxml.h>

#include "gebr-validator.h"
#include "utils.h"
#include "gebr-iexpr.h"
#include "gebr-arith-expr.h"
#include "gebr-string-expr.h"

/* Structures {{{1 */
struct _GebrValidator
{
	GebrArithExpr *arith_expr;
	GebrStringExpr *string_expr;

	GebrGeoXmlDocument **flow;
	GebrGeoXmlDocument **line;
	GebrGeoXmlDocument **proj;

	GHashTable *vars;
};

typedef struct {
	GebrGeoXmlParameter *param[3];
	gchar *name;
	GList *dep[3];
	GList *antidep;
	/* TODO: Change this to GError?? */
	gchar *error[3];
} HashData;

#define GET_VAR_NAME(p) (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(p)))
#define GET_VAR_VALUE(p) (gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE))
#define SET_VAR_VALUE(p,v) (gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(p), FALSE, (v)))

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
		g_free(n->error[i]);
	}
	g_list_foreach(n->antidep, (GFunc) g_free, NULL);
	g_list_free(n->antidep);
	g_free(n->name);
	g_free(n);
}

/* Private functions {{{1 */
static GebrIExpr *
get_validator(GebrValidator *self,
	      GebrGeoXmlParameter *param)
{
	GebrGeoXmlParameterType type;

	type = gebr_geoxml_parameter_get_type(param);

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

static void
setup_variables(GebrValidator *self, GebrIExpr *expr_validator, GebrGeoXmlParameter *param)
{
	const gchar *name;
	const gchar *value;
	gboolean is_dict_param;
	GebrGeoXmlSequence *dict;
	GebrGeoXmlDocument *doc;
	GebrGeoXmlDocumentType doctype;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *pparam;
	GebrGeoXmlDocument *docs[4] = {
		*self->proj,
		NULL,
		NULL,
		NULL
	};

	if (param) {
		doc = gebr_geoxml_object_get_owner_document(GEBR_GEOXML_OBJECT(param));
		doctype = gebr_geoxml_document_get_type(doc);
		is_dict_param = gebr_geoxml_parameter_is_dict_param(param);
	} else
		is_dict_param = FALSE;

	if (!is_dict_param || doctype == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
		docs[1] = *self->line;
		docs[2] = *self->flow;
	} else if (doctype == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		docs[1] = *self->line;


	for (int i = 0; docs[i]; i++) {
		dict = gebr_geoxml_document_get_dict_parameter(docs[i]);
		while (dict) {
			pparam = GEBR_GEOXML_PROGRAM_PARAMETER(dict);

			if (gebr_geoxml_program_parameter_get_is_list(pparam)) {
				g_warn_if_reached();
				gebr_geoxml_sequence_next(&dict);
				continue;
			}

			if (is_dict_param && GEBR_GEOXML_PARAMETER(dict) == param)
				break;

			type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(dict));
			name = gebr_geoxml_program_parameter_get_keyword(pparam);
			value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
			gebr_iexpr_set_var(expr_validator, name, type, value, NULL);

			gebr_geoxml_sequence_next(&dict);
		}
	}
}

static gboolean
is_variable_valid(GebrValidator *self,
		  GebrGeoXmlParameter *param)
{
	return TRUE;
}

static void
set_error(GebrValidator *self,
	  const gchar *name,
	  GebrGeoXmlDocumentType scope,
	  const gchar *msg)
{
}

static const gchar *
get_error(GebrValidator *self,
	  const gchar *name)
{
	HashData *data;

	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return NULL;

	for (int i = 0; i < 3; i++)
		if (data->param[i])
			return data->error[i];

	return NULL;
}

static const gchar *
get_value(GebrValidator *self,
	  const gchar *name)
{
	HashData *data;

	data = g_hash_table_lookup(self->vars, name);

	if (!data)
		return NULL;

	for (int i = 0; i < 3; i++)
	{
		if (!data->param[i])
			continue;

		return GET_VAR_VALUE(data->param[i]);
	}

	return NULL;
}

static gboolean
detect_cicle(GebrValidator *self,
	     GebrGeoXmlParameter *param)
{
	return FALSE;
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
	self->flow = flow;
	self->line = line;
	self->proj = proj;

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
	HashData *data;

	name = GET_VAR_NAME(param);

	data = g_hash_table_lookup(self->vars, name);

	if (data && !get_value(self, name)) {
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
	HashData *data = NULL;
	GebrGeoXmlDocumentType type;

	name = GET_VAR_NAME(param);
	type = gebr_geoxml_parameter_get_scope (param);
	data = g_hash_table_lookup (self->vars, name);

	if (!data)
		return;

	data->param[type] = NULL;

	if (get_value(self, name)) {
		// TODO: Revalidate anti-dependencies of name
	} else {
		if (data->antidep == NULL)
			g_hash_table_remove(self->vars, name);
		else {
			gchar *anti_name;
			HashData *errors;
			for (GList *v = data->antidep; v; v = v->next) {
				anti_name = v->data;
				errors = g_hash_table_lookup(self->vars, anti_name);
				g_free(errors->error[type]);
				errors->error[type] = g_strdup_printf(_("Variable %s not defined"), name);
				tmp = g_list_prepend(tmp, anti_name);
			}
		}
	}
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

	name = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));
	data = (HashData *)g_hash_table_lookup(self->vars, name);

	if (!data)
	{
		/*
		 * TODO: Create a new error domain/types
		 * for this operations
		 */
		g_set_error(error, GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_UNDEF_VAR,
			    _("Variable %s not found at rename method."),
			    name);
		return FALSE;
	}

	if (data->antidep)
		for (GList * i = data->antidep; i; i = i->next);
	else
	{
		/*
		GebrGeoXmlProgramParameter = new_var;
		gebr_validator_insert(self, new_var, NULL, error);
		gebr_validator_remove(self, new_var, NULL);
		*/
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
	HashData *data;
	const gchar *name;
	GError *err = NULL;
	GebrIExpr *expr;
	GebrGeoXmlDocumentType type;

	name = GET_VAR_NAME(param);
	data = g_hash_table_lookup(self->vars, name);
	type = gebr_geoxml_parameter_get_scope(param);

	g_return_val_if_fail(data != NULL, FALSE);

	for (GList *i = data->dep[type]; i; i = i->next) {
		HashData *dep;
		gchar *dep_name = i->data;
		GList *node;

		dep = g_hash_table_lookup(self->vars, dep_name);
		node = g_list_find_custom(dep->antidep, name, (GCompareFunc)g_strcmp0);
		g_free(node->data);
		dep->antidep = g_list_delete_link(dep->antidep, node);
	}

	expr = get_validator(self, param);
	if (!gebr_iexpr_is_valid(expr, new_value, &err)) {
		set_error(self, name, type, err->message);
		g_propagate_error(error, err);
		return FALSE;
	}

	g_list_foreach(data->dep[type], (GFunc)g_free, NULL);
	g_list_free(data->dep[type]);
	data->dep[type] = gebr_iexpr_extract_vars(expr, new_value);

	gboolean has_error = FALSE;

	for (GList *i = data->dep[type]; i; i = i->next) {
		HashData *dep;
		gchar *dep_name = i->data;

		dep = g_hash_table_lookup(self->vars, dep_name);
		if (!dep) {
			dep = hash_data_new(dep_name);
			g_hash_table_insert(self->vars, g_strdup(dep_name), dep);
		}

		if (!g_list_find_custom(dep->antidep, name, (GCompareFunc)g_strcmp0))
			dep->antidep = g_list_prepend(dep->antidep, g_strdup(name));

		if (!has_error && get_error(self, dep_name)) {
			gchar *msg;
			msg = g_strdup_printf(_("Variable %s depends on %s which has errors"),
					      name, dep_name);
			has_error = TRUE;
			set_error(self, name, type, msg);
			g_free(msg);
		}
	}

	SET_VAR_VALUE(param, new_value);

	if (detect_cicle(self, param)) {
		gchar *msg;
		msg = g_strdup_printf(_("The variable %s has cicle dependency"), name);
		set_error(self, name, type, msg);
		g_free(msg);
	} else if (!has_error)
		set_error(self, name, type, NULL);

	return !has_error;
}

GebrGeoXmlParameter*
gebr_validator_move(GebrValidator       *self,
		    GebrGeoXmlParameter *param,
		    GebrGeoXmlParameter *pivot,
		    GList              **affected)
{
	return NULL;
}

gboolean
gebr_validator_validate_param(GebrValidator       *self,
			      GebrGeoXmlParameter *param,
			      gchar              **validated,
			      GError             **err)
{
	GString *result;
	GError *error = NULL;
	const gchar *separator;
	const gchar *expression;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *pparam;
	GebrGeoXmlSequence *seq;
	GebrIExpr *expr_validator = NULL;
	gboolean is_first = TRUE;

	g_return_val_if_fail(param != NULL, FALSE);

	type = gebr_geoxml_parameter_get_type(param);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_ENUM ||
	    type == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return TRUE;

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FILE   ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT  ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_INT    ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_RANGE,
			     FALSE);

	pparam = GEBR_GEOXML_PROGRAM_PARAMETER(param);
	result = g_string_new(NULL);
	gebr_geoxml_program_parameter_get_value(pparam, FALSE, &seq, 0);
	separator = gebr_geoxml_program_parameter_get_list_separator(pparam);

	if (gebr_geoxml_program_parameter_get_required(pparam)) {
		GString *value;
		value = gebr_geoxml_program_parameter_get_string_value(pparam, FALSE);
		if (!value->len) {
			g_set_error(err,
			            GEBR_IEXPR_ERROR,
			            GEBR_IEXPR_ERROR_EMPTY_EXPR,
			            _("This parameter is required"));
			g_string_free(value, TRUE);
			return FALSE;
		}
		g_string_free(value, TRUE);
	}

	expr_validator = get_validator(self, param);

	gebr_iexpr_reset(expr_validator);

	if(self->line != NULL && self->proj != NULL)
		setup_variables(self, expr_validator, param);

	while (seq) {
		error = NULL;
		expression = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
		gebr_iexpr_is_valid(expr_validator, expression, &error);

		if (error) {
			g_propagate_error(err, error);
			g_string_free(result, TRUE);
			return FALSE;
		}

		if (is_first) {
			is_first = FALSE;
			g_string_append(result, expression);
		} else
			g_string_append_printf(result, "%s%s", separator, expression);

		gebr_geoxml_sequence_next(&seq);
	}

	*validated = g_string_free(result, FALSE);
	return TRUE;
}

gboolean
gebr_validator_validate_expr(GebrValidator          *self,
			     const gchar            *expr,
			     GebrGeoXmlParameterType type,
			     GError                **err)
{
	GError *error = NULL;
	GebrIExpr *expr_validator = NULL;

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FILE   ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT  ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_INT    ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_RANGE,
			     FALSE);

	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		expr_validator = GEBR_IEXPR(self->string_expr);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		expr_validator = GEBR_IEXPR(self->arith_expr);
		break;
	default:
		break;
	}

	gebr_iexpr_reset(expr_validator);

	if(self->line != NULL && self->proj != NULL)
		setup_variables(self, expr_validator, NULL);

	gebr_iexpr_is_valid(expr_validator, expr, &error);

	if (error) {
		g_propagate_error(err, error);
		return FALSE;
	}

	return TRUE;
}

void gebr_validator_get_documents(GebrValidator *self,
				  GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj)
{
	if (self->flow)
		*flow = *self->flow;
	else
		*flow = NULL;
		
	if (self->line)
		*line = *self->line;
	else
		*line = NULL;
		
	if (self->proj)
		*proj = *self->proj;
	else
		*proj = NULL;

}

void gebr_validator_free(GebrValidator *self)
{
	g_hash_table_unref(self->vars);
	g_object_unref(self->arith_expr);
	g_object_unref(self->string_expr);
	g_free(self);
}
