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

#ifndef __LIBGEBR_GEOXML_CATEGORY_H
#define __LIBGEBR_GEOXML_CATEGORY_H

#include <glib.h>

/**
 * \struct GeoXmlCategory category.h geoxml/category.h
 * \brief
 * A flow category.
 * \dot
 * digraph category {
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
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlSequence" -> "GeoXmlValueSequence";
 * 	"GeoXmlValueSequence" -> "GeoXmlCategory";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> { "GeoXmlCategory" };
 * }
 * \enddot
 * \see category.h
 */

/**
 * \file category.h
 * A flow category.
 * Inherits GeoXmlValueSequence
 */

/**
 * Promote a sequence to a category.
 */
#define GEOXML_CATEGORY(seq) ((GeoXmlCategory*)(seq))

/**
 * The GeoXmlCategory struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_category GeoXmlCategory;

#include "flow.h"
#include "macros.h"

/**
 * \deprecated
 * Use \ref geoxml_value_sequence_get instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_category_set_name(GeoXmlCategory * category, const gchar * name);

/**
 * \deprecated
 * Use \ref geoxml_value_sequence_set instead. Kept only for backwards compatible and should not be used in newly written code
 */
const gchar * GEOXML_DEPRECATED
geoxml_category_get_name(GeoXmlCategory * category);

/**
 * \deprecated
 * Kept only for backwards compatible and should not be used in newly written code
 */
GeoXmlFlow * GEOXML_DEPRECATED
geoxml_category_flow(GeoXmlCategory * category);

/**
 * \deprecated
 * Use \ref geoxml_sequence_previous instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_category_previous(GeoXmlCategory ** category);

/**
 * \deprecated
 * Use \ref geoxml_sequence_next instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_category_next(GeoXmlCategory ** category);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_category_remove(GeoXmlCategory * category);

#endif //__LIBGEBR_GEOXML_CATEGORY_H
