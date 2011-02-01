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


#define HTML_WINDOW_OBJECT "html-window-object"
#define CSS_BASENAME "css-basename"

//==============================================================================
// PROTOTYPES AND STATIC VARIABLES					       =
//==============================================================================
static void on_save_activate(GtkAction * action, GebrGuiHelpEditWidget * self);

static GtkWidget * create_help_edit_window(GebrGeoXmlDocument * document);

static void on_help_edit_window_destroy(GtkWidget * widget, gpointer user_data);

static void on_title_ready(GebrGuiHelpEditWidget * widget, const gchar * title, GtkWindow * window);

static void on_include_comment_activate (GtkToggleAction *action, GebrGuiHtmlViewerWindow *window);

static void on_include_flows_report_activate (GtkToggleAction *action, GebrGuiHtmlViewerWindow *window);

static void on_ptbl_changed (GtkRadioAction *action, GtkRadioAction *current, GebrGuiHtmlViewerWindow *window);

static void on_style_action_changed (GtkAction *action, GtkAction *current, GebrGuiHtmlViewerWindow *window);

static const GtkActionEntry action_entries[] = {
	{"SaveAction", GTK_STOCK_SAVE, NULL, NULL,
		N_("Save the current document"), G_CALLBACK(on_save_activate)},
};
static guint n_action_entries = G_N_ELEMENTS(action_entries);

static const GtkActionEntry html_viewer_entries[] = {
	{"OptionsMenu", NULL, N_("_Options")},
	{"ParameterTableMenu", NULL, N_("Parameter table")},
	{"StyleMenu", NULL, N_("_Style")},

};
static guint n_html_viewer_entries = G_N_ELEMENTS (html_viewer_entries);

static const GtkToggleActionEntry html_viewer_toggle_entries[] = {
	{"IncludeCommentAction", NULL, N_("_Include user's commentary"), NULL,
		NULL, G_CALLBACK (on_include_comment_activate), TRUE},

	{"IncludeFlowsReportAction", NULL, N_("Include _flows report"), NULL,
		NULL, G_CALLBACK (on_include_flows_report_activate), TRUE},
};
static guint n_html_viewer_toggle_entries = G_N_ELEMENTS (html_viewer_toggle_entries);

