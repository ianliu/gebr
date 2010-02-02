/**
 * @file menu.h DeBR Menu API
 * @ingroup debr
 */

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

/**
 * Indicates the status of a menu.
 */
typedef enum {
	MENU_STATUS_SAVED,	/**< The menu is saved on the system.				*/
	MENU_STATUS_UNSAVED	/**< The menu has changes since last load or never saved.	*/
} MenuStatus;

/**
 * A classification for an iterator on DeBR tree model.
 */
typedef enum {
	ITER_NONE = 0,		/**< Unknown iterator type.					*/
	ITER_FOLDER,		/**< Iterator is a folder in DeBR menus list.			*/
	ITER_FILE		/**< Iterator is a menu in DeBR menus list.			*/
} IterType;

/**
 * Colums for debr.ui_menu.model, that keeps all opened menus.
 */
enum {
	MENU_STATUS = 0,	/**< The #MenuStatus of this menu.				*/
	MENU_IMAGE,		/**< The stock image that represents this row type.		*/
	MENU_FILENAME,		/**< Title of this row.						*/
	MENU_MODIFIED_DATE,	/**< When this menu was last modified.				*/
	MENU_XMLPOINTER,	/**< Pointer to the Xml structure of this menu.			*/
	MENU_PATH,		/**< Absolute path for this menu in system.			*/
	MENU_N_COLUMN
};

/**
 * Structure to hold widgets & data for the menus visualization.
 */
struct ui_menu {
	GtkWidget *widget;

	GtkTreeStore *model;
	GtkWidget *tree_view;
	GtkTreeIter iter_other;

	struct ui_menu_details {
		GtkWidget *vbox;

		GtkWidget *title_label;
		GtkWidget *description_label;
		GtkWidget *author_label;
		GtkWidget *nprogs_label;
		GtkWidget *created_label;
		GtkWidget *created_date_label;
		GtkWidget *modified_label;
		GtkWidget *modified_date_label;
		GtkWidget *category_label;
		GtkWidget *categories_label[3];
		GtkWidget *help_button;

	} details;
};

/**
 * Creates the interface to display the menus list in DeBR.
 */
void menu_setup_ui(void);

/**
 * Creates a new (unsaved) menu and add it to the tree view.
 *
 * @param edit Whether to immediately edit the menu or not.
 */
void menu_new(gboolean edit);

/**
 * Loads an Xml representing a menu given by \p path and returns its structure.
 *
 * @param path The file path of the Xml file on the system.
 * @return The GebrGeoXmlFlow structure for this \p path.
 */
GebrGeoXmlFlow *menu_load(const gchar * path);

/**
 * Loads all menu files from the directories specified in the configuration file.
 *
 * DeBR searches for menu files in the directories listed in debr.config.menu_dir,
 * which is filled when the configuration is loaded. This search is not recursive
 * and only the *.mnu files are loaded.
 */
void menu_load_user_directory(void);

/**
 * Loads a menu at \p path and append it to \p parent.
 *
 * @param path The path for the menu on the file system.
 * @param parant The iterator that will hold this menu as a child.
 * @param select Whether to select this menu or not.
 */
void menu_open_with_parent(const gchar * path, GtkTreeIter * parent, gboolean select);

/**
 * Loads a menu at \p path and automatically places it on the correct folder.
 *
 * DeBR displays menus in a tree of folders, which is the direct representation of the menus
 * in the file system. The folders shown in DeBR are the ones registered in the configuration
 * file (~/.gebr/debr/debr.conf).
 *
 * @param path The path for the menu on the file system.
 * @param select Whether to select the menu of not.
 *
 * \see menu_load_user_directory menu_open_with_parent
 */
void menu_open(const gchar * path, gboolean select);

/**
 * Save the menu identified by \p iter on the file system.
 *
 * Save the menu on \ref debr.ui_menu.model pointed by \p iter.
 * Note that the file must exist on the system, otherwise this
 * function return FALSE and nothing is done.
 *
 * @param iter The iterator pointing to the menu.
 * @return TRUE if the menu was successfully saved, FALSE if it has never been saved.
 */
gboolean menu_save(GtkTreeIter * iter);

/**
 * Save all menus opened in DeBR that are unsaved.
 */
