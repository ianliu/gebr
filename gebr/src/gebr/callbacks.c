/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

/*
 * File: callbacks.c
 * Callbacks for notebook and menus items
 */

#include "callbacks.h"
#include "gebr.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_document.h"
#include "ui_paths.h"
#include "ui_job_control.h"
#include "ui_help.h"

/*
 * Function: on_copy_activate
 * Select copy target depending on the context
 */
void
on_copy_activate(void)
{
// 	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
// 	case NOTEBOOK_PAGE_PROJECT_LINE:
// 
// 		break;
// 	case NOTEBOOK_PAGE_FLOW_BROWSE:
// 
// 		break;
// 	default:
// 		break;
// 	}
}

/*
 * Function: on_paste_activate
 * Select paste target depending on the context
 */
void
on_paste_activate(void)
{
}

/*
 * Function: on_quit_activate
 * Call <gebr_quit>
 */
void
on_quit_activate(void)
{
	gebr_quit();
}

/*
 * Function: on_project_line_new_project_activate
 * Call <project_new> from <project.c>
 *
 */
void
on_project_line_new_project_activate(void)
{
	project_new();
}

/*
 * Function: on_project_line_new_line_activate
 * Call <project_new> from <project.c>
 *
 */
void
on_project_line_new_line_activate(void)
{
	line_new();
}

/*
 * Function: on_project_line_delete_activate
 * Call <project_delete> from <project.c>
 *
 */
void
on_project_line_delete_activate(void)
{
	if (geoxml_document_get_type(gebr.project_line) == GEOXML_DOCUMENT_TYPE_PROJECT)
		project_delete();
	else
		line_delete();
}

/*
 * Function: on_project_line_properties_activate
 * *Fill me in!*
 *
 */
void
on_project_line_properties_activate(void)
{
	if (geoxml_document_get_type(gebr.project_line) == GEOXML_DOCUMENT_TYPE_PROJECT)
		document_properties_setup_ui(GEOXML_DOC(gebr.project));
	else
		document_properties_setup_ui(GEOXML_DOC(gebr.line));
}

/*
 * Function: on_project_line_paths_activate
 * *Fill me in!*
 *
 */
void
on_project_line_paths_activate(void)
{
	path_list_setup_ui();
}

/*
 * Function: on_project_line_import_activate
 * Call <project_line_import> from <ui_project_line.c>
 *
 */
void
on_project_line_import_activate(void)
{
	project_line_import();
}

/*
 * Function: on_project_line_export_activate
 * Call <project_line_export> from <ui_project_line.c>
 *
 */
void
on_project_line_export_activate(void)
{
	project_line_export();
}

/*
 * Function: on_flow_new_activate
 * Call <flow_new> from <flow.c>
 *
 */
void
on_flow_new_activate(void)
{
	flow_new();
}

/*
 * Function: on_flow_import_activate
 * Call <flow_import> from <flow.c>
 *
 */
void
on_flow_import_activate(void)
{
	flow_import();
}

/*
 * Function: on_flow_export_activate
 * Call <flow_export> from <flow.c>
 *
 */
void
on_flow_export_activate(void)
{
	flow_export();
}

/*
 * Function: on_flow_export_as_menu_activate
 * Call <flow_export_as_menu> from <flow.c>
 */
void
on_flow_export_as_menu_activate(void)
{
	flow_export_as_menu();
}

/*
 * Function: on_flow_delete_activate
 * Call <flow_delete> from <flow.c>
 *
 */
void
on_flow_delete_activate(void)
{
	flow_delete();
}

/*
 * Function:
 * Call <flow_new> from <flow.c>
 *
 */
void
on_flow_properties_activate(void)
{
	document_properties_setup_ui(GEOXML_DOC(gebr.flow));
}

/*
 * Function: on_flow_io_activate
 * *Fill me in!*
 *
 */
void
on_flow_io_activate(void)
{
	flow_io_setup_ui(FALSE);
}

/*
 * Function: on_flow_execute_activate
 * Call <flow_run> from <flow.c>
 *
 */
void
on_flow_execute_activate(void)
{
	flow_run();
}

/*
 * Function: on_flow_revision_save_activate
 * Call <flow_revision_save> from <flow.c>
 *
 */
void
on_flow_revision_save_activate(void)
{
	flow_revision_save();
}

/*
 * Function: on_flow_component_help_activate
 * *Fill me in!*
 *
 */
void
on_flow_component_help_activate(void)
{
	program_help_show();
}

/*
 * Function: on_flow_component_duplicate_activate
 * Call <flow_program_duplicate>
 */
void
on_flow_component_duplicate_activate(void)
{
	flow_program_duplicate();
}


/*
 * Function: on_flow_component_delete_activate
 * Call <flow_program_remove>
 */
void
on_flow_component_delete_activate(void)
{
	flow_program_remove();
}

/*
 * Function: on_flow_component_properties_activate
 * *Fill me in!*
 *
 */
void
on_flow_component_properties_activate(void)
{
	flow_edition_component_activated();
}

/*
 * Function: on_flow_component_refresh_activate
 * *Fill me in!*
 *
 */
void
on_flow_component_refresh_activate(void)
{
	menu_list_create_index();
	menu_list_populate();
}

/*
 * Function: on_flow_component_status_activate
 * *Fill me in!*
 *
 */
void
on_flow_component_status_activate(void)
{
	flow_edition_status_changed();
}

/*
 * Function: on_job_control_save
 * Call <job_control_save>
 *
 */
void
on_job_control_save(void)
{
	job_control_save();
}

/*
 * Function: on_job_control_cancel
 * Call <job_control_cancel>
 *
 */
void
on_job_control_cancel(void)
{
	job_control_cancel();
}

/*
 * Function: on_job_control_close
 * Call <job_control_close>
 *
 */
void
on_job_control_close(void)
{
	job_control_close();
}

/*
 * Function: on_job_control_clear
 * Call <job_control_clear>
 *
 */
void
on_job_control_clear(void)
{
	job_control_clear();
}

/*
 * Function: on_job_control_stop
 * Call <job_control_stop>
 *
 */
void
on_job_control_stop(void)
{
	job_control_stop();
}

/*
 * Function: on_configure_preferences_activate
 * *Fill me in!*
 *
 */
void
on_configure_preferences_activate(void)
{
	preferences_setup_ui(FALSE);
}

/*
 * Function: on_configure_servers_activate
 * *Fill me in!*
 *
 */
void
on_configure_servers_activate(void)
{
	server_list_show(gebr.ui_server_list);
}

/*
 * Function: on_help_about_activate
 * *Fill me in!*
 *
 */
void
on_help_about_activate(void)
{
	gtk_widget_show_all(gebr.about.dialog);
}

/*
 * Function: navigation_bar_update
 * 
 */
void
navigation_bar_update(void)
{
	GString *	markup;

	if (gebr.project_line == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.navigation_box_label), "");
		return;
	}

	markup = g_string_new(NULL);
	g_string_append(markup, g_markup_printf_escaped("<i>%s</i>",
		geoxml_document_get_title(GEOXML_DOC(gebr.project))));
	if (gebr.line != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
			geoxml_document_get_title(GEOXML_DOC(gebr.line))));
	if (gebr.flow != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
			geoxml_document_get_title(GEOXML_DOC(gebr.flow))));
	
	gtk_label_set_markup(GTK_LABEL(gebr.navigation_box_label), markup->str);

	g_string_free(markup, TRUE);
}
