/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef __MENU_H
#define __MENU_H

#include <glib.h>
#include <gebrme.h>

void
menu_new(void);

void
menu_open(const gchar * path);

void
menu_load_user_directory(void);

void
menu_save(const gchar * path);

void
menu_selected(void);

/**
 * Asks for save and free memory allocated for menus
 */
gboolean
menu_cleanup(void);

void
menu_saved_status_set(MenuStatus status);

#endif //__MENU_H
