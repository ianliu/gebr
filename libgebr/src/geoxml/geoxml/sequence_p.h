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

#ifndef __LIBGEBR_GEOXML_SEQUENCE_P_H
#define __LIBGEBR_GEOXML_SEQUENCE_P_H

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_previous(GeoXmlSequence ** sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_next(GeoXmlSequence ** sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_remove(GeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
GeoXmlSequence *
__geoxml_sequence_append_clone(GeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_move_up(GeoXmlSequence * sequence);

/**
 * \internal
 * Do operation without checks, for library use
 */
int
__geoxml_sequence_move_down(GeoXmlSequence * sequence);

#endif //__LIBGEBR_GEOXML_SEQUENCE_P_H
