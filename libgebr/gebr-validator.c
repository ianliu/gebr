#include "gebr-validator.h"
#include "utils.h"

struct _GebrValidator
{
	GebrGeoXmlDocument **flow;
	GebrGeoXmlDocument **line;
	GebrGeoXmlDocument **proj;
};

GebrValidator *gebr_validator_new(GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj)
{
	GebrValidator *self = g_new(GebrValidator, 1);
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
	// TODO Implement
	return 0;
}

gboolean gebr_validator_validate_widget(GebrValidator            *self,
					GebrGuiValidatableWidget *widget,
					GebrGeoXmlParameter      *param)
{
	// TODO Implement
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
							   self->flow,
							   self->line,
							   self->proj,
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
						  self->flow,
						  self->line,
						  self->proj,
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
	*flow = self->flow;
	*line = self->line;
	*proj = self->proj;
}
