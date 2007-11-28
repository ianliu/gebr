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

#ifndef __LIBGEOXML_PROGRAM_H
#define __LIBGEOXML_PROGRAM_H

/**
 * \struct GeoXmlProgram program.h libgeoxml/program.h
 * \brief
 * Represents a program and its parameters.
 * \dot
 * digraph program {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 *		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 *
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlFlow" -> { "GeoXmlProgram" };
 * 	"GeoXmlProgram" -> "GeoXmlProgramParameter";
 * }
 * \enddot
 * \see program.h
 */

/**
 * \file program.h
 * Represents a program and its parameters.
 *
 * A flow is compounded of multiple programs. Each one treat the
 * that and send its output to next program. GeoXmlProgram intends
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

/**
 * Promote a sequence to a program.
 */
#define GEOXML_PROGRAM(seq) ((GeoXmlProgram*)(seq))

/**
 * The GeoXmlProgram struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_program GeoXmlProgram;

#include <glib.h>

#include "program_parameter.h"
#include "flow.h"
#include "macros.h"

/**
 * Get the flow to which \p program belongs to.
 *
 * If \p program is NULL nothing is done.
 */
GeoXmlFlow *
geoxml_program_flow(GeoXmlProgram * program);

/**
 * Create a new parameter.
 * Use geoxml_sequence_prepend or geoxml_sequence_append to add it to the
 * list of parameters.
 */
GeoXmlProgramParameter *
geoxml_program_new_parameter(GeoXmlProgram * program, enum GEOXML_PARAMETERTYPE parameter_type);

/**
 * Get the first paramater of \p program. 
 *
 * \note Due to internal implementation, it is very slow to get the nieth paramater. If you want so, you'll need to call geoxml_sequence_next
 */
GeoXmlProgramParameter *
geoxml_program_get_first_parameter(GeoXmlProgram * program);

/**
 * Get the number of parameters that \p program has.
 *
 * If \p program is NULL returns -1.
 */
glong
geoxml_program_get_parameters_number(GeoXmlProgram * program);

/**
 *
 */
void
geoxml_program_set_stdin(GeoXmlProgram * program, const gboolean enable);

/**
 *
 */
void
geoxml_program_set_stdout(GeoXmlProgram * program, const gboolean enable);

/**
 *
 */
void
geoxml_program_set_stderr(GeoXmlProgram * program, const gboolean enable);

/**
 *
 */
void
geoxml_program_set_status(GeoXmlProgram * program, const gchar * status);

/**
 *
 */
void
geoxml_program_set_title(GeoXmlProgram * program, const gchar * title);

/**
 *
 */
void
geoxml_program_set_menu(GeoXmlProgram * program, const gchar * menu, gulong index);

/**
 *
 */
void
geoxml_program_set_binary(GeoXmlProgram * program, const gchar * binary);

/**
 *
 */
void
geoxml_program_set_description(GeoXmlProgram * program, const gchar * description);

/**
 *
 */
void
geoxml_program_set_help(GeoXmlProgram * program, const gchar * help);

/**
 *
 */
gboolean
geoxml_program_get_stdin(GeoXmlProgram * program);

/**
 *
 */
gboolean
geoxml_program_get_stdout(GeoXmlProgram * program);

/**
 *
 */
gboolean
geoxml_program_get_stderr(GeoXmlProgram * program);

/**
 *
 */
const gchar *
geoxml_program_get_status(GeoXmlProgram * program);

/**
 *
 */
const gchar *
geoxml_program_get_title(GeoXmlProgram * program);

/**
 *
 */
void
geoxml_program_get_menu(GeoXmlProgram * program, gchar ** menu, gulong * index);

/**
 *
 */
const gchar *
geoxml_program_get_binary(GeoXmlProgram * program);

/**
 *
 */
const gchar *
geoxml_program_get_description(GeoXmlProgram * program);

/**
 *
 */
const gchar *
geoxml_program_get_help(GeoXmlProgram * program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_previous instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_previous(GeoXmlProgram ** program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_next instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_next(GeoXmlProgram ** program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_remove(GeoXmlProgram * program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_remove_parameter(GeoXmlProgram * program, GeoXmlProgramParameter * parameter);

#endif //__LIBGEOXML_PROGRAM_H
