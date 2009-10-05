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

#ifndef __DEBR_H
#define __DEBR_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

#include <libgebr/gui/about.h>
#include <libgebr/log.h>

#include "menu.h"
#include "program.h"
#include "parameter.h"
#include "validate.h"

extern struct debr debr;

enum NOTEBOOK_PAGE {
	NOTEBOOK_PAGE_MENU = 0,
	NOTEBOOK_PAGE_PROGRAM,
	NOTEBOOK_PAGE_PARAMETER,
	NOTEBOOK_PAGE_VALIDATE,
};

enum CategoryModel {
	CATEGORY_NAME,
	CATEGORY_REF_COUNT,
	CATEGORY_N_COLUMN
};

struct debr {
	/* current stuff being edited */
	GeoXmlFlow *		menu;
	GeoXmlProgram *		program;
	GeoXmlParameter *	parameter;

	/* diverse widgets */
	GtkWidget *		window;
	GtkWidget *		navigation_box_label;
	GtkWidget *		notebook;
	GtkWidget *		statusbar;
	struct about		about;
	GtkWidget *		invisible;
	GtkAccelGroup *		accel_group;
	GtkActionGroup *	action_group;

	/* notebook's widgets */
	struct ui_menu		ui_menu;
	struct ui_program	ui_program;
	struct ui_parameter	ui_parameter;
	struct ui_validate	ui_validate;
	GtkWidget *		parameter_type_menu;

        /* icons */
	struct debr_pixmaps {
		GdkPixbuf *	stock_apply;
		GdkPixbuf *	stock_cancel;
        	GdkPixbuf *	stock_no;
	} pixmaps;

	/* config file */
	struct debr_config {
		GKeyFile *	key_file;
		GString *	path;

		GString *	name;
		GString *	email;
		GString *	htmleditor;
		GString *	browser;
		gchar **	menu_dir;
	} config;

	GtkListStore *		categories_model;

	/* temporary files removed when DeBR quits */
	GSList *		tmpfiles;
};

void
debr_init(void);

gboolean
debr_quit(void);

gboolean
debr_config_load(void);

void
debr_config_save(void);

void
debr_message(enum log_message_type type, const gchar * message, ...);

gboolean
debr_has_category(const gchar * category, gboolean add);

#endif //__DEBR_H
