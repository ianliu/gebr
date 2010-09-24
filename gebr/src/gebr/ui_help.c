/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
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

/**
 * \file ui_help.c
 * Responsible for help/report exibition and edition
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <glib.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui.h>

#include "gebr-help-edit-widget.c"
#include "ui_help.h"
#include "gebr.h"
#include "document.h"
#include "../defines.h"
#include "menu.h"
#include "flow.h"
#include "ui_project_line.h"

//==============================================================================
// PROTOTYPES AND STATIC VARIABLES					       =
//==============================================================================
static void on_save_activate(GtkAction * action, GebrGuiHelpEditWidget * self);

static GtkWidget * create_help_edit_window(GebrGeoXmlDocument * document);

static void on_help_edit_window_destroy(GtkWidget * widget, gpointer user_data);

static const GtkActionEntry action_entries[] = {
	{"SaveAction", GTK_STOCK_SAVE, NULL, NULL,
		N_("Save the current document"), G_CALLBACK(on_save_activate)},
};

static guint n_action_entries = G_N_ELEMENTS(action_entries);

//==============================================================================
// PRIVATE METHODS 							       =
//==============================================================================
	static GtkWidget *
create_help_edit_window(GebrGeoXmlDocument * document)
{
	guint merge_id;
	const gchar * help;
	const gchar * filemenu;
	const gchar * mark;
	const gchar * document_type;
	GtkWidget * window;
	GtkWidget * widget;
	GtkUIManager * ui_manager;
	GtkActionGroup * action_group;
	GebrGuiHelpEditWidget * help_edit_widget;
	GebrGuiHelpEditWindow * help_edit_window;

	// Create the HelpEdit widget and window
	help = gebr_geoxml_document_get_help(document);
	widget = gebr_help_edit_widget_new(document, help);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(widget);
	window = gebr_gui_help_edit_window_new(help_edit_widget);
	help_edit_window = GEBR_GUI_HELP_EDIT_WINDOW(window);

	g_signal_connect(window, "destroy",
			 G_CALLBACK(on_help_edit_window_destroy), document);

	// Create the Save button and merge with the window UI
	switch(gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		document_type = _("flow");
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		document_type = _("line");
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		document_type = _("project");
		break;
	default:
		document_type = "";
		g_warn_if_reached();
	}

	action_group = gtk_action_group_new("GebrHelpEditButtons");
	gtk_action_group_add_actions(action_group, action_entries, n_action_entries, widget);

	ui_manager = gebr_gui_help_edit_window_get_ui_manager(help_edit_window);
	merge_id = gtk_ui_manager_new_merge_id(ui_manager);
	mark = gebr_gui_help_edit_window_get_tool_bar_mark(help_edit_window);
	filemenu = gebr_gui_help_edit_window_get_file_menu_path(help_edit_window);
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
	gtk_ui_manager_add_ui(ui_manager, merge_id, mark,
			      "SaveAction", "SaveAction", GTK_UI_MANAGER_TOOLITEM, TRUE);
	gtk_ui_manager_add_ui(ui_manager, merge_id, filemenu,
			      "SaveAction", "SaveAction", GTK_UI_MANAGER_MENUITEM, TRUE);
	g_object_unref(action_group);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);

	return window;
}

static void on_help_edit_window_destroy(GtkWidget * widget, gpointer document)
{
	g_hash_table_remove(gebr.help_edit_windows, document);
}

static void on_print_requested(GebrGuiHtmlViewerWidget * self, GebrGeoXmlDocument * document)
{
	GebrGeoXmlObjectType type;
	gchar * detailed_html;
	const gchar * title;
	const gchar * report;
	gchar * inner_body;
	gchar * styles;
	gchar * header;
	gchar * content;

	type = gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(document));
	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) 
		return ;

	title = gebr_geoxml_document_get_title(document);
	report = gebr_geoxml_document_get_help(document);
	inner_body = gebr_document_report_get_inner_body(report);

	if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
		if (gebr.config.print_option_line_use_gebr_css)
			styles = g_strdup("<link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />");
		else
			styles = gebr_document_report_get_styles_string(report);
		header = gebr_line_generate_header(document);
		content = NULL;
	} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		if (gebr.config.print_option_check_button_css)
			styles = g_strdup("<link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />");
		else
			styles = gebr_document_report_get_styles_string(report);
		header = gebr_flow_generate_header(GEBR_GEOXML_FLOW(document));

		if (gebr.config.print_option_check_button_param)
			content = gebr_flow_generate_parameter_value_table(GEBR_GEOXML_FLOW(document));
		else
			content = NULL;
	} else
		g_return_if_reached();

	gchar * real_content = g_strconcat(inner_body, content, NULL);

	if (!styles)
		styles = g_strdup("");
	detailed_html = gebr_generate_report(title, styles, header, real_content);
	gebr_gui_html_viewer_widget_show_html(self, detailed_html);

	g_free(styles);
	g_free(header);
	g_free(content);
	g_free(inner_body);
	g_free(real_content);
}

//==============================================================================
// PUBLIC METHODS 							       =
//==============================================================================
void gebr_help_show_selected_program_help(void)
{
	if (!flow_edition_get_selected_component(NULL, TRUE))
		return;

	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.program), FALSE, _("Program help"));
}

void gebr_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title)
{
	const gchar * html;
	GtkWidget * window;
	GebrGuiHtmlViewerWidget * html_viewer_widget;

	window = gebr_gui_html_viewer_window_new(title); 
	html_viewer_widget = gebr_gui_html_viewer_window_get_widget(GEBR_GUI_HTML_VIEWER_WINDOW(window));
	g_signal_connect(html_viewer_widget, "print-requested", G_CALLBACK(on_print_requested), object);

	if (menu)
		gebr_gui_html_viewer_widget_generate_links(html_viewer_widget, object);

	switch (gebr_geoxml_object_get_type(object)) {
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));
		gebr_gui_html_viewer_window_set_custom_tab(GEBR_GUI_HTML_VIEWER_WINDOW(window), _("Detailed Report"), gebr_flow_print_dialog_custom_tab);
		break;
	case GEBR_GEOXML_OBJECT_TYPE_LINE:
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));
		gebr_gui_html_viewer_window_set_custom_tab(GEBR_GUI_HTML_VIEWER_WINDOW(window), _("Detailed Report"), gebr_project_line_print_dialog_custom_tab);
		break;
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		html = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
		break;
	default:
		break;

	}
	gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);

	gtk_dialog_run(GTK_DIALOG(window));
}

void gebr_help_edit_document(GebrGeoXmlDocument * document)
{
	if (gebr.config.native_editor || gebr.config.editor->len == 0) {
		GtkWidget * window;

		window = g_hash_table_lookup(gebr.help_edit_windows, document);
		if (window != NULL)
			gtk_window_present(GTK_WINDOW(window));
		else {
			window = create_help_edit_window(document);
			g_hash_table_insert(gebr.help_edit_windows, document, window);
			gtk_widget_show(window);
		}
	} else {
		GString *prepared_html;
		GString *cmd_line;
		FILE *html_fp;
		GString *html_path;

		/* initialization */
		prepared_html = g_string_new(NULL);
		g_string_assign(prepared_html, gebr_geoxml_document_get_help(document));
		/* create temporary filename */
		html_path = gebr_make_temp_filename("gebr_XXXXXX.html");

		/* open temporary file with help from XML */
		html_fp = fopen(html_path->str, "w");
		if (html_fp == NULL) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to create temporary file."));
			goto out2;
		}
		fputs(prepared_html->str, html_fp);
		fclose(html_fp);

		cmd_line = g_string_new(NULL);
		g_string_printf(cmd_line, "%s  %s", gebr.config.editor->str, html_path->str);
		if (WEXITSTATUS(system(cmd_line->str))){
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error during editor execution."));
		}

		/* Add file to list of files to be removed */
		gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_path->str);

		html_fp = fopen(html_path->str, "r");
		if (html_fp == NULL) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to create temporary file."));
			goto out1;
		}
		gchar buffer[1000];
		g_string_assign(prepared_html, "");
		while (fgets(buffer, sizeof(buffer), html_fp))
			g_string_append(prepared_html, buffer);
		fclose(html_fp);

		gebr_geoxml_document_set_help(document, prepared_html->str);

out1:		g_string_free(cmd_line, TRUE);
out2:		g_string_free(html_path, FALSE);
		g_string_free(prepared_html, TRUE);

	}
}

static void on_save_activate(GtkAction * action, GebrGuiHelpEditWidget * self)
{
	GebrGeoXmlObject *object = NULL;

	gebr_gui_help_edit_widget_commit_changes(self);

	g_object_get(self, "geoxml-document", &object, NULL);	

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		g_object_set(gebr.ui_flow_browse->info.help_view,
			     "sensitive", strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(gebr.flow)))
			     ? TRUE : FALSE, NULL);
	else
		g_object_set(gebr.ui_project_line->info.help_view,
			     "sensitive", strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(gebr.project_line)))
			     ? TRUE : FALSE, NULL);
}

gchar * gebr_generate_report(const gchar * title,
			     const gchar * styles,
			     const gchar * header,
			     const gchar * table)
{
	static gchar * html = ""
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
		"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\">"
		"<head>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
		"<title>%s</title>"
		"%s"
		"</head>"
		"<body>"
		"<div id=\"header\">%s</div>"
		"<div id=\"content\">%s</div>"
		"</body>"
		"</html>";

	return g_strdup_printf(html, title, styles, header, table);
}
