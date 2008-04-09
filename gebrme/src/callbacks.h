/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

void
on_new_activate(void);

void
on_open_activate(void);

void
on_save_activate(void);

void
on_save_as_activate(void);

void
on_revert_activate(void);

void
on_delete_activate(void);

void
on_close_activate(void);

void
on_quit_activate(void);

void
on_cut_activate(void);

void
on_copy_activate(void);

void
on_paste_activate(void);

void
on_preferences_activate(void);

void
on_about_activate(void);

#endif //__CALLBACKS_H
