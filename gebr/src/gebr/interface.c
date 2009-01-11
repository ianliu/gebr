/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/*
 * File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preferences and menu bar.
 */

#include <string.h>

#include <gdk/gdkkeysyms.h>

#include <gui/pixmaps.h>

#include "interface.h"
#include "gebr.h"
#include "support.h"
#include "callbacks.h"

/*
 * Private data
 */

static const GtkActionEntry actions_entries [] = {
	{"main_quit",  GTK_STOCK_QUIT, NULL, "<Control>q", NULL, (GCallback)gebr_quit},
	/* Project/Line */
	{"project_line_new_project", "folder-new", _("New Project"), NULL, _("Create a new project"),
		(GCallback)on_project_line_new_project_activate},
	{"project_line_new_line", "tab-new-background", _("New Line"), NULL,  _("Create a new line"),
		(GCallback)on_project_line_new_line_activate},
	{"project_line_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete selected project or line"),
		(GCallback)on_project_line_delete_activate},
	{"project_line_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit properties"),
		(GCallback)on_project_line_properties_activate},
	{"project_line_line_paths", GTK_STOCK_DIRECTORY, _("Line paths"), NULL, _("Edit custom line paths"),
		(GCallback)on_project_line_paths_activate},
	/* Flow */
	{"flow_new", GTK_STOCK_NEW, NULL, NULL, _("Create a new flow"), (GCallback)on_flow_new_activate},
	{"flow_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete selected flow"), (GCallback)on_flow_delete_activate},
	{"flow_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit flow properties"),
		(GCallback)on_flow_properties_activate},
	{"flow_io", "system-switch-user", _("Input and Output"), NULL, _("Edit input/output flow files"),
		(GCallback)on_flow_io_activate},
	{"flow_execute", GTK_STOCK_EXECUTE, NULL, "<Control>r", _("Run current flow"),
		(GCallback)on_flow_execute_activate},
	{"flow_import", "document-import", _("Import"), NULL, _("Import a flow"), (GCallback)on_flow_import_activate},
	{"flow_export", "document-export", _("Export"), NULL, _("Export the flow"),
		(GCallback)on_flow_export_activate},
	{"flow_export_as_menu", GTK_STOCK_CONVERT, _("Export as menu"), NULL, _("Export the flow as a menu"),
		(GCallback)on_flow_export_as_menu_activate},
	/* Flow Edition */
	{"flow_edition_help", GTK_STOCK_HELP, NULL, NULL, _("Show component's help"),
		(GCallback)on_flow_component_help_activate},
	{"flow_edition_duplicate", GTK_STOCK_COPY, NULL, NULL, _("Duplicate component"),
		(GCallback)on_flow_component_duplicate_activate},
	{"flow_edition_delete", GTK_STOCK_DELETE, NULL, NULL, _("Delete component"),
		(GCallback)on_flow_component_delete_activate},
	{"flow_edition_properties", GTK_STOCK_PROPERTIES, NULL, NULL, _("Edit component parameters"),
		(GCallback)on_flow_component_properties_activate},
	{"flow_edition_refresh", GTK_STOCK_REFRESH, NULL, NULL, _("Refresh available components list"),
		(GCallback)on_flow_component_refresh_activate},
	/* Job control */
	{"job_control_save", GTK_STOCK_SAVE, NULL, NULL, _("Save job information in a file"),
		(GCallback)on_job_control_save},
	{"job_control_cancel", GTK_STOCK_MEDIA_STOP, NULL, NULL, _("Ask server to terminate the job"),
		(GCallback)on_job_control_cancel},
	{"job_control_close", "edit-clear", NULL, NULL, _("Clear current job log"),
		(GCallback)on_job_control_close},
	{"job_control_clear", GTK_STOCK_CLEAR, NULL, NULL, _("Clear all unactive job logs"),
		(GCallback)on_job_control_clear},
	{"job_control_stop", GTK_STOCK_STOP, NULL, NULL, _("Ask server to kill the job"),
		(GCallback)on_job_control_stop}
};

static const GtkRadioActionEntry status_radio_actions_entries [] = {
	{"flow_edition_status_configured", NULL, _("Configured"), NULL, NULL, 0},
	{"flow_edition_status_disabled", NULL, _("Disabled"), NULL, NULL, 1},
	{"flow_edition_status_unconfigured", NULL, _("Unconfigured"), NULL, NULL, 2}
};

/*
 * Prototypes functions
 */

static GtkWidget *
assembly_config_menu(void);

static GtkWidget *
assembly_help_menu(void);

/*
 * Public functions
 */

/*
 * Function: gebr_setup_ui
 * Assembly the whole interface.
 *
 */
