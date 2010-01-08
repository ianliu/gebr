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

#ifndef __GEBR_GEOXML_PROGRAM_PARAMETER_H
#define __GEBR_GEOXML_PROGRAM_PARAMETER_H

#include <glib.h>

#include "macros.h"

/**
 * \struct GebrGeoXmlProgramParameter program_parameter.h geoxml/program_parameter.h
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
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GebrGeoXmlEnumOption" [ URL = "\ref GebrGeoXmlEnumOption" ];
 * 	"GebrGeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GebrGeoXmlPropertyValue" [ URL = "\ref GebrGeoXmlPropertyValue" ];
 * 	"GebrGeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> "GebrGeoXmlFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameter";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlEnumOption";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlValueSequence";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlPropertyValue";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlProgramParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlParameterGroup";
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameterGroup";
 * 	"GebrGeoXmlProgramParameter" -> "GebrGeoXmlEnumOption"
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlFlow" -> "GebrGeoXmlProgram";
 * 	"GebrGeoXmlFlow" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GebrGeoXmlProgram" -> "GebrGeoXmlParameters";
 * 	"GebrGeoXmlParameterGroup" -> "GebrGeoXmlParameters";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1..*"
 * 	]
 * 	"GebrGeoXmlProgramParameter" -> "GebrGeoXmlPropertyValue";
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
 * Promote the instance \p program_parameter of GebrGeoXmlParameter to a GebrGeoXmlProgramParameter.
 */
#define GEBR_GEOXML_PROGRAM_PARAMETER(program_parameter) ((GebrGeoXmlProgramParameter*)(program_parameter))

/**
 * Promote a sequence of property's values \p sequence to a GebrGeoXmlPropertyValue.
 */
#define GEBR_GEOXML_PROPERTY_VALUE(sequence) ((GebrGeoXmlPropertyValue*)(sequence))

/**
 * The GebrGeoXmlProgramParameter struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_program_parameter GebrGeoXmlProgramParameter;

/**
 * The GebrGeoXmlPropertyValue struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_property_value GebrGeoXmlPropertyValue;

#include "program_parameter.h"
#include "program.h"
#include "enum_option.h"
#include "value_sequence.h"
#include "document.h"
#include "macros.h"

/**
 * Get the program to which \p program_parameter belongs to.
 *
 * If \p program_parameter is NULL nothing is done.
 */
GebrGeoXmlProgram *gebr_geoxml_program_parameter_program(GebrGeoXmlProgramParameter * program_parameter);

/**
 * Mark \p program_parameter as required or optional to run its program.
 *
 * If \p program_parameter is NULL nothing is done.
 */
void gebr_geoxml_program_parameter_set_required(GebrGeoXmlProgramParameter * program_parameter, gboolean required);

/**
 *
 *
 * If \p program_parameter is NULL returns FALSE.
 */
gboolean gebr_geoxml_program_parameter_get_required(GebrGeoXmlProgramParameter * program_parameter);

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
 * \see gebr_geoxml_program_parameter_get_keyword
 */
void gebr_geoxml_program_parameter_set_keyword(GebrGeoXmlProgramParameter * program_parameter, const gchar * keyword);

/**
 *
 *
 * If \p program_parameter is NULL returns NULL.
 */
const gchar *gebr_geoxml_program_parameter_get_keyword(GebrGeoXmlProgramParameter * program_parameter);

/**
 * Say that \p program_parameter's that this program_parameter's value is in fact a list of values
 * each one delimited by a separator, if \p is_list is TRUE
 * Flags parameters can't be kept a list of values.
 *
 * \see gebr_geoxml_program_parameter_get_is_list, gebr_geoxml_program_parameter_set_list_separator,
 * gebr_geoxml_program_parameter_get_list_separator
 */
void gebr_geoxml_program_parameter_set_be_list(GebrGeoXmlProgramParameter * program_parameter, gboolean is_list);

/**
 *
 *
 * \see gebr_geoxml_program_parameter_get_is_list, gebr_geoxml_program_parameter_set_be_list,
 * gebr_geoxml_program_parameter_get_list_separator
 */
void


gebr_geoxml_program_parameter_set_list_separator(GebrGeoXmlProgramParameter * program_parameter,
						 const gchar * separator);

/**
 * Set \p program_parameter 's value to \p value.
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * @see gebr_geoxml_program_parameter_set_keyword
 */
void