void menu_save_all(void);

/**
 * Validates the menu pointed by \p iter.
 *
 * Validation consists of checking sentences which are not capitalized, invalid
 * e-mail addresses, etc. This function reports all possible errors in a menu
 * and reports them.
 *
 * @param iter The iterator pointing to the menu to be validated.
 */
void menu_validate(GtkTreeIter * iter);

/**
 * Call GeBR to install selected(s) menus.
 */
void menu_install(void);

/**
 * Removes a menu pointed by \p iter and frees its memories.
 *
 * @param iter The iterator pointing to the menu.
 */
void menu_close(GtkTreeIter * iter);

/**
 * Update details of the selected menu on the GUI.
 */
void menu_selected(void);

/**
 * Asks for save all unsaved menus; if yes, free memory allocated for them.
 */
gboolean menu_cleanup(void);

/**
 * Change the status of the currently selected menu.
 *
 * This affects the status of the currently selected menu by changing #MENU_STATUS
 *
 * @param status This can be either #MENU_STATUS_SAVED or #MENU_STATUS_UNSAVED.
 *
 * \see MenuStatus
 */
void menu_saved_status_set(MenuStatus status);

/**
 * Change the status of \p iter to either #MENU_STATUS_SAVED or #MENU_STATUS_UNSAVED.
 *
 * @param iter The iterator to have its status changed.
 * @param status The new status to be set to @iter.
 */
void menu_status_set_from_iter(GtkTreeIter * iter, MenuStatus status);

/**
 * Connected to signal of components which change the menu.
 * 
 * This function changes the status of the current selected menu to unsaved.
 */
void menu_status_set_unsaved(void);

/**
 * Create a dialog to edit information about menu, like title, description, categories, etc.
 */
void menu_dialog_setup_ui(void);

/**
 * Sets \p iter to the selected item in \ref debr.ui_menu.tree_view.
 *
 * @param iter The iterator that will point to the selected item.
 * @param warn_unselected_menu Whether to warn the user if no menu is selected.
 * @return One of #MENU_NONE, #MENU_FOLDER or #MENU_FILE, representing the various types of an item.
 *
 * @see MenuType
 */
gboolean menu_get_selected(GtkTreeIter * iter, gboolean warn_unselected_menu);

/**
 * Fetches the iterator type of the row pointed by \p iter.
 * @param iter The row to get its type.
 * @return The #IterType of \p iter.
 * @see menu_get_selected_type
 */
IterType menu_get_type(GtkTreeIter * iter);

/**
 * You <em>must</em> be sure that \p iter is valid to #debr.ui_menu.model.
 *
 * @param iter The iterator for #debr.ui_menu.model to get the type.
 * @return a #IterType.
 */
IterType menu_get_selected_type(GtkTreeIter * iter, gboolean warn_unselected_menu);

/**
 * Selects \p iter from the menu's tree view, expanding folders if necessary.
 *
 * @param iter The #GtkTreeIter to be selected.
 */
void menu_select_iter(GtkTreeIter * iter);

/**
 * Load details of selected menu to the details view.
 */
void menu_details_update(void);

/**
 * Updates the details for the folder pointed by \p iter.
 * 
 * @param iter The folder iterator for #debr.ui_menu.model.
 */
void menu_folder_details_update(GtkTreeIter * iter);

/**
 * Reload all menus, but the ones inside "Other" folder.
 */
void menu_reset(void);

/**
 * Calculates the number of opened menus.
 *
 * @return The number of opened menus.
 */
gint menu_get_n_menus(void);

/**
 * Given the \p path of the menu, assigns the correct parent item so the menu can be appended.
 *
 * @param path The menu system path.
 * @param parent The #GtkTreeIter to be set as the correct parent.
 */
void menu_path_get_parent(const gchar * path, GtkTreeIter * parent);

/**
 * Counts the number of unsaved menus and return it.
 * 
 * @return The number of unsaved menus.
 */
glong menu_count_unsaved(void);

/**
 * Replace the current menu by the clone \p new_menu made on the dialog start.
 *
 * @param new_menu The clone passed to replace the current menu
 *
 */
void menu_replace(GebrGeoXmlFlow * new_menu);
#endif				//__MENU_H
