/**
 * \file menu.c Provides methods for loading menus
 */

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

G_BEGIN_DECLS
 
/**
 * \deprecated
 * Look for a given menu \p filename and load it if found.
 *
 * \see menu_load_path
 */
GebrGeoXmlFlow *menu_load_ancient(const gchar * filename);

/**
 * Loads a menu at the given \p path.
 */
GebrGeoXmlFlow *menu_load_path(const gchar * path);

/**
 * Verify if directories to search menus have changed.
 *
 * \return TRUE if there is change in menus' directories.
 */
gboolean menu_refresh_needed(void);

/**
 * Read index and add menus from it to the view.
 */
void menu_list_populate(void);

/**
 * Creates the indexes files for folders listed in \ref directory_list.
 *
 * \return TRUE if successfully scanned all directories.
 */
gboolean menu_list_create_index(void);

/**
 * Lists all system directories that GeBR looks for menus.
 * \return a vector of strings, terminated by NULL. You don't need to free this.
 */
const gchar ** menu_get_system_directories(void);

G_END_DECLS

#endif				//__MENU_H_
