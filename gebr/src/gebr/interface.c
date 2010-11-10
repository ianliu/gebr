/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/gui/gebr-gui-pixmaps.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_flow_edition.h"
#include "interface.h"
#include "gebr.h"
#include "flow.h"
#include "callbacks.h"

/**
 * @file interface.c Assembly the main components of the interface.
 */

/*
 * Private data
 */

/**
 * \internal
 * The various actions for GeBR interface.
 *
 * Items values are (action name, stock id, label, hotkey, tooltip, callback).
 */
static const GtkActionEntry actions_entries[] = {
	{"actions_preferences", GTK_STOCK_PREFERENCES, NULL,
		NULL, NULL, G_CALLBACK(on_configure_preferences_activate)},
	{"actions_servers", GTK_STOCK_NETWORK, N_("_Servers"),
		NULL, NULL, G_CALLBACK(on_configure_servers_activate)},
	{"actions_quit", GTK_STOCK_QUIT, NULL,
		"<Control>q", NULL, G_CALLBACK(on_quit_activate)},
	{"help_contents", GTK_STOCK_HELP, NULL,
		NULL, NULL, G_CALLBACK(on_help_contents_activate)},
	{"help_about", GTK_STOCK_ABOUT, NULL,
		NULL, NULL, G_CALLBACK(on_help_about_activate)},

	/*
	 * Project/Line
	 */
	{"project_line_new_project", "folder-new", N_("New Project"),
		"<Control>J", N_("Create a new project"), G_CALLBACK(on_new_project_activate)},
	{"project_line_new_line", "tab-new-background", N_("New Line"),
		"<Control>L", N_("Create a new line"), G_CALLBACK(on_new_line_activate)},
	{"project_line_delete", GTK_STOCK_DELETE, NULL,
		NULL, N_("Delete selected project or line"), G_CALLBACK(on_project_line_delete_activate)},
	{"project_line_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit project or line properties"), G_CALLBACK(on_document_properties_activate)},
	{"project_line_dict_edit", "accessories-dictionary", N_("Parameter dictionary"),
		NULL, N_("Edit parameter dictionary of current project or line"), G_CALLBACK(on_document_dict_edit_activate)},
	{"project_line_import", "document-import", N_("Import"),
		NULL, N_("Import project or line"), G_CALLBACK(on_project_line_import_activate)},
	{"project_line_export", "document-export", N_("Export"),
		NULL, N_("Export selected project or line"), G_CALLBACK(on_project_line_export_activate)},
	{"project_line_view", GTK_STOCK_INFO, N_("View Report"),
		NULL, N_("View the report related to project/line"), G_CALLBACK(on_project_line_show_help)},
	{"project_line_edit", GTK_STOCK_EDIT, N_("Edit Report"),
		NULL, N_("Edit the report related to project/line"), G_CALLBACK(on_project_line_edit_help)},
	{"project_line_dump", GTK_STOCK_PRINT_REPORT, N_("Detailed line report"),
		NULL, N_("View detailed line report"), G_CALLBACK(on_line_detailed_report_activate)},

	/*
	 * Flow
	 */
	{"flow_new", GTK_STOCK_NEW, NULL,
		"<Control>F", N_("Create a new flow"), G_CALLBACK(on_new_activate)},
	{"flow_delete", GTK_STOCK_DELETE, NULL, NULL,
		N_("Delete selected flow"), G_CALLBACK(on_flow_delete_activate)},
	{"flow_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit flow properties"), G_CALLBACK(on_document_properties_activate)},
	{"flow_dict_edit", "accessories-dictionary", N_("Parameter dictionary"),
		NULL, N_("Edit parameter dictionary of current flow"), G_CALLBACK(on_document_dict_edit_activate)},
	{"flow_change_revision", "document-open-recent", N_("Saved status"),
		NULL, NULL, NULL},
	{"flow_import", "document-import", N_("Import"), NULL,
		N_("Import a flow"), G_CALLBACK(on_flow_import_activate)},
	{"flow_export", "document-export", N_("Export"),
		NULL, N_("Export the flow"), G_CALLBACK(on_flow_export_activate)},
	{"flow_execute", GTK_STOCK_EXECUTE, NULL,
		"<Control>R", N_("Run current flow"), G_CALLBACK(on_flow_execute_activate)},
	{"flow_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy selected flow(s) to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste flow(s) from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_view", GTK_STOCK_INFO, N_("View Report"),
		NULL, N_("View the report related to flow"), G_CALLBACK(on_flow_browse_show_help)},
	{"flow_edit", GTK_STOCK_EDIT, N_("Edit Report"),
		NULL, N_("Edit the report related to flow"), G_CALLBACK(on_flow_browse_edit_help)},
	{"flow_dump", GTK_STOCK_PRINT_REPORT, N_("Detailed flow report"),
		NULL, N_("View detailed flow report"), G_CALLBACK(on_flow_detailed_report_activate)},

	/*
	 * Flow Edition
	 */
	{"flow_edition_help", GTK_STOCK_HELP, NULL,
		NULL, N_("Show component's help"), G_CALLBACK(on_flow_component_help_activate)},
	{"flow_edition_delete", GTK_STOCK_DELETE, NULL,
		"Delete", N_("Delete program"), G_CALLBACK(on_flow_component_delete_activate)},
	{"flow_edition_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit program parameters"), G_CALLBACK(on_flow_component_properties_activate)},
	{"flow_edition_refresh", GTK_STOCK_REFRESH, NULL,
		NULL, N_("Refresh available components list"), G_CALLBACK(on_flow_component_refresh_activate)},
	{"flow_edition_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy selected programs(s) to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_edition_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste program(s) from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_edition_top", GTK_STOCK_GOTO_TOP, N_("Move to Top"),
		"Home", NULL, G_CALLBACK(on_flow_component_move_top)},
	{"flow_edition_bottom", GTK_STOCK_GOTO_BOTTOM, N_("Move to Bottom"),
		"End", NULL, G_CALLBACK(on_flow_component_move_bottom)},

	/*
	 * Job control
	 */
	{"job_control_save", GTK_STOCK_SAVE, NULL,
		NULL, N_("Save job information in a file"), G_CALLBACK(on_job_control_save)},
	{"job_control_cancel", GTK_STOCK_MEDIA_STOP, NULL,
		NULL, N_("Ask server to terminate the job"), G_CALLBACK(on_job_control_cancel)},
	{"job_control_close", "edit-clear", N_("Clear current"),
		NULL, N_("Clear current job log"), G_CALLBACK(on_job_control_close)},
	{"job_control_clear", GTK_STOCK_CLEAR, NULL,
		NULL, N_("Clear all inactive job logs"), G_CALLBACK(on_job_control_clear)},
	{"job_control_stop", GTK_STOCK_STOP, NULL,
		NULL, N_("Ask server to kill the job"), G_CALLBACK(on_job_control_stop)}
};

