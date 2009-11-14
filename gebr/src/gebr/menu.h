/*   GeBR - An environment for seismic processing.
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

#ifndef __MENU_H_
#define __MENU_H_

#include <glib.h>

#include <libgebr/geoxml.h>

GebrGeoXmlFlow *
menu_load(const gchar * filename);

GebrGeoXmlFlow *
menu_load_path(const gchar * path);

GString *
menu_get_path(const gchar * filename);

gboolean
menu_refresh_needed(void);

void
menu_list_populate(void);

gboolean
menu_list_create_index(void);

GString *
menu_get_help_from_program_ref(GebrGeoXmlProgram * program);

#endif //__MENU_H_
