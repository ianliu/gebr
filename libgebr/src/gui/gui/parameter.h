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

#ifndef __GEBR_GUI_PARAMETER_H
#define __GEBR_GUI_PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml.h>

#include "gtkfileentry.h"
#include "valuesequenceedit.h"
#include "programedit.h"

struct gebr_gui_parameter_widget;
typedef void (*changed_callback) (struct gebr_gui_parameter_widget * gebr_gui_parameter_widget, gpointer user_data);

struct gebr_gui_parameter_widget {
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlProgramParameter *program_parameter;
	enum GEBR_GEOXML_PARAMETER_TYPE parameter_type;
	gboolean use_default_value;
	gpointer data;

	GtkWidget *widget;
	GtkWidget *value_widget;

	/* dict stuff */
	GebrGeoXmlProgramParameter *dict_parameter;
	struct gebr_gui_gebr_gui_program_edit_dicts *dicts;

	/* for lists */
	GtkWidget *list_value_widget;
	GebrGuiValueSequenceEdit *gebr_gui_value_sequence_edit;

	/* auto submit stuff */
	changed_callback callback;
	gpointer user_data;
};

struct gebr_gui_parameter_widget *gebr_gui_parameter_widget_new(GebrGeoXmlParameter * parameter,
								gboolean use_default_value, gpointer data);

void


gebr_gui_parameter_widget_set_dicts(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget,
				    struct gebr_gui_gebr_gui_program_edit_dicts *dicts);

GString *gebr_gui_parameter_widget_get_widget_value(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget);

void


gebr_gui_parameter_widget_set_auto_submit_callback(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget,
						   changed_callback callback, gpointer user_data);

void gebr_gui_parameter_widget_update(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget);

void gebr_gui_parameter_widget_validate(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget);

void gebr_gui_parameter_widget_update_list_separator(struct gebr_gui_parameter_widget
						     *gebr_gui_parameter_widget);

void gebr_gui_parameter_widget_reconfigure(struct gebr_gui_parameter_widget *gebr_gui_parameter_widget);

#endif				//__GEBR_GUI_PARAMETER_H
