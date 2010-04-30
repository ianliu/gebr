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

#ifndef __GEBR_GEOXML_PARAMETER_GROUP_H
#define __GEBR_GEOXML_PARAMETER_GROUP_H

/**
 * \struct GebrGeoXmlParameterGroup parameter_group.h geoxml/parameter_group.h
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
 * 	"GebrGeoXmlParameterGroup" -> "GebrGeoXmlParameters";
 * }
 * \enddot
 * \see parameter_group.h
 */

/**
 * The GebrGeoXmlParameterGroup struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_parameter_group GebrGeoXmlParameterGroup;

/**
 * Cast from a GebrGeoXmlSequence to a GebrGeoXmlParameterGroup
 */
#define GEBR_GEOXML_PARAMETER_GROUP(seq) ((GebrGeoXmlParameterGroup*)(seq))

#include <glib.h>

#include "parameters.h"
#include "sequence.h"

/**
 * \internal
 * Get the group template instance.
 */
GebrGeoXmlParameters *gebr_geoxml_parameter_group_get_template(GebrGeoXmlParameterGroup * parameter_group);

/**
 * Instanciate \p parameter_group, appending a new instance (which is a clone of the first instance) to the list of
 * instances.
 */
GebrGeoXmlParameters *gebr_geoxml_parameter_group_instanciate(GebrGeoXmlParameterGroup * parameter_group);

/**
 * Remove the last instance.
 */
gboolean gebr_geoxml_parameter_group_deinstanciate(GebrGeoXmlParameterGroup * parameter_group);

/**
 * Get \p group's parameters list.
 *
 * \see \ref parameters.h "GebrGeoXmlParameters"
 */
int gebr_geoxml_parameter_group_get_instance(GebrGeoXmlParameterGroup * parameter_group,
					     GebrGeoXmlSequence ** parameters, gulong index);

/**
 * Get the number of instances of \p parameter_group
 *
 * If \p parameter_group is NULL returns 0.
 */
glong gebr_geoxml_parameter_group_get_instances_number(GebrGeoXmlParameterGroup * parameter_group);

/**
 * Return the list of the parameters at \p index position in
 * each instance of \p parameter_group
 *
 * If \p parameter_group is NULL returns NULL.
 */
GSList *gebr_geoxml_parameter_group_get_parameter_in_all_instances(GebrGeoXmlParameterGroup * parameter_group,
								   guint index);

/**
 * Set wheter \p parameter_group can have more than one instance.
 * If \p enable is FALSE, group is deinstanciated till it has only one
 * one instance
 *
 */
void gebr_geoxml_parameter_group_set_is_instanciable(GebrGeoXmlParameterGroup * parameter_group, gboolean enable);

/**
 * Set it \p parameter_group should be expanded by default, according to \p enable.
 *
 * If \p parameter_group is NULL nothing is done.
 */
void gebr_geoxml_parameter_group_set_expand(GebrGeoXmlParameterGroup * parameter_group, const gboolean enable);

/**
 * Get wheter \p parameter_group can have more than one instance.
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean gebr_geoxml_parameter_group_get_is_instanciable(GebrGeoXmlParameterGroup * parameter_group);

/**
 * Return TRUE if \p parameter_group should be expanded by default, otherwise returns FALSE.
 *
 * If \p parameter_group is NULL returns FALSE.
 */
gboolean gebr_geoxml_parameter_group_get_expand(GebrGeoXmlParameterGroup * parameter_group);

#endif				//__GEBR_GEOXML_PARAMETER_GROUP_H
