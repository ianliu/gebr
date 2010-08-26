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

/**
 * @file menu.h DeBR Menu API
 * @ingroup debr
 */

#ifndef __MENU_H
#define __MENU_H

#include <glib.h>

#include <libgebr/geoxml.h>

G_BEGIN_DECLS

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
	MENU_STATUS = 0,		/**< The #MenuStatus of this menu. */
	MENU_IMAGE,			/**< The stock image that represents this row type. */
	MENU_FILENAME,			/**< Title of this row.	*/
	MENU_MODIFIED_DATE,		/**< When this menu was last modified. */
	MENU_XMLPOINTER,		/**< Pointer to the Xml structure of this menu.	*/
	MENU_PATH,			/**< Absolute path for this menu in system. */
	MENU_VALIDATE_NEED_UPDATE,	/**< If menu has changed than validation needs to be updated. */
	/** If this menu was validated, then keeps the validation structure. */
	MENU_VALIDATE_POINTER,		
	MENU_N_COLUMN
};

/**
 * Error messages to handle menu save */
typedef enum {
	MENU_MESSAGE_PERMISSION_DENIED = 0,	/**< The program don't has access to the file			*/ 
	MENU_MESSAGE_SUCCESS			/**< Normal execution						*/
} MenuMessage;

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
		GtkWidget *hbox;

		GtkWidget *title_label;
		GtkWidget *description_label;
		GtkWidget *author_label;
		GtkWidget *nprogs_label;
		GtkWidget *created_label;
		GtkWidget *created_date_label;
		GtkWidget *modified_label;
		GtkWidget *modified_date_label;
		GtkWidget *category_label;
		GtkWidget *categories_label;
		GtkWidget *help_edit;
		GtkWidget *help_view;
	} details;

};


/**
 * Creates the interface to display the menus list in DeBR.
 */
void menu_setup_ui(void);

/**
 * Creates a new (unsaved) menu and add it to the tree view.
 *
 * \param edit Whether to immediately edit the menu or not.
 */
void menu_new(gboolean edit);

/**
 */
void menu_new_from_menu(GebrGeoXmlFlow *menu, gboolean edit);

/**
 * Loads an Xml representing a menu given by \p path and returns its structure.
 *
 * \param path The file path of the Xml file on the system.
 * \return The GebrGeoXmlFlow structure for this \p path.
 */
GebrGeoXmlFlow *menu_load(const gchar * path);

/**
 * Loads all menu files from the directories specified in the configuration file.
 *
 * DeBR searches for menu files in the directories listed in debr.config.menu_dir, which is filled when the
 * configuration is loaded. This search is not recursive and only the *.mnu files are loaded.
 */
void menu_load_user_directory(void);

/**
 * Load \p menu saved at \p path into \p iter.
 * If \p select is TRUE then select \p iter.
 */
void menu_load_iter(const gchar * path, GtkTreeIter * iter, GebrGeoXmlFlow * menu, gboolean select);

/**
 * Loads a menu at \p path and append it to \p parent.
 *
 * \param path The path for the menu on the file system.
 * \param parant The iterator that will hold this menu as a child.
 * \param select Whether to select this menu or not.
 */
void menu_open_with_parent(const gchar * path, GtkTreeIter * parent, gboolean select);

/**
 * Loads a menu at \p path and automatically places it on the correct folder.
 *
 * DeBR displays menus in a tree of folders, which is the direct representation of the menus in the file system. The
 * folders shown in DeBR are the ones registered in the configuration file (~/.gebr/debr/debr.conf).
 *
 * \param path The path for the menu on the file system.
 * \param select Whether to select the menu of not.
 *
 * \see menu_load_user_directory menu_open_with_parent
 */
void menu_open(const gchar * path, gboolean select);

/**
 * Save the menu identified by \p iter on the file system.
 *
 * Save the menu on \ref debr.ui_menu.model pointed by \p iter.  Note that the file must exist on the system, otherwise
 * this function return FALSE and nothing is done.
 *
 * \param iter The iterator pointing to the menu.
 * \return SUCCESS if the menu was successfully saved, FIRST_TIME_SAVE if it has never been saved and PERMISSION_DENIED
 * if program has no write permission granted.
 */
MenuMessage menu_save(GtkTreeIter * iter);

/**
 * Save a copy of the menu pointed by \p iter.
 * \return TRUE if saving was done, FALSE otherwise.
 */
gboolean menu_save_as(GtkTreeIter * iter);

/**
 */
gboolean menu_save_folder(GtkTreeIter * folder);

/**
 * Save all menus opened in DeBR that are unsaved.
 */
gboolean menu_save_all(void);

/**
 * Validates the menu pointed by \p iter.
 *
 * Validation consists of checking sentences which are not capitalized, invalid e-mail addresses, etc. This function
 * reports all possible errors in a menu and reports them.
 *
 * \param iter The iterator pointing to the menu to be validated.
 */
void menu_validate(GtkTreeIter * iter);

