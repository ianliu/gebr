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

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-pixmaps.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_flow_edition.h"
#include "interface.h"
#include "gebr.h"
#include "defines.h"
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
		"<Control>h", NULL, G_CALLBACK(on_help_contents_activate)},
	{"help_about", GTK_STOCK_ABOUT, NULL,
		NULL, NULL, G_CALLBACK(on_help_about_activate)}
};

static const GtkActionEntry actions_entries_project_line[] = {
	/*
	 * Project/Line
	 */
	{"project_line_new_project", "folder-new", N_("New Project"),
		"<Control>J", N_("New Project"), G_CALLBACK(on_new_project_activate)},
	{"project_line_new_line", "tab-new-background", N_("New Line"),
		"<Control>L", N_("New Line"), G_CALLBACK(on_new_line_activate)},
	{"project_line_delete", GTK_STOCK_DELETE, NULL,
		"Delete", N_("Delete selected Projects and Lines"), G_CALLBACK(on_project_line_delete_activate)},
	{"project_line_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit properties"), G_CALLBACK(on_document_properties_activate)},
	{"project_line_dict_edit", "accessories-dictionary", N_("Parameter dictionary"),
		NULL, N_("Edit parameter dictionary"), G_CALLBACK(on_document_dict_edit_activate)},
	{"project_line_import", "document-import", N_("Import"),
		NULL, N_("Import Projects or Lines"), G_CALLBACK(on_project_line_import_activate)},
	{"project_line_export", "document-export", N_("Export"),
		NULL, N_("Export selected Projects and Lines"), G_CALLBACK(on_project_line_export_activate)},
	{"project_line_view", GTK_STOCK_INFO, N_("View Report"),
		NULL, N_("View report"), G_CALLBACK(on_project_line_show_help)},
	{"project_line_edit", GTK_STOCK_EDIT, N_("Edit Comments"),
		NULL, N_("Edit comments"), G_CALLBACK(on_project_line_edit_help)},
};

static const GtkActionEntry actions_entries_flow[] = {
	/*
	 * Flow
	 */
	{"flow_new", GTK_STOCK_NEW, NULL,
		"<Control>F", N_("New Flow"), G_CALLBACK(on_new_activate)},
	{"flow_delete", GTK_STOCK_DELETE, NULL, "Delete",
		N_("Delete selected Flows"), G_CALLBACK(on_flow_delete_activate)},
	{"flow_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit properties"), G_CALLBACK(on_document_properties_activate)},
	{"flow_dict_edit", "accessories-dictionary", N_("Parameter dictionary"),
		NULL, N_("Edit parameter dictionary"), G_CALLBACK(on_document_dict_edit_activate)},
	{"flow_change_revision", "document-open-recent", N_("Saved status"),
		NULL, NULL, NULL},
	{"flow_import", "document-import", N_("Import"), NULL,
		N_("Import Flows"), G_CALLBACK(on_flow_import_activate)},
	{"flow_export", "document-export", N_("Export"),
		NULL, N_("Export selected Flows"), G_CALLBACK(on_flow_export_activate)},
	{"flow_execute", GTK_STOCK_EXECUTE, NULL,
		"<Control>R", N_("Execute selected Flows in sequence"), G_CALLBACK(on_flow_execute_activate)},
	{"flow_execute_in_parallel", GTK_STOCK_EXECUTE, NULL,
		"<Control><Shift>R", N_("Execute selected Flows simultaneously"), G_CALLBACK(on_flow_execute_in_parallel_activate)},
	{"flow_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy selected Flows to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste Flows from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_view", GTK_STOCK_INFO, N_("View Report"),
		NULL, N_("View Flow report"), G_CALLBACK(on_flow_browse_show_help)},
	{"flow_edit", GTK_STOCK_EDIT, N_("Edit Comments"),
		NULL, N_("Edit Flow comments"), G_CALLBACK(on_flow_browse_edit_help)},
};

