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

G_BEGIN_DECLS

/**
 * Types for retrieving the selection in Projects/Line tab.
 */
enum ProjectLineSelectionType {
	DontWarnUnselection,	/**< Don't warn into GeBR status bar if nothing is selected. */
	ProjectSelection,	/**< Succeed only if a project is selected. */
	LineSelection,		/**< Succeed only if a line is selected. */
	ProjectLineSelection	/**< Succeed if line or project is selected. */
};

/**
 * Store fields
 */
enum {
	PL_TITLE = 0,
	PL_FILENAME,
	/* Reference to project/line document pointer. Non-NULL if selected. */
	PL_XMLPOINTER,
	PL_N_COLUMN
};

/**
 */
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
		GtkWidget *path;

		GtkWidget *help_view;
		GtkWidget *help_edit;
		GtkWidget *author;

		GtkWidget *numberoflines;
	} info;

};

/**
 * Assembly the project/lines widget.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_project_line *project_line_setup_ui(void);

/**
 * Creates a project or line based on the current selection.
 */
void project_line_new(void);

/**
 * Update information shown about the selected project or line.
 */
void project_line_info_update(void);

/**
 * Put selected iter in \p iter and return true if there is a selection.
 *
 * \param check_type one of #DontWarnUnselection, #ProjectSelection, #LineSelection, #ProjectLineSelection.
 */
gboolean project_line_get_selected(GtkTreeIter * iter, enum ProjectLineSelectionType check_type);

/**
 * Select \p iter and load it into UI.
 */
void project_line_select_iter(GtkTreeIter * iter);

/**
 * Import line or project
 */
void project_line_import(void);

/**
 * Export selected line or project.
 */
void project_line_export(void);

/**
 * Frees memory related to project and line.
 */
void project_line_free(void);

G_END_DECLS
#endif				//__UI_PROJECT_LINE_H
