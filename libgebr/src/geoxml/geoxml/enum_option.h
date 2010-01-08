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

#ifndef __GEBR_GEOXML_ENUM_OPTION_H
#define __GEBR_GEOXML_ENUM_OPTION_H

#include <glib.h>

/**
 * \struct GebrGeoXmlEnumOption enum_option.h geoxml/enum_option.h
 * \brief
 * A flow enum_option.
 * \dot
 * digraph enum_option {
 * 	fontlabel = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontlabel = "Bitstream Vera Sans"
 *   fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontlabel = "Bitstream Vera Sans"
 *   fontsize = 9
 * 	]
 *
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GebrGeoXmlEnumOption" [ URL = "\ref enum_option.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlEnumOption"
 * 	"GebrGeoXmlProgramParameter" -> "GebrGeoXmlEnumOption"
 * }
 * \enddot
 * \see enum_option.h
 */

/**
 * \file enum_option.h
 */

/**
 * Promote a sequence to a enum option.
 */
#define GEBR_GEOXML_ENUM_OPTION(seq) ((GebrGeoXmlEnumOption*)(seq))

/**
 * The GebrGeoXmlEnumOption struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_enum_option GebrGeoXmlEnumOption;

#include "macros.h"

/**
 *
 */
void gebr_geoxml_enum_option_set_label(GebrGeoXmlEnumOption * enum_option, const gchar * label);

/**
 *
 */
const gchar *gebr_geoxml_enum_option_get_label(GebrGeoXmlEnumOption * enum_option);

/**
 *
 */
void gebr_geoxml_enum_option_set_value(GebrGeoXmlEnumOption * enum_option, const gchar * value);

/**
 *
 */
const gchar *gebr_geoxml_enum_option_get_value(GebrGeoXmlEnumOption * enum_option);

#endif				//__GEBR_GEOXML_ENUM_OPTION_H
