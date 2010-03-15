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

#ifndef __GEBR_GEOXML_PROJECT_H
#define __GEBR_GEOXML_PROJECT_H

/**
 * \struct GebrGeoXmlProject project.h geoxml/project.h
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
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlProject" [ URL = "\ref project.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProjectLine" [ URL = "\ref GebrGeoXmlProjectLine" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> { "GebrGeoXmlProject" };
 * 	"GebrGeoXmlSequence" -> { "GebrGeoXmlProjectLine" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlProject" -> { "GebrGeoXmlProjectLine" };
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
 * read or edited you ask the project for its path (see \ref gebr_geoxml_project_get_line_source) and then
 * load it with \ref gebr_geoxml_document_load. New liness can be added to a project using
 * \ref gebr_geoxml_project_append_line.
 *
 * \see line.h
 */

/**
 * Cast project's document \p doc to GebrGeoXmlProject
 */
#define GEBR_GEOXML_PROJECT(doc) ((GebrGeoXmlProject*)(doc))

/**
 * Promote a sequence to a project line.
 */
#define GEBR_GEOXML_PROJECT_LINE(seq) ((GebrGeoXmlProjectLine*)(seq))

/**
 * Project class. Inherits GebrGeoXmlDocument
 *
 * The GebrGeoXmlProject struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_project GebrGeoXmlProject;

/**
 * Represents a reference to a line inside a project.
 * It contains the path of the line.
 *
 * The GebrGeoXmlProjectLine struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_project_line GebrGeoXmlProjectLine;

#include "sequence.h"
#include "macros.h"

/**
 * Create a new empty project and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GebrGeoXmlProject *gebr_geoxml_project_new();

/**
 * Create a new line and append it to list of flows references.
 *
 * If \p project or \p source is NULL returns NULL.
 */
GebrGeoXmlProjectLine *gebr_geoxml_project_append_line(GebrGeoXmlProject * project, const gchar * source);

/**
 * Removes \p source from \p project if applicable.
 *
 * \return TRUE if \p source was removed, FALSE otherwise.
 */
gboolean gebr_geoxml_project_remove_line(GebrGeoXmlProject * project, const gchar * source);

/**
 * Writes to \p project_line the \p index ieth line reference that \p project belong.
 * If an error ocurred, the content of \p project_line is assigned to NULL.
 *
 * If \p project is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 */
int gebr_geoxml_project_get_line(GebrGeoXmlProject * project, GebrGeoXmlSequence ** project_line, gulong index);

/**
 * Get the number of lines that \p project has.
 *
 * If \p project is NULL returns -1.
 */
glong gebr_geoxml_project_get_lines_number(GebrGeoXmlProject * project);

/**
 * Set the location of \p project_line to \p source.
 *
 * If \p project_line is NULL nothing is done.
 */
void gebr_geoxml_project_set_line_source(GebrGeoXmlProjectLine * project_line, const gchar * source);

/**
 * Returns the location of the line reference \p project_line.
 *
 * If \p project_line is NULL returns NULL.
 */
const gchar *gebr_geoxml_project_get_line_source(GebrGeoXmlProjectLine * project_line);

#endif				//__GEBR_GEOXML_PROJECT_H
