/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gui/pixmaps.h>

#include <gui/utils.h>
#include <gui/valuesequenceedit.h>

#include "interface.h"
#include "debr.h"
#include "callbacks.h"
#include "support.h"
#include "menu.h"
#include "program.h"
#include "parameter.h"

/*
 * File: interface.c
 * Interface creation: mainwindow, actions and callbacks assignmment.
 * See also <callbacks.c>
 */

/*
 * Declarations
 */

static const GtkActionEntry actions_entries [] = {
	/* Main */
	{"main_quit", GTK_STOCK_QUIT, _("Quit"), "<Control>q", _("Quit D�BR"), (GCallback)debr_quit},
	/* menu */
	{"menu_new", GTK_STOCK_NEW, NULL, NULL, _("Create new menu"), (GCallback)on_menu_new_activate},
	{"menu_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit menu properties"),
		(GCallback)on_menu_properties_activate},
	{"menu_validate", GTK_STOCK_APPLY, _("Validate"), NULL, _("Validate menu"),
		(GCallback)on_menu_validate_activate},
	{"menu_close", GTK_STOCK_CLOSE, NULL, NULL, _("Remove menu from list"),
		(GCallback)on_menu_close_activate},
	{"menu_open", GTK_STOCK_OPEN, NULL, NULL, _("Open an existing menu"),
		(GCallback)on_menu_open_activate},
	{"menu_save", GTK_STOCK_SAVE, NULL, NULL, _("Save current menu"), (GCallback)on_menu_save_activate},
	{"menu_save_as", GTK_STOCK_SAVE_AS, NULL, NULL, _("Save current menu to another file"),
		(GCallback)on_menu_save_as_activate},
	{"menu_save_all", "document-save-all", _("Save all"), NULL, _("Save all unsaved menus"),
		(GCallback)on_menu_save_all_activate},
	{"menu_revert", GTK_STOCK_REVERT_TO_SAVED, NULL, NULL, _("Revert current menu to last saved version"),
		(GCallback)on_menu_revert_activate},
	{"menu_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete menu file"),
		(GCallback)on_menu_delete_activate},
	/* program */
	{"program_new", GTK_STOCK_NEW, NULL, NULL, _("Create new program"),
		(GCallback)on_program_new_activate},
	{"program_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete current program"),
		(GCallback)on_program_delete_activate},
	{"program_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit program properties"),
		(GCallback)on_program_properties_activate},
	{"program_top", GTK_STOCK_GOTO_TOP, NULL, NULL, _("Move program to the top of the list"),
		(GCallback)on_program_top_activate},
	{"program_bottom", GTK_STOCK_GOTO_BOTTOM, NULL, NULL,
		_("Move program to the bottom of the list"), (GCallback)on_program_bottom_activate},
	{"program_copy", GTK_STOCK_COPY, _("Copy"), "<Control>c", _("Copy program to clipboard"),
		(GCallback)on_program_copy_activate},
	{"program_paste", GTK_STOCK_PASTE, _("Paste"), "<Control>v", _("Paste program from clipboard"),
		(GCallback)on_program_paste_activate},
	/* parameter */
	{"parameter_new", GTK_STOCK_NEW, NULL, NULL, _("Create new parameter"), (GCallback)on_parameter_new_activate},
	{"parameter_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete current parameter"),
		(GCallback)on_parameter_delete_activate},
	{"parameter_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit parameter properties"),
		(GCallback)on_parameter_properties_activate},
	{"parameter_top", GTK_STOCK_GOTO_TOP, NULL, NULL, _("Move parameter to the top of the list"),
		(GCallback)on_parameter_top_activate},
	{"parameter_bottom", GTK_STOCK_GOTO_BOTTOM, NULL, NULL,
		_("Move parameter to the bottom of the list"), (GCallback)on_parameter_bottom_activate},
	{"parameter_duplicate", GTK_STOCK_COPY, _("Duplicate"), NULL, _("Duplicate parameter"),
		(GCallback)on_parameter_duplicate_activate},
	{"parameter_change_type", GTK_STOCK_CONVERT, _("Change type"), NULL, _("Change parameter type"),
		(GCallback)on_parameter_change_type_activate},
	{"parameter_copy", GTK_STOCK_COPY, _("Copy"), "<Control>c", _("Copy parameter to clipboard"),
		(GCallback)on_parameter_copy_activate},
	{"parameter_paste", GTK_STOCK_PASTE, _("Paste"), "<Control>v", _("Paste parameter from clipboard"),
		(GCallback)on_parameter_paste_activate},
	/* validate */
	{"validate_close", "edit-clear", NULL, NULL, _("Clear selecteds validations reports"),
		(GCallback)on_validate_close},
	{"validate_clear", GTK_STOCK_CLEAR, NULL, NULL, _("Clear all validations reports"),
		(GCallback)on_validate_clear},

};

