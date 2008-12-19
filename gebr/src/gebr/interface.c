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

	gebr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event",
		GTK_SIGNAL_FUNC(gebr_quit), NULL);

	/* Create the main vbox to hold menu, notebook and status bar */
	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gebr.window), main_vbox);
	gtk_widget_show(main_vbox);

	/*
	 * Actions
	 */
	/* Main */
	gebr.actions.main.quit = gtk_action_new("main_quit",
		NULL, NULL, GTK_STOCK_QUIT);
	g_signal_connect(gebr.actions.main.quit, "activate",
		(GCallback)(gebr_quit), NULL);

	/* Project/Line */
	gebr.actions.project_line.new_project = gtk_action_new("project_line_new_project",
		_("New Project"), _("Create a new project"), "folder-new");
	g_signal_connect(gebr.actions.project_line.new_project, "activate",
		(GCallback)on_project_line_new_project_activate, NULL);
	gebr.actions.project_line.new_line = gtk_action_new("project_line_new_line",
		_("New Line"), _("Create a new line"), "tab-new-background");
	g_signal_connect(gebr.actions.project_line.new_line, "activate",
		(GCallback)on_project_line_new_line_activate, NULL);
	gebr.actions.project_line.delete = gtk_action_new("project_line_delete",
		NULL, _("Delete selected project or line"), GTK_STOCK_DELETE);
	g_signal_connect(gebr.actions.project_line.delete, "activate",
		(GCallback)on_project_line_delete_activate, NULL);
	gebr.actions.project_line.properties = gtk_action_new("project_line_properties",
		NULL, _("Edit properties"), GTK_STOCK_PROPERTIES);
	g_signal_connect(gebr.actions.project_line.properties, "activate",
		(GCallback)on_project_line_properties_activate, NULL);
	gebr.actions.project_line.line_paths = gtk_action_new("project_line_line_paths",
		_("Line paths"), _("Edit custom line paths"), GTK_STOCK_DIRECTORY);
	g_signal_connect(gebr.actions.project_line.line_paths, "activate",
		(GCallback)on_project_line_paths_activate, NULL);
	 
	/* Flow */
	gebr.actions.flow.new = gtk_action_new("flow_new",
		NULL, _("Create a new flow"), GTK_STOCK_NEW);
	g_signal_connect(gebr.actions.flow.new, "activate",
		(GCallback)on_flow_new_activate, NULL);
	gebr.actions.flow.delete = gtk_action_new("flow_delete",
		NULL, _("Delete selected flow"), GTK_STOCK_DELETE);
	g_signal_connect(gebr.actions.flow.delete, "activate",
		(GCallback)on_flow_delete_activate, NULL);
	gebr.actions.flow.properties = gtk_action_new("flow_properties",
		NULL, _("Edit flow properties"), GTK_STOCK_PROPERTIES);
	g_signal_connect(gebr.actions.flow.properties, "activate",
		(GCallback)on_flow_properties_activate, NULL);
	gebr.actions.flow.io = gtk_action_new("flow_io",
		_("Input and Output"), _("Edit input/output flow files"), "system-switch-user");
	g_signal_connect(gebr.actions.flow.io, "activate",
		(GCallback)on_flow_io_activate, NULL);
	gebr.actions.flow.execute = gtk_action_new("flow_execute",
		NULL, _("Run current flow"), GTK_STOCK_EXECUTE);
	g_signal_connect(gebr.actions.flow.execute, "activate",
		(GCallback)on_flow_execute_activate, NULL);
	gebr.actions.flow.import = gtk_action_new("flow_import",
		_("Import"), _("Import a flow"), "document-import");
	g_signal_connect(gebr.actions.flow.import, "activate",
		(GCallback)on_flow_import_activate, NULL);
	gebr.actions.flow.export = gtk_action_new("flow_export",
		_("Export"), _("Export the flow"), "document-export");
	g_signal_connect(gebr.actions.flow.export, "activate",
		(GCallback)on_flow_export_activate, NULL);
	gebr.actions.flow.export_as_menu = gtk_action_new("flow_export_as_menu",
		_("Export as menu"), _("Export the flow as a menu"), "document-export");
	g_signal_connect(gebr.actions.flow.export_as_menu, "activate",
		(GCallback)on_flow_export_as_menu_activate, NULL);

	/* Flow Edition */
	gebr.actions.flow_edition.help = gtk_action_new("flow_edition_help",
		NULL, _("Show component's help"), GTK_STOCK_HELP);
	g_signal_connect(gebr.actions.flow_edition.help, "activate",
		(GCallback)on_flow_component_help_activate, NULL);
	gebr.actions.flow_edition.delete = gtk_action_new("flow_edition_delete",
		NULL, _("Delete component"), GTK_STOCK_DELETE);
	g_signal_connect(gebr.actions.flow_edition.delete, "activate",
		(GCallback)on_flow_component_delete_activate, NULL);
	gebr.actions.flow_edition.properties = gtk_action_new("flow_edition_properties",
		NULL, _("Edit component parameters"), GTK_STOCK_PROPERTIES);
	g_signal_connect(gebr.actions.flow_edition.properties, "activate",
		(GCallback)on_flow_component_properties_activate, NULL);
	gebr.actions.flow_edition.refresh = gtk_action_new("flow_edition_refresh",
		NULL, _("Refresh available components list"), GTK_STOCK_REFRESH);
	g_signal_connect(gebr.actions.flow_edition.refresh, "activate",
		(GCallback)on_flow_component_refresh_activate, NULL);
	gebr.actions.flow_edition.configured = gtk_radio_action_new("configured", _("Configured"), NULL, NULL, 1<<0);
	gebr.actions.flow_edition.disabled = gtk_radio_action_new("disabled", _("Disabled"), NULL, NULL, 1<<1);
	gebr.actions.flow_edition.unconfigured = gtk_radio_action_new("unconfigured", _("Unconfigured"), NULL, NULL, 1<<2);
	gtk_radio_action_set_group(gebr.actions.flow_edition.disabled,
		gtk_radio_action_get_group(gebr.actions.flow_edition.configured));
	gtk_radio_action_set_group(gebr.actions.flow_edition.unconfigured,
		gtk_radio_action_get_group(gebr.actions.flow_edition.configured));
	g_signal_connect(gebr.actions.flow_edition.configured, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	g_signal_connect(gebr.actions.flow_edition.disabled, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);
	g_signal_connect(gebr.actions.flow_edition.unconfigured, "activate",
		GTK_SIGNAL_FUNC(on_flow_component_status_activate), NULL);

	/* Job control */
	gebr.actions.job_control.save = gtk_action_new("job_save",
		NULL, _("Save job information in a file"), GTK_STOCK_SAVE);
	g_signal_connect(gebr.actions.job_control.save, "activate",
		(GCallback)on_job_control_save, NULL);
	gebr.actions.job_control.cancel = gtk_action_new("job_cancel",
		NULL, _("Ask server to terminate the job"), GTK_STOCK_MEDIA_STOP);
	g_signal_connect(gebr.actions.job_control.cancel, "activate",
		(GCallback)on_job_control_cancel, NULL);
	gebr.actions.job_control.close = gtk_action_new("job_close",
		NULL, _("Clear current job log"), "edit-clear");
	g_signal_connect(gebr.actions.job_control.close, "activate",
		(GCallback)on_job_control_close, NULL);
	gebr.actions.job_control.clear = gtk_action_new("job_clear",
		NULL, _("Clear all unactive job logs"), GTK_STOCK_CLEAR);
	g_signal_connect(gebr.actions.job_control.clear, "activate",
		(GCallback)on_job_control_clear, NULL);
	gebr.actions.job_control.stop = gtk_action_new("job_stop",
		NULL, _("Ask server to kill the job"), GTK_STOCK_STOP);
	g_signal_connect(gebr.actions.job_control.stop, "activate",
		(GCallback)on_job_control_stop, NULL);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.project_line.new_project)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.project_line.new_line)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.project_line.delete)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.project_line.properties)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.project_line.line_paths)), -1);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.new)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.delete)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.properties)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.io)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.execute)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.import)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.export)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.export_as_menu)), -1);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow.execute)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow_edition.delete)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow_edition.properties)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.flow_edition.refresh)), -1);

	menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.configured)));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.disabled)));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.unconfigured)));
	

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.job_control.save)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.job_control.cancel)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.job_control.close)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.job_control.clear)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(gebr.actions.job_control.stop)), -1);

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

	/*
	 * Create some hot-keys
	 */
	/* CTRL+R - Run current flow */
	gtk_accel_group_connect(gebr.accel_group, GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
		gtk_action_get_accel_closure(gebr.actions.flow.execute));
	/* CTR+Q - Quit GeBR */
	gtk_accel_group_connect(gebr.accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
		gtk_action_get_accel_closure(gebr.actions.main.quit));
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
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebr.actions.main.quit));

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