gebr_geoxml_program_parameter_set_first_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					      const gchar * value);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 */
void


gebr_geoxml_program_parameter_set_first_boolean_value(GebrGeoXmlProgramParameter * program_parameter,
						      gboolean default_value, gboolean enabled);

/**
 * If \p program_parameter is a list, then get the string used to separate
 * the elements of the value.
 *
 * If \p program_parameter is NULL or it is not a list (see \ref gebr_geoxml_program_parameter_set_be_list)
 * NULL is returned
 *
 * \see gebr_geoxml_program_parameter_set_is_list, gebr_geoxml_program_parameter_set_list_separator,
 * gebr_geoxml_program_parameter_get_list_separator
 */
const gchar *gebr_geoxml_program_parameter_get_list_separator(GebrGeoXmlProgramParameter * program_parameter);

/**
 *
 *
 * If \p program_parameter is NULL returns NULL.
 */
const gchar *gebr_geoxml_program_parameter_get_first_value(GebrGeoXmlProgramParameter * program_parameter,
							   gboolean default_value);

/**
 *
 *
 * If \p program_parameter is NULL returns FALSE.
 *
 * @see GEBR_GEOXML_PARAMETER_TYPE_FLAG
 */
gboolean
gebr_geoxml_program_parameter_get_first_boolean_value(GebrGeoXmlProgramParameter * program_parameter,
						      gboolean default_value);

/**
 * Create a new value and append it to list of values of \p program_parameter.
 *
 * If \p program_parameter is NULL returns NULL
 */
GebrGeoXmlPropertyValue *gebr_geoxml_program_parameter_append_value(GebrGeoXmlProgramParameter * program_parameter,
								    gboolean default_value);

/**
 * Writes to \p property_value the \p index ieth value that \p program_parameter has.
 * If an error ocurred, the content of \p property_value is assigned to NULL.
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int


gebr_geoxml_program_parameter_get_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					GebrGeoXmlSequence ** property_value, gulong index);

/**
 * Get the number of values that \p program_parameter has.
 *
 * If \p program_parameter is NULL returns -1.
 */
glong
gebr_geoxml_program_parameter_get_values_number(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value);

/**
 * If \p program_parameter has a list of values, call
 * \ref gebr_geoxml_program_parameter_set_parse_list_value. Otherwise
 * call \ref gebr_geoxml_program_parameter_set_first_value
 *
 * If \p program_parameter or \p value is NULL nothing is done.
 */
void


gebr_geoxml_program_parameter_set_string_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					       const gchar * value);

/**
 * If \p program_parameter has a list of values, return them separated
 * by the list separator. Otherwise, return the same as
 * gebr_geoxml_program_parameter_get_first_value.
 * The GString pointer should be freed by you.
 *
 * * If \p program_parameter is NULL returns NULL.
 */
GString *gebr_geoxml_program_parameter_get_string_value(GebrGeoXmlProgramParameter * program_parameter,
							gboolean default_value);

/**
 * Find the dictionary parameter from \p dict_document
 * that \p program_parameter uses for its value.
 * If \p program_parameter don't use a dictionary parameter value, then NULL is returned.
 *
 * If \p program_parameter or \p dict_document is NULL returns NULL.
 */
GebrGeoXmlProgramParameter *gebr_geoxml_program_parameter_find_dict_parameter(GebrGeoXmlProgramParameter *
									      program_parameter,
									      GebrGeoXmlDocument * dict_document);

/**
 * Copies the value of \p source to \p program_parameter
 * They must be of the same type and both must be list or not.
 * If not returnfs FALSE, otherwise TRUE.
 *
 * If \p program_parameter or \p source is NULL returns FALSE.
 */
gboolean
gebr_geoxml_program_parameter_copy_value(GebrGeoXmlProgramParameter * program_parameter,
					 GebrGeoXmlProgramParameter * source, gboolean default_value);

/**
 * Split \p value using \p program_parameter's separator to
 * create a list of values or default values for it
 *
 * If \p program_parameter or \p value is NULL nothing is done.
 */
void


gebr_geoxml_program_parameter_set_parse_list_value(GebrGeoXmlProgramParameter * program_parameter,
						   gboolean default_value, const gchar * value);

/**
 * Indicates that \p program_parameter will use \p dict_parameter's value as its value-
 * If \p dict_parameter then \p program_parameter will use its own value.
 *
 * If \p program_parameter or \p dict_parameter nothing is done.
 */
void


