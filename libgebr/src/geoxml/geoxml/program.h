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

#ifndef __GEBR_GEOXML_PROGRAM_H
#define __GEBR_GEOXML_PROGRAM_H

/**
 * \struct GebrGeoXmlProgram program.h geoxml/program.h
 * \brief
 * Represents a program and its parameters.
 * \dot
 * digraph program {
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
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> "GebrGeoXmlFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProgram";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlFlow" -> { "GebrGeoXmlProgram" };
 * 	"GebrGeoXmlProgram" -> "GebrGeoXmlProgramParameter";
 * }
 * \enddot
 * \see program.h
 */

/**
 * \file program.h
 * Represents a program and its parameters.
 *
 * A flow is compounded of multiple programs. Each one treat the
 * that and send its output to next program. GebrGeoXmlProgram intends
 * to keep all the data necessary to deal with seismic processing
 * softwares like Seismic Unix (SU) and Madagascar.
 *
 * Programs has fields describing their role, which are 'title',
 * 'description' and 'help'. 'help' can contain HTML text to rich text
 * help. 'title' and 'description' only accepts plain text.
 *
 *
 * \par References:
 * - Seismic Unix: http://www.cwp.mines.edu/cwpcodes
 * - Madagascar: http://rsf.sourceforge.net
 */

typedef enum {
	GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN = 0,
	GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED,
	GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED,
	GEBR_GEOXML_PROGRAM_STATUS_DISABLED,
} GebrGeoXmlProgramStatus;

/**
 * Promote a sequence to a program.
 */
#define GEBR_GEOXML_PROGRAM(seq) ((GebrGeoXmlProgram*)(seq))

/**
 * The GebrGeoXmlProgram struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_program GebrGeoXmlProgram;

#include <glib.h>

#include "parameter.h"
#include "parameters.h"
#include "program_parameter.h"
#include "flow.h"
#include "macros.h"
#include "object.h"

/**
 * Call \p callback for each parameter of \p program
 *
 * If \p program is NULL nothing is done.
 */
void gebr_geoxml_program_foreach_parameter(GebrGeoXmlProgram * program, GebrGeoXmlCallback callback, gpointer user_data);

/**
 * Get the flow to which \p program belongs to.
 *
 * If \p program is NULL nothing is done.
 */
GebrGeoXmlFlow *gebr_geoxml_program_flow(GebrGeoXmlProgram * program);

/**
 * Get \p program 's parameters list.
 *
 * \see \ref parameters.h "GebrGeoXmlParameters"
 */
GebrGeoXmlParameters *gebr_geoxml_program_get_parameters(GebrGeoXmlProgram * program);

/**
 * Counts the number of parameters in this \p program.
 * Note that groups are not considered as a parameter.
 * \return The number of parameters in this program or -1 if \p program is NULL.
 */
gsize gebr_geoxml_program_count_parameters(GebrGeoXmlProgram * program);

/**
 * Specify wheter \p program accepts standard input or not,
 * depending on \p enable
 *
 * \see gebr_geoxml_program_get_stdin
 */
void gebr_geoxml_program_set_stdin(GebrGeoXmlProgram * program, const gboolean enable);

/**
 * Specify wheter \p program writes standard output or not,
 * depending on \p enable
 *
 * \see gebr_geoxml_program_get_stdout
 */
void gebr_geoxml_program_set_stdout(GebrGeoXmlProgram * program, const gboolean enable);

/**
 * Specify wheter \p program writes standard error or not,
 * depending on \p enable
 
 * \see gebr_geoxml_program_get_stderr
 */
void gebr_geoxml_program_set_stderr(GebrGeoXmlProgram * program, const gboolean enable);

/**
 *
 */
void gebr_geoxml_program_set_status(GebrGeoXmlProgram * program, GebrGeoXmlProgramStatus status);

/**
 *
 */
void gebr_geoxml_program_set_title(GebrGeoXmlProgram * program, const gchar * title);

/**
 *
 */
void gebr_geoxml_program_set_binary(GebrGeoXmlProgram * program, const gchar * binary);

/**
 *
 */
void gebr_geoxml_program_set_description(GebrGeoXmlProgram * program, const gchar * description);

/**
 *
 */
void gebr_geoxml_program_set_help(GebrGeoXmlProgram * program, const gchar * help);

/**
 * Sets the \p program's version attribute.
 *
 * If \p program or \p version is NULL nothing is done.
 */
void gebr_geoxml_program_set_version(GebrGeoXmlProgram * program, const gchar * version);

/**
 * Sets the \p program parallelization implementation to \p parallelization_type.
 *
 * If \p program or \p parallelization_type is NULL nothing is done.
 */
void gebr_geoxml_program_set_parallelization(GebrGeoXmlProgram * program, const gchar * parallelization_type);

/**
 *
 */
void gebr_geoxml_program_set_url(GebrGeoXmlProgram * program, const gchar * url);

/**
 *
 */
gboolean gebr_geoxml_program_get_stdin(GebrGeoXmlProgram * program);

/**
 *
 */
gboolean gebr_geoxml_program_get_stdout(GebrGeoXmlProgram * program);

/**
 *
 */
gboolean gebr_geoxml_program_get_stderr(GebrGeoXmlProgram * program);

/**
 *
 */
GebrGeoXmlProgramStatus gebr_geoxml_program_get_status(GebrGeoXmlProgram * program);

/**
 *
 */
const gchar *gebr_geoxml_program_get_title(GebrGeoXmlProgram * program);

/**
 *
 */
const gchar *gebr_geoxml_program_get_binary(GebrGeoXmlProgram * program);

/**
 *
 */
const gchar *gebr_geoxml_program_get_description(GebrGeoXmlProgram * program);

/**
 *
 */
const gchar *gebr_geoxml_program_get_help(GebrGeoXmlProgram * program);

/**
 * Gets the \p program's version.
 *
 * If \p program is NULL returns NULL.
 */
const gchar *gebr_geoxml_program_get_version(GebrGeoXmlProgram * program);

/**
 * Get \p program's parallelization implementation.
 *
 * If \p program is NULL returns NULL.
 */
const gchar *gebr_geoxml_program_get_parallelization(GebrGeoXmlProgram * program);

/**
 *
 */
const gchar *gebr_geoxml_program_get_url(GebrGeoXmlProgram * program);

#endif				//__GEBR_GEOXML_PROGRAM_H
