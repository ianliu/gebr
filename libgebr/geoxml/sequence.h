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

#ifndef __GEBR_GEOXML_SEQUENCE_H
#define __GEBR_GEOXML_SEQUENCE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * \struct GebrGeoXmlSequence sequence.h geoxml/sequence.h
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
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 	]
 *
 * 	"GebrGeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProjectLine" [ URL = "\ref GebrGeoXmlProjectLine" ];
 * 	"GebrGeoXmlLineFlow" [ URL = "\ref GebrGeoXmlLineFlow" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GebrGeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GebrGeoXmlParameter" [ URL = "\ref gebr-gui-parameter.h" ];
 * 	"GebrGeoXmlPropertyValue" [ URL = "\ref GebrGeoXmlPropertyValue" ];
 * 	"GebrGeoXmlEnumOption" [ URL = "\ref enum_option.h" ];
 * 	"GebrGeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlSequence"
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProjectLine";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlLineFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProgram";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameters";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlParameter";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlPropertyValue";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlEnumOption";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlValueSequence";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlLinePath";
 * 	"GebrGeoXmlValueSequence" -> "GebrGeoXmlPropertyValue";
 * }
 * \enddot
 * \see sequence.h
 */

/**
 * \file sequence.h
 * Abstract class for elements of a sequence in libgeoxml
 *
 * GebrGeoXmlProjectLine, GebrGeoXmlLineFlow, GebrGeoXmlProgram, GebrGeoXmlProgramParameter and GebrGeoXmlCategory
 * can be treated as a super class of GebrGeoXmlSequence.
 */

/**
 * Cast to super types of GebrGeoXmlSequence to it.
 */
#define GEBR_GEOXML_SEQUENCE(seq) ((GebrGeoXmlSequence*)(seq))

/**
 * The GebrGeoXmlSequence struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_sequence GebrGeoXmlSequence;

#include "parameter_group.h"

/**
 * Use as an auxiliary function to gebr_geoxml_sequence_next.
 * Assign \p sequence to the previous sequence sequenced
 * or NULL if there isn't.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR, GEBR_GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see gebr_geoxml_sequence_next
 */
int gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence);

/**
 * Use to iterate over sequences.
 * Assign \p sequence to the next sequence sequenced
 * or NULL if there isn't.
 * Example:
 * \code
 * GebrGeoXmlSequence * i;
 * gebr_geoxml_[parent]_get_[sequence]([parent], &i, 0);
 * while (i != NULL) {
 * 	...
 * 	gebr_geoxml_sequence_next(&i);
 * }
 * \endcode
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR, GEBR_GEOXML_RETV_NOT_A_SEQUENCE
 *
 * \see gebr_geoxml_sequence_previous
 */
int gebr_geoxml_sequence_next(GebrGeoXmlSequence ** sequence);

/**
 * Clone \p sequence element and add it to the end of the sequence.
 * Returns the cloned sequence element.
 *
 * If \p sequence is NULL or is not a sequence, NULL is returned.
 *
 */
GebrGeoXmlSequence *gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence);

/**
 * Get the index of \p sequence.
 *
 * If \p sequence is NULL returns -1.
 */
gint gebr_geoxml_sequence_get_index(GebrGeoXmlSequence * sequence);

/**
 * Returns the sequence at index in \p sequence.
 *
 * If \p sequence is NULL returns NULL.
 */
GebrGeoXmlSequence *gebr_geoxml_sequence_get_at(GebrGeoXmlSequence * sequence, gulong index);

/**
 * Removes \p sequence from its sequence.
 *
 * A special case are the parameter. It cannot be removed if it belongs to an
 * instanciated group. Also, if it is removed, all referenced parameters are
 * automatically removed
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR,
 * GEBR_GEOXML_RETV_NOT_A_SEQUENCE, GEBR_GEOXML_RETV_MORE_THAN_ONE_INSTANCES
 */
int gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence);

/**
 * Return TRUE if \p sequence and \p other are the same sequences.
 * Otherwise, return FALSE.
 *
 * If \p sequence or \p other is NULL, returns FALSE.
 */
gboolean gebr_geoxml_sequence_is_same_sequence(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * other);

/**
 * Move \p sequence to \p parameter_group, appending it to the list of is parameters.
 * \p sequence is removed from wherever it belonged.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR,
 * GEBR_GEOXML_RETV_NOT_A_SEQUENCE, GEBR_GEOXML_RETV_MORE_THAN_ONE_INSTANCES
 */
int gebr_geoxml_sequence_move_into_group(GebrGeoXmlSequence * sequence, GebrGeoXmlParameterGroup * parameter_group);

/**
 * Moves \p sequence to the position before \p position. If \p position is NULL then
 * moves to the end of the sequence
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR,
 * GEBR_GEOXML_RETV_NOT_A_SEQUENCE, GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES
 */
int gebr_geoxml_sequence_move_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position);

/**
 * Moves \p sequence to the position after \p position. If \p position is NULL then
 * moves to the beggining of the sequence
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NULL_PTR,
 * GEBR_GEOXML_RETV_NOT_A_SEQUENCE, GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES
 */
int gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position);

/**
 * Exchange positions of the sequence above \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR, GEBR_GEOXML_RETV_NOT_A_SEQUENCE
 */
int gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence);

/**
 * Exchange positions of the sequence below \p sequence with \p sequence in \p sequence.
 * \p sequence must belong to \p sequence.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR, GEBR_GEOXML_RETV_NOT_A_SEQUENCE
 */
int gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence);

G_END_DECLS
#endif				//__GEBR_GEOXML_SEQUENCE_H
