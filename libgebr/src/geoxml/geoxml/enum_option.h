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

#ifndef __LIBGEBR_GEOXML_ENUM_OPTION_H
#define __LIBGEBR_GEOXML_ENUM_OPTION_H

#include <glib.h>

/**
 * \struct GeoXmlEnumOption enum_option.h geoxml/enum_option.h
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
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref enum_option.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlSequence" -> "GeoXmlEnumOption"
 * 	"GeoXmlProgramParameter" -> "GeoXmlEnumOption"
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
#define GEOXML_ENUM_OPTION(seq) ((GeoXmlEnumOption*)(seq))

/**
 * The GeoXmlEnumOption struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_enum_option GeoXmlEnumOption;

#include "flow.h"
#include "macros.h"
#include "program_parameter.h"

/**
 *
 */
void
geoxml_enum_option_set_label(GeoXmlEnumOption * enum_option, const gchar * label);

/**
 *
 */
const gchar *
geoxml_enum_option_get_label(GeoXmlEnumOption * enum_option);

/**
 *
 */
void
geoxml_enum_option_set_value(GeoXmlEnumOption * enum_option, const gchar * value);

/**
 *
 */
const gchar *
geoxml_enum_option_get_value(GeoXmlEnumOption * enum_option);

#endif //__LIBGEBR_GEOXML_ENUM_OPTION_H
