/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

#ifndef __LIBGEOXML_PROJECT_H
#define __LIBGEOXML_PROJECT_H

/**
 * \struct GeoXmlProject project.h libgeoxml/project.h
 * \brief
 * Project compounds a list of lines references.
 * \dot
 * digraph project {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 *		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlProject" [ URL = "\ref project.h" ];
 * 
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlProject" };
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
 * \ref geoxml_project_add_line, or removed with \ref geoxml_project_remove_line.
 * Iteration can be done combining \ref geoxml_project_get_line and \ref geoxml_project_next_line.
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

#include "macros.h"

/**
 * Create a new empty project and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GeoXmlProject *
geoxml_project_new();

/**
 * Creates a new line reference located at \p source and returns a pointer to it.
 * The line returned should be added using geoxml_sequence_prepend or geoxml_sequence_append
 *
 * If \p project is NULL returns NULL.
 */
GeoXmlProjectLine *
geoxml_project_new_line(GeoXmlProject * project, const gchar * source);

/**
 * Writes to \p project_line the \p index ieth line reference that \p project belong.
 * If an error ocurred, the content of \p project_line is assigned to NULL.
 *
 * If \p project is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 */
int
geoxml_project_get_line(GeoXmlProject * project, GeoXmlProjectLine ** project_line, gulong index);

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

/**
 * \deprecated
 * Use geoxml_sequence_previous instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_project_previous_line(GeoXmlProjectLine ** project_line);

/**
 * \deprecated
 * Use geoxml_sequence_next instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_project_next_line(GeoXmlProjectLine ** project_line);

/**
 * \deprecated
 * See geoxml_project_new_line instead. Kept only for backwards compatible and should not be used in newly written code
 */
GeoXmlProjectLine * GEOXML_DEPRECATED
geoxml_project_add_line(GeoXmlProject * project, const gchar * source);

/**
 * \deprecated
 * Use geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_project_remove_line(GeoXmlProject * project, GeoXmlProjectLine * project_line);

#endif //__LIBGEOXML_PROJECT_H
