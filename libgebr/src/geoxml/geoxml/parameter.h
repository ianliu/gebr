/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
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

#include <glib.h>

/**
 * \p GEOXML_PARAMETERTYPE lists the program's parameters types
 * supported by libgeoxml. They were made to create a properly
 * link between the seismic processing softwares and this library.
 *
 * \see geoxml_parameter_get_is_program_parameter
 */
enum GEOXML_PARAMETERTYPE {
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
};

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
geoxml_parameter_set_type(GeoXmlParameter ** parameter, enum GEOXML_PARAMETERTYPE type);

/**
 * Returns \p parameter 's type.
 *
 * If \p parameter is NULL returns \ref GEOXML_PARAMETERTYPE_STRING.
 *
 * \see GEOXML_PARAMETERTYPE
 */
enum GEOXML_PARAMETERTYPE
geoxml_parameter_get_type(GeoXmlParameter * parameter);

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

#endif //__LIBGEBR_GEOXML_PARAMETER_H
