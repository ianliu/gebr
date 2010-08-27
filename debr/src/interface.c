/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <libgebr/gui/pixmaps.h>

#include <libgebr/gui/utils.h>
#include <libgebr/gui/valuesequenceedit.h>

#include <libgebr/intl.h>

#include "interface.h"
#include "debr.h"
#include "callbacks.h"
#include "menu.h"
#include "program.h"
#include "parameter.h"

/*
 * Prototypes
 */

static const GtkActionEntry actions_entries[] = {
	{"quit", GTK_STOCK_QUIT, N_("Quit"), NULL, N_("Quit DéBR"), G_CALLBACK(on_quit_activate)},
	{"help_contents", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(on_help_contents_activate)},
	{"help_about", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(on_help_about_activate)},
	/* menu */
	{"menu_new", GTK_STOCK_NEW, NULL, NULL, N_("Create new menu"), G_CALLBACK(on_menu_new_activate)},
	{"menu_create_from_menu", "document-import", N_("Import"),
		NULL, N_("Create menu from flow"), G_CALLBACK(on_menu_create_from_flow_activate)},
	{"menu_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit menu properties"),
	 G_CALLBACK(on_menu_properties_activate)},
	{"menu_validate", GTK_STOCK_APPLY, N_("Validate"), NULL, N_("Validate menu"),
	 G_CALLBACK(on_menu_validate_activate)},
	{"menu_install", "window-new", N_("Install"), NULL, N_("Make this menu visible for GêBR"),
	 G_CALLBACK(on_menu_install_activate)},
	{"menu_close", GTK_STOCK_CLOSE, NULL, NULL, N_("Remove selected menus from list"),
	 G_CALLBACK(on_menu_close_activate)},
	{"menu_open", GTK_STOCK_OPEN, NULL, NULL, N_("Open an existing menu"),
	 G_CALLBACK(on_menu_open_activate)},
	{"menu_save", GTK_STOCK_SAVE, NULL, NULL, N_("Save selected menus"), G_CALLBACK(on_menu_save_activate)},
	{"menu_save_as", GTK_STOCK_SAVE_AS, NULL, NULL, N_("Save current menu to another file"),
	 G_CALLBACK(on_menu_save_as_activate)},
	{"menu_save_all", "document-save-all", N_("Save all"), NULL, N_("Save all unsaved menus"),
	 G_CALLBACK(on_menu_save_all_activate)},
	{"menu_revert", GTK_STOCK_REVERT_TO_SAVED, NULL, NULL, N_("Revert current menu to last saved version"),
	 G_CALLBACK(on_menu_revert_activate)},
	{"menu_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete selected menus files"),
	 G_CALLBACK(on_menu_delete_activate)},
	{"menu_add_folder", GTK_STOCK_ADD, N_("Add folder"), NULL, N_("Add a new folder"),
	 G_CALLBACK(on_menu_add_folder_activate)},
	{"menu_remove_folder", GTK_STOCK_REMOVE, N_("Remove folder"), NULL, N_("Remove a folder"),
	 G_CALLBACK(on_menu_remove_folder_activate)},
	{"menu_select_all", GTK_STOCK_SELECT_ALL, N_("Select all"), NULL, N_("Select all menus"), NULL},
	{"menu_unselect_all", NULL, N_("Unselect all"), NULL, N_("Unselect all menus"), NULL},
	{"menu_help_view", GTK_STOCK_INFO, N_("View Help"),
		NULL, N_("View the help related to menu"), G_CALLBACK(on_menu_help_show_clicked)},
	{"menu_help_edit", GTK_STOCK_EDIT, N_("Edit Help"),
		NULL, N_("Edit the help related to menu"), G_CALLBACK(on_menu_help_edit_clicked)},

	/* program */
	{"program_new", GTK_STOCK_NEW, NULL, NULL, N_("Create new program"),
	 G_CALLBACK(on_program_new_activate)},
	{"program_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete current program"),
	 G_CALLBACK(on_program_delete_activate)},
	{"program_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit program properties"),
	 G_CALLBACK(on_program_properties_activate)},
	{"program_preview", GTK_STOCK_MEDIA_PLAY, N_("Preview parameters in GêBR"), NULL,
	 N_("Preview parameters in GêBR"), G_CALLBACK(on_program_preview_activate)},
	{"program_top", GTK_STOCK_GOTO_TOP, NULL, NULL, N_("Move program to top"),
	 G_CALLBACK(on_program_top_activate)},
	{"program_bottom", GTK_STOCK_GOTO_BOTTOM, NULL, NULL,
	 N_("Move program to bottom"), G_CALLBACK(on_program_bottom_activate)},
	{"program_copy", GTK_STOCK_COPY, N_("Copy"), NULL, N_("Copy selected program(s) to clipboard"),
	 G_CALLBACK(on_program_copy_activate)},
	{"program_paste", GTK_STOCK_PASTE, N_("Paste"), NULL, N_("Paste program(s) from clipboard"),
	 G_CALLBACK(on_program_paste_activate)},
	{"program_help_view", GTK_STOCK_INFO, N_("View Help"),
		NULL, N_("View the help related to program"), G_CALLBACK(on_program_help_show)},
	{"program_help_edit", GTK_STOCK_EDIT, N_("Edit Help"),
		NULL, N_("Edit the help related to program"), G_CALLBACK(on_program_help_edit)},

	/* parameter */
	// "paramter_new" is a special button, where a popup menu is created on its bottom.
	{"parameter_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete current parameter"),
	 G_CALLBACK(on_parameter_delete_activate)},
	{"parameter_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit parameter properties"),
	 G_CALLBACK(on_parameter_properties_activate)},
	{"parameter_top", GTK_STOCK_GOTO_TOP, NULL, NULL, N_("Move parameter to  top"),
	 G_CALLBACK(on_parameter_top_activate)},
	{"parameter_bottom", GTK_STOCK_GOTO_BOTTOM, NULL, NULL,
	 N_("Move parameter to bottom"), G_CALLBACK(on_parameter_bottom_activate)},
	{"parameter_change_type", GTK_STOCK_CONVERT, N_("Change type"), NULL, N_("Change parameter type"),
	 NULL},
	{"parameter_cut", GTK_STOCK_CUT, N_("Cut"), NULL, N_("Cut selected parameter(s) to clipboard"),
	 G_CALLBACK(on_parameter_cut_activate)},
	{"parameter_copy", GTK_STOCK_COPY, N_("Copy"), NULL, N_("Copy selected parameter(s) to clipboard"),
	 G_CALLBACK(on_parameter_copy_activate)},
	{"parameter_paste", GTK_STOCK_PASTE, N_("Paste"), NULL, N_("Paste parameter(s) from clipboard"),
	 G_CALLBACK(on_parameter_paste_activate)},
	/* validate */
	{"validate_close", GTK_STOCK_CLOSE, NULL, NULL, N_("Close selected reports"),
	 G_CALLBACK(on_validate_close_activate)},
	{"validate_clear", GTK_STOCK_CLEAR, NULL, NULL, N_("Close all reports"),
	 G_CALLBACK(on_validate_clear_activate)},
};

