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

#ifndef __LIBGEOXML_CATEGORY_H
#define __LIBGEOXML_CATEGORY_H

#include <glib.h>

/**
 * \struct GeoXmlCategory category.h libgeoxml/category.h
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
 * 
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 * 
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" };
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
 *
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
 * Change the name of \p category to \p name.
 *
 * If \p category is NULL nothing is done.
 */
void
geoxml_category_set_name(GeoXmlCategory * category, const gchar * name);

/**
 * Returns \p category 's name.
 *
 * If \p category is NULL returns NULL.
 */
const gchar *
geoxml_category_get_name(GeoXmlCategory * category);

/**
 * Get the flow to which \p category belongs to.
 *
 * If \p category is NULL nothing is done.
 */
GeoXmlFlow *
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

#endif //__LIBGEOXML_CATEGORY_H
