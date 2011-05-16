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

	GHashTable *xml_to_node;
	GNode *root;
};

typedef struct {
	gboolean has_error;
	GebrGeoXmlParameter *param;
} NodeData;

/* NodeData functions {{{1 */
static NodeData *
node_data_new(GebrGeoXmlParameter *param)
{
	NodeData *n = g_new(NodeData, 1);
	n->param = param;
	return n;
}

static void
node_data_free(NodeData *n)
{
	g_free(n);
}

/* Private functions {{{1*/
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

	self->xml_to_node = g_hash_table_new(NULL, NULL);
	self->root = g_node_new(NULL);

	return self;
}

gboolean
gebr_validator_def(GebrValidator       *self,
		   GebrGeoXmlParameter *key,
		   GList              **affected,
		   GError             **error)
{
	GebrIExpr *expr = get_validator(self, key);
	//g_node_prepend_data(self->root, );
	return 0;
}

gboolean
gebr_validator_move_before(GebrValidator       *self,
			   GebrGeoXmlParameter *key,
			   GebrGeoXmlParameter *pivot,
			   GList              **affected,
			   GError             **error)
{
	return 0;
}

gboolean
gebr_validator_undef(GebrValidator       *self,
		     GebrGeoXmlParameter *key,
		     GList              **affected,
		     GError             **error)
{
	return 0;
}

gboolean
gebr_validator_rename(GebrValidator       *self,
		      GebrGeoXmlParameter *key,
		      const gchar         *new_name,
		      GList              **affected,
		      GError             **error)
{
	return 0;
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
	*flow = *self->flow;
	*line = *self->line;
	*proj = *self->proj;
}

void gebr_validator_free(GebrValidator *self)
{
	g_hash_table_unref(self->xml_to_node);
	g_node_traverse(self->root, G_IN_ORDER, G_TRAVERSE_ALL, -1,
			(GNodeTraverseFunc)node_data_free, NULL);

	g_node_destroy(self->root);
	g_object_unref(self->arith_expr);
	g_object_unref(self->string_expr);
	g_free(self);
}
