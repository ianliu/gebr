/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2012 GeBR core team (http://www.gebrproject.com/)
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gebr-gettext.h"

#include <math.h>
#include <string.h>

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-pixmaps.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>

#include "interface.h"
#include "gebr.h"
#include "flow.h"
#include "callbacks.h"
#include "menu.h"
#include "ui_flow_execution.h"

#define SLIDER_MAX 8.0
#define SLIDER_100 5.0
#define VALUE_MAX 20.0

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
	{"actions_connections", GTK_STOCK_ABOUT, N_("_Connection Assistant"),
		NULL, NULL, G_CALLBACK(on_configure_wizard_activate)},
	{"actions_servers", "group", N_("_Maestro / Nodes"),
		NULL, NULL, G_CALLBACK(on_configure_servers_activate)},
	{"actions_quit", GTK_STOCK_QUIT, NULL,
		"<Control>q", NULL, G_CALLBACK(on_quit_activate)},
	{"help_contents", GTK_STOCK_HELP, NULL,
		"<Control>h", NULL, G_CALLBACK(on_help_contents_activate)},
	{"help_about", GTK_STOCK_ABOUT, NULL,
		NULL, NULL, G_CALLBACK(on_help_about_activate)},
	{"help_demos_su", NULL, N_("Samples")},
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
	{"project_line_properties", "kontact_todo", N_("Properties"),
		NULL, N_("Edit properties"), G_CALLBACK(on_document_properties_activate)},
	{"project_line_dict_edit", "accessories-dictionary", N_("Variables dictionary"),
		NULL, N_("Edit variables dictionary"), G_CALLBACK(on_document_dict_edit_activate)},
	{"project_line_import", "document-import", N_("Import"),
		NULL, N_("Import Projects or Lines"), G_CALLBACK(on_project_line_import_activate)},
	{"project_line_export", "document-export", N_("Export"),
		NULL, N_("Export selected Projects and Lines"), G_CALLBACK(on_project_line_export_activate)},
	{"project_line_view", "document_review", N_("View Report"),
		NULL, N_("View Report"), G_CALLBACK(on_project_line_show_help)},
	{"project_line_edit", "document_comment", N_("Edit Comments"),
		NULL, N_("Edit Comments"), G_CALLBACK(on_project_line_edit_help)},
};

static const GtkActionEntry actions_entries_flow[] = {
	/*
	 * Flow
	 */
	{"flow_new", GTK_STOCK_NEW, N_("New Flow"),
		"<Control>F", N_("New"), G_CALLBACK(on_new_activate)},
	{"flow_new_program", GTK_STOCK_NEW, N_("New Program"),
		NULL, NULL, G_CALLBACK(on_new_program_activate)},
	{"flow_delete", GTK_STOCK_DELETE, NULL,
		NULL, N_("Delete"), G_CALLBACK(on_flow_delete_activate)},
	{"flow_properties", "kontact_todo", N_("Properties"),
		NULL, N_("Edit properties"), G_CALLBACK(on_document_properties_activate)},
	{"flow_dict_edit", "accessories-dictionary", N_("Variables dictionary"),
		NULL, N_("Edit variables dictionary"), G_CALLBACK(on_document_dict_edit_activate)},
	{"flow_change_revision", "document-open-recent", N_("Take a Snapshot"),
		"<Control>S", N_("Take a snapshot"), G_CALLBACK(on_flow_revision_save_activate)},
	{"flow_import", "document-import", N_("Import"), NULL,
		N_("Import Flows"), G_CALLBACK(on_flow_import_activate)},
	{"flow_export", "document-export", N_("Export"),
		NULL, N_("Export selected Flows"), G_CALLBACK(on_flow_export_activate)},
	{"flow_execute", GTK_STOCK_EXECUTE, NULL,
		"<Control>R", N_("Execute"), G_CALLBACK(on_flow_execute_activate)},
	{"flow_execute_details", "detailed_execution", NULL,
		NULL, N_("Execution details"), G_CALLBACK(on_flow_execute_details_activate)},
	{"flow_execute_parallel", NULL, NULL,
		"<Control><Shift>R", NULL, G_CALLBACK(on_flow_execute_parallel_activate)},
	{"flow_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_view", "document_review", N_("View Report"),
		NULL, N_("View report"), G_CALLBACK(on_flow_browse_show_help)},
	{"flow_edit", "document_comment", N_("Edit Comments"),
		NULL, N_("Edit comments"), G_CALLBACK(on_flow_browse_edit_help)},
	{"escape", "escape", NULL, "Escape", NULL, G_CALLBACK(on_flows_escape_context)},
};

