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

#ifndef __GEBR_GEOXML_SEQUENCE_P_H
#define __GEBR_GEOXML_SEQUENCE_P_H

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_previous(GebrGeoXmlSequence ** sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_next(GebrGeoXmlSequence ** sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_remove(GebrGeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
GebrGeoXmlSequence *__gebr_geoxml_sequence_append_clone(GebrGeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_move_before(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_move_after(GebrGeoXmlSequence * sequence, GebrGeoXmlSequence * position);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_move_up(GebrGeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int __gebr_geoxml_sequence_move_down(GebrGeoXmlSequence * sequence);

#endif				//__GEBR_GEOXML_SEQUENCE_P_H
