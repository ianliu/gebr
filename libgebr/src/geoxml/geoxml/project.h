/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_GEOXML_PROJECT_H
#define __LIBGEBR_GEOXML_PROJECT_H

/**
 * \struct GeoXmlProject project.h geoxml/project.h
 * \brief
 * Project compounds a list of lines references.
 * \dot
 * digraph project {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 *		color = palegreen2, style = filled
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
 * 	"GeoXmlProject" [ URL = "\ref project.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProjectLine" [ URL = "\ref GeoXmlProjectLine" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlProject" };
 * 	"GeoXmlSequence" -> { "GeoXmlProjectLine" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlProject" -> { "GeoXmlProjectLine" };
 * }
 * \enddot
 * \see project.h
 */

/**
 * \file project.h
 * Project compounds a list of lines references.
 *
 * Project's idea in analogous to the concept of lines as a container of flows.
 * It groups a number of lines that has some objective in common.
 *
 * Projects references lines using their path.
 * Therefore, a project is a list of lines files. When a line needs to be
 * read or edited you ask the project for its path (see \ref geoxml_project_get_line_source) and then
 * load it with \ref geoxml_document_load. New liness can be added to a project using
 * \ref geoxml_project_append_line.
 *
 * \see line.h
 */

/**
 * Cast project's document \p doc to GeoXmlProject
 */
#define GEOXML_PROJECT(doc) ((GeoXmlProject*)(doc))

/**
 * Promote a sequence to a project line.
 */
#define GEOXML_PROJECT_LINE(seq) ((GeoXmlProjectLine*)(seq))

/**
 * Project class. Inherits GeoXmlDocument
 *
 * The GeoXmlProject struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_project GeoXmlProject;

/**
 * Represents a reference to a line inside a project.
 * It contains the path of the line.
 *
 * The GeoXmlProjectLine struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_project_line GeoXmlProjectLine;

#include "sequence.h"
#include "macros.h"

/**
 * Create a new empty project and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GeoXmlProject *
geoxml_project_new();

/**
 * Create a new line and append it to list of flows references.
 *
 * If \p project or \p source is NULL returns NULL.
 */
GeoXmlProjectLine *
geoxml_project_append_line(GeoXmlProject * project, const gchar * source);

/**
 * Writes to \p project_line the \p index ieth line reference that \p project belong.
 * If an error ocurred, the content of \p project_line is assigned to NULL.
 *
 * If \p project is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 */
int
geoxml_project_get_line(GeoXmlProject * project, GeoXmlSequence ** project_line, gulong index);

/**
 * Get the number of lines that \p project has.
 *
 * If \p project is NULL returns -1.
 */
glong
geoxml_project_get_lines_number(GeoXmlProject * project);

/**
 * Set the location of \p project_line to \p source.
 *
 * If \p project_line is NULL nothing is done.
 */
void
geoxml_project_set_line_source(GeoXmlProjectLine * project_line, const gchar * source);

/**
 * Returns the location of the line reference \p project_line.
 *
 * If \p project_line is NULL returns NULL.
 */
const gchar *
geoxml_project_get_line_source(GeoXmlProjectLine * project_line);

#endif //__LIBGEBR_GEOXML_PROJECT_H