static const GtkActionEntry actions_entries_flow_edition[] = {
	/*
	 * Flow Edition
	 */
	{"flow_edition_help", GTK_STOCK_HELP, NULL,
		"<Control><Shift>h", N_("Show Program's help"), G_CALLBACK(on_flow_component_help_activate)},
	{"flow_edition_delete", GTK_STOCK_DELETE, NULL,
		"Delete", N_("Delete Program"), G_CALLBACK(on_flow_component_delete_activate)},
	{"flow_edition_properties", GTK_STOCK_PROPERTIES, NULL,
		NULL, N_("Edit Program parameters"), G_CALLBACK(on_flow_component_properties_activate)},
	{"flow_edition_refresh", GTK_STOCK_REFRESH, NULL,
		NULL, N_("Refresh available Menu's list"), G_CALLBACK(on_flow_component_refresh_activate)},
	{"flow_edition_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy selected Programs to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_edition_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste Programs from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_edition_top", GTK_STOCK_GOTO_TOP, N_("Move to Top"),
		"Home", NULL, G_CALLBACK(on_flow_component_move_top)},
	{"flow_edition_bottom", GTK_STOCK_GOTO_BOTTOM, N_("Move to Bottom"),
		"End", NULL, G_CALLBACK(on_flow_component_move_bottom)},
	{"flow_edition_execute", GTK_STOCK_EXECUTE, NULL,
		"<Control>R", N_("Execute this Flow"), G_CALLBACK (on_flow_component_execute_single)}
};

static const GtkActionEntry actions_entries_job_control[] = {
	/*
	 * Job control - Job Actions
	 */
	{"job_control_save", GTK_STOCK_SAVE, NULL,
		NULL, N_("Save Job information in a file"), G_CALLBACK(on_job_control_save)},
	{"job_control_close", "gnome-fs-trash-empty", N_("Close"),
		"Delete", N_("Clear selected Jobs"), G_CALLBACK(on_job_control_close)},
	{"job_control_stop", GTK_STOCK_STOP, N_("Cancel"),
		NULL, N_("Ask server to cancel the selected Job"), G_CALLBACK(on_job_control_stop)},
	/*
	 * Job Control - Queue Actions
	 */
	{"job_control_queue_save", GTK_STOCK_SAVE, N_("Save All"),
		NULL, N_("Ask server to save all Jobs from queue"), G_CALLBACK(on_job_control_queue_save)},
	{"job_control_queue_close", "gnome-fs-trash-empty", N_("Close All"),
		NULL, N_("Clear all Jobs from selected queue"), G_CALLBACK(on_job_control_queue_close)},
	{"job_control_queue_stop", GTK_STOCK_STOP, N_("Cancel All"),
		NULL, N_("Ask server to cancel all Jobs from selected queue"), G_CALLBACK(on_job_control_queue_stop)}

};

static const GtkActionEntry status_action_entries[] = {
	{"flow_edition_status_configured", NULL, N_("Configured"),
		NULL, N_("Change selected Programs status to configured"), NULL},
	{"flow_edition_status_disabled", NULL, N_("Disabled"),
		NULL, N_("Change selected Programs status to disabled"), NULL},
	{"flow_edition_status_unconfigured", NULL, N_("Not configured"),
		NULL, N_("Change selected Programs status to not configured"), NULL}
};

