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

#ifndef __GEBR_GEOXML_LINE_H
#define __GEBR_GEOXML_LINE_H

#include <glib.h>

#include "gebr-geo-types.h"

G_BEGIN_DECLS

/**
 * \struct GebrGeoXmlLine line.h geoxml/line.h
 * \brief
 * Line compounds a list of flows references.
 * \dot
 * digraph line {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 *   fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 *   fontsize = 9
 * 	]
 *
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlLineFlow" [ URL = "\ref GebrGeoXmlLineFlow" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> { "GebrGeoXmlLine" };
 * 	"GebrGeoXmlSequence" -> { "GebrGeoXmlLineFlow" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlLine" -> { "GebrGeoXmlLineFlow" };
 * }
 * \enddot
 * \see line.h
 */

/**
 * \file line.h
 * Line compounds a list of flows references.
 *
 * Lines is concept that comes from the way humans work. In seismic processing,
 * flows tends to be grouped by some categorying reason or because some team
 * work on it. Another reason for grouping flow into lines is to separate the work
 * each computer/cluster will do.
 *
 * As flows are kept saved as files, line references them with their path.
 * Therefore, a line is a list of flows files. When a flow needs to be
 * read or edited you ask the line for its path (see \ref gebr_geoxml_line_get_flow_source) and then
 * load it with \ref gebr_geoxml_document_load. New flows can be added to a line using
 * \ref gebr_geoxml_line_append_flow.
 */


/**
 * Create a new empty line and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GebrGeoXmlLine *gebr_geoxml_line_new();

/**
 *
 */
void gebr_geoxml_line_set_base_path(GebrGeoXmlLine *line, const gchar *base);

gchar * gebr_geoxml_line_get_import_path(GebrGeoXmlLine *line);
/**
 * Sets the import path
 */
void gebr_geoxml_line_set_import_path(GebrGeoXmlLine *line, const gchar *import);
/**
 * Create a new flow and append it to list of flows references.
 *
 * If \p line or \p source is NULL returns NULL
 */
GebrGeoXmlLineFlow *gebr_geoxml_line_append_flow(GebrGeoXmlLine * line, const gchar * source);

/**
 * Writes to \p line_flow the \p index ieth flow reference that \p line belong.
 * If an error ocurred, the content of \p line_flow is assigned to NULL.
 *
 * If \p line is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_line_get_flow(GebrGeoXmlLine * line, GebrGeoXmlSequence ** line_flow, gulong index);

/**
 * Get the number of flows that \p line has.
 *
 * If \p line is NULL returns -1.
 */
glong gebr_geoxml_line_get_flows_number(GebrGeoXmlLine * line);

/**
 * Set the location of \p line_flow to \p source.
 *
 * If \p line_flow is NULL nothing is done.
 */
void gebr_geoxml_line_set_flow_source(GebrGeoXmlLineFlow * line_flow, const gchar * source);

/**
 * Returns the location of the flow reference \p line_flow.
 *
 * If \p line_flow is NULL returns NULL.
 */
const gchar *gebr_geoxml_line_get_flow_source(GebrGeoXmlLineFlow * line_flow);

/**
 * Creates a new path and append it to the list of path.
 *
 * \see gebr_geoxml_line_new_path
 */
GebrGeoXmlLinePath *gebr_geoxml_line_append_path(GebrGeoXmlLine * line, const gchar *name, const gchar * path);

/**
 * Writes to \p path the \p index ieth path that belongs to \p line.
 * If an error ocurred, the content of \p path is assigned to NULL.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_line_get_path(GebrGeoXmlLine * line, GebrGeoXmlSequence ** path, gulong index);

/**
 *
 */
gchar *gebr_geoxml_line_path_get_name(GebrGeoXmlLinePath *line_path);

/**
 * Get the number of path that \p line has.
 *
 * If \p line is NULL returns -1.
 */
glong gebr_geoxml_line_get_paths_number(GebrGeoXmlLine * line);

/**
 * gebr_geoxml_line_set_maestro:
 * @line:
 * @maestro_group: the Maestro address.
 */
void gebr_geoxml_line_set_maestro(GebrGeoXmlLine *line,
				  const gchar *maestro_addr);

/**
 * gebr_geoxml_line_get_maestro:
 * @maestro_addr: Return location for the maestro server address.
 */
gchar * gebr_geoxml_line_get_maestro(GebrGeoXmlLine *line);

/**
 * gebr_geoxml_line_get_paths:
 *
 * Returns: A vector of pairs of string of type (path, name).
 * Free with gebr_pair_strfreev().
 */
gchar *** gebr_geoxml_line_get_paths(GebrGeoXmlLine *line);

/**
 * gebr_geoxml_line_get_path_by_name:
 *
 * Returns: %NULL if @line does not have a path named @name. Otherwise, returns
 * a newly allocated string containing the path. Free with g_free().
 */
gchar *gebr_geoxml_line_get_path_by_name(GebrGeoXmlLine *line,
					 const gchar *name);

gchar *gebr_geoxml_escape_path(const gchar *path);

gchar *gebr_geoxml_get_paths_for_base(const gchar *base);

G_END_DECLS
#endif				//__GEBR_GEOXML_LINE_H
