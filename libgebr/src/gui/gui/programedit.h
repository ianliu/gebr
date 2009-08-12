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

#ifndef __LIBGEBR_GUI_PROGRAM_EDIT_H
#define __LIBGEBR_GUI_PROGRAM_EDIT_H

#include <gtk/gtk.h>

#include <geoxml.h>

typedef void (*LibGeBRGUIShowHelpCallback)(GtkWidget * button, GeoXmlProgram * program);
struct libgebr_gui_program_edit {
	GeoXmlProgram *			program;

	GtkWidget *			widget;
	gpointer			file_parameter_widget_data;
	LibGeBRGUIShowHelpCallback	show_help_callback;
	gboolean			use_default;

	GtkWidget *			scrolled_window;
	GtkWidget *			title_label;
	GtkWidget *			hbox;
};

struct libgebr_gui_program_edit
libgebr_gui_program_edit_setup_ui(GeoXmlProgram * program, gpointer file_parameter_widget_data,
LibGeBRGUIShowHelpCallback show_help_callback, gboolean use_default);
void
libgebr_gui_program_edit_reload(struct libgebr_gui_program_edit * program_edit, GeoXmlProgram * program);

#endif //__LIBGEBR_GUI_PROGRAM_EDIT_H
