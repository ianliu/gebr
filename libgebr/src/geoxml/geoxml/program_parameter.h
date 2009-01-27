/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#ifndef __LIBGEBR_GEOXML_PROGRAM_PARAMETER_H
#define __LIBGEBR_GEOXML_PROGRAM_PARAMETER_H

#include <glib.h>

#include "macros.h"

/**
 * \struct GeoXmlProgramParameter program_parameter.h geoxml/program_parameter.h
 * \brief
 * Represents one of the program_parameters that describes a program.
 * \dot
 * digraph program_parameter {
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
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref GeoXmlEnumOption" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlPropertyValue" [ URL = "\ref GeoXmlPropertyValue" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlSequence" -> "GeoXmlParameter";
 * 	"GeoXmlSequence" -> "GeoXmlEnumOption";
 * 	"GeoXmlSequence" -> "GeoXmlValueSequence";
 * 	"GeoXmlValueSequence" -> "GeoXmlCategory";
 * 	"GeoXmlValueSequence" -> "GeoXmlPropertyValue";
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameters" -> "GeoXmlParameterGroup";
 * 	"GeoXmlProgramParameter" -> "GeoXmlEnumOption"
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> "GeoXmlProgram";
 * 	"GeoXmlFlow" -> "GeoXmlCategory";
 * 	"GeoXmlParameters" -> "GeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GeoXmlProgram" -> "GeoXmlParameters";
 * 	"GeoXmlParameterGroup" -> "GeoXmlParameters";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1..*"
 * 	]
 * 	"GeoXmlProgramParameter" -> "GeoXmlPropertyValue";
 * }
 * \enddot
 * \see program_parameter.h
 */

/**
 * \file program_parameter.h
 * Represents one of the program_parameters that define the program usage.
 *
 *
 */

/**
 * Promote the instance \p program_parameter of GeoXmlParameter to a GeoXmlProgramParameter.
 */
#define GEOXML_PROGRAM_PARAMETER(program_parameter) ((GeoXmlProgramParameter*)(program_parameter))

/**
 * Promote a sequence of property's values \p sequence to a GeoXmlPropertyValue.
 */
#define GEOXML_PROPERTY_VALUE(sequence) ((GeoXmlPropertyValue*)(sequence))

/**
 * The GeoXmlProgramParameter struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_program_parameter GeoXmlProgramParameter;

/**
 * The GeoXmlPropertyValue struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_property_value GeoXmlPropertyValue;

#include "program_parameter.h"
#include "program.h"
#include "enum_option.h"
#include "value_sequence.h"
#include "macros.h"

/**
 * Get the program to which \p program_parameter belongs to.
 *
 * If \p program_parameter is NULL nothing is done.
 */
GeoXmlProgram *
geoxml_program_parameter_program(GeoXmlProgramParameter * program_parameter);

/**
 * Mark \p program_parameter as required or optional to run its program.
 *
 * If \p program_parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_required(GeoXmlProgramParameter * program_parameter, gboolean required);

/**
 *
 *
 * If \p program_parameter is NULL returns FALSE.
 */
gboolean
geoxml_program_parameter_get_required(GeoXmlProgramParameter * program_parameter);

/**
 * Set \p program_parameter keyword to \p keyword.
 * The keyword is the identifier of the program_parameter in program. For example,
 * if keyword is 'infile' and value is 'data.su' a command line with
 * 'infile=data.su' will be generated to run the program.
 * If \p program_parameter is a flag, you should set \p keyword to the string
 * that activates it on the program (e.g. style=normal in SU ximage).
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * \see geoxml_program_parameter_get_keyword
 */
void
geoxml_program_parameter_set_keyword(GeoXmlProgramParameter * program_parameter, const gchar * keyword);

/**
 *
 *
 * If \p program_parameter is NULL returns NULL.
 */
const gchar *
geoxml_program_parameter_get_keyword(GeoXmlProgramParameter * program_parameter);

/**
 * Say that \p program_parameter's that this program_parameter's value is in fact a list of values
 * each one delimited by a separator, if \p is_list is TRUE
 * Flags parameters can't be kept a list of values.
 *
 * \see geoxml_program_parameter_get_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
void
geoxml_program_parameter_set_be_list(GeoXmlProgramParameter * program_parameter, gboolean is_list);

/**
 *
 *
 * \see geoxml_program_parameter_get_is_list, geoxml_program_parameter_set_be_list,
 * geoxml_program_parameter_get_list_separator
 */
void
geoxml_program_parameter_set_list_separator(GeoXmlProgramParameter * program_parameter, const gchar * separator);

