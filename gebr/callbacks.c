/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <locale.h>

#include "callbacks.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"
#include "ui_flow_execution.h"
#include "ui_flow_browse.h"
#include "ui_document.h"
#include "ui_paths.h"
#include "ui_project_line.h"
#include "gebr-job-control.h"
#include "ui_help.h"
#include "gebr-gui-tool-button.h"

static gboolean line_check_maestro_connected(void);

void on_new_activate(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :{
		if (!flow_browse_get_selected(&iter, FALSE)) {
			flow_new();
			gebr_gui_tool_button_toggled_active(gebr.menu_button, TRUE);
			return;
		}

		GebrUiFlowBrowseType type;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT_TYPE, &type, -1);


		flow_new();
		gebr_gui_tool_button_toggled_active(gebr.menu_button, TRUE);
		break;
	}
	default:
		break;
	}
}

void on_new_project_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_PROJECT_LINE:
		project_new();
		break;
	default:
		break;
	}
}

void on_new_line_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_PROJECT_LINE:
		if (!line_check_maestro_connected())
			return;
		line_new();
		break;
	default:
		break;
	}
}

void on_new_program_activate(void)
{
	gebr_gui_tool_button_toggled_active(gebr.menu_button, FALSE);
}

void on_copy_activate(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		gebr_geoxml_clipboard_clear();
		gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
			GebrUiFlowBrowseType type;
			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
			                   FB_STRUCT_TYPE, &type,
			                   -1);

			if (type == STRUCT_TYPE_FLOW)
				break;

			if (type == STRUCT_TYPE_PROGRAM) {
				flow_program_copy();
				return;
			}
		}
		flow_copy();
		break;
	default:
		break;
	}
}

void on_paste_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :{
		gboolean is_flow = gebr.flow_clipboard && (g_list_length(gebr.flow_clipboard) > 0);
		if (is_flow)
			flow_paste();
		else
			flow_program_paste();
		break;
	}
	default:
		break;
	}
}

void on_quit_activate(void)
{
	GtkTreeIter iter;
	GebrMaestroServer *m = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (m && gebr_maestro_server_get_state(m) == SERVER_STATE_LOGGED) {
		// Save configuration before quit
		gebr_config_save(FALSE);
		gebr.quit = TRUE;

		// Disconnect client from maestro
		gebr_maestro_server_disconnect(m, TRUE);

		GtkTreeModel *model = gebr_maestro_server_get_model(m, TRUE, NULL);

		gebr_gui_gtk_tree_model_foreach_hyg(iter, model, 1) {
			GebrDaemonServer *daemon;
			gtk_tree_model_get(model, &iter, 0, &daemon, -1);
			g_object_unref(daemon);
		}
		g_object_unref(model);
	} else
		gebr_quit(TRUE);
}

void on_document_properties_activate(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :{
		if (!flow_browse_get_selected(&iter, FALSE))
			return;

		GebrUiFlowBrowseType type;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT_TYPE, &type, -1);

		if (type == STRUCT_TYPE_FLOW)
			document_properties_setup_ui(document_get_current(), NULL, FALSE);
		else if (type ==  STRUCT_TYPE_PROGRAM)
			flow_edition_component_activated();
		break;
	}
	default:
		document_properties_setup_ui(document_get_current(), NULL, FALSE);
		break;
	}
}

void on_document_dict_edit_activate(void)
{
	document_dict_edit_setup_ui();
}

void on_project_line_delete_activate(void)
{
	project_line_delete();
}

void on_project_line_import_activate(void)
{
	project_line_import();
}

void on_project_line_export_activate(void)
{
	project_line_export();
}

void on_flow_import_activate(void)
{
	flow_import();
}

void on_flow_export_activate(void)
{
	flow_export();
}

void on_flow_delete_activate(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :{
		if (!flow_browse_get_selected(&iter, FALSE))
			return;

		GebrUiFlowBrowseType type;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT_TYPE, &type, -1);

		if (type == STRUCT_TYPE_FLOW)
			flow_delete(TRUE);
		else if (type ==  STRUCT_TYPE_PROGRAM)
			on_flow_component_delete_activate();
		break;
	}
	default:
		break;
	}
}

static gboolean
line_check_maestro_connected(void)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED)
		return TRUE;

	GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
	                                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                       GTK_MESSAGE_WARNING,
	                                                       GTK_BUTTONS_OK,
	                                                       _("\nA connected maestro is required to accomplish this task.\n"));

	gchar *win_title = g_strdup(_("Maestro disconnected"));
	gtk_window_set_title(GTK_WINDOW(dialog), win_title);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	on_configure_servers_activate();

	return FALSE;
}