/*
 * Section: Public
 */

/*
 * Function: debr_setup_ui
 * Create DeBR's main window and its childs
 */
void
debr_setup_ui(void)
{
	GtkWidget *		vbox;

	GtkWidget *		menu_bar;
	GtkWidget *		navigation_hbox;
	GtkWidget *		notebook;
	GtkWidget *		statusbar;

	GtkWidget *		menu;
	GtkWidget *		menu_item;
	GtkWidget *		child_menu_item;
	GtkWidget *		toolbar;
	GtkToolItem *		tool_item;

	/*
	 * Main window and its vbox contents
	 */
	gtk_window_set_default_icon(pixmaps_gebr_icon_16x16());
	debr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(debr.window), "DéBR");
	gtk_widget_set_size_request(debr.window, 600, 450);
	debr.about = about_setup_ui("DéBR", _("Flow designer for GêBR"));

	g_signal_connect(debr.window, "delete_event",
		GTK_SIGNAL_FUNC(debr_quit), NULL);
	g_signal_connect(debr.window, "show",
		GTK_SIGNAL_FUNC(debr_init), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(debr.window), vbox);

	menu_bar = gtk_menu_bar_new();
	gtk_widget_show(menu_bar);
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

	navigation_hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), navigation_hbox, FALSE, FALSE, 4);
	debr.navigation_box_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(navigation_hbox), debr.navigation_box_label, FALSE, FALSE, 0);
	gtk_widget_show_all(navigation_hbox);

	debr.notebook = notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	debr.statusbar = statusbar = gtk_statusbar_new();
	gtk_widget_show(statusbar);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

	/*
	 * Actions
	 */
	debr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(debr.window), debr.accel_group);
	debr.action_group = gtk_action_group_new("General");
	gtk_action_group_add_actions(debr.action_group, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gtk_action_group_add_radio_actions(debr.action_group, parameter_type_radio_actions_entries,
		combo_type_map_size, -1, (GCallback)on_parameter_type_activate, NULL);

	/*
	 * Menu: Actions
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("_Actions"));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, debr.accel_group);
	gtk_container_add(GTK_CONTAINER(menu), child_menu_item);
	g_signal_connect(child_menu_item, "activate",
		(GCallback)on_configure_preferences_activate, NULL);
	gtk_container_add(GTK_CONTAINER(menu),gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "main_quit")));

	/*
	 * Menu: Help
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, debr.accel_group);
	gtk_container_add(GTK_CONTAINER(menu), child_menu_item);
	g_signal_connect(child_menu_item, "activate",
		(GCallback)on_help_about_activate, NULL);

	gtk_widget_show_all(menu_bar);

	/*
	 * Notebook page: Menu
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Menu")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_validate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_close"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_open"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_save_as"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_save_all"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_revert"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "menu_delete"))), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	menu_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_menu.widget, TRUE, TRUE, 0);

	/*
	 * Notebook page: Program
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Program")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "program_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "program_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "program_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "program_delete"))), -1);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "program_properties"))), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	program_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_program.widget, TRUE, TRUE, 0);

	/*
	 * Notebook page: Parameter
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Parameter")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_duplicate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "parameter_properties"))), -1);

	debr.parameter_type_menu = menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock(GTK_STOCK_CONVERT);
	g_signal_connect(tool_item, "clicked",
		(GCallback)on_parameter_change_type_activate, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);
	guint i, l = combo_type_map_size;
	for (i = 0; i < l; ++i)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, parameter_type_radio_actions_entries[i].name)));

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	parameter_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_parameter.widget, TRUE, TRUE, 0);

	/*
	 * Notebook page: Validate
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Validate")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "validate_close"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(debr.action_group, "validate_clear"))), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	validate_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_validate.widget, TRUE, TRUE, 0);
}

/*
 * Section: Private
 */
