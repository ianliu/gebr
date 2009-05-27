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

#ifndef __LIBGEBR_GEOXML_PARAMETER_H
#define __LIBGEBR_GEOXML_PARAMETER_H

/**
 * \struct GeoXmlParameter parameter.h geoxml/parameter.h
 * \brief
 * Represents a list of parameters.
 * \dot
 * digraph parameter {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlSequence" -> "GeoXmlParameter";
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameters" -> "GeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> "GeoXmlProgram";
 * 	"GeoXmlParameters" -> "GeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GeoXmlProgram" -> "GeoXmlParameters";
 * }
 * \enddot
 * \see parameter.h
 */

/**
 * \file parameter.h
 * Represents a parameter, that is, a GeoXmlParameterGroup or a GeoXmlParameter.
 *
 *
 */

/**
 * Get the base parameter class of \p super , which can
 * be a GeoXmlParameterGroup or GeoXmlParameter instance
 */
#define GEOXML_PARAMETER(super) ((GeoXmlParameter*)(super))

/**
 * The GeoXmlParameter struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameter GeoXmlParameter;

/**
 * \p GEOXML_PARAMETERTYPE lists the program's parameters types
 * supported by libgeoxml. They were made to create a properly
 * link between the seismic processing softwares and this library.
 *
 * \see geoxml_parameter_get_is_program_parameter
 */
enum GEOXML_PARAMETERTYPE {
	/**
	 * In case of error.
	 */
	GEOXML_PARAMETERTYPE_UNKNOWN = 0,
	/**
	 * A parameter able to store a string on it.
	 */
	GEOXML_PARAMETERTYPE_STRING,
	/**
	 * A parameter able to store an integer number on it.
	 */
	GEOXML_PARAMETERTYPE_INT,
	/**
	 * A parameter able to store a file/directory path on it.
	 */
	GEOXML_PARAMETERTYPE_FILE,
	/**
	 * A parameter able to store the state of a flag on it.
	 */
	GEOXML_PARAMETERTYPE_FLAG,
	/**
	 * A parameter able to store a floating point number on it.
	 */
	GEOXML_PARAMETERTYPE_FLOAT,
	/**
	 * A parameter able to store a number with maximum and minimum values on it.
	 */
	GEOXML_PARAMETERTYPE_RANGE,
	/**
	 * A parameter able to store a value in a list options.
	 */
	GEOXML_PARAMETERTYPE_ENUM,
	/**
	 * A sequence of parameters.
	 */
	GEOXML_PARAMETERTYPE_GROUP,
	/**
	 * A reference to other parameter. If the referenced parameter is a program parameter,
	 * then this parameter will only have its value as a difference.
	 */
	GEOXML_PARAMETERTYPE_REFERENCE,
};

#include <glib.h>

#include "parameters.h"

/**
 * Get GeoXmlParameters in which \p parameter is inside
 *
 * If \p parameter is NULL returns NULL
 */
GeoXmlParameters *
geoxml_parameter_get_parameters(GeoXmlParameter * parameter);

/**
 * Change \p parameter type to \p type.
 * Only label of \p parameter will have the same value after the change;
 * all other properties will have their value lost.
 *
 * If \p parameter is NULL nothing is done.
 */
gboolean
geoxml_parameter_set_type(GeoXmlParameter * parameter, enum GEOXML_PARAMETERTYPE type);

/**
 * Change \p parameter to reference \p reference.
 *
 * Return one of GEOXML_RETV_SUCCESS, GEOXML_RETV_REFERENCE_TO_ITSELF, GEOXML_RETV_NULL_PTR
 */
int
geoxml_parameter_set_be_reference(GeoXmlParameter * parameter, GeoXmlParameter * reference);

/**
 * Returns \p parameter 's type.
 * GEOXML_PARAMETERTYPE_REFERENCE is not returned here. The "true" type
 * of the parameter is returned instead.
 *
 * If \p parameter is NULL returns \ref GEOXML_PARAMETERTYPE_UNKNOWN.
 *
 * \see GEOXML_PARAMETERTYPE
 */
enum GEOXML_PARAMETERTYPE
geoxml_parameter_get_type(GeoXmlParameter * parameter);

/**
 * Return the type name of \p parameter
 *
 * Return NULL if \p parameter is NULL
 */
const gchar *
geoxml_parameter_get_type_name(GeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is a reference
 *
 * If \p parameter is NULL return FALSE
 */
gboolean
geoxml_parameter_get_is_reference(GeoXmlParameter * parameter);

/**
 * Get the list of GeoXmlParameter references to \p parameter
 *
 * If \p parameter is NULL returns NULL.
 */
GSList *
geoxml_parameter_get_references_list(GeoXmlParameter * parameter);

/**
 * If \p parameter_reference is of the type GEOXML_PARAMETERTYPE_REFERENCE
 * returns the parameter that it references. Otherwise, returns NULL.
 *
 * If \p parameter_reference is NULL returns NULL.
 */
GeoXmlParameter *
geoxml_parameter_get_referencee(GeoXmlParameter * parameter_reference);

/**
 * Return TRUE if \p parameter is a GeoXmlProgramParameter.
 * All parameters of any GEOXML_PARAMETERTYPE's types except GEOXML_PARAMETERTYPE_GROUP
 * are GeoXmlProgramParameter.
 *
 * If \p parameter is NULL returns FALSE.
 */
gboolean
geoxml_parameter_get_is_program_parameter(GeoXmlParameter * parameter);

/**
 * Set \p parameter 's one line description to \p label.
 *
 * If \p parameter or \p label is NULL nothing is done.
 *
 * \see geoxml_parameter_get_label
 */
void
geoxml_parameter_set_label(GeoXmlParameter * parameter, const gchar * label);

/**
 * Get \p parameter 's one line description.
 *
 * If \p parameter is NULL returns NULL.
 */
const gchar *
geoxml_parameter_get_label(GeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is part of a group
 *
 * If \p parameter is NULL returns FALSE.
 * @see geoxml_parameter_get_group
 */
gboolean
geoxml_parameter_get_is_in_group(GeoXmlParameter * parameter);

/**
 * If \p parameter is in a group, return this group;
 * otherwise, returns NULL.
 *
 * If \p parameter is NULL returns NULL.
 * @see geoxml_parameter_get_is_in_group
 */
GeoXmlParameterGroup *
geoxml_parameter_get_group(GeoXmlParameter * parameter);

/**
 * Reset \p parameter's value and default. If \p recursive, do it for groups and do recursively.
 */
void
geoxml_parameter_reset(GeoXmlParameter * parameter, gboolean recursive);

#endif //__LIBGEBR_GEOXML_PARAMETER_H