static const GtkRadioActionEntry html_viewer_radio_entries[] = {
	{"NoTableAction", NULL, N_("No table at all"),
		NULL, NULL, GEBR_PARAM_TABLE_NO_TABLE},
	{"OnlyChangedAction", NULL, N_("Just parameters which differ from default"),
		NULL, NULL, GEBR_PARAM_TABLE_ONLY_CHANGED},
	{"OnlyFilledAction", NULL, N_("Just filled in parameters"),
		NULL, NULL, GEBR_PARAM_TABLE_ONLY_FILLED},
	{"AllAction", NULL, N_("All parameters"),
		NULL, NULL, GEBR_PARAM_TABLE_ALL},
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
"   <separator />"
"   <menu action='StyleMenu'>"
"    <menuitem action='StyleNoneAction' />"
"   </menu>"
"  </menu>"
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
		title = g_strdup_printf (_("Comments of the %s \"%s\""), document_type, doc_title);
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

static void on_include_comment_activate (GtkToggleAction *action, GebrGuiHtmlViewerWindow *window)
{
	gboolean flag;
	gchar *report;
	GebrGeoXmlObject *object;
	GebrGeoXmlObjectType type;

	flag = gtk_toggle_action_get_active (action);
	object = g_object_get_data (G_OBJECT (window), HTML_WINDOW_OBJECT);
	type = gebr_geoxml_object_get_type (object);

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		gebr.config.detailed_flow_include_report = flag;
	else
		gebr.config.detailed_line_include_report = flag;

	report = gebr_document_generate_report (GEBR_GEOXML_DOCUMENT (object));
	gebr_gui_html_viewer_window_show_html (window, report);
	g_free (report);
}

static void on_include_flows_report_activate (GtkToggleAction *action, GebrGuiHtmlViewerWindow *window)
{
	gchar *report;
	gboolean flag;
	GebrGeoXmlObject *object;
	GtkAction *action_param_table;
	GtkUIManager *manager;
	const gchar *path_param_table;

	object = g_object_get_data (G_OBJECT (window), HTML_WINDOW_OBJECT);
	flag = gtk_toggle_action_get_active (action);
	gebr.config.detailed_line_include_flow_report = flag;

	manager = gebr_gui_html_viewer_window_get_ui_manager (window);

	path_param_table = "/" GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "/OptionsMenu/ParameterTableMenu";
	action_param_table = gtk_ui_manager_get_action(manager, path_param_table);
	gtk_action_set_sensitive(action_param_table, gebr.config.detailed_line_include_flow_report);

	report = gebr_document_generate_report (GEBR_GEOXML_DOCUMENT (object));
	gebr_gui_html_viewer_window_show_html (window, report);
	g_free (report);
}

static void on_ptbl_changed (GtkRadioAction *action, GtkRadioAction *current, GebrGuiHtmlViewerWindow *window)
{
	gint flag;
	gchar *report;
	GebrGeoXmlObject *object;
	GebrGeoXmlObjectType type;

	flag = gtk_radio_action_get_current_value (current);
	object = g_object_get_data (G_OBJECT (window), HTML_WINDOW_OBJECT);
	type = gebr_geoxml_object_get_type (object);

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		gebr.config.detailed_flow_parameter_table = flag;
	else
		gebr.config.detailed_line_parameter_table = flag;

	report = gebr_document_generate_report (GEBR_GEOXML_DOCUMENT (object));
	gebr_gui_html_viewer_window_show_html (window, report);
	g_free (report);
}

static void on_style_action_changed (GtkAction *action, GtkAction *current, GebrGuiHtmlViewerWindow *window)
{
	gchar *fname;
	gchar *report;
	GebrGeoXmlObject *object;
	GebrGeoXmlObjectType type;

	object = g_object_get_data (G_OBJECT (window), HTML_WINDOW_OBJECT);
	type = gebr_geoxml_object_get_type (object);
	fname = g_object_get_data (G_OBJECT (current), CSS_BASENAME);

	if (!fname)
		fname = "";

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		g_string_assign (gebr.config.detailed_flow_css, fname);
	} else {
		g_string_assign (gebr.config.detailed_line_css, fname);
	}

	report = gebr_document_generate_report (GEBR_GEOXML_DOCUMENT (object));
	gebr_gui_html_viewer_window_show_html (window, report);
	g_free (report);
}

//==============================================================================
// PUBLIC METHODS 							       =
//==============================================================================
void gebr_help_show_selected_program_help(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter)) {
		return;
	}
	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.program), FALSE);
}

