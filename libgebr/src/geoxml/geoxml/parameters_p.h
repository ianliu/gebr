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

#ifndef __LIBGEBR_GEOXML_PARAMETERS_P_H
#define __LIBGEBR_GEOXML_PARAMETERS_P_H

/**
 * \internal
 * Returns TRUE if \p parameters can be changed: if this is from a program one or the master
 * instance of a group.
 * The first instance of a group can be said as the master and the other ones as slaves. The changes made to
 * it is reflected on all others. The slaves can't be changed directly.
 */
gboolean
__geoxml_parameters_group_check(GeoXmlParameters * parameters);

/**
 * \internal
 * Create a new parameter with type \p type.
 */
GeoXmlParameter *
__geoxml_parameters_new_parameter(enum GEOXML_PARAMETERTYPE type, gboolean adjust_npar);

#endif //__LIBGEBR_GEOXML_PARAMETERS_P_H