static const GtkActionEntry actions_entries_flow_edition[] = {
	/*
	 * Flow Edition
	 */
	{"flow_edition_help", GTK_STOCK_HELP, NULL,
		"<Control><Shift>h", N_("Show Program's help"), G_CALLBACK(on_flow_component_help_activate)},
	{"flow_edition_delete", GTK_STOCK_DELETE, NULL,
		NULL, N_("Delete"), G_CALLBACK(on_flow_component_delete_activate)},
	{"flow_edition_properties",  "kontact_todo", N_("Edit Parameters"),
		NULL, N_("Edit the Program's parameters"), G_CALLBACK(on_flow_component_properties_activate)},
	{"flow_edition_refresh", GTK_STOCK_REFRESH, NULL,
		NULL, N_("Refresh Menu list"), G_CALLBACK(on_flow_component_refresh_activate)},
	{"flow_edition_copy", GTK_STOCK_COPY, N_("Copy"),
		NULL, N_("Copy to clipboard"), G_CALLBACK(on_copy_activate)},
	{"flow_edition_paste", GTK_STOCK_PASTE, N_("Paste"),
		NULL, N_("Paste from clipboard"), G_CALLBACK(on_paste_activate)},
	{"flow_edition_top", GTK_STOCK_GOTO_TOP, N_("Move to Top"),
		"Home", NULL, G_CALLBACK(on_flow_component_move_top)},
	{"flow_edition_bottom", GTK_STOCK_GOTO_BOTTOM, N_("Move to Bottom"),
		"End", NULL, G_CALLBACK(on_flow_component_move_bottom)},
	{"flow_edition_execute", GTK_STOCK_EXECUTE, NULL,
		"<Control>R", N_("Execute this Flow"), G_CALLBACK (on_flow_execute_activate)},
	{"escape", "escape", NULL, "Escape", NULL, G_CALLBACK(on_flows_escape_context)}
};

static const GtkActionEntry actions_entries_job_control[] = {
	/*
	 * Job control - Job Actions
	 */
	{"job_control_save", GTK_STOCK_SAVE, NULL,
		NULL, N_("Save Job information to a file"), G_CALLBACK(on_job_control_save)},
	{"job_control_close", "trash-empty", N_("Close"),
		"Delete", N_("Clear selected Jobs"), G_CALLBACK(on_job_control_close)},
	{"job_control_stop", GTK_STOCK_STOP, N_("Cancel"),
		NULL, N_("Ask node to cancel the selected Job"), G_CALLBACK(on_job_control_stop)},
	{"job_control_filter", "filter", N_("Filter"),
		NULL, N_("Filter jobs by group, node and status"), NULL},
};

