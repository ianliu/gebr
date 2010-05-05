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

#ifndef __GEBR_GUI_PROGRAM_EDIT_H
#define __GEBR_GUI_PROGRAM_EDIT_H

/**
 * \file programedit.h
 */

#include <gtk/gtk.h>

#include <geoxml.h>

G_BEGIN_DECLS

typedef struct {
	GebrGeoXmlProgram *program;

	GtkWidget *widget;
	gpointer parameter_widget_data;
	gboolean use_default;

	GtkWidget *scrolled_window;
	GtkWidget *title_label;
	GtkWidget *hbox;

	struct gebr_gui_gebr_gui_program_edit_dicts {
		GebrGeoXmlDocument *project;
		GebrGeoXmlDocument *line;
		GebrGeoXmlDocument *flow;
	} dicts;
} GebrGuiProgramEdit;

/**
 * Setup UI for \p program.
 */
GebrGuiProgramEdit *
gebr_gui_program_edit_setup_ui(GebrGeoXmlProgram * program, gpointer parameter_widget_data, gboolean use_default);

/**
 * \internal
 * Just free.
 */
void gebr_gui_gebr_gui_program_edit_destroy(GebrGuiProgramEdit *program_edit);

/**
 * \internal
 *
 * Reload UI of \p gebr_gui_program_edit.
 * \param program If not NULL, then use it as new program.
 */
void gebr_gui_program_edit_reload(GebrGuiProgramEdit *program_edit, GebrGeoXmlProgram * program);

G_END_DECLS
#endif				//__GEBR_GUI_PROGRAM_EDIT_H
