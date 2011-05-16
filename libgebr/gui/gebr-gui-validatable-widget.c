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

	type = gebr_geoxml_parameter_get_type(param);
	expression = gebr_gui_validatable_widget_get_value(widget);
	retval = gebr_validator_validate_expr(self, expression, type, &error);

	if (!error)
	{
		if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING && 
		    gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param)) &&
		    g_strcmp0(g_strstrip(expression), "") == 0)
			g_set_error(&error,
				    GEBR_IEXPR_ERROR,
				    GEBR_IEXPR_ERROR_EMPTY_EXPRESSION,
				    _("Expression does not evaluate to a value"));
	}
	else if (error->code == GEBR_IEXPR_ERROR_EMPTY_EXPRESSION &&
		 !gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param)))
		g_clear_error(&error);

	gebr_gui_validatable_widget_set_icon(widget, param, error);

	if (error)
		g_clear_error(&error);
	g_free(expression);

	return retval;
}
