#include "gebr-gui-validatable-widget.h"
#include "../gebr-iexpr.h"
#include <glib/gi18n.h>

void gebr_gui_validatable_widget_set_icon(GebrGuiValidatableWidget *widget,
					  GebrGeoXmlParameter *param,
					  GError *error)
{
	widget->set_icon(widget, param, error);
}

gchar *gebr_gui_validatable_widget_get_value(GebrGuiValidatableWidget *widget)
{
	return widget->get_value(widget);
}

void gebr_gui_validatable_widget_set_value(GebrGuiValidatableWidget *widget,
					   const gchar *validated)
{
	widget->set_value(widget, validated);
}

gboolean gebr_gui_validatable_widget_validate(GebrGuiValidatableWidget *widget,
					      GebrValidator            *self,
					      GebrGeoXmlParameter      *param)
{
	GError *error = NULL;
	gchar *expression;
	gboolean retval;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *pparam;

	type = gebr_geoxml_parameter_get_type(param);
	expression = gebr_gui_validatable_widget_get_value(widget);
	retval = gebr_validator_validate_expr(self, expression, type, &error);
	pparam = GEBR_GEOXML_PROGRAM_PARAMETER(param);

	if (!error && !*expression && gebr_geoxml_program_parameter_get_required(pparam))
		g_set_error(error,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_EMPTY_EXPR,
			    _("This parameter is required"));

	gebr_gui_validatable_widget_set_icon(widget, param, error);

	if (error)
		g_clear_error(&error);

	return retval;
}
