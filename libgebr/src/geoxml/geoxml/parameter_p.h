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

#ifndef __GEBR_GEOXML_PARAMETER_P_H
#define __GEBR_GEOXML_PARAMETER_P_H

G_BEGIN_DECLS

/**
 * \internal
 * Create element type and add it to \p parameter, according to \p type
 *
 */
GdomeElement *__gebr_geoxml_parameter_insert_type(GebrGeoXmlParameter * parameter,
						  GebrGeoXmlParameterType type);

/**
 * \internal
 *
 * Return the type element of \p parameter, according to \p resolve_references
 */
GdomeElement *__gebr_geoxml_parameter_get_type_element(GebrGeoXmlParameter * parameter);

/**
 * \internal
 * Return type of \p parameter, according to \p resolve_references
 *
 */
GebrGeoXmlParameterType
__gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter, gboolean resolve_references);

/**
 * \internal
 * Set \p parameter to be a reference to \p reference.
 * If \p new is TRUE indicate that it was previously cloned.
 */
void __gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter, GebrGeoXmlParameter * reference);

/**
 * \internal
 * Get the list of GebrGeoXmlParameter referencees of \p id, inside the element
 * _context_
 */
GSList *__gebr_geoxml_parameter_get_referencee_list(GdomeElement * context, const gchar * id);

G_END_DECLS
#endif				//__GEBR_GEOXML_PARAMETER_P_H
