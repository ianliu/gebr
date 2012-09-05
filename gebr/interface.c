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

#include "gebr-flow-edition.h"
#include "interface.h"
#include "gebr.h"
#include "flow.h"
#include "callbacks.h"
#include "menu.h"

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
	{"flow_delete", GTK_STOCK_DELETE, NULL, "Delete",
		N_("Delete"), G_CALLBACK(on_flow_delete_activate)},
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
		"Delete", N_("Delete"), G_CALLBACK(on_flow_component_delete_activate)},
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

static const GtkActionEntry status_action_entries[] = {
	{"flow_edition_status_configured", NULL, N_("Configured"),
		NULL, N_("Change the selected Programs status to configured"), NULL},
	{"flow_edition_status_disabled", NULL, N_("Disabled"),
		NULL, N_("Change the selected Programs status to disabled"), NULL},
	{"flow_edition_status_unconfigured", NULL, N_("Not configured"),
		NULL, N_("Change the selected Programs status to not configured"), NULL}
};

/*
 * Prototypes functions
 */

static void assembly_menus(GtkMenuBar * menu_bar);

static gdouble
calculate_speed_from_slider_value(gdouble x)
{
	if (x > SLIDER_100)
		return (VALUE_MAX - SLIDER_100) / (SLIDER_MAX - SLIDER_100) * (x - SLIDER_100) + SLIDER_100;
	else
		return x;
}

gdouble
gebr_interface_calculate_slider_from_speed(gdouble speed)
{
	if (speed > SLIDER_100)
		return (SLIDER_MAX - SLIDER_100) * (speed - SLIDER_100) / (VALUE_MAX - SLIDER_100) + SLIDER_100;
	else
		return speed;
}

static void
adjustment_value_changed(GtkAdjustment *adj)
{
	gdouble value = gtk_adjustment_get_value(adj);
	gebr.config.flow_exec_speed = calculate_speed_from_slider_value(value);
}

static void
value_changed(GtkRange *range, gpointer user_data)
{
	GtkWidget *speed_button = user_data;
	GtkImage *speed_button_image = GTK_IMAGE(gtk_bin_get_child(GTK_BIN(speed_button)));
	gdouble value = gtk_range_get_value(range);
	const gchar *icon = gebr_interface_get_speed_icon(value);
	if (icon)
		gtk_image_set_from_stock(speed_button_image, icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
}

static gboolean
change_value(GtkRange *range, GtkScrollType scroll, gdouble value)
{
	GtkAdjustment *adj = gtk_range_get_adjustment(range);
	gdouble min, max;

	min = gtk_adjustment_get_lower(adj);
	max = gtk_adjustment_get_upper(adj);

	gdouble speed = CLAMP (value, min, max);

	gtk_adjustment_set_value(adj, speed);

	return TRUE;
}

const gchar *
gebr_interface_set_text_for_performance(gdouble value)
{
	if (value <= 0.1)
	    return _(g_strdup_printf("1 core"));
	else if (value < (SLIDER_100))
	    return _(g_strdup_printf("%.0lf%% of total number of cores", value*20));
	else if (value <= 400)
	    return _(g_strdup_printf("%.0lf%% of total number of cores", value*100 - 400));
	else
		g_return_val_if_reached(NULL);
}

static gboolean
speed_controller_query_tooltip(GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       GtkTooltip *tooltip,
			       gpointer    user_data)
{
	GtkRange *scale = GTK_RANGE(widget);
	gdouble value = gtk_range_get_value(scale);
	const gchar *text_tooltip;
	text_tooltip = gebr_interface_set_text_for_performance(value);
	gtk_tooltip_set_text (tooltip, text_tooltip);
	return TRUE;
}

static gboolean
speed_button_tooltip (GtkWidget  *widget,
                      gint        x,
                      gint        y,
                      gboolean    keyboard_mode,
                      GtkTooltip *tooltip,
                      gpointer    user_data)
{
	gdouble value = gebr_interface_calculate_slider_from_speed(gebr.config.flow_exec_speed);

	const gchar *speed;
	speed = gebr_interface_set_text_for_performance(value);
	const gchar * text_tooltip = g_strdup_printf(_("Execution dispersion: %s"), speed);
	gtk_tooltip_set_text (tooltip, text_tooltip);

	return TRUE;
}

static void
priority_button_toggled(GtkToggleButton *b1,
			GtkToggleButton *b2)
{
	gboolean active = gtk_toggle_button_get_active(b1);

	if (active) {
		g_signal_handlers_block_by_func(b2, priority_button_toggled, b1);
		gtk_toggle_button_set_active(b2, FALSE);
		g_signal_handlers_unblock_by_func(b2, priority_button_toggled, b1);
		gebr.config.niceness = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b1), "nice"));
	} else {
		g_signal_handlers_block_by_func(b1, priority_button_toggled, b2);
		gtk_toggle_button_set_active(b1, TRUE);
		g_signal_handlers_unblock_by_func(b1, priority_button_toggled, b2);
	}
}

/*
 * Updates the value of the scale
 */
static void 
on_show_scale(GtkWidget * scale)
{
	gtk_range_set_value(GTK_RANGE(scale),
			    gebr_interface_calculate_slider_from_speed(gebr.config.flow_exec_speed));
}

/*
 * Inserts a speed controler inside a toolbar,
 * which is a GtkScale to control the performance of flow execution.
 */