void gebr_help_show(GebrGeoXmlObject *object, gboolean menu)
{
	const gchar * html;
	GtkWidget * window;
	GebrGuiHtmlViewerWidget * html_viewer_widget;
	GebrGeoXmlObjectType type;

	type = gebr_geoxml_object_get_type (object);
	window = gebr_gui_html_viewer_window_new (); 
	g_object_set_data (G_OBJECT (window), HTML_WINDOW_OBJECT, object);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);

	html_viewer_widget = gebr_gui_html_viewer_window_get_widget(GEBR_GUI_HTML_VIEWER_WINDOW(window));

	g_object_set_data (G_OBJECT (html_viewer_widget), "geoxml-object", object);
	g_signal_connect (html_viewer_widget, "title-ready", G_CALLBACK (on_title_ready), window);

	if (menu) {
		gebr_gui_html_viewer_widget_generate_links(html_viewer_widget, object);
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));
		gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);
	}
	else switch (type) {
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
	case GEBR_GEOXML_OBJECT_TYPE_LINE: {
		GDir *dir;
		gchar *str;
		gint merge_id;
		GtkUIManager *manager;
		GError *error = NULL;
		GtkActionGroup *group;
		GtkAction *action;
		GebrGuiHtmlViewerWindow *html_window;

		html_window = GEBR_GUI_HTML_VIEWER_WINDOW (window);
		manager = gebr_gui_html_viewer_window_get_ui_manager (html_window);
		group = gtk_action_group_new ("HtmlViewerGroup");
		gtk_action_group_set_translation_domain(group, GETTEXT_PACKAGE);

		gtk_action_group_add_actions (group, html_viewer_entries,
					      n_html_viewer_entries, window);

		gtk_action_group_add_toggle_actions (group, html_viewer_toggle_entries,
						     n_html_viewer_toggle_entries, window);

		gtk_action_group_add_radio_actions (group, html_viewer_radio_entries,
						    n_html_viewer_radio_entries,
						    0, G_CALLBACK (on_ptbl_changed), window);

		gint i = 1;
		GtkRadioAction *radio_action;
		GSList *style_group = NULL;

		radio_action = gtk_radio_action_new ("StyleNoneAction", _("None"), NULL, NULL, 0);
		gtk_radio_action_set_group (radio_action, style_group);
		style_group = gtk_radio_action_get_group (radio_action);
		g_signal_connect (radio_action, "changed", G_CALLBACK (on_style_action_changed), window);
		gtk_action_group_add_action (group, GTK_ACTION (radio_action));
		dir = g_dir_open (GEBR_STYLES_DIR, 0, &error);

		if (error) {
			g_critical ("%s", error->message);
			g_clear_error (&error);
		} else {
			const gchar *fname;
			fname = g_dir_read_name (dir);
			while (fname != NULL) {
				if (fnmatch ("*.css", fname, 1) == 0) {
					gchar *action_name;
					gchar *css_title;
					gchar *abs_path;
					gchar *fname_cpy;

					action_name = g_strdup_printf ("StyleAction%d", i);
					abs_path = g_strconcat (GEBR_STYLES_DIR, "/", fname, NULL);
					css_title = gebr_document_get_css_header_field (abs_path, "title");
					radio_action = gtk_radio_action_new (action_name,
									     css_title,
									     NULL, NULL, i);
					fname_cpy = g_strdup (fname);
					g_object_set_data (G_OBJECT (radio_action), CSS_BASENAME, fname_cpy);
					g_object_weak_ref (G_OBJECT (radio_action), (GWeakNotify)g_free, fname_cpy);
					gtk_radio_action_set_group (radio_action, style_group);
					style_group = gtk_radio_action_get_group (radio_action);
					gtk_action_group_add_action (group, GTK_ACTION (radio_action));
					g_free (action_name);

					if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
						if (g_strcmp0 (fname, gebr.config.detailed_line_css->str) == 0)
							gtk_radio_action_set_current_value (radio_action, i);
					} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
						if (g_strcmp0 (fname, gebr.config.detailed_flow_css->str) == 0)
							gtk_radio_action_set_current_value (radio_action, i);
					}
					i++;
				}
				fname = g_dir_read_name (dir);
			}
			g_dir_close (dir);
		}

		gtk_ui_manager_insert_action_group (manager, group, 0);
		merge_id = gtk_ui_manager_add_ui_from_string (manager, html_viewer_ui_def, -1, &error);

		// Merge style actions
		for (int k = 1; k < i; k++) {
			gchar *name;
			const gchar *path;

			path = "/" GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "/OptionsMenu/StyleMenu";
			name = g_strdup_printf ("StyleAction%d", k);

			gtk_ui_manager_add_ui (manager, merge_id, path, name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
			g_free (name);
		}

		if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
			const gchar *name;
			const gchar *path;
			const gchar *path_param_table;

			path = "/" GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "/OptionsMenu/IncludeCommentAction";
			name = "IncludeFlowsReportAction";
			gtk_ui_manager_add_ui (manager, merge_id, path, name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

			action = gtk_action_group_get_action (group, "IncludeCommentAction");
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), gebr.config.detailed_line_include_report);
			action = gtk_action_group_get_action (group, "IncludeFlowsReportAction");
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), gebr.config.detailed_line_include_flow_report);

			path_param_table = "/" GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "/OptionsMenu/ParameterTableMenu";
			action = gtk_ui_manager_get_action(manager, path_param_table);
			gtk_action_set_sensitive(action, gebr.config.detailed_line_include_flow_report);

			action = gtk_action_group_get_action (group, "NoTableAction");
			gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), gebr.config.detailed_line_parameter_table);
		} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
			action = gtk_action_group_get_action (group, "IncludeCommentAction");
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), gebr.config.detailed_flow_include_report);
			action = gtk_action_group_get_action (group, "NoTableAction");
			gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), gebr.config.detailed_flow_parameter_table);
		}

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
	case GEBR_GEOXML_OBJECT_TYPE_PROJECT:
		html = gebr_geoxml_document_get_help (GEBR_GEOXML_DOCUMENT (object));
		gebr_gui_html_viewer_window_show_html (GEBR_GUI_HTML_VIEWER_WINDOW (window), html);
		break;
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
