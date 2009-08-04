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

/**
 * The GeoXmlParameterGroup struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_parameter_group GeoXmlParameterGroup;

/**
 * Cast from a GeoXmlSequence to a GeoXmlParameterGroup
 */
#define GEOXML_PARAMETER_GROUP(seq) ((GeoXmlParameterGroup*)(seq))

#include <glib.h>

#include "parameters.h"
#include "sequence.h"

/**
 * Instanciate \p parameter_group, appending a new instances
 * (which is a clone of the first instance) to the list of instances.
 */
GeoXmlParameters *
geoxml_parameter_group_instanciate(GeoXmlParameterGroup * parameter_group);

/**
 * Remove the last instance.
 */
gboolean
geoxml_parameter_group_deinstanciate(GeoXmlParameterGroup * parameter_group);

/**
 * Get \p group's parameters list.
 *
 * \see \ref parameters.h "GeoXmlParameters"
 */
int
geoxml_parameter_group_get_instance(GeoXmlParameterGroup * parameter_group,
	GeoXmlSequence ** parameters, gulong index);

/**
 * Get the number of instances of \p parameter_group
 *
 * If \p parameter_group is NULL returns 0.
 */
glong
geoxml_parameter_group_get_instances_number(GeoXmlParameterGroup * parameter_group);

/**
 * Return the list of the parameters at \p index position in
 * each instance of \p parameter_group
 *
 * If \p parameter_group is NULL returns NULL.
 */
GSList *
geoxml_parameter_group_get_parameter_in_all_instances(GeoXmlParameterGroup * parameter_group, guint index);

/**
 * Set wheter \p parameter_group can have more than one instance.
 * If \p enable is FALSE, group is deinstanciated till it has only one
 * one instance
 *
 */
void
geoxml_parameter_group_set_is_instanciable(GeoXmlParameterGroup * parameter_group, gboolean enable);

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
geoxml_parameter_group_get_is_instanciable(GeoXmlParameterGroup * parameter_group);

/**
 * Return TRUE if \p parameter_group should be expanded by default, otherwise returns FALSE.
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group);

#endif //__LIBGEBR_GEOXML_PARAMETER_GROUP_H
