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

#ifndef __GEBR_GEOXML_TYPES_H
#define __GEBR_GEOXML_TYPES_H

G_BEGIN_DECLS

/**
 * \internal
 * defined in gebr-gui-parameter.c
 */
extern const char *parameter_type_to_str[];
extern const int parameter_type_to_str_len;

/**
 * \internal
 */
#define ENCODING "UTF-8"

/**
 * \internal
 * The extremelly anoying and persintant GdomeException.
 * Declaring one global variable makes possible to use gdome
 * functions in defines, like groxml_document_root_element
 * Defined in document.c.
 */
extern GdomeException exception;

G_END_DECLS
#endif				// __GEBR_GEOXML_TYPES_H
