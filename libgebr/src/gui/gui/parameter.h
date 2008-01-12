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

#include <geoxml.h>

struct parameter_widget;
typedef void (*changed_callback)(struct parameter_widget * parameter_widget, gpointer user_data);

struct parameter_widget {
	GtkWidget *		widget;
	GeoXmlParameter *	parameter;

	GtkWidget *		value_widget;
	gboolean		use_default_value;

	/* for lists */
	GtkWidget *		list_value_widget;
	GtkWidget *		sequence_edit;

	/* auto submit stuff */
	changed_callback	callback;
	gpointer		user_data;
};

struct parameter_widget *
parameter_widget_new_float(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_int(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_string(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_range(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_file(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_enum(GeoXmlParameter * parameter, gboolean use_default_value);

struct parameter_widget *
parameter_widget_new_flag(GeoXmlParameter * parameter, gboolean use_default_value);

void
parameter_widget_set_widget_value(struct parameter_widget * parameter_widget, const gchar * value);

GString *
parameter_widget_get_widget_value(struct parameter_widget * parameter_widget);

void
parameter_widget_set_auto_submit_callback(struct parameter_widget * parameter_widget,
	changed_callback callback, gpointer user_data);

void
parameter_widget_submit(struct parameter_widget * parameter_widget);

void
parameter_widget_update(struct parameter_widget * parameter_widget);

#endif //__LIBGEBR_GUI_PARAMETER_H
