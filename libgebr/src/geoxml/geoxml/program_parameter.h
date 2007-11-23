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

#ifndef __LIBGEOXML_PROGRAM_PARAMETER_H
#define __LIBGEOXML_PROGRAM_PARAMETER_H

#include <glib.h>

/**
 * \struct GeoXmlProgramParameter program_parameter.h libgeoxml/program_parameter.h
 * \brief
 * Represents one of the parameters that describes a program.
 * \dot
 * digraph program_parameter {
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
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 *
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> { "GeoXmlProgram" };
 * 	"GeoXmlProgram" -> "GeoXmlProgramParameter";
 * }
 * \enddot
 * \see program_parameter.h
 */

/**
 * \file program_parameter.h
 * Represents one of the parameters that define the program usage.
 *
 *
 */

/**
 * Promote the instance \p parameter of GeoXmlParameter to a program parameter.
 */
#define GEOXML_PROGRAM_PARAMETER(parameter) ((GeoXmlProgramParameter*)(parameter))

/**
 * The GeoXmlProgramParameter struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_program_parameter GeoXmlProgramParameter;

#include "parameter.h"
#include "program.h"
#include "macros.h"

/**
 * Get the program to which \p parameter belongs to.
 *
 * If \p parameter is NULL nothing is done.
 */
GeoXmlProgram *
geoxml_program_parameter_program(GeoXmlProgramParameter * parameter);

/**
 * Change \p parameter type to \p type.
 * This funcion will create a new parameter and delete the old, as
 * needed to change its type. Because of this, \p parameter pointer
 * will be changed.
 * Keyword and label of \p parameter will have the same value after the change;
 * all other properties will lost their value.
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_type(GeoXmlProgramParameter ** parameter, enum GEOXML_PARAMETERTYPE type);

/**
 * Returns \p parameter 's type.
 *
 * If \p parameter is NULL returns \ref GEOXML_PARAMETERTYPE_STRING.
 *
 * @see GEOXML_PARAMETERTYPE
 */
enum GEOXML_PARAMETERTYPE
geoxml_program_parameter_get_type(GeoXmlProgramParameter * parameter);

/**
 * Mark \p parameter as required or optional to run its program.
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_required(GeoXmlProgramParameter * parameter, gboolean required);

/**
 * Set \p parameter keyword to \p keyword.
 * The keyword is the identifier of the parameter in program. For example,
 * if keyword is 'infile' and value is 'data.su' a command line with
 * 'infile=data.su' will be generated to run the program.
 * If \p parameter is a flag, you should set \p keyword to the string
 * that activates it on the program (e.g. style=normal in SU ximage).
 *
 * If \p parameter is NULL nothing is done.
 *
 * \see geoxml_program_parameter_get_keyword
 */
void
geoxml_program_parameter_set_keyword(GeoXmlProgramParameter * parameter, const gchar * keyword);

/**
 * Set \p parameter 's one line description to \p label.
 *
 * If \p parameter is NULL nothing is done.
 *
 * \see geoxml_program_parameter_get_label
 */
void
geoxml_program_parameter_set_label(GeoXmlProgramParameter * parameter, const gchar * label);

/**
 * Say that \p parameter 'sthat this parameter's value is in fact a list of values
 * each one delimited by a separator, if \p is_list is TRUE
 *
 * \see geoxml_program_parameter_get_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
void
geoxml_program_parameter_set_be_list(GeoXmlProgramParameter * parameter, gboolean is_list);

/**
 *
 *
 * \see geoxml_program_parameter_get_is_list, geoxml_program_parameter_set_be_list,
 * geoxml_program_parameter_get_list_separator
 */
void
geoxml_program_parameter_set_list_separator(GeoXmlProgramParameter * parameter, const gchar * separator);

/**
 * Set \p parameter 's default value to \p value.
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_default(GeoXmlProgramParameter * parameter, const gchar * value);

/**
 * Set \p parameter 's (of type flag) state to \p state.
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_flag_default(GeoXmlProgramParameter * parameter, gboolean state);

/**
 * Set \p parameter 's value to \p value.
 *
 * If \p parameter is NULL nothing is done.
 *
 * @see geoxml_program_parameter_set_keyword
 */
void
geoxml_program_parameter_set_value(GeoXmlProgramParameter * parameter, const gchar * value);

/**
 *
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_flag_state(GeoXmlProgramParameter * parameter, gboolean enabled);

/**
 *
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_file_be_directory(GeoXmlProgramParameter * parameter, gboolean is_directory);

/**
 *
 *
 * If \p parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_range_properties(GeoXmlProgramParameter * parameter,
		const gchar * min, const gchar * max, const gchar * inc);

/**
 *
 *
 * If \p parameter is NULL returns FALSE.
 */
gboolean
geoxml_program_parameter_get_required(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns NULL.
 */
const gchar *
geoxml_program_parameter_get_keyword(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns NULL.
 */
const gchar *
geoxml_program_parameter_get_label(GeoXmlProgramParameter * parameter);


/**
 * Return TRUE if \p parameter that this parameter's value is in fact a list of values
 * each one delimited by a separator, if \p is_list is TRUE.
 *
 * If \p parameter is NULL returns FALSE
 *
 * \see geoxml_program_parameter_set_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
gboolean
geoxml_program_parameter_get_is_list(GeoXmlProgramParameter * parameter);

/**
 * If \p parameter is a list, then get the string used to separate
 * the elements of the value.
 *
 * If \p parameter is NULL or it is not a list (see \ref geoxml_program_parameter_set_be_list)
 * NULL is returned
 *
 * \see geoxml_program_parameter_set_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
const gchar *
geoxml_program_parameter_get_list_separator(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns NULL.
 *
 * \see geoxml_program_parameter_set_default
 */
const gchar *
geoxml_program_parameter_get_default(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns FALSE.
 *
 * \see geoxml_program_parameter_set_flag_default, geoxml_program_parameter_get_default,
 * geoxml_program_parameter_set_default
 */
gboolean
geoxml_program_parameter_get_flag_default(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns NULL.
 */
const gchar *
geoxml_program_parameter_get_value(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL or returns FALSE.
 *
 * @see GEOXML_PARAMETERTYPE_FLAG
 */
gboolean
geoxml_program_parameter_get_flag_status(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL returns NULL.
 *
 * @see GEOXML_PARAMETERTYPE_FILE
 */
gboolean
geoxml_program_parameter_get_file_be_directory(GeoXmlProgramParameter * parameter);

/**
 *
 *
 * If \p parameter is NULL nothing is done.
 *
 * @see GEOXML_PARAMETERTYPE_RANGE
 */
void
geoxml_program_parameter_get_range_properties(GeoXmlProgramParameter * parameter,
		gchar ** min, gchar ** max, gchar ** inc);

/**
 * \deprecated
 * Use \ref geoxml_sequence_previous instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_parameter_previous(GeoXmlProgramParameter ** parameter);

/**
* \deprecated
 * Use \ref geoxml_sequence_next instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_parameter_next(GeoXmlProgramParameter ** parameter);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_program_parameter_remove(GeoXmlProgramParameter * parameter);

#endif //__LIBGEOXML_PROGRAM_PARAMETER_H
