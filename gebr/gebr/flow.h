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
 * @file flow.c Flow manipulation
 */

#ifndef __FLOW_H
#define __FLOW_H

#include "server.h"
#include <libgebr/gui/gebr-gui-html-viewer-widget.h>

G_BEGIN_DECLS

/** 
 * Creates a new flow.
 */
void flow_new(void);
/** 
 * Frees the memory allocated to a flow.
 * Besides, update the detailed view of a flow in the interface.
 */
void flow_free(void);
/** 
 * Delete the selected flow in flow browser.
 */
void flow_delete(gboolean confirm);
/** 
 * Import flow from file to the current line.
 */
void flow_import(void);
/** 
 * Export selected(s) flow(s).
 */
void flow_export(void);
/**
 * Copy all values of parameters linked to dictionaries' parameters.
 */
void flow_copy_from_dicts(GebrGeoXmlFlow * flow);
/**
 * Change all paths to relative or absolute according to \p relative.
 */
void flow_set_paths_to(GebrGeoXmlFlow * flow, gboolean relative);
/** 
 * Runs a flow.
 */
void flow_run(struct server *server, GebrCommServerRunConfig * config, gboolean single);
/**
 * Make a revision from current flow.
 * Opens a dialog asking the user for a comment of it.
 */
gboolean flow_revision_save(void);

/**
 * Remove selected program from flow process.
 */
void flow_program_remove(void);
/**
 * Move selected program to top in the processing flow.
 */
void flow_program_move_top(void);
/** 
 * Move selected program to bottom in the processing flow.
 */
void flow_program_move_bottom(void);

/**
 * Copy selected(s) flows(s) to clipboard.
 */
void flow_copy(void);
/** 
 * Paste flow(s) from clipboard.
 */
void flow_paste(void);
/** 
 * Copy selected(s) program(s) to clipboard.
 */
void flow_program_copy(void);

/** 
 * Paste program(s) from clipboard.
 */
void flow_program_paste(void);

/**
 * gebr_flow_generate_parameter_value_table:
 * @flow: a #GebrGeoXmlFlow
 *
 * Creates a string containing a HTML table for the programs of @flow.
 *
 * Returns: a newly allocated string containing HTML markup.
 */
gchar * gebr_flow_generate_parameter_value_table(GebrGeoXmlFlow * flow);

/**
 * gebr_flow_generate_header:
 * @flow: a #GebrGeoXmlFlow
 * @include_date: whether to include the date
 *
 * Creates a string containing a HTML description of @flow.
 *
 * Returns: a newly allocated string containing HTML markup.
 */
gchar * gebr_flow_generate_header(GebrGeoXmlFlow * flow, gboolean include_date);

/**
 * gebr_flow_get_detailed_report:
 * @flow: a #GebrGeoXmlFlow
 * @include_table: whether to include the parameter/value table 
 * @include_table: whether to include the date in the header
 *
 * Generates the detailed report for @flow and returns it as a string.
 * The detailed report includes a header containing informations such
 * as author, modified date, list of enabled programs, etc.
 * If @include_table is %TRUE, a table containing the parameter label
 * and its value is included.
 *
 * Returns: a newly allocated string, which must be freed with g_free().
 */
gchar * gebr_flow_get_detailed_report (GebrGeoXmlFlow * flow, gboolean include_table, gboolean include_date);

G_END_DECLS

#endif				//__FLOW_H