gebr_geoxml_program_parameter_set_value_from_dict(GebrGeoXmlProgramParameter * program_parameter,
						  GebrGeoXmlProgramParameter * dict_parameter);

/**
 * Change wheter this parameter keeps a file or a directory.
 *
 * If \p program_parameter is NULL nothing is done.
 * @see gebr_geoxml_program_parameter_get_file_be_directory
 */
void


gebr_geoxml_program_parameter_set_file_be_directory(GebrGeoXmlProgramParameter * program_parameter,
						    gboolean is_directory);

/**
 * Return TRUE directory indicador is enabled.
 * Default: FALSE
 *
 * If \p program_parameter is NULL returns NULL.
 *
 * @see GEBR_GEOXML_PARAMETER_TYPE_FILE, gebr_geoxml_program_parameter_set_file_be_directory
 */
gboolean gebr_geoxml_program_parameter_get_file_be_directory(GebrGeoXmlProgramParameter * program_parameter);

/**
 * Filter file system view to show only selected file types.
 * Example: "*jpg *png"
 *
 * If \p program_parameter or \p filter is NULL nothing is done.
 * @see gebr_geoxml_program_parameter_get_file_filter
 */
void


gebr_geoxml_program_parameter_set_file_filter(GebrGeoXmlProgramParameter * program_parameter,
					      const gchar * name, const gchar * pattern);

/**
 * Set \p name and \p pattern the currently set filter
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * @see GEBR_GEOXML_PARAMETER_TYPE_FILE, gebr_geoxml_program_parameter_set_file_filter
 */
void


gebr_geoxml_program_parameter_get_file_filter(GebrGeoXmlProgramParameter * program_parameter,
					      gchar ** name, gchar ** pattern);

/**
 * For integer, float and range parameters type, set mininimum
 * and maximum boundaries
 *
 * If \p program_parameter is NULL nothing is done.
 * @see gebr_geoxml_program_parameter_get_number_min_max
 */
void


gebr_geoxml_program_parameter_set_number_min_max(GebrGeoXmlProgramParameter * program_parameter,
						 const gchar * min, const gchar * max);

/**
 * For integer, float and range parameters type, get mininimum
 * and maximum boundaries
 *
 * If \p program_parameter is NULL nothing is done.
 * @see gebr_geoxml_program_parameter_set_number_min_max
 */
void


gebr_geoxml_program_parameter_get_number_min_max(GebrGeoXmlProgramParameter * program_parameter,
						 gchar ** min, gchar ** max);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 */
void


gebr_geoxml_program_parameter_set_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   const gchar * min, const gchar * max, const gchar * inc,
						   const gchar * digits);

/**
 *
 *
 * If \p program_parameter is NULL nothing is done.
 *
 * @see GEBR_GEOXML_PARAMETER_TYPE_RANGE
 */
void


gebr_geoxml_program_parameter_get_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   gchar ** min, gchar ** max, gchar ** inc, gchar ** digits);

/**
 * Return TRUE if \p program_parameter that this program_parameter's value is in fact a list of values
 * each one delimited by a separator.
 *
 * If \p program_parameter is NULL returns FALSE
 *
 * \see gebr_geoxml_program_parameter_set_is_list, gebr_geoxml_program_parameter_set_list_separator,
 * gebr_geoxml_program_parameter_get_list_separator
 */
gboolean gebr_geoxml_program_parameter_get_is_list(GebrGeoXmlProgramParameter * program_parameter);

/**
 *
 * If \p program_parameter or \p label or \p value is NULL returns NULL.
 */
GebrGeoXmlEnumOption *gebr_geoxml_program_parameter_append_enum_option(GebrGeoXmlProgramParameter * program_parameter,
								       const gchar * label, const gchar * value);

/**
 * Writes to \p enum_option the \p index ieth enum option that \p program_parameter has.
 * If an error ocurred, the content of \p enum_option is assigned to NULL.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_PARAMETER_NOT_ENUM, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int


gebr_geoxml_program_parameter_get_enum_option(GebrGeoXmlProgramParameter * program_parameter,
					      GebrGeoXmlSequence ** enum_option, gulong index);

/**
 * Get the number of enum options in \p program_parameter
 *
 * If \p program_parameter is NULL returns 0.
 */
glong gebr_geoxml_program_parameter_get_enum_options_number(GebrGeoXmlProgramParameter * program_parameter);

#endif				//__GEBR_GEOXML_PROGRAM_PARAMETER_H
