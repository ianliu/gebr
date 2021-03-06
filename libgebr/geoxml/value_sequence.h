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

#ifndef __GEBR_GEOXML_VALUE_SEQUENCE_H
#define __GEBR_GEOXML_VALUE_SEQUENCE_H

#include <glib.h>

#include "gebr-geo-types.h"

G_BEGIN_DECLS

/**
 * \struct GebrGeoXmlValueSequence value_sequence.h geoxml/value_sequence.h
 * \brief
 * Abstract class for elements of a sequence in libgeoxml
 * \dot
 * digraph sequence {
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
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GebrGeoXmlLinePath" [ URL = "\ref GebrGeoXmlLinePath" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlValueSequence";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlLinePath";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlPropertyValue";
 * }
 * \enddot
 * \see value_sequence.h
 */

/**
 * \file value_sequence.h
 * Abstract class for elements of a sequence in libgeoxml
 *
 * GebrGeoXmlEnumOption and GebrGeoXmlCategory inherits GebrGeoXmlValueSequence.
 */

/**
 * Set the \p value_sequence element's value to \p value.
 *
 * If \p value_sequence or \p value is NULL nothing is done.
 */
void gebr_geoxml_value_sequence_set(GebrGeoXmlValueSequence * value_sequence, const gchar * value);

/**
 * Get the \p value_sequence element's value.
 *
 * If \p value_sequence is NULL returns NULL.
 */
gchar *gebr_geoxml_value_sequence_get(GebrGeoXmlValueSequence * value_sequence);

/**
 * gebr_geoxml_value_sequence_get_dictkey:
 */
const gchar *gebr_geoxml_value_sequence_get_dictkey (GebrGeoXmlValueSequence *self);

/**
 * gebr_geoxml_value_sequence_set_dictkey:
 */
void gebr_geoxml_value_sequence_set_dictkey (GebrGeoXmlValueSequence *self, const gchar *dictkey);

G_END_DECLS
#endif				//__GEBR_GEOXML_VALUE_SEQUENCE_H