/**
 * Set \p program_parameter 's value to \p value.
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * @see geoxml_program_parameter_set_keyword
 */
void
geoxml_program_parameter_set_first_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_first_boolean_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, gboolean enabled);

/**
 * If \p program_parameter is a list, then get the string used to separate
 * the elements of the value.
 *
 * If \p program_parameter is NULL or it is not a list (see \ref geoxml_program_parameter_set_be_list)
 * NULL is returned
 *
 * \see geoxml_program_parameter_set_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
const gchar *
geoxml_program_parameter_get_list_separator(GeoXmlProgramParameter * program_parameter);

/**
 *
 *
 * If \p program_parameter is NULL returns NULL.
 */
const gchar *
geoxml_program_parameter_get_first_value(GeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 *
 *
 * If \p program_parameter is NULL returns FALSE.
 *
 * @see GEOXML_PARAMETERTYPE_FLAG
 */
gboolean
geoxml_program_parameter_get_first_boolean_value(GeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 * Create a new value and append it to list of values of \p program_parameter.
 *
 * If \p program_parameter is NULL returns NULL
 */
GeoXmlPropertyValue *
geoxml_program_parameter_append_value(GeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 * Writes to \p property_value the \p index ieth value that \p program_parameter has.
 * If an error ocurred, the content of \p property_value is assigned to NULL.
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_program_parameter_get_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, GeoXmlSequence ** property_value, gulong index);

/**
 * Get the number of values that \p program_parameter has.
 *
 * If \p program_parameter is NULL returns -1.
 */
glong
geoxml_program_parameter_get_values_number(GeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 * If \p program_parameter has a list of values, call
 * \ref geoxml_program_parameter_set_parse_list_value. Otherwise
 * call \ref geoxml_program_parameter_set_first_value
 *
 * If \p program_parameter or \p value is NULL nothing is done.
 */
void
geoxml_program_parameter_set_string_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value);

/**
 * If \p program_parameter has a list of values, return them separated
 * by the list separator. Otherwise, return the same as
 * geoxml_program_parameter_get_first_value.
 * The GString pointer should be freed by you.
 *
 * * If \p program_parameter is NULL returns NULL.
 */
GString *
geoxml_program_parameter_get_string_value(GeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 * Split \p value using \p program_parameter's separator to
 * create a list of values or default values for it
 *
 * If \p program_parameter or \p value is NULL nothing is done.
 */
void
geoxml_program_parameter_set_parse_list_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_file_be_directory(GeoXmlProgramParameter * program_parameter, gboolean is_directory);

/**
 *
 *
 * If \p program_parameter is NULL returns NULL.
 *
 * @see GEOXML_PARAMETERTYPE_FILE
 */
gboolean
geoxml_program_parameter_get_file_be_directory(GeoXmlProgramParameter * program_parameter);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 */
void
geoxml_program_parameter_set_range_properties(GeoXmlProgramParameter * program_parameter,
	const gchar * min, const gchar * max, const gchar * inc, const gchar * digits);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * @see GEOXML_PARAMETERTYPE_RANGE
 */
void
geoxml_program_parameter_get_range_properties(GeoXmlProgramParameter * program_parameter,
	gchar ** min, gchar ** max, gchar ** inc, gchar ** digits);

/**
 * Return TRUE if \p program_parameter that this program_parameter's value is in fact a list of values
 * each one delimited by a separator.
 *
 * If \p program_parameter is NULL returns FALSE
 *
 * \see geoxml_program_parameter_set_is_list, geoxml_program_parameter_set_list_separator,
 * geoxml_program_parameter_get_list_separator
 */
gboolean
geoxml_program_parameter_get_is_list(GeoXmlProgramParameter * program_parameter);

/**
 *
 * If \p program_parameter or \p label or \p value is NULL returns NULL.
 */
GeoXmlEnumOption *
geoxml_program_parameter_append_enum_option(GeoXmlProgramParameter * program_parameter,
	const gchar * label, const gchar * value);

/**
 * Writes to \p enum_option the \p index ieth enum option that \p program_parameter has.
 * If an error ocurred, the content of \p enum_option is assigned to NULL.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_PARAMETER_NOT_ENUM, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_program_parameter_get_enum_option(GeoXmlProgramParameter * program_parameter, GeoXmlSequence ** enum_option, gulong index);

/**
 * Get the number of enum options in \p program_parameter
 *
 * If \p program_parameter is NULL returns 0.
 */
glong
geoxml_program_parameter_get_enum_options_number(GeoXmlProgramParameter * program_parameter);

#endif //__LIBGEBR_GEOXML_PROGRAM_PARAMETER_H
