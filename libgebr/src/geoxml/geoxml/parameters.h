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

#ifndef __GEBR_GEOXML_PARAMETERS_H
#define __GEBR_GEOXML_PARAMETERS_H

/**
 * \struct GebrGeoXmlParameters parameters.h geoxml/parameters.h
 * \brief
 * Represents a list of parameters.
 * \dot
 * digraph parameters {
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
 * 	"GebrGeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> "GebrGeoXmlFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlProgramParameter";
 * 	"GebrGeoXmlParameter" -> "GebrGeoXmlParameters";
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
 * 	"GebrGeoXmlParameters" -> "GebrGeoXmlParameters";
 * }
 * \enddot
 * \see parameters.h
 */

/**
 * Cast a GebrGeoXmlSequence to a GebrGeoXmlParameters.
 */
#define GEBR_GEOXML_PARAMETERS(seq) ((GebrGeoXmlParameters*)(seq))

/**
 * The GebrGeoXmlParameters struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_parameters GebrGeoXmlParameters;

#include <glib.h>

#include "parameter_group.h"
#include "parameter.h"
#include "sequence.h"

/**
 * Create a new parameter and append it to \p parameters.
 *
 * FIXME: If \p parameters is a from a group parameter and it has more than one instance,
 * then NULL is returned
 *
 * If \p parameters is NULL returns NULL.
 */
GebrGeoXmlParameter *gebr_geoxml_parameters_append_parameter(GebrGeoXmlParameters * parameters,
							     enum GEBR_GEOXML_PARAMETER_TYPE type);

/**
 * Set this \p parameters to be an exclusive group and \p parameter
 * to be its exclusive parameter by default.
 * If \p parameter is NULL then you want to use the group in non-exclusive mode
 *
 * If \p parameters is NULL nothing is done.
 *
 * \see gebr_geoxml_parameters_get_exclusive
 * gebr_geoxml_parameters_set_selected
 */
void gebr_geoxml_parameters_set_exclusive(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter);

/**
 *
 * If \p parameters is NULL returns FALSE.
 */
GebrGeoXmlParameter *gebr_geoxml_parameters_get_exclusive(GebrGeoXmlParameters * parameters);

/**
 * If \p parameters is a exclusive group (aka has a non-NULL exclusive parameter set)
 * then select \p parameter to be current derised for use parameter; otherwise, nothing is done.
 *
 * If \p parameters is NULL nothing is done.
 */
void gebr_geoxml_parameters_set_selected(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter);

/**
 * If \p parameters is a exclusive group (aka has a non-NULL exclusive parameter set)
 * then select \p parameter to be current derised for use parameter; otherwise, returns NULL
 *
 * If \p parameters is NULL returns NULL.
 */
GebrGeoXmlParameter *gebr_geoxml_parameters_get_selected(GebrGeoXmlParameters * parameters);

/**
 * Get the first paramater of \p program.
 *
 * If \p parameters is NULL returns NULL.
 */
GebrGeoXmlSequence *gebr_geoxml_parameters_get_first_parameter(GebrGeoXmlParameters * parameters);

/**
 * Get the parameter at \p index
 */
int gebr_geoxml_parameters_get_parameter(GebrGeoXmlParameters * parameters, GebrGeoXmlSequence ** parameter,
					 gulong index);

/**
 * Get the number of parameters that \p parameters has.
 *
 * If \p parameters is NULL returns -1.
 */
glong gebr_geoxml_parameters_get_number(GebrGeoXmlParameters * parameters);

/**
 * Return TRUE if \p parameters is part of a group
 *
 * If \p parameters is NULL returns FALSE.
 * @see gebr_geoxml_parameters_get_group
 */
gboolean gebr_geoxml_parameters_get_is_in_group(GebrGeoXmlParameters * parameters);

/**
 * If \p parameters is in a group, return this group;
 * otherwise, returns NULL.
 *
 * If \p parameters is NULL returns NULL.
 * @see gebr_geoxml_parameters_get_is_in_group
 */
GebrGeoXmlParameterGroup *gebr_geoxml_parameters_get_group(GebrGeoXmlParameters * parameters);

/**
 * Reset \p parameters' values and default values.
 * If \p recursive is true also do it for group (recursively)
 */
void gebr_geoxml_parameters_reset(GebrGeoXmlParameters * parameters, gboolean recursive);

#endif				//__GEBR_GEOXML_PARAMETERS_H
