/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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
 * 	"GeoXmlParameterGroup" -> "GeoXmlParameters";
 * }
 * \enddot
 * \see parameter_group.h
 */

#include "parameters.h"

/**
 * The GeoXmlParameterGroup struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameter_group GeoXmlParameterGroup;

/**
 * Cast from a GeoXmlSequence to a GeoXmlParameterGroup
 */
#define GEOXML_PARAMETER_GROUP(seq) ((GeoXmlParameterGroup*)(seq))

#include <glib.h>

/**
 * Get \p group's parameters list.
 *
 * \see \ref parameters.h "GeoXmlParameters"
 */
GeoXmlParameters *
geoxml_parameter_group_get_parameters(GeoXmlParameterGroup * parameter_group);

/**
 * Get the first parameter of the last instance of \p parameter_group
 */
GeoXmlParameter *
geoxml_parameter_group_last_instance_parameter(GeoXmlParameterGroup * parameter_group);

/**
 * Instanciate \p parameter_group.
 * Duplicates the parameters of one instance of parameter_group
 */
gboolean
geoxml_parameter_group_instanciate(GeoXmlParameterGroup * parameter_group);

/**
 *
 */
gboolean
geoxml_parameter_group_deinstanciate(GeoXmlParameterGroup * parameter_group);

/**
 * Get the number of instances of \p parameter_group
 *
 * If \p parameter_group is NULL returns 0.
 */
gulong
geoxml_parameter_group_get_instances(GeoXmlParameterGroup * parameter_group);

/**
 * Set wheter \p parameter_group can have more than one instance.
 * If \p can_instanciate is FALSE, group is deinstanciated till it has only one
 * one instance
 *
 */
void
geoxml_parameter_group_set_can_instanciate(GeoXmlParameterGroup * parameter_group, gboolean can_instanciate);

/**
 * Get the number of parameters that one instance of \p parameter_group has.
 *
 * If \p parameter_group is NULL nothing is done.
 */
void
geoxml_parameter_group_set_parameters_by_instance(GeoXmlParameterGroup * parameter_group, gulong number);

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
 * Get wheter \p parameter_group can have more than one instance.
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean
geoxml_parameter_group_get_can_instanciate(GeoXmlParameterGroup * parameter_group);

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
