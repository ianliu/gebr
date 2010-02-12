/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/**
 * @file ui_project_line.c Builds the "Project and Lines" UI and distribute callbacks
 */

#ifndef __UI_PROJECT_LINE_H
#define __UI_PROJECT_LINE_H

#include <gtk/gtk.h>

enum ProjectLineSelectionType {
	DontWarnUnselection,
	ProjectSelection,
	LineSelection,
	ProjectLineSelection
};

/* Store fields */
enum {
	PL_TITLE = 0,
	PL_FILENAME,
	PL_N_COLUMN
};

struct ui_project_line {
	GtkWidget *widget;

	GtkTreeStore *store;
	GtkWidget *view;

	struct ui_project_line_info {
		GtkWidget *title;
		GtkWidget *description;

		GtkWidget *created_label;
		GtkWidget *created;
		GtkWidget *modified_label;
		GtkWidget *modified;

		GtkWidget *path_label;
		GtkWidget *path1;
		GtkWidget *path2;

		GtkWidget *help;
		GtkWidget *author;

		GtkWidget *numberoflines;
	} info;

};

struct ui_project_line *project_line_setup_ui(void);

void project_line_info_update(void);

gboolean project_line_get_selected(GtkTreeIter * iter, enum ProjectLineSelectionType check_type);

/**
 * Select _iter_ and load it into UI_
 */
void project_line_select_iter(GtkTreeIter * iter);

void project_line_import(void);

void project_line_export(void);

void project_line_free(void);

#endif				//__UI_PROJECT_LINE_H
