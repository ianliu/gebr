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

#include <glib.h>

#include "gebr-geo-types.h"

G_BEGIN_DECLS

/**
 * \struct GebrGeoXmlParameter gebr-gui-parameter.h geoxml/gebr-gui-parameter.h
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
 * 	"GebrGeoXmlParameter" [ URL = "\ref gebr-gui-parameter.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program-parameter.h" ];
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
 * \see gebr-gui-parameter.h
 */

/**
 * \file gebr-gui-parameter.h
 * Represents a parameter, that is, a GebrGeoXmlParameterGroup or a GebrGeoXmlParameter.
 */

void __gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label);

/**
 * Get GebrGeoXmlParameters in which \p parameter is inside
 *
 * Will fail if \p parameter is NULL.
 */
GebrGeoXmlParameters *gebr_geoxml_parameter_get_parameters(GebrGeoXmlParameter * parameter);

/**
 * Get the program to which \p parameter belongs to.
 *
 * If \p parameter is NULL nothing is done.
 */
GebrGeoXmlProgram *gebr_geoxml_parameter_get_program(GebrGeoXmlParameter * parameter);

/**
 * Change \p parameter type to \p type.
 * Only label of \p parameter will have the same value after the change;
 * all other properties will have their value lost.
 *
 * If \p parameter is NULL nothing is done.
 */
gboolean gebr_geoxml_parameter_set_type(GebrGeoXmlParameter * parameter, GebrGeoXmlParameterType type);

/**
 * Returns \p parameter 's type.
 * GEBR_GEOXML_PARAMETER_TYPE_REFERENCE is not returned here. The "true" type
 * of the parameter is returned instead.
 *
 * If \p parameter is NULL returns \ref GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN.
 *
 * \see GEBR_GEOXML_PARAMETER_TYPE
 */
GebrGeoXmlParameterType gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter);

/**
 * Return the type name of \p parameter
 *
 * Return NULL if \p parameter is NULL
 */
const gchar *gebr_geoxml_parameter_get_type_name(GebrGeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is a reference
 *
 * If \p parameter is NULL return FALSE
 */
gboolean gebr_geoxml_parameter_get_is_reference(GebrGeoXmlParameter * parameter);

/**
 * If \p parameter_reference is of the type GEBR_GEOXML_PARAMETER_TYPE_REFERENCE
 * returns the parameter that it references. Otherwise, returns NULL.
 *
 * If \p parameter_reference is NULL returns NULL.
 */
GebrGeoXmlParameter *gebr_geoxml_parameter_get_referencee(GebrGeoXmlParameter * parameter_reference);

/**
 * Return TRUE if \p parameter is a GebrGeoXmlProgramParameter.
 * All parameters of any GEBR_GEOXML_PARAMETER_TYPE's types except GEBR_GEOXML_PARAMETER_TYPE_GROUP
 * are GebrGeoXmlProgramParameter.
 *
 * If \p parameter is NULL returns FALSE.
 */
gboolean gebr_geoxml_parameter_get_is_program_parameter(GebrGeoXmlParameter * parameter);

/**
 * Set \p parameter 's one line description to \p label.
 *
 * If \p parameter or \p label is NULL nothing is done.
 *
 * \see gebr_geoxml_parameter_get_label
 */
void gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label);

/**
 * gebr_geoxml_parameter_get_label:
 * @parameter: parameter from where the label will be get
 *
 * Get @parameter's one line description.
 * Will fail if @parameter is NULL.
 */
const gchar *gebr_geoxml_parameter_get_label(GebrGeoXmlParameter * parameter);

/**
 * Return TRUE if \p parameter is part of a group
 *
 * If \p parameter is NULL returns FALSE.
 * @see gebr_geoxml_parameter_get_group
 */
gboolean gebr_geoxml_parameter_get_is_in_group(GebrGeoXmlParameter * parameter);

/**
 * If \p parameter is in a group, return this group;
 * otherwise, returns NULL.
 *
 * If \p parameter is NULL returns NULL.
 * @see gebr_geoxml_parameter_get_is_in_group
 */
GebrGeoXmlParameterGroup *gebr_geoxml_parameter_get_group(GebrGeoXmlParameter * parameter);

/**
 * gebr_geoxml_parameter_is_dict_param:
 * @parameter:
 *
 * Returns: %TRUE if @parameter is a dictionary parameter, %FALSE otherwise.
 */
gboolean gebr_geoxml_parameter_is_dict_param(GebrGeoXmlParameter *parameter);

/*
 *
 */
GebrGeoXmlDocumentType gebr_geoxml_parameter_get_scope(GebrGeoXmlParameter *parameter);

G_END_DECLS
#endif				//__GEBR_GEOXML_PARAMETER_H
