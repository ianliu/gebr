/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

struct ui_flow_io {
	GtkWidget *		dialog;

	gboolean		focus_output;

	GtkWidget *		input;
	GtkWidget *		output;
	GtkWidget *		error;
};

struct ui_flow_io *
flow_io_setup_ui(gboolean focus_output);

void
flow_io_customized_paths_from_line(GtkFileChooser * chooser);

void
flow_add_program_sequence_to_view(GeoXmlSequence * program);

#endif //__UI_FLOW_H
