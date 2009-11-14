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

#ifndef __GEBR_GEOXML_PARAMETERS_P_H
#define __GEBR_GEOXML_PARAMETERS_P_H

/**
 * \internal
 * Create new parameters element into the end of \p parent
 */
GebrGeoXmlParameters *
__gebr_geoxml_parameters_append_new(GdomeElement * parent);

/**
 * \internal
 * Returns TRUE if \p parameters can be changed: if this is from a program one or the master
 * instance of a group.
 * The first instance of a group can be said as the master and the other ones as slaves. The changes made to
 * it is reflected on all others. The slaves can't be changed directly.
 */
gboolean
__gebr_geoxml_parameters_group_check(GebrGeoXmlParameters * parameters);

/**
 * \internal
 * If \p parameters is in a group, append references of parameter in group instances
 */
void
__gebr_geoxml_parameters_do_insert_in_group_stuff(GebrGeoXmlParameters * parameters, GebrGeoXmlParameter * parameter);

#endif //__GEBR_GEOXML_PARAMETERS_P_H