static gboolean
flows_check_maestro_connected(void)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);

	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED)
		return TRUE;

	gchar *maestro_line = gebr_geoxml_line_get_maestro(gebr.line);

	GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
	                                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                       GTK_MESSAGE_WARNING,
	                                                       GTK_BUTTONS_OK,
	                                                       _("<span size='large'><b>The maestro of this line is disconnected.</b></span>\n\n"
	                                                	 "Connect to maestro <b>%s</b> to execute this line."), maestro_line);

	gchar *win_title = g_strdup(_("Maestro disconnected"));
	gtk_window_set_title(GTK_WINDOW(dialog), win_title);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	g_free(maestro_line);

	on_configure_servers_activate();

	return FALSE;
}

gboolean
flow_check_before_execution(GebrGeoXmlFlow *flow,
                            gboolean is_snapshot)
{

	GError *error = NULL;
	GebrGeoXmlFlowError error_code;

	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
	gebr_geoxml_flow_validate(flow, gebr.validator, &error);
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	if (!error)
		return TRUE;

	error_code = error->code;
	switch (error_code)
	{
	case GEBR_GEOXML_FLOW_ERROR_NO_INPUT:
	case GEBR_GEOXML_FLOW_ERROR_NO_OUTPUT:
	case GEBR_GEOXML_FLOW_ERROR_PROGRAM:
		if (is_snapshot) {
			gdk_threads_enter();
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Broken Flow"), error->message);
			gdk_threads_leave();
		} else {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Broken Flow"), error->message);
		}
		break;
	case GEBR_GEOXML_FLOW_ERROR_NO_INFILE:
	case GEBR_GEOXML_FLOW_ERROR_INVALID_INFILE:
	case GEBR_GEOXML_FLOW_ERROR_INVALID_OUTFILE:
	case GEBR_GEOXML_FLOW_ERROR_INVALID_ERRORFILE:
	case GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS:
	case GEBR_GEOXML_FLOW_ERROR_LOOP_ONLY:
		if (is_snapshot) {
			gdk_threads_enter();
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Warning"), error->message);
			gdk_threads_leave();
		} else {
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Warning"), error->message);
		}

		break;
	}

	g_clear_error(&error);
	return FALSE;
}

/*
 * Returns TRUE if all flows at "Flows" tab
 * can be executed. If they can't, return false
 * and pop-up a dialog with the error.
 */
static gboolean flows_check_before_execution(void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GebrGeoXmlFlow *flow;
	GtkTreeSelection *selection;
	GList *rows;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	GebrUiFlowBrowseType type;
	GebrUiFlow *ui_flow;
	for (GList * i = rows; i; i = i->next)
	{
		gtk_tree_model_get_iter(model, &iter, i->data); 
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT_TYPE, &type,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		if (type != STRUCT_TYPE_FLOW)
			flow = gebr.flow;
		else
			flow = gebr_ui_flow_get_flow(ui_flow);

		if (g_list_length(rows) == 1)
			flow_browse_revalidate_programs(gebr.ui_flow_browse);

		if (flow_check_before_execution(flow, FALSE))
			continue;

		return FALSE;
	}

	return TRUE;
}

void on_flow_execute_activate(void)
{
	if (!flows_check_before_execution())
		return;

	if (!flows_check_maestro_connected())
		return;

	gebr_ui_flow_run(NULL, FALSE, FALSE);
}

void on_flow_execute_details_activate(void)
{
	if (!flows_check_before_execution())
		return;

	if (!flows_check_maestro_connected())
		return;

	gboolean sensitive = gebr_ui_flow_browse_update_speed_slider_sensitiveness(gebr.ui_flow_browse);

	/* Count the number of Flows selected*/
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gint n = gtk_tree_selection_count_selected_rows(selection);

	if (n == 0)
		return;

	gboolean multiple = (n == 1) ? FALSE : TRUE;

	gebr.ui_flow_execution = gebr_ui_flow_execution_details_setup_ui(sensitive, multiple);
}

void
on_flow_execute_parallel_activate(void)
{
	if (!flows_check_before_execution())
		return;

	if (!flows_check_maestro_connected())
		return;
	gebr_ui_flow_run(NULL, TRUE, FALSE);
}

void on_flow_revision_save_activate(void)
{
	flow_revision_save();
}

void on_flow_revision_show_menu(void)
{
	flow_browse_single_selection();
}

void on_flow_component_help_activate(void)
{
	gebr_help_show_selected_program_help();
}

void on_flow_component_delete_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :
		flow_program_remove();
		if(gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
			gebr_gui_tool_button_toggled_active(gebr.menu_button, FALSE);
		break;
	default:
		break;
	}
}

void on_flow_component_properties_activate(void)
{
	flow_edition_component_activated();
}

