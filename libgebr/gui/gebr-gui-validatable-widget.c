/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../libgebr-gettext.h"

#include "gebr-gui-validatable-widget.h"
#include "../gebr-iexpr.h"
#include <glib/gi18n-lib.h>

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
	pparam = GEBR_GEOXML_PROGRAM_PARAMETER(param);

	if (gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(param)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
		retval = gebr_validator_validate_control_parameter(self, gebr_geoxml_program_parameter_get_keyword(pparam), expression, &error);
	else
		retval = gebr_validator_validate_expr(self, expression, type, &error);

	if (!error && !*expression && gebr_geoxml_program_parameter_get_required(pparam) && !gebr_geoxml_program_parameter_has_value(pparam)) {
		g_set_error(&error,
			    GEBR_IEXPR_ERROR,
			    GEBR_IEXPR_ERROR_EMPTY_EXPR,
			    _("This parameter is required"));
		retval = FALSE;
	}

	gebr_gui_validatable_widget_set_icon(widget, param, error);

	if (error)
		g_clear_error(&error);

	return retval;
}
