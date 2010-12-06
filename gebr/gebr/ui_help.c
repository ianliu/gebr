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

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gui.h>

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

static void on_title_ready(GebrGuiHelpEditWidget * widget, const gchar * title, GtkWindow * window);

static void on_include_comment_activate ();

static const GtkActionEntry action_entries[] = {
	{"SaveAction", GTK_STOCK_SAVE, NULL, NULL,
		N_("Save the current document"), G_CALLBACK(on_save_activate)},
};
static guint n_action_entries = G_N_ELEMENTS(action_entries);

static const GtkActionEntry html_viewer_entries[] = {
	{"OptionsMenu", NULL, N_("_Options")},
	{"ParameterTableMenu", NULL, N_("Parameter table")},
};
static guint n_html_viewer_entries = G_N_ELEMENTS (html_viewer_entries);

static const GtkToggleActionEntry html_viewer_toggle_entries[] = {
	{"IncludeCommentAction", NULL, N_("_Include user's commentary"), NULL,
		N_("If checked, include your commentary into the report"),
		G_CALLBACK (on_include_comment_activate), TRUE},
};
static guint n_html_viewer_toggle_entries = G_N_ELEMENTS (html_viewer_toggle_entries);

typedef enum {
	PTBL_NO_TABLE,
	PTBL_ONLY_CHANGED,
	PTBL_ONLY_FILLED,
	PTBL_ALL
} ParamTable;

static const GtkRadioActionEntry html_viewer_radio_entries[] = {
	{"NoTableAction", NULL, N_("No table at all"),
		NULL, NULL, PTBL_NO_TABLE},
	{"OnlyChangedAction", NULL, N_("Just parameters which differ from default"),
		NULL, NULL, PTBL_ONLY_CHANGED},
	{"OnlyFilledAction", NULL, N_("Just filled in parameters"),
		NULL, NULL, PTBL_ONLY_FILLED},
	{"AllAction", NULL, N_("Just filled in parameters"),
		NULL, NULL, PTBL_ALL},
};
static guint n_html_viewer_radio_entries = G_N_ELEMENTS (html_viewer_radio_entries);

static const gchar *html_viewer_ui_def =
"<ui>"
" <menubar name='" GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "'>"
"  <menu action='OptionsMenu'>"
"   <menuitem action='IncludeCommentAction' />"
"   <separator />"
"   <menu action='ParameterTableMenu'>"
"    <menuitem action='NoTableAction' />"
"    <menuitem action='OnlyChangedAction' />"
"    <menuitem action='OnlyFilledAction' />"
"    <menuitem action='AllAction' />"
"   </menu>"
"  <menu>"
" </menubar>"
"</ui>";

//==============================================================================
// PRIVATE METHODS 							       =
//==============================================================================
static GtkWidget *
create_help_edit_window(GebrGeoXmlDocument * document)
{
	guint merge_id;
	gchar * title;
	const gchar * doc_title;
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

	doc_title = gebr_geoxml_document_get_title (document);
	if (strlen (doc_title))
		title = g_strdup_printf (_("Report of the %s \"%s\""), document_type, doc_title);
	else
		title = g_strdup_printf (_("Report of the %s"), document_type);
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);

	// Create the Save button and merge with the window UI
	action_group = gtk_action_group_new("GebrHelpEditButtons");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
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

static void on_title_ready(GebrGuiHelpEditWidget * widget, const gchar * title, GtkWindow * window)
{
	GString * final_title;
	const gchar * obj_name;
	const gchar * obj_title;
	GebrGeoXmlObject * object;

	if (title && strlen (title)) {
		gtk_window_set_title (window, title);
		return;
	}

	object = g_object_get_data (G_OBJECT (widget), "geoxml-object");

	switch (gebr_geoxml_object_get_type (object)) {
	case GEBR_GEOXML_OBJECT_TYPE_PROJECT:
		obj_name = _("project");
		obj_title = gebr_geoxml_document_get_title (GEBR_GEOXML_DOC (object));
		break;
	case GEBR_GEOXML_OBJECT_TYPE_LINE:
		obj_name = _("line");
		obj_title = gebr_geoxml_document_get_title (GEBR_GEOXML_DOC (object));
		break;
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
		obj_name = _("flow");
		obj_title = gebr_geoxml_document_get_title (GEBR_GEOXML_DOC (object));
		break;
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		obj_name = _("program");
		obj_title = gebr_geoxml_program_get_title (GEBR_GEOXML_PROGRAM (object));
		break;
	default:
		obj_name = "";
		obj_title = "";
		break;
	}

	final_title = g_string_new ("");

	if (strlen(obj_title) > 0)
		g_string_printf (final_title, _("Report of the %s \"%s\""), obj_name, obj_title);
	else
		g_string_printf (final_title, _("Report of the %s"), obj_name);

	gtk_window_set_title (window, final_title->str);
	g_string_free (final_title, TRUE);
}

static void on_include_comment_activate ()
{
}

//==============================================================================
// PUBLIC METHODS 							       =
//==============================================================================
void gebr_help_show_selected_program_help(void)
{
	if (!flow_edition_get_selected_component(NULL, TRUE))
		return;

	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.program), FALSE);
}