static const GtkActionEntry common_actions_entries[] = {
	{"new", GTK_STOCK_NEW, "new", NULL, "new", G_CALLBACK(on_new_activate)},
	{"cut", GTK_STOCK_CUT, "cut", NULL, "cut", G_CALLBACK(on_cut_activate)},
	{"copy", GTK_STOCK_COPY, "copy", NULL, "copy", G_CALLBACK(on_copy_activate)},
	{"paste", GTK_STOCK_PASTE, "paste", NULL, "paste", G_CALLBACK(on_paste_activate)},
};

/*
 * Public methods
 */

void debr_setup_ui(void)
{
	GtkWidget *vbox;
	GtkWidget *menu_vbox;
	GtkWidget *program_vbox;
	GtkWidget *parameter_vbox;
	GtkWidget *validate_vbox;

	GtkWidget *menu_bar;
	GtkWidget *navigation_hbox;
	GtkWidget *notebook;
	GtkWidget *statusbar;

	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkWidget *child_menu_item;
	GtkWidget *toolbar;

	GtkActionGroup *common_action_group;

	/*
	 * Main window and its vbox contents
	 */
	gtk_window_set_default_icon(gebr_gui_pixmaps_gebr_icon_16x16());
	debr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(debr.window), "DéBR");
	gtk_widget_set_size_request(debr.window, 600, 450);
	debr.about = gebr_gui_about_setup_ui("DéBR", _("Menu designer for GêBR"));

	g_signal_connect(debr.window, "delete_event", G_CALLBACK(debr_quit), NULL);
	g_signal_connect(debr.window, "show", G_CALLBACK(debr_init), NULL);

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
	g_signal_connect(notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);

	debr.statusbar = statusbar = gtk_statusbar_new();
	gtk_widget_show(statusbar);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

	/*
	 * Actions
	 */
	debr.action_group = gtk_action_group_new("General");
	gtk_action_group_set_translation_domain(debr.action_group, PACKAGE);
	gtk_action_group_add_actions(debr.action_group, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gtk_action_group_add_radio_actions(debr.action_group, parameter_type_radio_actions_entries,
					   combo_type_map_size, -1, G_CALLBACK(on_parameter_type_activate), NULL);
	debr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(debr.window), debr.accel_group);
	gebr_gui_gtk_action_group_set_accel_group(debr.action_group, debr.accel_group);

	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "menu_new"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "program_new"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "program_copy"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "parameter_cut"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "parameter_copy"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "program_paste"));
	gtk_action_disconnect_accelerator(gtk_action_group_get_action(debr.action_group, "parameter_paste"));

	common_action_group = gtk_action_group_new("Common");
	gtk_action_group_add_actions(common_action_group, common_actions_entries,
				     G_N_ELEMENTS(common_actions_entries), NULL);
	gebr_gui_gtk_action_group_set_accel_group(common_action_group, debr.accel_group);

	/*
	 * Menu: Actions
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("_Actions"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, debr.accel_group);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(child_menu_item, "activate", G_CALLBACK(on_configure_preferences_activate), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "quit")));

	/*
	 * Menu: Help
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	//gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      //gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "help_contents")));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "help_about")));

	gtk_widget_show_all(menu_bar);

	/*
	 * Notebook page: Menu
	 */
	menu_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(menu_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), menu_vbox, gtk_label_new(_("Menu")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_open"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save_as"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save_all"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_revert"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_delete"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_validate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_create_from_menu"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_close"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_install"))), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(menu_vbox), toolbar, FALSE, FALSE, 0);

	/*
	 * Notebook page: Program
	 */
	program_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(program_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), program_vbox, gtk_label_new(_("Program")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save_as"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_revert"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_validate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_properties"))), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(program_vbox), toolbar, FALSE, FALSE, 0);

	/*
	 * Notebook page: Parameter
	 */
	parameter_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(parameter_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), parameter_vbox, gtk_label_new(_("Parameter")));

	/* Treating the 'new' button */
	debr.tool_item_new = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_tool_item_set_tooltip_text(debr.tool_item_new, _("Create a new parameter"));
	gebr_gui_gtk_widget_set_drop_down_menu_on_click(gtk_bin_get_child(GTK_BIN(debr.tool_item_new)),
							(GebrGuiDropDownFunc)on_drop_down_menu_requested, GINT_TO_POINTER(0));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save_as"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_revert"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_validate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(debr.tool_item_new), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "parameter_cut"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "parameter_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "parameter_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "parameter_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "parameter_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "program_preview"))), -1);

	debr.tool_item_change_type = gtk_tool_button_new_from_stock(GTK_STOCK_CONVERT);
	gtk_tool_item_set_tooltip_text(debr.tool_item_change_type, _("Change parameter type"));
	gebr_gui_gtk_widget_set_drop_down_menu_on_click(gtk_bin_get_child(GTK_BIN(debr.tool_item_change_type)),
							(GebrGuiDropDownFunc)on_drop_down_menu_requested, GINT_TO_POINTER(1));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), debr.tool_item_change_type, -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), toolbar, FALSE, FALSE, 0);

	/*
	 * Notebook page: Validate
	 */
	validate_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(validate_vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), validate_vbox, gtk_label_new(_("Validate")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_save_as"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_revert"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "menu_validate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "validate_close"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(debr.action_group, "validate_clear"))), -1);


	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(validate_vbox), toolbar, FALSE, FALSE, 0);

	menu_setup_ui();
	program_setup_ui();
	parameter_setup_ui();
	validate_setup_ui();

	gtk_box_pack_start(GTK_BOX(menu_vbox), debr.ui_menu.widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(program_vbox), debr.ui_program.widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), debr.ui_parameter.widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(validate_vbox), debr.ui_validate.widget, TRUE, TRUE, 0);

	gtk_widget_show(debr.window);
}

void debr_set_actions_sensitive(gchar ** names, gboolean sensitive)
{
	gint i;

	if (!names)
		return;

	i = 0;
	while (names[i])
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, names[i++]), sensitive);
}

