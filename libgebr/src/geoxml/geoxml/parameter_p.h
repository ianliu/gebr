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

#ifndef __LIBGEBR_GEOXML_PARAMETER_P_H
#define __LIBGEBR_GEOXML_PARAMETER_P_H

/**
 * \internal
 * Create element type and add it to \p parameter, according to \p type
 *
 */
GdomeElement *
__geoxml_parameter_insert_type(GeoXmlParameter * parameter, enum GEOXML_PARAMETERTYPE type);

/**
 * \internal
 *
 * Return the type element of \p parameter, according to \p resolve_references
 */
GdomeElement *
__geoxml_parameter_get_type_element(GeoXmlParameter * parameter, gboolean resolve_references);

/**
 * \internal
 * Return type of \p parameter, according to \p resolve_references
 *
 */
enum GEOXML_PARAMETERTYPE
__geoxml_parameter_get_type(GeoXmlParameter * parameter, gboolean resolve_references);

/**
 * \internal
 * Set \p parameter to be a reference to \p reference.
 * If \p new is TRUE indicate that it was previously cloned.
 */
void
__geoxml_parameter_set_be_reference(GeoXmlParameter * parameter, GeoXmlParameter * reference);

/**
 * \internal
 * Get the list of GeoXmlParameter referencees of \p id, inside the element
 * _context_
 */
GSList *
__geoxml_parameter_get_referencee_list(GdomeElement * context, const gchar * id);

#endif //__LIBGEBR_GEOXML_PARAMETER_P_H
