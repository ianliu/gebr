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

#ifndef __GEBR_GEOXML_PARAMETER_H
#define __GEBR_GEOXML_PARAMETER_H

/**
 * \struct GebrGeoXmlParameter parameter.h geoxml/parameter.h
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
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GebrGeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> "GebrGeoXmlFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlProgramParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlParameterGroup";
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlFlow" -> "GebrGeoXmlProgram";
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GebrGeoXmlProgram" -> "GebrGeoXmlParameters";
 * }
 * \enddot
 * \see parameter.h
 */

/**
 * \file parameter.h
 * Represents a parameter, that is, a GebrGeoXmlParameterGroup or a GebrGeoXmlParameter.
 *
 *
 */

/**
 * Get the base parameter class of \p super , which can
 * be a GebrGeoXmlParameterGroup or GebrGeoXmlParameter instance
 */
#define GEBR_GEOXML_PARAMETER(super) ((GebrGeoXmlParameter*)(super))

/**
 * The GebrGeoXmlParameter struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_parameter GebrGeoXmlParameter;

/**
 * \p GEBR_GEOXML_PARAMETER_TYPE lists the program's parameters types
 * supported by libgeoxml. They were made to create a properly
 * link between the seismic processing softwares and this library.
 *
 * \see gebr_geoxml_parameter_get_is_program_parameter
 */
enum GEBR_GEOXML_PARAMETER_TYPE {
	/**
	 * In case of error.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN = 0,
	/**
	 * A parameter able to store a string on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_STRING,
	/**
	 * A parameter able to store an integer number on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_INT,
	/**
	 * A parameter able to store a file/directory path on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_FILE,
	/**
	 * A parameter able to store the state of a flag on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_FLAG,
	/**
	 * A parameter able to store a floating point number on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	/**
	 * A parameter able to store a number with maximum and minimum values on it.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_RANGE,
	/**
	 * A parameter able to store a value in a list options.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_ENUM,
	/**
	 * A sequence of parameters.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_GROUP,
	/**
	 * A reference to other parameter. If the referenced parameter is a program parameter,
	 * then this parameter will only have its value as a difference.
	 */
	GEBR_GEOXML_PARAMETER_TYPE_REFERENCE,
};

#include <glib.h>

#include "parameters.h"

/**
 * Get GebrGeoXmlParameters in which \p parameter is inside
 *
 * If \p parameter is NULL returns NULL
 */
GebrGeoXmlParameters *
gebr_geoxml_parameter_get_parameters(GebrGeoXmlParameter * parameter);

/**
 * Change \p parameter type to \p type.
 * Only label of \p parameter will have the same value after the change;
 * all other properties will have their value lost.
 *
 * If \p parameter is NULL nothing is done.
 */
gboolean
gebr_geoxml_parameter_set_type(GebrGeoXmlParameter * parameter, enum GEBR_GEOXML_PARAMETER_TYPE type);

/**
 * Change \p parameter to reference \p reference.
 *
 * Return one of GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_REFERENCE_TO_ITSELF, GEBR_GEOXML_RETV_NULL_PTR
 */
int
gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter, GebrGeoXmlParameter * reference);

/**
 * Returns \p parameter 's type.
 * GEBR_GEOXML_PARAMETER_TYPE_REFERENCE is not returned here. The "true" type
 * of the parameter is returned instead.
 *
 * If \p parameter is NULL returns \ref GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN.
 *
 * \see GEBR_GEOXML_PARAMETER_TYPE
 */
enum GEBR_GEOXML_PARAMETER_TYPE
gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter);

/**
 * Return the type name of \p parameter
 *
 * Return NULL if \p parameter is NULL
 */
const gchar *
gebr_geoxml_parameter_get_type_name(GebrGeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is a reference
 *
 * If \p parameter is NULL return FALSE
 */
gboolean
gebr_geoxml_parameter_get_is_reference(GebrGeoXmlParameter * parameter);

/**
 * Get the list of GebrGeoXmlParameter references to \p parameter
 *
 * If \p parameter is NULL returns NULL.
 */
GSList *
gebr_geoxml_parameter_get_references_list(GebrGeoXmlParameter * parameter);

/**
 * If \p parameter_reference is of the type GEBR_GEOXML_PARAMETER_TYPE_REFERENCE
 * returns the parameter that it references. Otherwise, returns NULL.
 *
 * If \p parameter_reference is NULL returns NULL.
 */
GebrGeoXmlParameter *
gebr_geoxml_parameter_get_referencee(GebrGeoXmlParameter * parameter_reference);

/**
 * Return TRUE if \p parameter is a GebrGeoXmlProgramParameter.
 * All parameters of any GEBR_GEOXML_PARAMETER_TYPE's types except GEBR_GEOXML_PARAMETER_TYPE_GROUP
 * are GebrGeoXmlProgramParameter.
 *
 * If \p parameter is NULL returns FALSE.
 */
gboolean
gebr_geoxml_parameter_get_is_program_parameter(GebrGeoXmlParameter * parameter);

/**
 * Set \p parameter 's one line description to \p label.
 *
 * If \p parameter or \p label is NULL nothing is done.
 *
 * \see gebr_geoxml_parameter_get_label
 */
void
gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label);

/**
 * Get \p parameter 's one line description.
 *
 * If \p parameter is NULL returns NULL.
 */
const gchar *
gebr_geoxml_parameter_get_label(GebrGeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is part of a group
 *
 * If \p parameter is NULL returns FALSE.
 * @see gebr_geoxml_parameter_get_group
 */
gboolean
gebr_geoxml_parameter_get_is_in_group(GebrGeoXmlParameter * parameter);

/**
 * If \p parameter is in a group, return this group;
 * otherwise, returns NULL.
 *
 * If \p parameter is NULL returns NULL.
 * @see gebr_geoxml_parameter_get_is_in_group
 */
GebrGeoXmlParameterGroup *
gebr_geoxml_parameter_get_group(GebrGeoXmlParameter * parameter);

/**
 * Reset \p parameter's value and default. If \p recursive, do it for groups and do recursively.
 */
void
gebr_geoxml_parameter_reset(GebrGeoXmlParameter * parameter, gboolean recursive);

#endif //__GEBR_GEOXML_PARAMETER_H
