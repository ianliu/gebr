/**
 * @file ui_flow.h Server IO Gui
 * @ingroup gebr
 */

/*   GeBR - An environment for seismic processing.
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

#ifndef __UI_FLOW_H
#define __UI_FLOW_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>

enum {
	FLOW_IO_ICON,
	FLOW_IO_SERVER_NAME,
	FLOW_IO_INPUT,
	FLOW_IO_OUTPUT,
	FLOW_IO_ERROR,
	FLOW_IO_SERVER_LISTED,
	FLOW_IO_FLOW_SERVER_POINTER,
	FLOW_IO_SERVER_POINTER,
	FLOW_IO_IS_SERVER_ADD,
	FLOW_IO_IS_SERVER_ADD2,
	FLOW_IO_N
};

/**
 * Structure to work with Server IO dialog.
 */
struct ui_flow_io {
	GtkListStore *store;		/**< Model to hold server IO entries. */
	GtkWidget *dialog;		/**< #GtkDialog to be shown to the user. */
	GtkWidget *treeview;		/**< #GtkTreeView to present model data. */
	GtkWidget *execute_button;	/**< The execute button. */
	GtkTreeIter row_new_server;	/**< Special row, that is used to add new rows. */
};

/**
 * The simple IO dialog.
 */
struct ui_flow_simple {
	GtkWidget *dialog;

	gboolean focus_output;

	GtkWidget *input;
	GtkWidget *output;
	GtkWidget *error;
};

/**
 * A dialog for user selection of the flow IO files.
 * 
 * @param executable Whether execute button is visible or not.
 */
void flow_io_setup_ui(gboolean executable);

/**
 * Fills in \p inter a reference for the selected row if there is a selection.
 * @param iter A reference to a #GtkTreeIter or #NULL.
 * @return #TRUE if there is a selection, #FALSE otherwise.
 */
gboolean flow_io_get_selected(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter);

/**
 * Sets the selection of the server dialog to \p iter.
 * You must be sure \p iter is valid.
 *
 * @param iter The new row to be selected.
 */
void flow_io_select_iter(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter);

/**
 * Add line's paths as shortcuts in \p chooser.
 * @param chooser The #GtkFileChooser to have the shortcuts set.
 */
void flow_io_customized_paths_from_line(GtkFileChooser * chooser);

/**
 * Sets the IO information for server pointed by \p server_iter.
 * @param server_iter The iterator for \ref gebr.ui_server_list->common.store.
 * @param input Input file.
 * @param output Output file.
 * @param error Error file.
 */
void flow_io_set_server(GtkTreeIter * server_iter, const gchar * input, const gchar * output, const gchar * error);

/**
 * Creates a simple dialog that permits IO configuration for the currently selected server.
 * @param focus_output Whether to focus the output entry.
 */
void flow_io_simple_setup_ui(gboolean focus_output);

/**
 * Runs the last used IO configuration.
 */
void flow_fast_run(void);

/**
 * Adds all programs in the sequence \p program into the flow edition view.
 * @param program A #GebrGeoXmlSequence of #GebrGeoXmlProgram to be added to the view.
 * @param select_last Whether to select the last program.
 */
void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program, gboolean select_last);

#endif				//__UI_FLOW_H
