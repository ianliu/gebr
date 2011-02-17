/*   DeBR - GeBR Designer
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

#ifndef __HELP_H
#define __HELP_H

#include <glib.h>
#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

/**
 * debr_help_edit:
 * @object: a #GebrGeoXmlProgram or #GebrGeoXmlFlow
 *
 * Starts the editing @object's help.
 */
void debr_help_edit(GebrGeoXmlObject * object);

/**
 * debr_help_show:
 * @object: a #GebrGeoXmlProgram or #GebrGeoXmlFlow
 *
 * Shows  @object's help.
 */
void debr_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title);

G_END_DECLS

#endif				//__HELP_H
