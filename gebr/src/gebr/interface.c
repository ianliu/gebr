/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include <libgebrintl.h>
#include <gui/pixmaps.h>
#include <gui/utils.h>

#include "interface.h"
#include "gebr.h"
#include "callbacks.h"

/*
 * Private data
 */

static const GtkActionEntry actions_entries [] = {
	{"quit",  GTK_STOCK_QUIT, NULL, "<Control>q", NULL, (GCallback)on_quit_activate},
	/* Project/Line */
	{"project_line_new_project", "folder-new", N_("New Project"), NULL, N_("Create a new project"),
		(GCallback)on_project_line_new_project_activate},
	{"project_line_new_line", "tab-new-background", N_("New Line"), NULL,  N_("Create a new line"),
		(GCallback)on_project_line_new_line_activate},
	{"project_line_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete selected project or line"),
		(GCallback)on_project_line_delete_activate},
	{"project_line_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit properties"),
		(GCallback)on_project_line_properties_activate},
	{"project_line_line_paths", GTK_STOCK_DIRECTORY, N_("Line paths"), NULL, N_("Edit custom line paths"),
		(GCallback)on_project_line_paths_activate},
	{"project_line_import", "document-import", N_("Import"), NULL, N_("Import project or line"),
		(GCallback)on_project_line_import_activate},
	{"project_line_export", "document-export", N_("Export"), NULL, N_("Export selected project or line"),
		(GCallback)on_project_line_export_activate},
	/* Flow */
	{"flow_new", GTK_STOCK_NEW, NULL, "<Control>n", N_("Create a new flow"), (GCallback)on_flow_new_activate},
	{"flow_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete selected flow"), (GCallback)on_flow_delete_activate},
	{"flow_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit flow properties"),
		(GCallback)on_flow_properties_activate},
	{"flow_change_revision", "document-open-recent", N_("Saved status"), NULL, NULL, NULL},
	{"flow_import", "document-import", N_("Import"), NULL, N_("Import a flow"), (GCallback)on_flow_import_activate},
	{"flow_export", "document-export", N_("Export"), NULL, N_("Export the flow"),
		(GCallback)on_flow_export_activate},
	{"flow_export_as_menu", GTK_STOCK_CONVERT, N_("Export as menu"), NULL, N_("Export the flow as a menu"),
		(GCallback)on_flow_export_as_menu_activate},
	{"flow_io", "system-switch-user", N_("Input and Output"), NULL, N_("Edit input/output flow files"),
		(GCallback)on_flow_io_activate},
	{"flow_execute", GTK_STOCK_EXECUTE, NULL, "<Control>r", N_("Run current flow"),
		(GCallback)on_flow_execute_activate},
	/* Flow Edition */
	{"flow_edition_help", GTK_STOCK_HELP, NULL, NULL, N_("Show component's help"),
		(GCallback)on_flow_component_help_activate},
	{"flow_edition_duplicate", GTK_STOCK_COPY, N_("Duplicate"), NULL, N_("Duplicate component"),
		(GCallback)on_flow_component_duplicate_activate},
	{"flow_edition_delete", GTK_STOCK_DELETE, NULL, NULL, N_("Delete component"),
		(GCallback)on_flow_component_delete_activate},
	{"flow_edition_properties", GTK_STOCK_PROPERTIES, NULL, NULL, N_("Edit component parameters"),
		(GCallback)on_flow_component_properties_activate},
	{"flow_edition_refresh", GTK_STOCK_REFRESH, NULL, NULL, N_("Refresh available components list"),
		(GCallback)on_flow_component_refresh_activate},
	/* Job control */
	{"job_control_save", GTK_STOCK_SAVE, NULL, NULL, N_("Save job information in a file"),
		(GCallback)on_job_control_save},
	{"job_control_cancel", GTK_STOCK_MEDIA_STOP, NULL, NULL, N_("Ask server to terminate the job"),
		(GCallback)on_job_control_cancel},
	{"job_control_close", "edit-clear", NULL, NULL, N_("Clear current job log"),
		(GCallback)on_job_control_close},
	{"job_control_clear", GTK_STOCK_CLEAR, NULL, NULL, N_("Clear all unactive job logs"),
		(GCallback)on_job_control_clear},
	{"job_control_stop", GTK_STOCK_STOP, NULL, NULL, N_("Ask server to kill the job"),
		(GCallback)on_job_control_stop}
};

static const GtkRadioActionEntry status_radio_actions_entries [] = {
	{"flow_edition_status_configured", NULL, N_("Configured"), NULL, NULL, 0},
	{"flow_edition_status_disabled", NULL, N_("Disabled"), NULL, NULL, 1},
	{"flow_edition_status_unconfigured", NULL, N_("Unconfigured"), NULL, NULL, 2}
};

static const GtkActionEntry common_actions_entries [] = {
	{"copy", GTK_STOCK_COPY, "copy", NULL, "copy", (GCallback)on_copy_activate},
	{"paste", GTK_STOCK_PASTE, "paste", NULL, "paste", (GCallback)on_paste_activate},
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
	GtkWidget *		main_vbox;
	GtkWidget *		menu_bar;
	GtkWidget *		navigation_hbox;

	GtkWidget *		vbox;
	GtkWidget *		toolbar;
	GtkToolItem *		tool_item;
	GtkWidget *		menu;
	GtkWidget *		revisions_menu;

	GtkActionGroup *	common_action_group;

	gebr.about = about_setup_ui("GêBR", _("A plug-and-play environment to\nseismic processing tools"));
	gebr.ui_server_list = server_list_setup_ui();

	/* Create the main window */
	gtk_window_set_default_icon(pixmaps_gebr_icon_16x16());
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(gebr.window, 700, 400);
	gtk_widget_show(gebr.window);

	gebr.action_group = gtk_action_group_new("General");
	gtk_action_group_set_translation_domain(gebr.action_group, PACKAGE);
	gtk_action_group_add_actions(gebr.action_group, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gtk_action_group_add_radio_actions(gebr.action_group, status_radio_actions_entries, 3, -1,
		(GCallback)on_flow_component_status_activate, NULL);
	gebr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group);
	libgebr_gtk_action_group_set_accel_group(gebr.action_group, gebr.accel_group);

	common_action_group = gtk_action_group_new("Common");
	gtk_action_group_add_actions(common_action_group, common_actions_entries,
		G_N_ELEMENTS(common_actions_entries), NULL);
	libgebr_gtk_action_group_set_accel_group(common_action_group, gebr.accel_group);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "project_line_export"))), -1);

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

	revisions_menu = menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock("document-open-recent");
	g_signal_connect(GTK_OBJECT(tool_item), "clicked",
		G_CALLBACK(on_flow_revision_save_activate), NULL);
	set_tooltip(GTK_WIDGET(tool_item), _("Save flow state"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_export"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_export_as_menu"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_io"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_execute"))), -1);

	gebr.ui_flow_browse = flow_browse_setup_ui(revisions_menu);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_browse->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flows")));
	gtk_widget_show_all(vbox);

	/*
	 * Notebook's page "Flow edition"
	 */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_duplicate"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_edition_properties"))), -1);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(gebr.action_group, "flow_execute"))), -1);

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
		gtk_action_group_get_action(gebr.action_group, "quit")));

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
