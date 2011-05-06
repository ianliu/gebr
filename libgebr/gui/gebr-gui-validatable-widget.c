#include "gebr-validatable-widget.h"

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
