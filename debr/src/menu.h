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

#ifndef __MENU_H
#define __MENU_H

#include <glib.h>

#include <libgebr/geoxml.h>

typedef enum {
	MENU_STATUS_SAVED,
	MENU_STATUS_UNSAVED
} MenuStatus;

enum {
	MENU_STATUS = 0,
	MENU_IMAGE,
	MENU_FILENAME,
	MENU_MODIFIED_DATE,
	MENU_XMLPOINTER,
	MENU_PATH,
	MENU_N_COLUMN
};

struct ui_menu {
	GtkWidget *		widget;

	GtkTreeStore *		model;
	GtkWidget *		tree_view;
	GtkTreeIter		iter_other;
	
	struct ui_menu_details {
		GtkWidget *	title_label;
		GtkWidget *	description_label;
		GtkWidget *	author_label;
		GtkWidget *	nprogs_label;
		GtkWidget *	created_label;
		GtkWidget *	created_date_label;
		GtkWidget *	modified_label;
		GtkWidget *	modified_date_label;
		GtkWidget *	category_label;
		GtkWidget *	categories_label[3];
		GtkWidget *     help_button;

	} details;
};

void
menu_setup_ui(void);
void
menu_new(gboolean edit);
GeoXmlFlow *
menu_load(const gchar * path);
void
menu_load_user_directory(void);
void
menu_open_with_parent(const gchar * path, GtkTreeIter * parent, gboolean select);
void
menu_open(const gchar * path, gboolean select);
gboolean
menu_save(GtkTreeIter * iter);
void
menu_save_all(void);
void
menu_validate(GtkTreeIter * iter);
void
menu_install(void);
void
menu_close(GtkTreeIter * iter);
void
menu_selected(void);
gboolean
menu_cleanup(void);
void
menu_saved_status_set(MenuStatus status);
void
menu_saved_status_set_from_iter(GtkTreeIter * iter, MenuStatus status);
void
menu_saved_status_set_unsaved(void);
void
menu_dialog_setup_ui(void);
void
menu_reset(void);
gint
menu_get_n_menus(void);

gboolean
menu_get_selected(GtkTreeIter * iter);
void
menu_load_selected(void);
void
menu_select_iter(GtkTreeIter * iter);
void
menu_details_update(void);
void
menu_path_get_parent(const gchar * path, GtkTreeIter * parent);

#endif //__MENU_H
