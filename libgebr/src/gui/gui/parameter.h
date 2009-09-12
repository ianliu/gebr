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

#ifndef __LIBGEBR_GUI_PARAMETER_H
#define __LIBGEBR_GUI_PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml.h>
#include "gtkfileentry.h"
#include "valuesequenceedit.h"

struct parameter_widget;
typedef void (*changed_callback)(struct parameter_widget * parameter_widget, gpointer user_data);
struct parameter_widget;
typedef void (*value_context_menu_callback)(struct parameter_widget * parameter_widget, gpointer user_data);

struct parameter_widget {
	GeoXmlParameter *	parameter;
	gboolean		use_default_value;
	gpointer		data;

	GtkWidget *		widget;
	GtkWidget *		value_widget;

	/* for lists */
	GtkWidget *		list_value_widget;
	ValueSequenceEdit *	value_sequence_edit;

	/* auto submit stuff */
	changed_callback	callback;
	gpointer		user_data;
};

struct parameter_widget *
parameter_widget_new(GeoXmlParameter * parameter, gboolean use_default_value, gpointer data);

GString *
parameter_widget_get_widget_value(struct parameter_widget * parameter_widget);

void
parameter_widget_set_auto_submit_callback(struct parameter_widget * parameter_widget,
	changed_callback callback, gpointer user_data);

void
parameter_widget_update(struct parameter_widget * parameter_widget);

void
parameter_widget_update_list_separator(struct parameter_widget * parameter_widget);

void
parameter_widget_reconfigure(struct parameter_widget * parameter_widget);

#endif //__LIBGEBR_GUI_PARAMETER_H
