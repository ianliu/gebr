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

struct ui_flow_io {
	GtkListStore *store;
	GtkWidget *dialog;
	GtkWidget *treeview;
	GtkWidget *execute_button;
	GtkTreeIter row_new_server;
};

struct ui_flow_simple {
	GtkWidget *dialog;

	gboolean focus_output;

	GtkWidget *input;
	GtkWidget *output;
	GtkWidget *error;
};

/*  */
void flow_io_setup_ui(gboolean executable);

gboolean flow_io_get_selected(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter);
void flow_io_select_iter(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter);
void flow_io_customized_paths_from_line(GtkFileChooser * chooser);
void flow_io_set_server(GtkTreeIter * server_iter, const gchar * input, const gchar * output, const gchar * error);
void flow_io_simple_setup_ui(gboolean focus_output);

void flow_fast_run(void);
void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program, gboolean select_last);

#endif				//__UI_FLOW_H