void on_flow_component_refresh_activate(void)
{
	menu_list_populate();
}

void on_flow_component_move_top(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :
		gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
		if (!flow_browse_get_selected(&iter, TRUE))
			return;

		GebrUiFlowBrowseType type;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT_TYPE, &type,
		                   -1);

		if (type == STRUCT_TYPE_PROGRAM)
			flow_program_move_top();
		break;
	default:
		break;
	}
	flow_browse_program_check_sensitiveness();
}

void on_flow_component_move_bottom(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE :
		gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
		if (!flow_browse_get_selected(&iter, TRUE))
			return;

		GebrUiFlowBrowseType type;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT_TYPE, &type,
		                   -1);

		if (type == STRUCT_TYPE_PROGRAM)
			flow_program_move_bottom();
		break;
	default:
		break;
	}
	flow_browse_program_check_sensitiveness();
}

void on_job_control_save(void)
{
	gebr_job_control_save_selected(gebr.job_control);
}

void on_job_control_close(void)
{
	gebr_job_control_close_selected(gebr.job_control);
}

void on_job_control_stop(void)
{
	gebr_job_control_stop_selected(gebr.job_control);
}

void on_configure_preferences_activate(void)
{
	preferences_setup_ui(FALSE, FALSE, FALSE, -1);
}

void on_configure_wizard_activate(void)
{
	preferences_setup_ui(FALSE, TRUE, FALSE, -1);
}

void on_configure_servers_activate(void)
{
	gebr_maestro_controller_create_dialog(gebr.maestro_controller);
}

void on_help_contents_activate(void)
{
	gchar *loc;
	const gchar *path;

	loc = setlocale(LC_MESSAGES, NULL);
	if (g_str_has_prefix (loc, "pt"))
		path = "file://" GEBR_USERDOC_DIR "/pt_BR/html/index.html";
	else
		path = "file://" GEBR_USERDOC_DIR "/en/html/index.html";

	if (!gtk_show_uri(NULL, path, GDK_CURRENT_TIME, NULL)) {
		gtk_show_uri(NULL, "http://www.gebrproject.com", GDK_CURRENT_TIME, NULL);
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not load help. "
				"Certify it was installed correctly."));
	}
}

void on_help_about_activate(void)
{
	gtk_widget_show_all(gebr.about.dialog);
}

void import_demo(GtkWidget *menu_item, const gchar *path)
{
	project_line_import_path(path);
}

void on_flow_browse_show_help(void)
{
    flow_browse_show_help();
}

void on_flow_browse_edit_help(void)
{
    flow_browse_edit_help();
}

void on_flow_browse_show_search_bar(void)
{
	flow_browse_show_search_bar(gebr.ui_flow_browse);
}

void on_project_line_show_help (void)
{
    project_line_show_help();
}

void on_project_line_edit_help (void)
{
    project_line_edit_help();
}

void on_notebook_switch_page (GtkNotebook     *notebook,
                              GtkNotebookPage *page,
                              guint            page_num,
                              gpointer         user_data)
{

	if (gebr.last_notebook >= 0)
		gtk_window_remove_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[page_num]);

	switch (gebr.last_notebook)
	{
	case NOTEBOOK_PAGE_JOB_CONTROL:
		gebr_job_control_hide(gebr.job_control);
		break;
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		gebr_flow_browse_hide(gebr.ui_flow_browse);
		break;
	case NOTEBOOK_PAGE_PROJECT_LINE:
		gebr_project_line_hide(gebr.ui_project_line);
		break;
	default:
		break;
	}

	switch (page_num)
	{
	case NOTEBOOK_PAGE_JOB_CONTROL:
		gebr_job_control_show(gebr.job_control);
		break;
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		gebr_flow_browse_show(gebr.ui_flow_browse);
		break;
	case NOTEBOOK_PAGE_PROJECT_LINE:
		gebr_project_line_show(gebr.ui_project_line);
		break;
	default:
		break;
	}

	gebr.last_notebook = page_num;
}

void on_server_common_autoconnect_changed(void)
{
	g_warning("TODO: Implement %s", __func__);
}

//void on_server_common_remove(void)
//{
//	g_warning("TODO: Implement %s", __func__);
//}
//
//void on_server_common_stop(void)
//{
//	g_warning("TODO: Implement %s", __func__);
//}

void open_url_on_press_event(void)
{
	gebr_gui_show_uri("http://www.gebrproject.com/install-guide/download");
}

void
on_flows_escape_context(void)
{
	gebr_flow_browse_escape_context(gebr.ui_flow_browse);
}

void on_flow_component_status_activate(GtkToggleAction *toggleaction)
{
	flow_browse_toggle_selected_program_status(gebr.ui_flow_browse);
}