void gebr_help_show(GebrGeoXmlObject * object, gboolean menu)
{
	const gchar * html;
	GtkWidget * window;
	GebrGuiHtmlViewerWidget * html_viewer_widget;

	window = gebr_gui_html_viewer_window_new(); 
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);

	html_viewer_widget = gebr_gui_html_viewer_window_get_widget(GEBR_GUI_HTML_VIEWER_WINDOW(window));

	g_object_set_data (G_OBJECT (html_viewer_widget), "geoxml-object", object);
	g_signal_connect (html_viewer_widget, "title-ready", G_CALLBACK (on_title_ready), window);

	if (menu) {
		gebr_gui_html_viewer_widget_generate_links(html_viewer_widget, object);
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));
		gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);
	}
	else switch (gebr_geoxml_object_get_type(object)) {
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
	case GEBR_GEOXML_OBJECT_TYPE_LINE:
	case GEBR_GEOXML_OBJECT_TYPE_PROJECT: {
		gchar *str;
		GtkWidget *manager;
		GError *error = NULL;
		GtkActionGroup *group;
		GebrGuiHtmlViewerWindow *html_window;

		html_window = GEBR_GUI_HTML_VIEWER_WINDOW (window);
		manager = gebr_gui_html_viewer_window_get_ui_manager (html_window);
		group = gtk_action_group_new ("HtmlViewerGroup");
		gtk_action_group_add_actions (group, html_viewer_entries,
					      n_html_viewer_entries, NULL);

		gtk_action_group_add_toggle_actions (group, html_viewer_toggle_entries,
						     n_html_viewer_toggle_entries, NULL);

		gtk_action_group_add_radio_actions (group, html_viewer_radio_entries,
						    n_html_viewer_radio_entries,
						    0, on_ptbl_changed, NULL);

		gtk_ui_manager_insert_action_group (manager, group, 0);
		gtk_ui_manager_add_ui_from_string (manager, html_viewer_ui_def, -1, &error);
		g_object_unref (group);

		if (error) {
			g_critical ("%s", error->message);
			g_clear_error (&error);
		}

		str = gebr_document_generate_report (GEBR_GEOXML_DOCUMENT (object));
		gebr_gui_html_viewer_window_show_html (GEBR_GUI_HTML_VIEWER_WINDOW (window), str);
		g_free (str);
		break;
	}
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		html = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
		gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);
		break;
	default:
		g_return_if_reached ();
	}

	gtk_widget_show (window);
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
		GString *prepared_html = g_string_new(NULL);
		GString *html_path = gebr_make_temp_filename("gebr_XXXXXX.html");
		FILE *html_fp;

		/* open temporary file with help from XML */
		html_fp = fopen(html_path->str, "w");
		if (html_fp == NULL) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to create temporary file."));
			goto out;
		}
		g_string_assign(prepared_html, gebr_geoxml_document_get_help(document));
		fputs(prepared_html->str, html_fp);
		fclose(html_fp);

		gchar *quote = g_shell_quote(html_path->str);
		if (gebr_system("%s %s", gebr.config.editor->str, quote)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error during editor execution."));
		}
		g_free(quote);

		/* Add file to list of files to be removed */
		gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_path->str);

		html_fp = fopen(html_path->str, "r");
		if (html_fp == NULL) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to create temporary file."));
			goto out;
		}
		gchar buffer[1000];
		g_string_assign(prepared_html, "");
		while (fgets(buffer, sizeof(buffer), html_fp))
			g_string_append(prepared_html, buffer);
		fclose(html_fp);

		gebr_help_set_on_xml(document, prepared_html->str);

		/* The html_path->str is not freed here since this
		   responsability is passed to gebr.tempfiles list.

		   This is a BAD practice and should be avoided.
		   */
out:		g_string_free(html_path, FALSE);
		g_string_free(prepared_html, TRUE);
	}
}

void gebr_help_set_on_xml(GebrGeoXmlDocument *document, const gchar *help)
{
	gebr_geoxml_document_set_help(document, help);
	document_save(document, TRUE, TRUE);

	GtkWidget * widget;
	if (gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(document)) == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		widget = gebr.ui_flow_browse->info.help_view;
	else
		widget = gebr.ui_project_line->info.help_view;

	g_object_set(widget, "sensitive", strlen(help) ? TRUE : FALSE, NULL);
}

static void on_save_activate(GtkAction * action, GebrGuiHelpEditWidget * self)
{
	gebr_gui_help_edit_widget_commit_changes(self);
}

gchar * gebr_generate_report(const gchar * title,
			     const gchar * styles,
			     const gchar * header,
			     const gchar * table)
{
	static gchar * html = ""
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
		"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
		"<head>\n"
		"  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
		"  <title>%s</title>\n"
		"  %s\n"
		"</head>\n"
		"<body>\n"
		"<div class=\"header\">%s</div>\n"
		"%s\n"
		"</body>\n"
		"</html>";

	return g_strdup_printf(html, title, styles, header, table);
}
