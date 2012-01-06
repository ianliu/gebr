/*
 * clipboard.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_GEOXML_CLIPBOARD_H__
#define __GEBR_GEOXML_CLIPBOARD_H__

#include <glib.h>
#include "object.h"

G_BEGIN_DECLS

/**
 * gebr_geoxml_clipboard_clear:
 *
 * Clear clipboard.
 */
void gebr_geoxml_clipboard_clear(void);

/**
 * gebr_geoxml_clipboard_copy:
 *
 * Add @object to the clipboard.
 */
void gebr_geoxml_clipboard_copy(GebrGeoXmlObject * object);

/**
 * gebr_geoxml_clipboard_paste:
 *
 * Paste all clipboard into @object.
 *
 * Returns: The first pasted parameter, or %NULL if failed.
 */
GebrGeoXmlObject *gebr_geoxml_clipboard_paste(GebrGeoXmlObject * object);

/**
 * gebr_geoxml_clipboard_has_forloop:
 *
 * Returns: %TRUE if the clipboard contains a For loop.
 */
gboolean gebr_geoxml_clipboard_has_forloop(void);

G_END_DECLS

#endif /* __GEBR_GEOXML_CLIPBOARD_H__ */
