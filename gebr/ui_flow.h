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

/**
 * @file ui_flow.h Server IO Gui
 * @ingroup gebr
 */

#ifndef __UI_FLOW_H
#define __UI_FLOW_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

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
 * Runs the last used IO configuration.
 */
void gebr_ui_flow_run(gboolean parellel, gboolean single);

/**
 * flow_add_program_sequence_to_view:
 * @program: A #GebrGeoXmlSequence of #GebrGeoXmlProgram to be added to the view.
 * @select_last: Whether to select the last program.
 *
 * Adds all programs in the sequence @program into the flow edition view.
 */
void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program,
				       gboolean select_last,
				       gboolean never_opened);

/**
 * Checks if the first program has input entrance, the last one has output exit and if even one of then has error exit.
 * If one of this is false, so the respective component are made insensitive. 
 */
void flow_program_check_sensitiveness (void);

G_END_DECLS

#endif				//__UI_FLOW_H
