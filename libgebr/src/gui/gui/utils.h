/*   libgebr - G�BR Library
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_GUI_UTILS_H
#define __LIBGEBR_GUI_UTILS_H

#include <gtk/gtk.h>

gboolean
gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter);

gboolean
gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter);

gboolean
gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter);

gboolean
gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter);

#endif //__LIBGEBR_GUI_UTILS_H
