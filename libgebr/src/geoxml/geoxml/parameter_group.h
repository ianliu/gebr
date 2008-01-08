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

#ifndef __LIBGEBR_GEOXML_PARAMETER_GROUP_H
#define __LIBGEBR_GEOXML_PARAMETER_GROUP_H

/**
 * \struct GeoXmlParameterGroup parameter_group.h geoxml/parameter_group.h
 * \brief
 * A group of parameters.
 * \dot
 * digraph parameter_group {
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
 * \see parameter_group.h
 */

/**
 * The GeoXmlParameterGroup struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameter_group GeoXmlParameterGroup;

#include <glib.h>

/**
 * Instanciate \p parameter_group.
 * Duplicates the parameters of one instance of parameter_group
 */
void
geoxml_parameter_group_instantiate(GeoXmlParameterGroup * parameter_group);

/**
 *
 */
void
geoxml_parameter_group_deinstantiate(GeoXmlParameterGroup * parameter_group);

/**
 *
 * If \p parameter_group is NULL nothing is done.
 */
void
geoxml_parameter_group_set_exclusive(GeoXmlParameterGroup * parameter_group, const gboolean enable);

/**
 * Set it \p parameter_group should be expanded by default, according to \p enable.
 *
 * If \p parameter_group is NULL nothing is done.
 */
void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable);

/**
 * Get the number of instances of \p parameter_group
 *
 * If \p parameter_group is NULL returns 0.
 */
gulong
geoxml_parameter_group_get_instances(GeoXmlParameterGroup * parameter_group);

/**
 * Get the number of parameters that one instance of \p parameter_group has.
 *
 * If \p parameter_group is NULL returns 0.
 */
gulong
geoxml_parameter_group_get_parameters_by_instance(GeoXmlParameterGroup * parameter_group);

/**
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean
geoxml_parameter_group_get_exclusive(GeoXmlParameterGroup * parameter_group);

/**
 * Return TRUE if \p parameter_group should be expanded by default, otherwise returns FALSE.
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group);

#endif //__LIBGEBR_GEOXML_PARAMETER_GROUP_H
