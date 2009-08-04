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

#ifndef __LIBGEBR_GEOXML_LINE_H
#define __LIBGEBR_GEOXML_LINE_H

#include <glib.h>

/**
 * \struct GeoXmlLine line.h geoxml/line.h
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
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref GeoXmlLineFlow" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlLine" };
 * 	"GeoXmlSequence" -> { "GeoXmlLineFlow" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlLine" -> { "GeoXmlLineFlow" };
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
 * read or edited you ask the line for its path (see \ref geoxml_line_get_flow_source) and then
 * load it with \ref geoxml_document_load. New flows can be added to a line using
 * \ref geoxml_line_append_flow.
 */

/**
 * Cast line's document \p doc to GeoXmlLine
 */
#define GEOXML_LINE(doc) ((GeoXmlLine*)(doc))

/**
 * Promote a sequence to a line flow.
 */
#define GEOXML_LINE_FLOW(seq) ((GeoXmlLineFlow*)(seq))

/**
 * Line class. Inherits GeoXmlDocument.
 *
 * The GeoXmlLine struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_line GeoXmlLine;

/**
 * Represents a reference to a flow inside a line.
 * It contains the path of the line.
 *
 * The GeoXmlLineFlow struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_line_flow GeoXmlLineFlow;

/**
 *
 *
 * The GeoXmlLinePath struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_line_path GeoXmlLinePath;

#include "sequence.h"
#include "macros.h"

/**
 * Create a new empty line and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GeoXmlLine *
geoxml_line_new();

/**
 * Create a new flow and append it to list of flows references.
 *
 * If \p line or \p source is NULL returns NULL
 */
GeoXmlLineFlow *
geoxml_line_append_flow(GeoXmlLine * line, const gchar * source);

/**
 * Writes to \p line_flow the \p index ieth flow reference that \p line belong.
 * If an error ocurred, the content of \p line_flow is assigned to NULL.
 *
 * If \p line is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_line_get_flow(GeoXmlLine * line, GeoXmlSequence ** line_flow, gulong index);

/**
 * Get the number of flows that \p line has.
 *
 * If \p line is NULL returns -1.
 */
glong
geoxml_line_get_flows_number(GeoXmlLine * line);

/**
 * Set the location of \p line_flow to \p source.
 *
 * If \p line_flow is NULL nothing is done.
 */
void
geoxml_line_set_flow_source(GeoXmlLineFlow * line_flow, const gchar * source);

/**
 * Returns the location of the flow reference \p line_flow.
 *
 * If \p line_flow is NULL returns NULL.
 */
const gchar *
geoxml_line_get_flow_source(GeoXmlLineFlow * line_flow);

/**
 * Creates a new path and append it to the list of path.
 *
 * \see geoxml_line_new_path
 */
GeoXmlLinePath *
geoxml_line_append_path(GeoXmlLine * line, const gchar * path);

/**
 * Writes to \p path the \p index ieth path that belongs to \p line.
 * If an error ocurred, the content of \p path is assigned to NULL.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_line_get_path(GeoXmlLine * line, GeoXmlSequence ** path, gulong index);

/**
 * Get the number of path that \p line has.
 *
 * If \p line is NULL returns -1.
 */
glong
geoxml_line_get_paths_number(GeoXmlLine * line);

#endif //__LIBGEBR_GEOXML_LINE_H