static void
insert_speed_controler(GtkToolbar *toolbar,
		       GtkWidget **toggle_high,
		       GtkWidget **toggle_low,
		       GtkWidget **speed,
		       GtkWidget **speed_slider,
		       GtkWidget **ruler
		       )
{
	if (!gebr.flow_exec_adjustment) {
		gebr.flow_exec_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, SLIDER_MAX, 0.1, 1, 0.1));
		g_signal_connect(gebr.flow_exec_adjustment, "value-changed", G_CALLBACK(adjustment_value_changed), NULL);
	}

	GtkToolItem *speed_item = gtk_tool_item_new();
	GtkWidget *speed_button = gebr_gui_tool_button_new();
	GtkWidget *h_separator = gtk_hseparator_new();
	gtk_button_set_relief(GTK_BUTTON(speed_button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(speed_button), gtk_image_new());

	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget *scale = gtk_hscale_new(gebr.flow_exec_adjustment);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_scale_set_digits(GTK_SCALE(scale), 1);

	gdouble med = SLIDER_100 / 2.0;
	gtk_scale_add_mark(GTK_SCALE(scale), 0, GTK_POS_LEFT, "<span size='x-small'>1 Core</span>");
	gtk_scale_add_mark(GTK_SCALE(scale), (med/2), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), med, GTK_POS_LEFT, "<span size='x-small'>50%</span>");
	gtk_scale_add_mark(GTK_SCALE(scale), ((med+SLIDER_100)/2), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), SLIDER_100, GTK_POS_LEFT, "<span size='x-small'>100%</span>");
	gtk_scale_add_mark(GTK_SCALE(scale), (SLIDER_100+1), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), (SLIDER_MAX-1), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), SLIDER_MAX, GTK_POS_LEFT, "<span size='x-small'>400%</span>");

	g_object_set(scale, "has-tooltip",TRUE, NULL);

	g_signal_connect(scale, "change-value", G_CALLBACK(change_value), NULL);
	g_signal_connect(scale, "value-changed", G_CALLBACK(value_changed), speed_button);
	g_signal_connect(scale, "query-tooltip", G_CALLBACK(speed_controller_query_tooltip), NULL);
	g_signal_connect(scale, "map", G_CALLBACK(on_show_scale), NULL);

	gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), h_separator, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 5);

	GtkWidget *high = gtk_toggle_button_new_with_label(_("High priority"));
	GtkWidget *low = gtk_toggle_button_new_with_label(_("Low priority"));
	gtk_widget_set_can_focus(high, FALSE);
	gtk_widget_set_can_focus(low, FALSE);
	gtk_button_set_relief(GTK_BUTTON(high), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(low), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(high, _("Share available resources"));
	gtk_widget_set_tooltip_text(low, _("Wait for free resources"));
	g_object_set_data(G_OBJECT(high), "nice", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(low), "nice", GINT_TO_POINTER(19));
	g_signal_connect(high, "toggled", G_CALLBACK(priority_button_toggled), low);
	g_signal_connect(low, "toggled", G_CALLBACK(priority_button_toggled), high);
	gtk_box_pack_end(GTK_BOX(hbox), high, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), low, TRUE, TRUE, 0);

	*toggle_high = high;
	*toggle_low = low;
	*speed = speed_button;
	*speed_slider = scale;
	*ruler = h_separator;

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show_all(vbox);
	gebr_gui_tool_button_add(GEBR_GUI_TOOL_BUTTON(speed_button), vbox);

	g_object_set(speed_button, "has-tooltip",TRUE, NULL);
	g_signal_connect(speed_button, "query-tooltip", G_CALLBACK(speed_button_tooltip), NULL);

	gtk_widget_show_all(speed_button);
	gtk_container_add(GTK_CONTAINER(speed_item), speed_button);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), speed_item, -1);
}

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
	GtkAction *action;

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
	gtk_action_group_add_actions(gebr.action_group_status, status_action_entries, G_N_ELEMENTS(status_action_entries), NULL);
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
	gebr.ui_flow_edition = flow_edition_setup_ui();

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

	gebr.ui_flow_browse = flow_browse_setup_ui();

	insert_speed_controler(GTK_TOOLBAR(toolbar),
			       &gebr.ui_flow_browse->nice_button_high,
			       &gebr.ui_flow_browse->nice_button_low,
			       &gebr.ui_flow_browse->speed_button,
			       &gebr.ui_flow_browse->speed_slider,
			       &gebr.ui_flow_browse->ruler);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gebr.ui_flow_browse->widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(gebr.notebook), vbox, gtk_label_new(_("Flows")));
	gtk_widget_show_all(vbox);

	gtk_widget_hide(gebr.ui_flow_browse->info_jobs);
	gtk_widget_hide(gebr.ui_flow_browse->context[CONTEXT_MENU]);

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

	insert_speed_controler(GTK_TOOLBAR(toolbar),
			       &gebr.ui_flow_edition->nice_button_high,
			       &gebr.ui_flow_edition->nice_button_low,
			       &gebr.ui_flow_edition->speed_button,
			       &gebr.ui_flow_edition->speed_slider,
			       &gebr.ui_flow_edition->ruler);

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
					  GtkWidget *ruler,
					  gboolean sensitive)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(button));

	g_return_if_fail(GTK_IS_IMAGE(child) == TRUE);

	if (!sensitive) {
		gtk_image_set_from_stock(GTK_IMAGE(child), gebr_interface_get_speed_icon(0),
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_hide(slider);
		gtk_widget_hide(ruler);
	} else {
		gdouble speed = gebr_interface_calculate_slider_from_speed(gebr.config.flow_exec_speed);
		gtk_image_set_from_stock(GTK_IMAGE(child), gebr_interface_get_speed_icon(speed),
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_show(slider);
		gtk_widget_show(ruler);
	}
}
