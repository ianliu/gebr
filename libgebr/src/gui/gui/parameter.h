/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_GUI_PARAMETER_H
#define __LIBGEBR_GUI_PARAMETER_H

#include <gtk/gtk.h>

struct parameter_widget {
	GtkWidget *		widget;
	GeoXmlParameter *	parameter;

	GtkWidget *		value_widget;
};

struct parameter_widget *
parameter_widget_new_float(GeoXmlParameter * parameter);

struct parameter_widget *
parameter_widget_new_int(GeoXmlParameter * parameter);

struct parameter_widget *
parameter_widget_new_string(GeoXmlParameter * parameter);

struct parameter_widget *
parameter_widget_new_range(GeoXmlParameter * parameter);

struct parameter_widget *
parameter_widget_new_flag(GeoXmlParameter * parameter);

struct parameter_widget *
parameter_widget_new_file(GeoXmlParameter * parameter);

void
parameter_widget_submit(struct parameter_widget * parameter_widget);

void
parameter_widget_update(struct parameter_widget * parameter_widget);

#endif //__LIBGEBR_GUI_PARAMETER_H
