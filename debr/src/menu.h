/*   DeBR - GeBR Designer
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

#ifndef __MENU_H
#define __MENU_H

#include <glib.h>

#include <geoxml.h>

typedef enum {
        MENU_STATUS_SAVED,
        MENU_STATUS_UNSAVED
} MenuStatus;

enum {
        MENU_STATUS,
	MENU_FILENAME,
	MENU_XMLPOINTER,
	MENU_PATH,
	MENU_N_COLUMN
};

struct ui_menu {
	GtkWidget *		widget;

	GtkListStore *		list_store;
	GtkWidget *		tree_view;
	
	struct ui_menu_details {
		GtkWidget *	title_label;
		GtkWidget *	description_label;
		GtkWidget *	author_label;
	} details;
};



void
menu_setup_ui(void);
void
menu_new(void);
GeoXmlFlow *
menu_load(const gchar * path);
void
menu_load_user_directory(void);
void
menu_open(const gchar * path, gboolean select);
void
menu_save(const gchar * path);
void
menu_save_all(void);
void
menu_selected(void);
gboolean
menu_cleanup(void);
void
menu_saved_status_set(MenuStatus status);
void
menu_saved_status_set_unsaved(void);
void
menu_dialog_setup_ui(void);

gboolean
menu_get_selected(GtkTreeIter * iter);
void
menu_load_selected(void);
void
menu_select_iter(GtkTreeIter * iter);

#endif //__MENU_H