void
gebr_setup_ui(void)
{
	GtkWidget *	main_vbox;
	GtkWidget *	menu_bar;
	GtkWidget *	navigation_hbox;

	GtkWidget *	vbox;
	GtkWidget *	toolbar;

	gebr.about = about_setup_ui("GêBR", _("A plug-and-play environment to\nseismic processing tools"));
	gebr.ui_server_list = server_list_setup_ui();

	/* Create the main window */
	gtk_window_set_default_icon (pixmaps_gebr_icon_16x16());
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(gebr.window, 700, 400);
	gtk_widget_show(gebr.window);

	gebr.ui_manager = gtk_ui_manager_new();
	gebr.action_group = gtk_action_group_new("General");
	gtk_action_group_add_actions(gebr.action_group, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gtk_action_group_add_radio_actions(gebr.action_group, status_radio_actions_entries, 3, -1,
		(GCallback)on_flow_component_status_activate, NULL);
	gtk_ui_manager_insert_action_group(gebr.ui_manager, gebr.action_group, 0);
	gebr.accel_group = gtk_ui_manager_get_accel_group(gebr.ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event",
		GTK_SIGNAL_FUNC(gebr_quit), NULL);

	/* Create the main vbox to hold menu, notebook and status bar */
	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gebr.window), main_vbox);
	gtk_widget_show(main_vbox);

	/*
	 * Create the main menu
	 */
	menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 0);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), assembly_config_menu());
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), assembly_help_menu());

	gtk_widget_show_all(menu_bar);

	/*
	 * Create navigation bar
	 */
	navigation_hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_vbox), navigation_hbox, FALSE, FALSE, 4);
	gebr.navigation_box_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(navigation_hbox), gebr.navigation_box_label, FALSE, FALSE, 0);

	gtk_widget_show_all(navigation_hbox);

	/*
	 * Notebook
	 */
	gebr.notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), gebr.notebook, TRUE, TRUE, 0);
	gtk_widget_show(gebr.notebook);

	/*
	 * Notebook's page "Projects and Lines"
	 */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbar), TRUE);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_new_project"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_new_line"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_line_paths"))), -1);

	gebr.ui_project_line = project_line_setup_ui();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_project_line->widget, TRUE, TRUE, 0);	
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Projects and Lines")));
	gtk_widget_show_all(vbox);

	/*
	 * Notebook's page "Flows"
	 */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_io"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_execute"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_export"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_export_as_menu"))), -1);

	gebr.ui_flow_browse = flow_browse_setup_ui();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_browse->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flows")));
	gtk_widget_show_all(vbox);

	/*
	 * Notebook's page "Flow edition"
	 */
	GtkToolItem *	tool_item;
	GtkWidget *	menu;

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_execute"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_duplicate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_refresh"))), -1);

	menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_status_configured")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_status_disabled")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_status_unconfigured")));

	gebr.ui_flow_edition = flow_edition_setup_ui();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_edition->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flow edition")));
	gtk_widget_show_all(vbox);
	
	/*
	 * Notebook's page "Job control"
	 */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "job_control_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "job_control_cancel"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "job_control_close"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "job_control_clear"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "job_control_stop"))), -1);

	gebr.ui_job_control = job_control_setup_ui();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_job_control->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Job control")));
	gtk_widget_show_all(vbox);

	/*
	 * Log status bar
	 */
	gebr.ui_log = log_setup_ui();
	gtk_widget_show_all(gebr.ui_log->widget);
	gtk_box_pack_end(GTK_BOX(main_vbox), gebr.ui_log->widget, FALSE, FALSE, 0);
}

/*
 * Function: assembly_config_menu
 * Assembly the config menu
 *
 */
static GtkWidget *
assembly_config_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Actions menu"));

	/* Preferences entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
		G_CALLBACK(on_configure_preferences_activate), NULL);

	/* Server entry */
	child_menu_item =  gtk_image_menu_item_new_with_mnemonic(_("_Servers"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(child_menu_item),
		gtk_image_new_from_stock(GTK_STOCK_NETWORK, 1));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
		GTK_SIGNAL_FUNC(on_configure_servers_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Actions"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	/* Quit entry */
	gtk_container_add(GTK_CONTAINER(menu),gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(gebr.action_group, "main_quit")));

	return menu_item;
}

/*
 * Function: assembly_help_menu
 * Assembly the help menu
 *
 */
static GtkWidget *
assembly_help_menu(void)
{
	GtkWidget *	menu_item;
	GtkWidget *	menu;
	GtkWidget *	child_menu_item;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), _("Help menu"));

	/* About entry */
	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), child_menu_item);
	g_signal_connect(GTK_OBJECT(child_menu_item), "activate",
			GTK_SIGNAL_FUNC(on_help_about_activate), NULL);

	menu_item = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));

	return menu_item;
}
