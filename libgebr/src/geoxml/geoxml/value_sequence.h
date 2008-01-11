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

#ifndef __LIBGEBR_GEOXML_VALUE_SEQUENCE_H
#define __LIBGEBR_GEOXML_VALUE_SEQUENCE_H

/**
 * \struct GeoXmlValueSequence value_sequence.h geoxml/value_sequence.h
 * \brief
 * Abstract class for elements of a sequence in libgeoxml
 * \dot
 * digraph sequence {
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
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref GeoXmlEnumOption" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlSequence" -> "GeoXmlSequence";
 * 	"GeoXmlValueSequence" -> "GeoXmlEnumOption";
 * 	"GeoXmlValueSequence" -> "GeoXmlCategory";
 * }
 * \enddot
 * \see value_sequence.h
 */

/**
 * \file value_sequence.h
 * Abstract class for elements of a sequence in libgeoxml
 *
 * GeoXmlEnumOption and GeoXmlCategory inherits GeoXmlValueSequence.
 */

/**
 * Cast to super types of GeoXmlValueSequence to it.
 */
#define GEOXML_VALUE_SEQUENCE(seq) ((GeoXmlValueSequence*)(seq))

/**
 * The GeoXmlValueSequence struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_value_sequence GeoXmlValueSequence;

#include <glib.h>

/**
 * Set the \p value_sequence element's value to \p value.
 *
 * If \p value_sequence or \p value is NULL nothing is done.
 */
void
geoxml_value_sequence_set(GeoXmlValueSequence * value_sequence, const gchar * value);

/**
 * Get the \p value_sequence element's value.
 *
 * If \p value_sequence is NULL returns NULL.
 */
const gchar *
geoxml_value_sequence_get(GeoXmlValueSequence * value_sequence);

#endif //__LIBGEBR_GEOXML_VALUE_SEQUENCE_H
