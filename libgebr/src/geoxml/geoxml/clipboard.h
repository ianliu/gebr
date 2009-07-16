/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#ifndef __LIBGEBR_GEOXML_CLIPBOARD_H
#define __LIBGEBR_GEOXML_CLIPBOARD_H

#include <glib.h>

#include "object.h"

/**
 * Clear clipboard
 */
void
geoxml_clipboard_clear(void);

/**
 * Add \p object to the clipboard
 */
void
geoxml_clipboard_copy(GeoXmlObject * object);

/**
 * Paste all clipboard into \p object
 * Return the first pasted parameter, or NULL if failed.
 */
GeoXmlObject *
geoxml_clipboard_paste(GeoXmlObject * object);

#endif //__LIBGEBR_GEOXML_CLIPBOARD_H
