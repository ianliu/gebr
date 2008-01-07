/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://gebr.sourceforge.net)
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

/*
 * Function: switch_page
 * Hide/Show the corresponding menus to the selected page
 *
 */
void
switch_page(GtkNotebook * notebook, GtkNotebookPage * page, guint page_num)
{
	switch (page_num) {
	case 0: /* Project page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	case 1: /* Flow browse page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	case 2: /* Flow edit page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", TRUE, NULL);
		break;
	case 3: /* Job control page */
		g_object_set(gebr.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
		g_object_set(gebr.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
		break;
	default:
		break;
	}
}

/*
 * Function: on_project_new_activate
 * Call <project_new> from <project.c>
 *
 */
void
on_project_new_activate(void)
{
	project_new();
}

/*
 * Function: on_project_delete_activate
 * Call <project_delete> from <project.c>
 *
 */
void
on_project_delete_activate(void)
{
	project_delete();
}

/*
 * Function: on_project_properties_activate
 * *Fill me in!*
 *
 */
void
on_project_properties_activate(void)
{
	document_properties_setup_ui(GEOXML_DOC(gebr.project));
}

/*
 * Function: on_project_refresh_activate
 * Call <project_list_populate> from <project.c>
 *
 */
void
on_project_refresh_activate(void)
{
	project_list_populate();
}

/*
 * Function: on_line_new_activate
 * Call <line_new> from <line.c>
 *
 */
void
on_line_new_activate(void)
{
	line_new();
}

/*
 * Function: on_line_delete_activate
 * Call <line_delete> from <line.c>
 *
 */
void
on_line_delete_activate(void)
{
	line_delete();
}

/*
 * Function: on_line_properties_activate
 * *Fill me in!*
 *
 */
void
on_line_properties_activate(void)
{
	document_properties_setup_ui(GEOXML_DOC(gebr.line));
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
	flow_io_setup_ui();
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
 * Function: on_flow_component_properties_activate
 * *Fill me in!*
 *
 */
void
on_flow_component_properties_activate(void)
{
	flow_edition_component_change_parameters();
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
on_flow_component_status_activate(GtkMenuItem * menuitem)
{
	flow_edition_set_status(menuitem);
}

/*
 * Function: on_configure_preferences_activate
 * *Fill me in!*
 *
 */
void
on_configure_preferences_activate(void)
{
	preferences_setup_ui();
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
