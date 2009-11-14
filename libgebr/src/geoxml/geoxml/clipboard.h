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

#ifndef __GEBR_GEOXML_CLIPBOARD_H
#define __GEBR_GEOXML_CLIPBOARD_H

#include <glib.h>

#include "object.h"

/**
 * Clear clipboard
 */
void
gebr_geoxml_clipboard_clear(void);

/**
 * Add \p object to the clipboard
 */
void
gebr_geoxml_clipboard_copy(GebrGeoXmlObject * object);

/**
 * Paste all clipboard into \p object
 * Return the first pasted parameter, or NULL if failed.
 */
GebrGeoXmlObject *
gebr_geoxml_clipboard_paste(GebrGeoXmlObject * object);

#endif //__GEBR_GEOXML_CLIPBOARD_H
