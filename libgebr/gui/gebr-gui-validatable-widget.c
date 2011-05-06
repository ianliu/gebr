#include "gebr-gui-validatable-widget.h"

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
