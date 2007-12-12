/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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

#ifndef __GEOXML_DOCUMENT_P_H
#define __GEOXML_DOCUMENT_P_H

/**
 * \internal
 * Private constructor. Used by super classes to create a new document
 * @param name refer to the root element (flow, line or project) @param version
 * to its corresponding last version (support by this version of libgeoxml)
 */
GeoXmlDocument *
geoxml_document_new(const gchar * name, const gchar * version);

/**
 * \internal
 *
 */
#define geoxml_document_root_element(document)	\
	gdome_doc_documentElement((GdomeDocument*)document, &exception)

#endif //__GEOXML_DOCUMENT_P_H
