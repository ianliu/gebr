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

#ifndef __LIBGEBR_GEOXML_SEQUENCE_H
#define __LIBGEBR_GEOXML_SEQUENCE_H

/**
 * \struct GeoXmlSequence sequence.h geoxml/sequence.h
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
 * 	"GeoXmlProjectLine" [ URL = "\ref GeoXmlProjectLine" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref GeoXmlLineFlow" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref GeoXmlEnumOption" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlSequence" -> { "GeoXmlProjectLine" };
 * 	"GeoXmlSequence" -> { "GeoXmlLineFlow" };
 * 	"GeoXmlSequence" -> { "GeoXmlCategory" };
 * 	"GeoXmlSequence" -> { "GeoXmlProgram" };
 * 	"GeoXmlSequence" -> { "GeoXmlParameter" };
 * 	"GeoXmlSequence" -> { "GeoXmlValueSequence" };
 * 	"GeoXmlValueSequence" -> { "GeoXmlEnumOption" };
 * }
 * \enddot
 * \see sequence.h
 */

/**
 * \file sequence.h
 * Abstract class for elements of a sequence in libgeoxml
 *
 * GeoXmlProjectLine, GeoXmlLineFlow, GeoXmlProgram, GeoXmlProgramParameter and GeoXmlCategory
 * can be treated as a super class of GeoXmlSequence.
 */

/**
 * Cast to super types of GeoXmlSequence to it.
 */
#define GEOXML_SEQUENCE(seq) ((GeoXmlSequence*)(seq))

/**
 * The GeoXmlSequence struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_sequence GeoXmlSequence;

#include <glib.h>

/**
 * Use as an auxiliary function to geoxml_sequence_next.
 * Assign \p sequence to the previous sequence sequenced
 * or NULL if there isn't.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see geoxml_sequence_next
 */
int
geoxml_sequence_previous(GeoXmlSequence ** sequence);

/**
 * Use to iterate over sequences.
 * Assign \p sequence to the next sequence sequenced
 * or NULL if there isn't.
 * Example:
 * \code
 * GeoXmlSequence * i;
 * geoxml_sequence_get_sequence(sequence, &sequence, 0);
 * while (i != NULL) {
 * 	...
 * 	geoxml_sequence_next(&i);
 * }
 * \endcode
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see geoxml_sequence_previous
 */
int
geoxml_sequence_next(GeoXmlSequence ** sequence);

/**
 * Removes \p sequence from its sequence. It is not deleted and can be reinserted
 * into sequence using geoxml_sequence_prepend or geoxml_sequence_append.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 */
int
geoxml_sequence_remove(GeoXmlSequence * sequence);

/**
 *
 * In case that \p sequence is a parameter, \p before should be
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE, GEOXML_RETV_DIFFERENT_SEQUENCES
 */
int
geoxml_sequence_move(GeoXmlSequence * sequence, GeoXmlSequence * before);

/**
 * Exchange positions of the sequence above \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 */
int
geoxml_sequence_move_up(GeoXmlSequence * sequence);

/**
 * Exchange positions of the sequence below \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR, GEOXML_RETV_NOT_A_SEQUENCE
 */
int
geoxml_sequence_move_down(GeoXmlSequence * sequence);

#endif //__LIBGEBR_GEOXML_SEQUENCE_H
