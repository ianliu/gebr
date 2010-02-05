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

#include <libgebr/gui/help.h>

#include "callbacks.h"
#include "../defines.h"
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

/**
 * Select copy target depending on the context
 */
void on_copy_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		on_flow_copy_activate();
		break;
	case NOTEBOOK_PAGE_FLOW_EDITION:
		on_flow_component_copy_activate();
		break;
	default:
		break;
	}
}

/**
 * Select paste target depending on the context
 */
void on_paste_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		on_flow_paste_activate();
		break;
	case NOTEBOOK_PAGE_FLOW_EDITION:
		on_flow_component_paste_activate();
		break;
	default:
		break;
	}
}

/** 
 * Call <gebr_quit>
 */
void on_quit_activate(void)
{
	gebr_quit();
}

/** 
 * Call <document_properties_setup_ui>
 */
gboolean on_document_properties_activate(void)
{
	return document_properties_setup_ui(document_get_current());
}

/** 
 * Call <document_dict_edit_setup_ui>
 */
void on_document_dict_edit_activate(void)
{
	document_dict_edit_setup_ui();
}

/** 
 * Call <project_new> from <project.c>
 */
void on_project_line_new_project_activate(void)
{
	project_new();
}

/**
 * Call <project_new> from <project.c>
 */
void on_project_line_new_line_activate(void)
{
	line_new();
}

/**
 * Call <project_delete> from <project.c>
 */
void on_project_line_delete_activate(void)
{
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		project_delete(TRUE);
	else
		line_delete(TRUE);
}

/**
 * *Fill me in!*
 */
void on_project_line_paths_activate(void)
{
	path_list_setup_ui();
}

/**
 * Call <project_line_import> from <ui_project_line.c>
 */
void on_project_line_import_activate(void)
{
	project_line_import();
}

/**
 * Call <project_line_export> from <ui_project_line.c>
 */
void on_project_line_export_activate(void)
{
	project_line_export();
}

/**
 * Call <flow_new> from <flow.c>
 */
void on_flow_new_activate(void)
{
	flow_new();
}

/**
 * Call <flow_import> from <flow.c>
 */
void on_flow_import_activate(void)
{
	flow_import();
}

/**
 * Call <flow_export> from <flow.c>
 */
void on_flow_export_activate(void)
{
	flow_export();
}

/**
 * Call <flow_export_as_menu> from <flow.c>
 */
void on_flow_export_as_menu_activate(void)
{
	flow_export_as_menu();
}

/**
 * Call <flow_delete> from <flow.c>
 */
void on_flow_delete_activate(void)
{
	flow_delete(TRUE);
}

/** 
 * Adjust selection and show flow IO dialog
 */
void on_flow_io_activate(void)
{
	flow_browse_single_selection();
	flow_io_setup_ui(TRUE);
}

/**
 * Call <flow_io_setup_ui> from <flow.c>
 *
 */
void on_flow_execute_activate(void)
{
	flow_fast_run();
}

/** 
 * Call <flow_revision_save>
 */
void on_flow_revision_save_activate(void)
{
	flow_revision_save();
}

/** 
 * Call <flow_browse_single_selection>
 */
void on_flow_revision_show_menu(void)
{
	flow_browse_single_selection();
}

/** 
 * Call <flow_copy>
 */
void on_flow_copy_activate(void)
{
	flow_copy();
}

/** 
 * Call <flow_paste>
 */
void on_flow_paste_activate(void)
{
	flow_paste();
}

/**
 * *Fill me in!*
 *
 */
void on_flow_component_help_activate(void)
{
	program_help_show();
}

/**
 * Call <flow_program_remove>
 */
void on_flow_component_delete_activate(void)
{
	flow_program_remove();
}

/**
 * *Fill me in!*
 */
void on_flow_component_properties_activate(void)
{
	flow_edition_component_activated();
}

/**
 * *Fill me in!*
 */
void on_flow_component_refresh_activate(void)
{
	menu_list_create_index();
	menu_list_populate();
}

/* Function: on_flow_component_status_activate
 * Call <flow_edition_status_changed>
 */
void on_flow_component_status_activate(GtkRadioAction * action, GtkRadioAction * current)
{
	flow_edition_status_changed();
}

/* Function: on_flow_copy_activate
 * Call <flow_edition_component_copy>
 */
void on_flow_component_copy_activate(void)
{
	flow_program_copy();
}

/* Function: on_flow_paste_activate
 * Call <flow_edition_component_paste>
 */
void on_flow_component_paste_activate(void)
{
	flow_program_paste();
}

/* Function: on_job_control_save
 * Call <job_control_save>
 */
void on_job_control_save(void)
{
	job_control_save();
}

/*
 * Function: on_job_control_cancel
 * Call <job_control_cancel>
 *
 */
void on_job_control_cancel(void)
{
	job_control_cancel();
}

/*
 * Function: on_job_control_close
 * Call <job_control_close>
 *
 */
void on_job_control_close(void)
{
	job_control_close();
}

/*
 * Function: on_job_control_clear
 * Call <job_control_clear>
 *
 */
void on_job_control_clear(void)
{
	job_control_clear();
}

/*
 * Function: on_job_control_stop
 * Call <job_control_stop>
 *
 */
void on_job_control_stop(void)
{
	job_control_stop();
}

/*
 * Function: on_configure_preferences_activate
 * *Fill me in!*
 *
 */
void on_configure_preferences_activate(void)
{
	preferences_setup_ui(FALSE);
}

/* Function: on_configure_servers_activate
 * Show servers's configuration dialog
 */
void on_configure_servers_activate(void)
{
	server_list_show(gebr.ui_server_list);
}

/* Function: on_help_contents_activate
 * *Fill me in!*
 */
void on_help_contents_activate(void)
{
	gebr_gui_help_show(GEBR_USERDOC_HTML, gebr_geoxml_document_get_title(document_get_current()),
			   gebr.config.browser->str);
}

/* Function: on_help_about_activate
 * Show about dialog
 */
void on_help_about_activate(void)
{
	gtk_widget_show_all(gebr.about.dialog);
}

/*
 * Function: navigation_bar_update
 * 
 */
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
