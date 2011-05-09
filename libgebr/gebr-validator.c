#include "gebr-validator.h"
#include "utils.h"
#include "gebr-expr.h"
#include "gebr-iexpr.h"
#include "gebr-string-expr.h"

struct _GebrValidator
{
	GebrExpr *arith_expr;
	GebrStringExpr *string_expr;

	GebrGeoXmlDocument **flow;
	GebrGeoXmlDocument **line;
	GebrGeoXmlDocument **proj;
};

GebrValidator *gebr_validator_new(GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj)
{
	GebrValidator *self = g_new(GebrValidator, 1);
	self->arith_expr = gebr_expr_new(NULL);
	self->string_expr = gebr_string_expr_new();
	self->flow = flow;
	self->line = line;
	self->proj = proj;

	return self;
}

gboolean gebr_validator_validate_param(GebrValidator        *self,
				       GebrGeoXmlParameter  *param,
				       gchar               **validated,
				       GError              **err)
{
	GString *result;
	GError *error = NULL;
	const gchar *separator;
	const gchar *expression;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *pparam;
	GebrGeoXmlSequence *seq;
	gboolean is_first = TRUE;

	pparam = GEBR_GEOXML_PROGRAM_PARAMETER(param);
	result = g_string_new(NULL);
	type = gebr_geoxml_parameter_get_type(param);
	gebr_geoxml_program_parameter_get_value(pparam, FALSE, &seq, 0);
	separator = gebr_geoxml_program_parameter_get_list_separator(pparam);

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FILE   ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT  ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_INT    ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_RANGE,
			     FALSE);

	while (seq) {
		error = NULL;
		expression = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
		switch (type) {
		case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		case GEBR_GEOXML_PARAMETER_TYPE_FILE:
			gebr_iexpr_is_valid(GEBR_IEXPR(self->string_expr), expression, &error);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			gebr_expr_eval(self->arith_expr, expression, NULL, &error);
			break;
		default:
			break;
		}

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

gboolean gebr_validator_validate_widget(GebrValidator            *self,
					GebrGuiValidatableWidget *widget,
					GebrGeoXmlParameter      *param)
{
	// TODO Implement
#if 0
	GError *error = NULL;
	gchar *validated;
	gchar *expression = gebr_gui_validatable_widget_get_value(widget);
	gboolean retval;

	retval = gebr_validator_validate(validator, expression, &validated, param, &error);
	gebr_gui_validatable_widget_set_icon(widget, param, error);
	gebr_gui_validatable_widget_set_value(widget, validated);

	if (error)
		g_clear_error(&error);
	g_free(expression);

	return retval;
#endif

	return 0;
}


void gebr_validator_free(GebrValidator *self)
{
	// TODO Implement
}

gboolean gebr_validator_validate(GebrValidator          *self,
					const gchar            *expression,
					gchar                 **validated,
					GebrGeoXmlParameterType type,
					GebrGeoXmlParameter    *param,
					GError                **err)
{
	GError *error = NULL;
	GebrGeoXmlProgramParameter *pparam;

	pparam = GEBR_GEOXML_PROGRAM_PARAMETER(param);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN && param)
		type = gebr_geoxml_parameter_get_type(param);
	else
		g_return_val_if_reached(FALSE);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_INT || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
	{
		gchar *end_str;
		const gchar *min, *max;

		g_ascii_strtod(expression, &end_str);
		if(*end_str) {
			gebr_geoxml_document_validate_expr(expression,
							   *self->flow,
							   *self->line,
							   *self->proj,
							   &error);
			if (error) {
				g_propagate_error(err, error);
				return FALSE;
			}
			*validated = g_strdup(expression);
			return TRUE;
		}

		if (pparam) {
			gebr_geoxml_program_parameter_get_number_min_max(pparam, &min, &max);

			if (type == GEBR_GEOXML_PARAMETER_TYPE_INT)
				*validated = g_strdup(gebr_validate_int(expression, min, max));
			else
				*validated = g_strdup(gebr_validate_float(expression, min, max));
		}
		return TRUE;
	}

	if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING || type == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
		gebr_geoxml_document_validate_str(expression,
						  *self->flow,
						  *self->line,
						  *self->proj,
						  &error);

		if (error) {
			g_propagate_error(err, error);
			return FALSE;
		}
		*validated = g_strdup(expression);
		return TRUE;
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
