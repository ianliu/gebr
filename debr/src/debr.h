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
	GebrGeoXmlFlow *menu;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameter *parameter;

	/* diverse widgets */
	GtkWidget *window;
	GtkWidget *navigation_box_label;
	GtkWidget *notebook;
	GtkWidget *statusbar;
	struct about about;
	GtkWidget *invisible;
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;

	/* notebook's widgets */
	struct ui_menu ui_menu;
	struct ui_program ui_program;
	struct ui_parameter ui_parameter;
	struct ui_validate ui_validate;
	GtkWidget *parameter_type_menu;

	/* icons */
	struct debr_pixmaps {
		GdkPixbuf *stock_apply;
		GdkPixbuf *stock_cancel;
		GdkPixbuf *stock_no;
	} pixmaps;

	/* config file */
	struct debr_config {
		GKeyFile *key_file;
		GString *path;

		GString *name;
		GString *email;
		GString *htmleditor;
		GString *browser;
		gboolean menu_sort_ascending;
		gint menu_sort_column;
		gchar **menu_dir;
	} config;

	GtkListStore *categories_model;

	/* temporary files removed when DeBR quits */
	GSList *tmpfiles;


	/*
	 * Structure to hold data and status to recovery menu.
	 */
	struct  {
		MenuStatus status;
		GebrGeoXmlFlow * clone;
	} menu_recovery;
};

/**
 * Initializes debr structures.
 */
void debr_init(void);

/**
 * Quits DeBR and frees its structures.
 */
gboolean debr_quit(void);

/**
 * Loads the configuration file into debr.config structure.
 *
 * DeBR is not considered configured if there is no configuration
 * file or there is no searching path defined on the configuration
 * file.
 *
 * @return #TRUE if debr is configured, #FALSE otherwise.
 */
gboolean debr_config_load(void);

/**
 * Save the current DeBR state into the configuration file.
 *
 * \see debr_config_load
 */
void debr_config_save(void);

/**
 * Logs \p message into stdout and DeBR status bar.
 *
 * @param type The log level, for example #GEBR_LOG_ERROR.
 * @param message A printf-like formated string.
 */
void debr_message(enum gebr_log_message_type type, const gchar * message, ...);

/**
 * Tells if \p category is present in the list of categories.
 *
 * @param category The category to look for.
 * @param add If #TRUE and \p category is not present, add it.
 * @return #TRUE if \p category is not present, #FALSE otherwise.
 */
gboolean debr_has_category(const gchar * category, gboolean add);

#endif				//__DEBR_H
