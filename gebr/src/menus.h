/*   GÍBR - An environment for seismic processing.
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

#ifndef _GEBR_MENUS_H_
#define _GEBR_MENUS_H_

#include <glib.h>

extern const gchar * no_menu_selected_error;
extern const gchar * selected_menu_instead_error;

int
menus_populate(void);

int
menus_create_index(void);

int
menus_fname(const gchar *menu, 
	    GString    **fname);

void
menu_show_help (void);

#endif //_GEBR_MENUS_H_