static const GtkActionEntry status_action_entries[] = {
	{"flow_edition_status_configured", NULL, N_("Configured"),
		NULL, N_("Change current program status to configured"), NULL},
	{"flow_edition_status_disabled", NULL, N_("Disabled"),
		NULL, N_("Change current program status to disabled"), NULL},
	{"flow_edition_status_unconfigured", NULL, N_("Not configured"),
		NULL, N_("Change current program status to not configured"), NULL}
};

/*
 * Prototypes functions
 */

static void assembly_menus(GtkMenuBar * menu_bar);

/*
 * Public functions
 */

/**
 * Assembly the whole interface.
 */
void gebr_setup_ui(void)
{
	GtkWidget *main_vbox;
	GtkWidget *menu_bar;
	GtkWidget *navigation_hbox;

	GtkWidget *vbox;
	GtkWidget *toolbar;
	GtkToolItem *tool_item;
	GtkWidget *menu;
	GtkAction *action;

	gebr.about = gebr_gui_about_setup_ui("GêBR", _("A plug-and-play environment for\nseismic processing tools"));
	gebr.ui_server_list = server_list_setup_ui();

	/* Create the main window */
	gtk_window_set_default_icon(gebr_gui_pixmaps_gebr_icon_16x16());
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(gebr.window, 700, 400);
	gtk_widget_show(gebr.window);

	gebr.action_group = gtk_action_group_new("General");
	gtk_action_group_set_translation_domain(gebr.action_group, PACKAGE);
	gtk_action_group_add_actions(gebr.action_group, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gtk_action_group_add_actions(gebr.action_group, status_action_entries, G_N_ELEMENTS(status_action_entries), NULL);
	gebr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group);
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group, gebr.accel_group);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event", G_CALLBACK(gebr_quit), NULL);

	/* Create the main vbox to hold menu, notebook and status bar */
	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gebr.window), main_vbox);
	gtk_widget_show(main_vbox);

	/*
	 * Create the main menu
	 */
	menu_bar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 0);
	assembly_menus(GTK_MENU_BAR(menu_bar));
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
#if !GTK_CHECK_VERSION(2,14,0)
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbar), TRUE);
#endif

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_new_project"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_new_line"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_properties"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_dict_edit"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_dump"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "project_line_export"))), -1);

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
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_properties"))), -1);

	menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock("document-open-recent");
	g_signal_connect(GTK_OBJECT(tool_item), "clicked", G_CALLBACK(on_flow_revision_save_activate), NULL);
	g_signal_connect(GTK_OBJECT(tool_item), "show-menu", G_CALLBACK(on_flow_revision_show_menu), NULL);
	gebr_gui_gtk_widget_set_tooltip(GTK_WIDGET(tool_item), _("Save flow state"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_dump"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_export"))), -1);

	gebr.ui_flow_browse = flow_browse_setup_ui(menu);
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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_edition_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_edition_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_edition_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_edition_properties"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_dict_edit"))), -1);

	action = gtk_action_group_get_action(gebr.action_group, "flow_edition_status_configured");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED));

	action = gtk_action_group_get_action(gebr.action_group, "flow_edition_status_disabled");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_DISABLED));

	action = gtk_action_group_get_action(gebr.action_group, "flow_edition_status_unconfigured");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "flow_execute"))), -1);

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
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "job_control_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "job_control_cancel"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "job_control_close"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "job_control_clear"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group, "job_control_stop"))), -1);

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
 * Function: assembly_menus
 * Assembly menus
 *
 */
static void assembly_menus(GtkMenuBar * menu_bar)
{
	GtkWidget *menu_item;
	GtkWidget *menu;

	menu_item = gtk_menu_item_new_with_mnemonic(_("_Actions"));
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "actions_preferences")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "actions_servers")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "actions_quit")));

	menu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "help_contents")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "help_about")));
}
