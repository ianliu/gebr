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

#ifndef __GEBRME_H
#define __GEBRME_H

#include <gtk/gtk.h>
#include <geoxml.h>

#include <gui/about.h>
#include <misc/log.h>

extern struct gebrme gebrme;

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

enum {
	CATEGORY_NAME,
	CATEGORY_XMLPOINTER,
	CATEGORY_N_COLUMN
};

struct gebrme {
	/* menu being edited */
	GeoXmlFlow *		current;

	/* diverse widgets */
	GtkWidget *		window;
	struct about		about;
	GtkWidget *		statusbar;
	GtkWidget *		invisible;

	/* actions */
	GtkAction *		save_action;
	GtkAction *		revert_action;

	/* menus list */
	GtkListStore *		menus_liststore;
	GtkWidget *		menus_treeview;

	/* title, description, author and email */
	GtkWidget *		title_entry;
	GtkWidget *		description_entry;
	GtkWidget *		author_entry;
	GtkWidget *		email_entry;

	/* menu info: categories */
	GtkWidget *		categories_combo;
	GtkWidget *		categories_sequence_edit;

	/* menu's programs */
	GtkWidget *		programs_vbox;

        /* icons */
	struct gebrme_pixmaps {
        	GdkPixbuf *	stock_no;
	} pixmaps;

	/* temporary files removed when GeBRME quits */
	GSList *		tmpfiles;

	/* config file */
	struct gebrme_config {
		GKeyFile *	keyfile;
		GString *	path;

		GString *	name;
		GString *	email;
		GString *	menu_dir;
		GString *	htmleditor;
		GString *	browser;
	} config;
};

void
gebrme_init(void);

void
gebrme_quit(void);

void
gebrme_config_load(void);

void
gebrme_config_save(void);

void
gebrme_message(enum log_message_type type, const gchar * message, ...);

#endif //__GEBRME_H
