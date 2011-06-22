/*   libgebr - GêBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <gtk/gtk.h>
#include <libgebr/gui/gui.h>

void test_gebr_gui_utils_iter_equal(void) {
	GtkListStore *store;
	GtkTreeIter iter1;
	GtkTreeIter iter2;

	store = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_append(store, &iter1);
	iter2 = iter1;

	g_assert(gebr_gui_gtk_tree_iter_equal_to(&iter1, &iter2));
	//g_assert(!gebr_gui_gtk_tree_iter_equal_to(NULL, &iter2));
	//g_assert(!gebr_gui_gtk_tree_iter_equal_to(&iter1, NULL));
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/gui/utils", test_gebr_gui_utils_iter_equal);

	return g_test_run();
}



