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
 * \file callbacks.c General interface callbacks.
 * \see interface.c
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

/**
 *
 */
void on_notebook_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num);

/**
 * Callback to update navigation bar content according to menu, program and parameter selected.
 */
void do_navigation_bar_update(void);

/**
 * Select new target depending on the context.
 */
void on_new_activate(void);

/**
 * Selects cut target depending on the context.
 */
void on_cut_activate(void);

/**
 * Select copy target depending on the context.
 */
void on_copy_activate(void);

/**
 * Select paste target depending on the context.
 */
void on_paste_activate(void);

/**
 * Calls \ref debr_quit.
 */
void on_quit_activate(void);

/**
 * Calls \ref menu_new.
 */
void on_menu_new_activate(void);

/**
 * Create a open dialog and manage it to open a menu.
 */
void on_menu_open_activate(void);

/**
 * Saves the selected menu; if it doesn't have a path assigned, calls \ref on_menu_save_as_activate.
 */
void on_menu_save_activate(void);

/**
 * Open a save dialog; get the path and save the menu at it.
 */
void on_menu_save_as_activate(void);

/**
 * Calls \ref menu_save_all.
 */
void on_menu_save_all_activate(void);

/**
 * Confirm action and if confirmed reload menu from file.
 */
void on_menu_revert_activate(void);

/**
 * Confirm action and if confirm delete it from the disk and calls \ref on_menu_close_activate.
 */
void on_menu_delete_activate(void);

/**
 * Calls \ref menu_dialog_setup_ui.
 */
void on_menu_properties_activate(void);

/**
 * Calls \ref menu_validate.
 */
void on_menu_validate_activate(void);

/**
 * Calls \ref menu_install.
 */
void on_menu_install_activate(void);

/**
 * Delete menu from the view.
 */
void on_menu_close_activate(void);

/**
 * Calls \ref program_new.
 */
void on_program_new_activate(void);

/**
 * Calls \ref program_remove.
 */
void on_program_delete_activate(void);

/**
 * Calls \ref program_dialog_setup_ui.
 */
gboolean on_program_properties_activate(void);

/**
 * Calls \ref program_preview.
 */
void on_program_preview_activate(void);

/**
 * Calls \ref program_top.
 */
void on_program_top_activate(void);

/**
 * Calls \ref program_bottom.
 */
void on_program_bottom_activate(void);

/**
 * Calls \ref program_copy.
 */
void on_program_copy_activate(void);

/**
 * Calls \ref program_paste.
 */
void on_program_paste_activate(void);

/**
 * Calls \ref parameter_remove.
 */
void on_parameter_delete_activate(void);

/**
 * Calls \ref parameter_remove.
 */
void on_parameter_properties_activate(void);

/**
 * Calls \ref parameter_top.
 */
void on_parameter_top_activate(void);

/**
 * Calls \ref parameter_bottom.
 */
void on_parameter_bottom_activate(void);

/**
 * Calls \ref parameter_remove.
 */
void on_parameter_change_type_activate(void);

/**
 * Calls \ref parameter_change_type.
 */
void on_parameter_type_activate(GtkRadioAction * first_action);

/**
 * Calls \ref parameter_copy.
 */
void on_parameter_copy_activate(void);

/**
 * Calls \ref parameter_cut.
 */
void on_parameter_cut_activate(void);

/**
 * Calls \ref parameter_paste.
 */
void on_parameter_paste_activate(void);

/**
 * Calls \ref validate_close.
 */
void on_validate_close_activate(void);

/**
 * Calls \ref validate_clear.
 */
void on_validate_clear_activate(void);

/**
 * Calls \ref preferences_dialog_setup_ui.
 */
void on_configure_preferences_activate(void);

/**
 * Calls \ref gebr_gui_help_show.
 */
void on_help_contents_activate(void);

/**
 * Show \ref debr.about.dialog.
 */
void on_help_about_activate(void);

/**
 * Pops a \ref GtkMenu for showing parameters types for creation.
 */
gboolean on_parameter_tool_item_new_press(GtkWidget * tool_button);

/**
 * Calls \ref parameter_create_menu_with_types.
 */
void on_drop_down_menu_requested(GtkWidget * button, gpointer data);

#endif				//__CALLBACKS_H
