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

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>

#include "callbacks.h"
#include "../defines.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_flow_browse.h"
#include "ui_document.h"
#include "ui_paths.h"
#include "ui_project_line.h"
#include "ui_job_control.h"
#include "ui_help.h"

void on_new_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		flow_new();
		break;
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
		line_new();
		break;
	default:
		break;
	}
}

void on_copy_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		flow_copy();
		break;
	case NOTEBOOK_PAGE_FLOW_EDITION:
		flow_program_copy();
		break;
	default:
		break;
	}
}

void on_paste_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		flow_paste();
		break;
	case NOTEBOOK_PAGE_FLOW_EDITION:
		flow_program_paste();
		break;
	default:
		break;
	}
}

void on_quit_activate(void)
{
	gebr_quit(TRUE);
}

void on_document_properties_activate(void)
{
	document_properties_setup_ui(document_get_current(), NULL);
}

void on_document_dict_edit_activate(void)
{
	document_dict_edit_setup_ui();
}

void on_project_line_delete_activate(void)
{
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		project_delete(TRUE);
	else
		line_delete(TRUE);
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
	flow_delete(TRUE);
}

void on_flow_execute_activate(void)
{
	flow_fast_run(FALSE, FALSE);
}

void on_flow_execute_in_parallel_activate(void)
{
	flow_fast_run(TRUE, FALSE);
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
	case NOTEBOOK_PAGE_FLOW_EDITION :
		flow_program_remove();
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
	menu_list_create_index();
	menu_list_populate();
}

void on_flow_component_move_top(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_EDITION :
		gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
		if (!flow_edition_get_selected_component(&iter, TRUE))
			return;
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
			return;
		}
		flow_program_move_top();
		break;
	default:
		break;
	}
}

void on_flow_component_move_bottom(void)
{
	GtkTreeIter iter;
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_EDITION :
		gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
		if (!flow_edition_get_selected_component(&iter, TRUE))
			return;
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
			return;
		}
		flow_program_move_bottom();
		break;
	default:
		break;
	}
}

void on_flow_component_status_activate(GtkAction *action,
				       gpointer user_data)
{
	guint status = GPOINTER_TO_UINT(user_data);
	flow_edition_status_changed(status);
}

void on_flow_component_execute ()
{
	/* not parallel, single flow execution */

	const gchar *input = NULL;
	const gchar *output = NULL;
	GebrGeoXmlSequence *first_program;
	gulong i = 0, max = 0;

	input = gebr_geoxml_flow_server_io_get_input(gebr.flow_server);
	output = gebr_geoxml_flow_server_io_get_output(gebr.flow_server);

	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);

	if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program)) && (input == NULL || g_strcmp0("", input) == 0))
	{
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				       	_("Warning"),_("No input file specified for this program."));
		return;
	}

	/*
	 * Checking if the flow has at least one configured program
	 */
	max = gebr_geoxml_flow_get_programs_number(gebr.flow); 
	gint status = 0;

	for (i = 0; i < max; i++)
	{
		gebr_geoxml_flow_get_program(gebr.flow, &first_program, i);
		status = gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(first_program));

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			break;
	}

	if (status != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
	{
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				       	_("Warning"),_("No configured or enabled programs found"));
		return;
	}

	flow_fast_run (FALSE, TRUE);
}

void on_job_control_save(void)
{
	job_control_save();
}

void on_job_control_cancel(void)
{
	job_control_cancel();
}

void on_job_control_close(void)
{
	job_control_close();
}

void on_job_control_clear(void)
{
	job_control_clear(FALSE);
}

void on_job_control_stop(void)
{
	job_control_stop();
}

/*
 * Job Control - Queue Actions
 */

void on_job_control_queue_stop(void)
{
	job_control_queue_stop();
}

void on_job_control_queue_save(void)
{
	job_control_queue_save();
}

void on_job_control_queue_close(void)
{
	job_control_queue_close();
}

void on_configure_preferences_activate(void)
{
	preferences_setup_ui(FALSE);
}

void on_configure_servers_activate(void)
{
	server_list_show(gebr.ui_server_list);
}

void on_help_contents_activate(void)
{
	GString *html_path = g_string_new("");
	g_string_printf(html_path, "file://" GEBR_USERDOC_DIR "/pt_BR/index.html");
	if (!gebr_gui_show_uri(html_path->str))
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not load help. Certify it was installed correctly."));
	g_string_free(html_path, TRUE);
}

void on_help_about_activate(void)
{
	gtk_widget_show_all(gebr.about.dialog);
}

void navigation_bar_update(void)
{
	GString *markup;

	if (gebr.project_line == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.navigation_box_label), "");
		return;
	}

	markup = g_string_new(NULL);
	g_string_append(markup, g_markup_printf_escaped("<i>%s</i>",
							gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.project))));
	if (gebr.line != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
								gebr_geoxml_document_get_title(GEBR_GEOXML_DOC
											       (gebr.line))));
	if (gebr.flow != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
								gebr_geoxml_document_get_title(GEBR_GEOXML_DOC
											       (gebr.flow))));

	gtk_label_set_markup(GTK_LABEL(gebr.navigation_box_label), markup->str);

	g_string_free(markup, TRUE);
}

void on_flow_browse_show_help(void) {
    flow_browse_show_help();
}

void on_flow_browse_edit_help(void) {
    flow_browse_edit_help();
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
    gebr.last_notebook = page_num;
}
