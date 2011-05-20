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

#include <libgebr/log.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/gui/gebr-gui-about.h>

#include "menu.h"
#include "program.h"
#include "parameter.h"
#include "interface.h"
#include "debr-validate.h"

G_BEGIN_DECLS

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

	/* Menu parameter validator */
	GebrValidator *validator;

	gint last_notebook;
	GtkAccelGroup *accel_group_array[ACCEL_N];
	GtkActionGroup *action_group_general;
	GtkActionGroup *action_group_menu;
	GtkActionGroup *action_group_program;
	GtkActionGroup *action_group_parameter;
	GtkActionGroup *action_group_validate;
	GtkActionGroup *common_action_group;
	GtkActionGroup *parameter_type_radio_actions_group;

	/* 'special' tool items */
	GtkToolItem *tool_item_new;
	GtkToolItem *tool_item_change_type;

	/* HashTable for holding HelpEditWindow's */
	GHashTable *help_edit_windows;

	/* notebook's widgets */
	struct ui_menu ui_menu;
	struct ui_program ui_program;
	struct ui_parameter ui_parameter;
	struct ui_validate ui_validate;

	/* icons */
	struct debr_pixmaps {
		GdkPixbuf *stock_apply;
		GdkPixbuf *stock_cancel;
		GdkPixbuf *stock_no;
		GdkPixbuf *stock_warning;
	} pixmaps;

	/* config file */
	struct debr_config {
		GKeyFile *key_file;
		GString *path;
		GHashTable *opened_folders;

		GString *name;
		GString *email;
		GString *htmleditor;
		gboolean native_editor;
		gboolean menu_sort_ascending;
		gint menu_sort_column;
	} config;

	GtkListStore *categories_model;

	/* temporary files removed when DeBR quits */
	GSList *tmpfiles;

	/* Structure to hold data and status to recovery menu. */
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
 * Loads the configuration file into \ref debr.config structure.
 * DeBR is not considered configured if there is no configuration file or there is no searching path defined on the
 * configuration file.
 *
 * \return #TRUE if debr is configured, #FALSE otherwise.
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
 * \param type The log level, for example #GEBR_LOG_ERROR.
 * \param message A printf-like formated string.
 */
void debr_message(enum gebr_log_message_type type, const gchar * message, ...);

/**
 * Tells if \p category is present in the list of categories.
 *
 * \param category The category to look for.
 * \param add If #TRUE and \p category is not present, add it.
 * \return #TRUE if \p category is not present, #FALSE otherwise.
 */
gboolean debr_has_category(const gchar * category, gboolean add);

/**
 * debr_remove_help_edit_window:
 * @object: The #GebrGeoXmlObject to be removed from the hash table.
 * @remove_only: If %TRUE don't destroy the window widget.
 * @destroy_children: If %TRUE, remove and destroy children windows.
 *
 * Removes the @object key in #debr.help_edit_windows, destroying the
 * associated #GebrGuiHelpEditWindow if @remove_only is %FALSE.
 */
void debr_remove_help_edit_window(gpointer object, gboolean remove_only, gboolean destroy_children);

G_END_DECLS
#endif				//__DEBR_H