/**
 * Call GeBR to install selected(s) menus.
 */
void menu_install(void);

/**
 * Removes a menu pointed by \p iter and frees its memories.
 *
 * \param iter The iterator pointing to the menu.
 */
void menu_close(GtkTreeIter * iter, gboolean warn_user);

/**
 * Update details of the selected menu on the GUI.
 */
void menu_selected(void);

/**
 * Asks for save all unsaved menus; if yes, free memory allocated for them.
 */
gboolean menu_cleanup_folder(GtkTreeIter * folder);

/**
 */
gboolean menu_cleanup_iter_list(GList * list);

/**
 * Change the status of the currently selected menu.
 *
 * This affects the status of the currently selected menu by changing #MENU_STATUS
 *
 * \param status This can be either #MENU_STATUS_SAVED or #MENU_STATUS_UNSAVED.
 *
 * \see MenuStatus
 */
void menu_saved_status_set(MenuStatus status);

/**
 * Change the status of \p iter to either #MENU_STATUS_SAVED or #MENU_STATUS_UNSAVED.
 *
 * \param iter The iterator to have its status changed.
 * \param status The new status to be set to @iter.
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
gboolean menu_dialog_setup_ui(gboolean new_menu);

/**
 * Sets \p iter to the selected item in \ref debr.ui_menu.tree_view.
 *
 * \param iter The iterator that will point to the selected item.
 * \param warn_unselected_menu Whether to warn the user if no menu is selected.
 * \return One of #MENU_NONE, #MENU_FOLDER or #MENU_FILE, representing the various types of an item.
 *
 * \see MenuType
 */
gboolean menu_get_selected(GtkTreeIter * iter, gboolean warn_unselected_menu);

/**
 * Fetches the iterator type of the row pointed by \p iter.
 * \param iter The row to get its type.
 * \return The #IterType of \p iter.
 * \see menu_get_selected_type
 */
IterType menu_get_type(GtkTreeIter * iter);

/**
 * You <em>must</em> be sure that \p iter is valid to #debr.ui_menu.model.
 *
 * \param iter The iterator for #debr.ui_menu.model to get the type.
 * \return a #IterType.
 */
IterType menu_get_selected_type(GtkTreeIter * iter, gboolean warn_unselected_menu);

/**
 * Selects \p iter from the menu's tree view, expanding folders if necessary.
 *
 * \param iter The #GtkTreeIter to be selected.
 */
void menu_select_iter(GtkTreeIter * iter);

/**
 * Load details of selected menu to the details view.
 */
void menu_details_update(void);

/**
 * Updates the details for the folder pointed by \p iter.
 * 
 * \param iter The folder iterator for #debr.ui_menu.model.
 */
void menu_folder_details_update(GtkTreeIter * iter);

/**
 * Reload all menus, but the ones inside "Other" folder.
 */
void menu_reset(void);

/**
 * Calculates the number of opened menus.
 *
 * \return The number of opened menus.
 */
gint menu_get_n_menus(void);

/**
 * Given the \p path of the menu, assigns the correct parent item so the menu can be appended.
 *
 * \param path The menu system path.
 * \param parent The #GtkTreeIter to be set as the correct parent.
 */
void menu_path_get_parent(const gchar * path, GtkTreeIter * parent);

/**
 * Counts the number of unsaved menus and return it.
 * 
 * \return The number of unsaved menus.
 */
glong menu_count_unsaved(GtkTreeIter * folder);

/**
 * Replace the current menu by the clone made on the dialog start, by \ref menu_archive.
 */
void menu_replace(void);

/**
 * Store the menu data and status before a dialog has chance to change it.
 * Could be restored by \ref menu_replace 
 */
void menu_archive(void);

/**
 * Select program at \p program_path_string and parameter \p parameter_path_string
 * of current menu.
 * If \p parameter_path_string is NULL only select program.
 */
void menu_select_program_and_paramater(const gchar *program_path_string, const gchar *parameter_path_string);

/*
 * Folder manipulation functions
 */

/**
 * Opens folder \p path and load all its menus in interface.
 * \return TRUE if could open \p path or \p path is already open, FALSE otherwise.
 */
gboolean menu_open_folder(const gchar * path);

/**
 * Closes folder pointed by \p iter, unloading all its child menus.
 */
void menu_close_folder(GtkTreeIter * iter);

/**
 * Closes folder \p path, unloading all its menus and removing them from interface.
 */
void menu_close_folder_from_path(const gchar * path);

//======================================================================================================================
// Getters & Setters												       =
//======================================================================================================================

/**
 * menu_get_xml_pointer:
 * Gets the #GebrGeoXmlMenu associated to @iter.
 *
 * @iter: A #GtkTreeIter pointing to a row in DeBR's menus list.
 *
 * Returns: A #GebrGeoXmlMenu.
 */
GebrGeoXmlFlow * menu_get_xml_pointer(GtkTreeIter * iter);

G_END_DECLS
#endif				//__MENU_H
