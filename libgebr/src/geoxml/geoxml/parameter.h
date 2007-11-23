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

#ifndef __LIBGEOXML_PARAMETER_H
#define __LIBGEOXML_PARAMETER_H

/**
 * \struct GeoXmlParameters parameters.h libgeoxml/parameters.h
 * \brief
 * Represents a list of parameters.
 * \dot
 * digraph program {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 *
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
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
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
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
 * Represents a parameter, that is, a GeoXmlParameterGroup or a GeoXmlProgramParameter.
 *
 *
 */

/**
 * Get the base parameter class of \p super , which can
 * be a GeoXmlParameterGroup or GeoXmlProgramParameter instance
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
	 * A parameter able to store a .
	 */
	GEOXML_PARAMETERTYPE_ENUM,
	/**
	 * A parameter able to store a number with maximum and minimum values on it.
	 */
	GEOXML_PARAMETERTYPE_GROUP,
};



#endif //__LIBGEOXML_PARAMETER_H