static const GtkActionEntry actions_entries_server[] = {
	/*
	 *  Server Configurations
	 */
	{"server_connect", GTK_STOCK_CONNECT, N_("Connect"),
		NULL, N_("Connect to the server"), G_CALLBACK(on_server_common_connect)},
	{"server_disconnect", GTK_STOCK_DISCONNECT, N_("Disconnect"),
		NULL, N_("Disconnect from server"), G_CALLBACK(on_server_common_disconnect)},
	{"server_autoconnect", NULL, N_("Auto connect"),
		NULL, N_("Connect the server at startup"), G_CALLBACK(on_server_common_autoconnect_changed)},
	{"server_remove", GTK_STOCK_REMOVE, N_("Remove"),
		"Delete", N_("Remove server from list"), G_CALLBACK(on_server_common_remove)},
	{"server_stop", GTK_STOCK_STOP, N_("Stop server"),
		NULL, N_("Stop server from running"), G_CALLBACK(on_server_common_stop)},
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

	gebr.action_group_general = gtk_action_group_new("General");
	gtk_action_group_set_translation_domain(gebr.action_group_general, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_general, actions_entries, G_N_ELEMENTS(actions_entries), NULL);
	gebr.accel_group_array[ACCEL_GENERAL] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_general, gebr.accel_group_array[ACCEL_GENERAL]);
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[ACCEL_GENERAL]);

	gebr.action_group_project_line = gtk_action_group_new("Project and Line");
	gtk_action_group_set_translation_domain(gebr.action_group_project_line, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_project_line, actions_entries_project_line, G_N_ELEMENTS(actions_entries_project_line), NULL);
	gebr.accel_group_array[ACCEL_PROJECT_AND_LINE] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_project_line, gebr.accel_group_array[ACCEL_PROJECT_AND_LINE]);

	gebr.action_group_flow = gtk_action_group_new("Flow");
	gtk_action_group_set_translation_domain(gebr.action_group_flow, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_flow, actions_entries_flow, G_N_ELEMENTS(actions_entries_flow), NULL);
	gebr.accel_group_array[ACCEL_FLOW] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_flow, gebr.accel_group_array[ACCEL_FLOW]);

	gebr.action_group_flow_edition = gtk_action_group_new("Flow Edition");
	gtk_action_group_set_translation_domain(gebr.action_group_flow_edition, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_flow_edition, actions_entries_flow_edition, G_N_ELEMENTS(actions_entries_flow_edition), NULL);
	gebr.accel_group_array[ACCEL_FLOW_EDITION] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_flow_edition, gebr.accel_group_array[ACCEL_FLOW_EDITION]);

	gebr.action_group_job_control = gtk_action_group_new("Job Control");
	gtk_action_group_set_translation_domain(gebr.action_group_job_control, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_job_control, actions_entries_job_control, G_N_ELEMENTS(actions_entries_job_control), NULL);
	gebr.accel_group_array[ACCEL_JOB_CONTROL] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_job_control, gebr.accel_group_array[ACCEL_JOB_CONTROL]);

	gebr.action_group_status = gtk_action_group_new("Status");
	gtk_action_group_set_translation_domain(gebr.action_group_status, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_status, status_action_entries, G_N_ELEMENTS(status_action_entries), NULL);
	gebr.accel_group_array[ACCEL_STATUS] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_status, gebr.accel_group_array[ACCEL_STATUS]);

	gebr.action_group_server = gtk_action_group_new("Server Configuration");
	gtk_action_group_set_translation_domain(gebr.action_group_server, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(gebr.action_group_server, actions_entries_server, G_N_ELEMENTS(actions_entries_server), NULL);
	gebr.accel_group_array[ACCEL_SERVER] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_server, gebr.accel_group_array[ACCEL_SERVER]);

	/* Signals */
	g_signal_connect(GTK_OBJECT(gebr.window), "delete_event", G_CALLBACK(on_quit_activate), NULL);

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
	gebr.last_notebook = -1;
	g_signal_connect(GTK_OBJECT(gebr.notebook), "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
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
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_new_project"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_new_line"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_properties"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_dict_edit"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_project_line, "project_line_export"))), -1);

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
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_new"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_properties"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new (), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_execute"))), -1);

	menu = gtk_menu_new();
	tool_item = gtk_menu_tool_button_new_from_stock("document-open-recent");
	g_signal_connect(GTK_OBJECT(tool_item), "clicked", G_CALLBACK(on_flow_revision_save_activate), NULL);
	g_signal_connect(GTK_OBJECT(tool_item), "show-menu", G_CALLBACK(on_flow_revision_show_menu), NULL);
	gebr_gui_gtk_widget_set_tooltip(GTK_WIDGET(tool_item), _("Save or Restore a version of the selected Flows"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item), menu);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_export"))), -1);

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
					 (gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_copy"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_paste"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_delete"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_properties"))),
			   -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_dict_edit"))), -1);

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_disabled");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_DISABLED));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_unconfigured");
	g_signal_connect(action, "activate", G_CALLBACK(on_flow_component_status_activate), GUINT_TO_POINTER(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_execute"))), -1);

	gebr.ui_flow_edition = flow_edition_setup_ui();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_edition->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flow Editor")));
	gtk_widget_show_all(vbox);

	/*
	 * Notebook's page "Job control"
	 */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"))), -1);

	gebr.ui_job_control = job_control_setup_ui();

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_job_control->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Job Control")));
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
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_general, "actions_preferences")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_general, "actions_servers")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_general, "actions_quit")));

	menu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_general, "help_contents")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_general, "help_about")));
}
