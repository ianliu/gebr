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

#ifndef __GEBR_GUI_VALIDATABLE_WIDGET_H__
#define __GEBR_GUI_VALIDATABLE_WIDGET_H__

#include <glib.h>
#include <geoxml/geoxml.h>

G_BEGIN_DECLS

typedef struct _GebrGuiValidatableWidget GebrGuiValidatableWidget;

struct _GebrGuiValidatableWidget{

	void (*set_icon) (GebrGuiValidatableWidget *widget,
			  GebrGeoXmlParameter      *param,
			  GError                   *error);

	gchar * (*get_value) (GebrGuiValidatableWidget *widget);

	void (*set_value) (GebrGuiValidatableWidget *widget,
			   const gchar *value);
};

void gebr_gui_validatable_widget_set_icon(GebrGuiValidatableWidget *widget,
					  GebrGeoXmlParameter      *param,
					  GError                   *error);

gchar *gebr_gui_validatable_widget_get_value(GebrGuiValidatableWidget *widget);

void gebr_gui_validatable_widget_set_value(GebrGuiValidatableWidget *widget,
					   const gchar *validated);

G_END_DECLS

#endif /* __GEBR_GUI_VALIDATABLE_WIDGET_H__ */
