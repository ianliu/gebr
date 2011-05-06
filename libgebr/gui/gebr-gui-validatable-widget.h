/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_VALIDATOR_H__
#define __GEBR_VALIDATOR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef void (*GebrGuiValidatableIconFunc) (GebrGuiValidatableWidget *widget,
					    GebrGeoXmlParameter *param,
					    GError *error);

typedef gchar * (*GebrGuiValidatableGetValueFunc) (GebrGuiValidatableWidget *widget);

typedef struct {
	GtkWidget *widget;
	GebrGuiValidatableIconFunc set_icon;
	GebrGuiValidatableGetValueFunc get_value;
} GebrGuiValidatableWidget;

void gebr_gui_validatable_widget_set_icon(GebrGuiValidatableWidget *widget,
					  GebrGeoXmlParameter *param,
					  GError *error);

gchar *gebr_gui_validatable_widget_get_value(GebrGuiValidatableWidget *widget);

G_END_DECLS

#endif /* __GEBR_VALIDATOR_H__ */