static const GtkToggleActionEntry status_action_entries[] = {
	{"flow_edition_toggle_status", NULL, N_("Enabled"),
		NULL, N_("Toggle the status of the selected programs"), NULL}
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

	GtkWidget *vbox;
	GtkWidget *toolbar;

	gebr.about = gebr_gui_about_setup_ui("GêBR", _("A plug-and-play environment for\nseismic processing tools"));

	/* Create the main window */
	gtk_window_set_default_icon(gebr_gui_pixmaps_gebr_icon_16x16());
	gebr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gebr.window), "GêBR");
	gtk_widget_set_size_request(gebr.window, 700, 400);
	gtk_widget_show(gebr.window);
	gebr_maestro_controller_set_window(gebr.maestro_controller, GTK_WINDOW(gebr.window));

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
	gtk_action_group_add_toggle_actions(gebr.action_group_status, status_action_entries, G_N_ELEMENTS(status_action_entries), NULL);
	gebr.accel_group_array[ACCEL_STATUS] = gtk_accel_group_new();
	gebr_gui_gtk_action_group_set_accel_group(gebr.action_group_status, gebr.accel_group_array[ACCEL_STATUS]);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new (), -1);

	GtkToolItem *help_view = GTK_TOOL_ITEM(gtk_action_create_tool_item
				(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_view")));
	GtkToolItem *help_edit = GTK_TOOL_ITEM(gtk_action_create_tool_item
				(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_edit")));

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(help_view), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(help_edit), -1);

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

	gebr.ui_project_line->info.help_edit = help_edit;
	gebr.ui_project_line->info.help_view = help_view;


	/* Hide line and project properties */
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "main")));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "main")));

	/*
	 * Create Structure of Job Control (to use on Flows)
	 */
	gebr.job_control = gebr_job_control_new();

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
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		                   GTK_TOOL_ITEM(gtk_action_create_tool_item
		                                 (gtk_action_group_get_action(gebr.action_group_flow, "flow_dict_edit"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
	                   GTK_TOOL_ITEM(gtk_action_create_tool_item
	                                 (gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new (), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_view"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_edit"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_import"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_export"))), -1);


	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new (), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_execute"))), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
			   GTK_TOOL_ITEM(gtk_action_create_tool_item
					 (gtk_action_group_get_action(gebr.action_group_flow, "flow_execute_details"))), -1);
	GtkAction *action;
	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_toggle_status");
	g_signal_connect(action, "toggled", G_CALLBACK(on_flow_component_status_activate), NULL);

	gebr.ui_flow_browse = flow_browse_setup_ui();

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_browse->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flows")));
	gtk_widget_show_all(vbox);

	gtk_widget_hide(gebr.ui_flow_browse->info_jobs);
	gtk_widget_hide(gebr.ui_flow_browse->context[CONTEXT_MENU]);

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

	GtkWidget *tool_button = gebr_gui_tool_button_new();
	GtkWidget *filter_button = gtk_image_new_from_stock("filter", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_container_add(GTK_CONTAINER(tool_button), filter_button);
	gtk_widget_set_tooltip_text( filter_button, "Filter jobs by group, nodes or status");

	gtk_button_set_relief(GTK_BUTTON(tool_button), GTK_RELIEF_NONE);
	gebr_job_control_setup_filter_button(gebr.job_control, GEBR_GUI_TOOL_BUTTON(tool_button));

	GtkToolItem *item = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(item), tool_button);
	gtk_widget_show_all(GTK_WIDGET(item));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr_job_control_get_widget(gebr.job_control), TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Jobs")));
	gtk_widget_show(vbox);
	gtk_widget_show(toolbar);

	/*
	 * Log status bar
	 */
	gebr.ui_log = log_setup_ui();
	gtk_widget_show_all(gebr.ui_log->box);
	gtk_box_pack_end(GTK_BOX(main_vbox), gebr.ui_log->box, FALSE, FALSE, 0);

	gebr.last_notebook = -1;
	g_signal_connect(GTK_OBJECT(gebr.notebook), "switch-page", G_CALLBACK(on_notebook_switch_page), NULL);
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
	                                              (gebr.action_group_general, "actions_connections")));
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

	menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_general, "help_demos_su"));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);

        gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_general, "help_about")));

	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
	demos_list_create(GTK_MENU(submenu));
}

gdouble
gebr_interface_get_execution_speed(void)
{
	return gebr.config.flow_exec_speed;
}

gint
gebr_interface_get_niceness(void)
{
	return gebr.config.niceness;
}

void
gebr_interface_change_tab(enum NOTEBOOK_PAGE page)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), page);
}

const gchar *
gebr_interface_get_speed_icon(gdouble value)
{
	if (value == 0)
		return "gebr-speed-one-core";
	else if (value <= 1)
		return "gebr-speed-verylow";
	else if (value <= 2)
		return "gebr-speed-low";
	else if ((value <= 3) || (value <= 4))
		return "gebr-speed-medium";
	else if (value <= SLIDER_100)
		return "gebr-speed-high";
	else if (value <= SLIDER_MAX)
		return "gebr-speed-veryhigh";
	else
		g_return_val_if_reached(NULL);
}

void
gebr_interface_update_speed_sensitiveness(GtkWidget *button,
					  GtkWidget *slider,
					  gboolean sensitive)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(button));

	g_return_if_fail(GTK_IS_IMAGE(child) == TRUE);

	if (!sensitive) {
		gtk_image_set_from_stock(GTK_IMAGE(child), gebr_interface_get_speed_icon(0),
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_hide(slider);
	} else {
		gdouble speed = gebr_ui_flow_execution_calculate_slider_from_speed(gebr.config.flow_exec_speed);
		gtk_image_set_from_stock(GTK_IMAGE(child), gebr_interface_get_speed_icon(speed),
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show(slider);
	}
}
