/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include <sys/wait.h>
#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>

#include "help.h"
#include "debr.h"
#include "defines.h"
#include "debr-help-edit-widget.h"

//==============================================================================
// PROTOTYPES AND STATIC VARIABLES					       =
//==============================================================================
static void add_program_parameter_item(GString * str, GebrGeoXmlParameter * par);

static void help_insert_parameters_list(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_subst_fields(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_edit_on_finished(GebrGeoXmlObject * object, const gchar * _help);

static gsize strip_block(GString * buffer, const gchar * tag);

static void help_edit_on_commit_request(GebrGuiHelpEditWidget * self);

static void help_edit_window_on_destroy(GtkWidget * window);

static void help_edit_on_jump_to_activate(GtkAction * action, GebrGuiHelpEditWindow * window);

static void help_edit_on_refresh(GtkAction * action, GebrGuiHelpEditWindow * window);

static void help_edit_on_revert(GtkAction * action, GebrGuiHelpEditWindow * window);

static void help_edit_on_content_loaded(GebrGuiHelpEditWidget * self, GebrGuiHelpEditWindow * window);

static gchar * generate_help_from_template(GebrGeoXmlObject * object);

static void on_preview_toggled(GtkToggleAction * action, GtkUIManager * manager);

static void on_title_ready (GebrGuiHtmlViewerWidget * widget, const gchar * title, GtkWindow * window);

static const GtkActionEntry action_entries[] = {
	{"JumpToMenu", NULL, N_("_Jump To"), NULL, NULL,
		G_CALLBACK(help_edit_on_jump_to_activate)},

	{"RefreshAction", GTK_STOCK_REFRESH, NULL, NULL,
		N_("Update editor's content with data from menu"), G_CALLBACK(help_edit_on_refresh)},

	{"RevertAction", GTK_STOCK_REVERT_TO_SAVED, NULL, NULL,
		N_("Revert help content to the saved state"), G_CALLBACK(help_edit_on_revert)}
};

static const guint n_action_entries = G_N_ELEMENTS(action_entries);

static const gchar * ui_def = 
"<ui>"
" <menubar name='" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "'>"
"  <menu action='JumpToMenu' />"
" </menubar>"
" <toolbar name='" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "'>"
"  <toolitem action='RefreshAction' position='top' />"
"  <toolitem action='RevertAction' position='top' />"
" </toolbar>"
"</ui>";

#define JUMP_TO_ACTION_GROUP "jump-to-action-group"
#define JUMP_TO_MERGE_ID "jump-to-merge-id"

//==============================================================================
// PRIVATE METHODS							       =
//==============================================================================
static void merge_ui_def(GebrGuiHelpEditWindow * window, gboolean revert_visible)
{
	GtkActionGroup * action_group;
	GtkUIManager * ui_manager;
	GtkAction * action;
	GError * error = NULL;
	gchar * path;

	ui_manager = gebr_gui_help_edit_window_get_ui_manager(window);

	action_group = gtk_action_group_new("DebrHelpEditGroup");
	gtk_action_group_add_actions(action_group, action_entries, n_action_entries, window);
	action = gtk_action_group_get_action(action_group, "JumpToMenu");
	g_object_set(action, "hide-if-empty", FALSE, NULL);
	action = gtk_action_group_get_action(action_group, "RevertAction");
	gtk_action_set_sensitive(action, revert_visible);

	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
	gtk_ui_manager_add_ui_from_string(ui_manager, ui_def, -1, &error);
	g_object_unref(action_group);

	path = g_strconcat (gebr_gui_help_edit_window_get_tool_bar_mark (window),
			    "/PreviewAction",
			    NULL);
	action = gtk_ui_manager_get_action (ui_manager, path);
	g_signal_connect (action, "toggled", G_CALLBACK (on_preview_toggled), ui_manager);
	g_free (path);

	if (error != NULL) {
		g_warning("%s\n", error->message);
		g_clear_error(&error);
	}
}

static void create_help_edit_window(GebrGeoXmlObject * object, const gchar * help)
{
	gchar * title;
	gchar * help_backup;
	gboolean is_menu_selected;
	const gchar * object_title;
	GtkTreeIter iter;
	GebrGuiHelpEditWidget * help_edit_widget;
	GtkWidget * help_edit_window;

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		help_backup = debr_program_get_backup_help_from_pointer(object);
		object_title = gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(object));
		title = g_strdup_printf(_("Program: %s"), object_title);
	} else {
		help_backup = debr_menu_get_backup_help_from_pointer(object);
		object_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(object));
		title = g_strdup_printf(_("Menu: %s"), object_title);
	}

	is_menu_selected = menu_get_selected(&iter, TRUE);
	help_edit_widget = debr_help_edit_widget_new(object, help, help_backup != NULL);
	help_edit_window = gebr_gui_help_edit_window_new(help_edit_widget);
	gebr_gui_help_edit_window_set_auto_save(GEBR_GUI_HELP_EDIT_WINDOW(help_edit_window), TRUE);
	merge_ui_def(GEBR_GUI_HELP_EDIT_WINDOW(help_edit_window), help_backup != NULL);
	g_free(help_backup);

	g_signal_connect (help_edit_window, "destroy",
			  G_CALLBACK (help_edit_window_on_destroy),
			  NULL);
	g_signal_connect (help_edit_widget, "commit-request",
			  G_CALLBACK (help_edit_on_commit_request),
			  NULL);
	g_signal_connect (help_edit_widget, "content-loaded",
			  G_CALLBACK (help_edit_on_content_loaded),
			  help_edit_window);

	g_hash_table_insert(debr.help_edit_windows, object, help_edit_window);
	gtk_window_set_title(GTK_WINDOW(help_edit_window), title);
	gtk_window_set_default_size(GTK_WINDOW(help_edit_window), 600, 500);
	gtk_widget_show(help_edit_window);
	g_free(title);
}

static void help_edit_on_refresh(GtkAction * action, GebrGuiHelpEditWindow * window)
{
	GebrGeoXmlObject * object;
	GebrGuiHelpEditWidget * widget;
	DebrHelpEditWidget * dwidget;
	gchar * help_content;
	GString * help_string;

	gtk_action_set_sensitive (action, FALSE);
	g_object_get(window, "help-edit-widget", &widget, NULL);
	g_object_get(widget, "geoxml-object", &object, NULL);
	dwidget = DEBR_HELP_EDIT_WIDGET (widget);

	if (gebr_geoxml_object_get_type(object) != GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		object = NULL;

	/* If there is no content, we must regenerate the template file.
	 */
	if (debr_help_edit_widget_is_content_empty (dwidget)) {
		help_content = generate_help_from_template(object);
		help_string = g_string_new(help_content);
	} else {
		help_content = gebr_gui_help_edit_widget_get_content(widget);
		help_string = g_string_new(help_content);
		help_subst_fields(help_string, GEBR_GEOXML_PROGRAM(object), TRUE);
	}
	gebr_gui_help_edit_widget_set_content(widget, help_string->str);
	g_free(help_content);
	g_string_free(help_string, TRUE);
}

static GtkAction * insert_jump_to_action(GebrGeoXmlObject * object,
					 GebrGuiHelpEditWindow * window,
					 GtkActionGroup * action_group,
					 GtkUIManager * ui_manager,
					 const gchar * jump_to_path,
					 guint merge_id)
{
	gchar * label;
	const gchar * title;
	GtkAction * action;

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		title = gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(object));
		label = g_strdup_printf(_("Program: %s"), title);
	} else {
		title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(object));
		label = g_strdup_printf(_("Menu: %s"), title);
	}

	action = g_object_new(GTK_TYPE_ACTION,
			      "name", label,
			      "label", label,
			      NULL);

	gtk_action_group_add_action(action_group, action);
	g_signal_connect_swapped(action, "activate", G_CALLBACK(debr_help_edit), object);
	gtk_ui_manager_add_ui(ui_manager, merge_id, jump_to_path,
			      label, label, GTK_UI_MANAGER_MENUITEM, FALSE);

	g_free(label);

	return action;
}

static void create_jump_to_menu(GebrGeoXmlObject * object, GebrGuiHelpEditWindow * window)
{
	guint merge_id;
	GtkAction * action;
	GtkUIManager * manager;
	GtkActionGroup * group;
	GebrGeoXmlFlow * flow;
	GebrGeoXmlSequence * program;
	gchar * jumpto_path;

	manager = gebr_gui_help_edit_window_get_ui_manager(window);

	// Remove old action group and merge_id from the window and insert the new ones
	//
	group = g_object_get_data(G_OBJECT(window), JUMP_TO_ACTION_GROUP);
	merge_id = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(window), JUMP_TO_MERGE_ID));
	if (group != NULL) {
		gtk_ui_manager_remove_action_group(manager, group);
		gtk_ui_manager_remove_ui(manager, merge_id);
	}
	group = gtk_action_group_new(JUMP_TO_ACTION_GROUP);
	merge_id = gtk_ui_manager_new_merge_id(manager);

	g_object_set_data(G_OBJECT(window), JUMP_TO_ACTION_GROUP, group);
	g_object_set_data(G_OBJECT(window), JUMP_TO_MERGE_ID, GUINT_TO_POINTER(merge_id));

	// Calculates the path for JumpToMenu
	//
	jumpto_path = g_strconcat(gebr_gui_help_edit_window_get_menu_bar_path(window), "/JumpToMenu", NULL);

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		flow = GEBR_GEOXML_FLOW(gebr_geoxml_object_get_owner_document(object));
	else
		flow = GEBR_GEOXML_FLOW(object);

	gtk_ui_manager_insert_action_group(manager, group, 0);
	action = insert_jump_to_action(GEBR_GEOXML_OBJECT(flow), window, group,
				       manager, jumpto_path, merge_id);

	if (GEBR_GEOXML_OBJECT(flow) == object)
		gtk_action_set_sensitive(action, FALSE);

	gtk_ui_manager_add_ui(manager, merge_id, jumpto_path,
			      "JumpToSep", NULL, GTK_UI_MANAGER_SEPARATOR, FALSE);

	gebr_geoxml_flow_get_program(flow, &program, 0);
	while (program) {
		action = insert_jump_to_action(GEBR_GEOXML_OBJECT(program), window, group,
					       manager, jumpto_path, merge_id);
		if (GEBR_GEOXML_OBJECT(program) == object)
			gtk_action_set_sensitive(action, FALSE);
		gebr_geoxml_sequence_next(&program);
	}

	g_free(jumpto_path);
}

static void add_program_parameter_item(GString * str, GebrGeoXmlParameter * par)
{
	if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(par))) {
		g_string_append_printf(str, "<li><span class=\"reqlabel\">%s</span><br/>",
				       gebr_geoxml_parameter_get_label(par));
	} else {
		g_string_append_printf(str, "<li><span class=\"label\">%s</span><br/>"
				       " detailed description comes here.",
				       gebr_geoxml_parameter_get_label(par));
	}
	if (gebr_geoxml_parameter_get_type(par) == GEBR_GEOXML_PARAMETER_TYPE_ENUM) {
		GebrGeoXmlSequence *enum_option;

		g_string_append_printf(str, "\n<ul>");
		gebr_geoxml_program_parameter_get_enum_option(GEBR_GEOXML_PROGRAM_PARAMETER(par), &enum_option, 0);
		for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option))
			g_string_append_printf(str, "<li>%s</li>", gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)));
		g_string_append_printf(str, "\n</ul>");
	}
	g_string_append_printf(str, "</li>\n");
}

static void help_insert_parameters_list(GString * help, GebrGeoXmlProgram * program, gboolean refresh)
{
	GString *label;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;
	gchar *ptr;
	gsize pos;

	if (program == NULL)
		return;

	parameters = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);

	label = g_string_new(NULL);
	g_string_assign(label, "<ul>\n");
	while (parameter != NULL) {

		if (gebr_geoxml_parameter_get_is_program_parameter(GEBR_GEOXML_PARAMETER(parameter)) == TRUE) {
			add_program_parameter_item(label, GEBR_GEOXML_PARAMETER(parameter));
		} else {
			GebrGeoXmlParameters *template;
			GebrGeoXmlSequence *subpar;

			g_string_append_printf(label, "<li class=\"group\"><span class=\"grouplabel\">%s</span><br/>"
					       " detailed description comes here.\n\n",
					       gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)));

			g_string_append_printf(label, "<ul>\n");

			template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(parameter));
			subpar = gebr_geoxml_parameters_get_first_parameter(template);
			while (subpar != NULL) {
				add_program_parameter_item(label, GEBR_GEOXML_PARAMETER(subpar));
				gebr_geoxml_sequence_next(&subpar);
			}
			g_string_append_printf(label, "</ul></li>\n\n");

		}

		gebr_geoxml_sequence_next(&parameter);
	}
	g_string_append(label, "</ul>\n");

	if (refresh) {
		ptr = strstr(help->str, "<!-- end lst -->");
		pos = (ptr != NULL) ? (ptr - help->str) / sizeof(gchar) : 0;
	} else {
		pos = strip_block(help, "lst");
	}

	if (pos > 0) {
		g_string_insert(help, pos, label->str);
	} else {
		/* Try to find a parameter's block, for */
		/* helps generated before this function */
		GString *mark;
		gchar *ptr1;
		gchar *ptr2;
		gsize pos;

		mark = g_string_new(NULL);

		g_string_printf(mark, "<div class=\"parameters\">");
		ptr1 = strstr(help->str, mark->str);

		if (ptr1 != NULL) {
			g_string_printf(mark, "</div>");
			ptr2 = strstr(ptr1, mark->str);
			pos = (ptr2 - help->str) / sizeof(gchar);
			g_string_insert(help, pos, label->str);
		} else {
			debr_message(GEBR_LOG_WARNING, "Unable to reinsert parameter's list");
		}
	}

	g_string_free(label, TRUE);
}

static void help_subst_fields(GString * help, GebrGeoXmlProgram * program, gboolean refresh)
{
	gchar *content;
	gchar *escaped_content;
	GString *text;
	gsize pos;

	text = g_string_new(NULL);

	/* Title replacement */
	if (program != NULL)
		content = (gchar *) gebr_geoxml_program_get_title(program);
	else
		content = (gchar *) gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu));

	escaped_content = g_markup_escape_text((const gchar *) content, -1);

	if (strlen(escaped_content)) {
		pos = strip_block(help, "ttl");
		if (pos) {
			g_string_printf(text, "\n  <title>G&ecirc;BR - %s</title>\n  ", escaped_content);
			g_string_insert(help, pos, text->str);
		}

		pos = strip_block(help, "tt2");
		if (pos) {
			g_string_printf(text, "\n         <span class=\"flowtitle\">%s</span>\n         ", escaped_content);
			g_string_insert(help, pos, text->str);
		}
	}

	g_free(escaped_content);

	/* Description replacement */
	if (program != NULL)
		content = (gchar *) gebr_geoxml_program_get_description(program);
	else
		content = (gchar *) gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(debr.menu));
	
	escaped_content = g_markup_escape_text((const gchar *) content, -1);
	
	if (strlen(escaped_content)) {
		pos = strip_block(help, "des");
		if (pos) {
			g_string_printf(text, "\n            %s\n            ", escaped_content);
			g_string_insert(help, pos, text->str);
		}
	}
	
	g_free(escaped_content);

	/* Categories replacement */
	GebrGeoXmlSequence *category;
	GString *catstr;

	catstr = g_string_new("");
	pos = strip_block(help, "cat");
	if (pos) {
		gebr_geoxml_flow_get_category(debr.menu, &category, 0);
		if (category) {
			content = (gchar *) gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category));
			escaped_content = g_markup_escape_text(content, -1);
			g_string_append(catstr, escaped_content);
			g_free(escaped_content);
			gebr_geoxml_sequence_next(&category);
		}
		while (category != NULL) {
			g_string_append(catstr, " | ");
			content = (gchar *) gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category));
			escaped_content = g_markup_escape_text(content, -1);
			g_string_append(catstr, escaped_content);
			g_free(escaped_content);
			gebr_geoxml_sequence_next(&category);
		}
		g_string_insert(help, pos, catstr->str);
		g_string_free(catstr, TRUE);
	}

	pos = strip_block(help, "dtd");
	if (pos)
		g_string_insert(help, pos, gebr_geoxml_document_get_version(GEBR_GEOXML_DOCUMENT(debr.menu)));

        GDate *date;
        gchar datestr[13];
	gchar *original_locale = NULL, *new_locale = NULL;

	/* Temporarily set the current date/time locale to English. */
       	original_locale = g_strdup(setlocale(LC_TIME, NULL));
       	new_locale = g_strdup(setlocale(LC_TIME, "C"));

        date = g_date_new();
        g_date_set_time_t(date, time(NULL));
        g_date_strftime(datestr, 13, "%b %d, %Y", date);

	/* Restore the original locale. */
	setlocale(LC_TIME, original_locale);
	
	if (original_locale)
		g_free(original_locale);

	if (new_locale)
		g_free(new_locale);

	if (program != NULL) {
		pos = strip_block(help, "ver");
		if (pos)
			g_string_insert(help, pos, gebr_geoxml_program_get_version(program));
		help_insert_parameters_list(help, program, refresh);
	} else {		/* strip parameter section for flow help */
		strip_block(help, "par");
		strip_block(help, "mpr");
		pos = strip_block(help, "ver");
		if (pos)
			g_string_insert(help, pos, datestr);
	}

	/* Credits for menu */
	if (program == NULL) {
		gchar *ptr1;
		gchar *ptr2;
		gsize pos;
		const gchar *author, *email;
		gchar *escaped_author, *escaped_email;

		author = gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu));
		escaped_author = g_markup_escape_text(author, -1);
		email = gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu));
		escaped_email = g_markup_escape_text(email, -1);

		g_string_printf(text, "\n          <p>%s: written by %s &lt;%s&gt;</p>\n          ",
				datestr, escaped_author, escaped_email);

		g_free(escaped_author);
		g_free(escaped_email);

		ptr1 = strstr(help->str, "<!-- begin cpy -->");
		ptr2 = strstr(help->str, "<!-- end cpy -->");

		if (ptr1 != NULL && ptr2 != NULL) {
			gsize len;
			len = (ptr2 - ptr1) / sizeof(gchar);

			if ((refresh) || (len < 40)) {
				pos = (ptr2 - help->str) / sizeof(gchar);
				g_string_insert(help, pos, text->str);
			}
		}

		g_date_free(date);
	}

	g_string_free(text, TRUE);
}

static void help_edit_on_finished(GebrGeoXmlObject * object, const gchar * _help)
{
	GString * help;

	help = g_string_new(_help);

	switch (gebr_geoxml_object_get_type(object)) {
	case GEBR_GEOXML_OBJECT_TYPE_FLOW:
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOC(object), help->str);
		break;
	case GEBR_GEOXML_OBJECT_TYPE_PROGRAM:
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(object), help->str);
		break;
	default:
		break;
	}
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	g_string_free(help, TRUE);
}

/*
 * strip_block:
 * @buffer: the string to be processed
 * @tag: the tag to be stripped
 *
 * Strips a block delimited by <!-- begin tag --> and <!-- end tag -->, returning the position of the beginning of the
 * block, suitable for text insertion. @tag must have 3 letters.
 */
static gsize strip_block(GString * buffer, const gchar * tag)
{
	static GString *mark = NULL;
	gchar *ptr;
	gsize pos;
	gsize len;

	if (mark == NULL)
		mark = g_string_new(NULL);

	g_string_printf(mark, "<!-- begin %s -->", tag);
	ptr = strstr(buffer->str, mark->str);

	if (ptr != NULL) {
		pos = (ptr - buffer->str) / sizeof(gchar) + 18;
	} else {
		return 0;
	}

	g_string_printf(mark, "<!-- end %s -->", tag);
	ptr = strstr(buffer->str, mark->str);

	if (ptr != NULL) {
		len = (ptr - buffer->str) / sizeof(gchar) - pos;
	} else {
		return 0;
	}

	g_string_erase(buffer, pos, len);

	return pos;

}

static void help_edit_on_commit_request(GebrGuiHelpEditWidget * self)
{
	gboolean sensitive;
	const gchar * help;
	GebrGeoXmlObject *object = NULL;
	gboolean valid;
	gboolean interrupt = FALSE;
	GtkTreeIter iter;
	GtkTreeIter child;
	GtkTreeModel * model;

	g_object_get(self, "geoxml-object", &object, NULL);	

	// Searches the menu's model for 'menu', returning the corresponding help backup
	model = GTK_TREE_MODEL (debr.ui_menu.model);
	valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid && !interrupt) {
		valid = gtk_tree_model_iter_children(model, &child, &iter);
		while (valid) {
			gpointer ptr;
			gtk_tree_model_get(model, &child,
					   MENU_XMLPOINTER, &ptr,
					   -1);
			if (ptr == object){
				interrupt = TRUE;
				break;
			}
			valid = gtk_tree_model_iter_next(model, &child);
		}
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	iter = child;

	if (interrupt)
		menu_status_set_from_iter(&iter, MENU_STATUS_UNSAVED);
	

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
		sensitive = strlen(help) > 0 ? TRUE : FALSE;
		g_object_set(debr.ui_program.details.help_view, "sensitive", sensitive, NULL);
		validate_image_set_check_help(debr.ui_program.help_validate_image, help);
	} else {
		help = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));
		sensitive = strlen(help) > 0 ? TRUE : FALSE;
		g_object_set(debr.ui_menu.details.help_view, "sensitive", sensitive, NULL);
		validate_image_set_check_help(debr.ui_menu.help_validate_image, help);
	}
}

static void help_edit_window_on_destroy(GtkWidget * window)
{
	GebrGuiHelpEditWidget *widget;
	GebrGeoXmlObject * object;

	g_object_get(window, "help-edit-widget", &widget, NULL);
	g_object_get(widget, "geoxml-object", &object, NULL);

	if (debr_help_edit_widget_is_content_empty (DEBR_HELP_EDIT_WIDGET (widget))) {
		if (gebr_geoxml_object_get_type (object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
			gebr_geoxml_program_set_help (GEBR_GEOXML_PROGRAM (object), "");
		else
			gebr_geoxml_document_set_help (GEBR_GEOXML_DOCUMENT (object), "");
	}

	g_hash_table_remove(debr.help_edit_windows, object);
}

static void help_edit_on_jump_to_activate(GtkAction * action, GebrGuiHelpEditWindow * window)
{
	GebrGuiHelpEditWidget * widget;
	GebrGeoXmlObject * object;

	g_object_get(window, "help-edit-widget", &widget, NULL);
	g_object_get(widget, "geoxml-object", &object, NULL);

	create_jump_to_menu(object, window);
}

static void help_edit_on_revert(GtkAction * action, GebrGuiHelpEditWindow * window)
{
	GtkWidget * dialog;
	gchar * help_backup;
	GebrGeoXmlObject * object;
	GebrGeoXmlObjectType type;
	GebrGuiHelpEditWidget * widget;

	dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW (window),
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_WARNING,
						    GTK_BUTTONS_YES_NO,
						    _("<span font_weight='bold' size='large'>"
						      "Do you really want to revert this help?"
						      "</span>"));

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog),
						 _("By choosing yes, you will lose all modifications"
						   " made after the associated menu was saved."));

	if (gtk_dialog_run(GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
		goto out;

	g_object_get(window, "help-edit-widget", &widget, NULL);
	g_object_get(widget, "geoxml-object", &object, NULL);

	type = gebr_geoxml_object_get_type(object);
	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		help_backup = debr_program_get_backup_help_from_pointer(object);
	else
		help_backup = debr_menu_get_backup_help_from_pointer(object);

	if (help_backup != NULL) {
		gebr_gui_help_edit_widget_set_content(widget, help_backup);
		g_free(help_backup);
	}

out:
	gtk_widget_destroy(dialog);
}

static void help_edit_on_content_loaded(GebrGuiHelpEditWidget * self, GebrGuiHelpEditWindow * window)
{
	GtkAction * action;
	GtkUIManager * manager;

	manager = gebr_gui_help_edit_window_get_ui_manager (window);
	action = gtk_ui_manager_get_action (manager, "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "/RefreshAction");
	gtk_action_set_sensitive (action, TRUE);
}

/*
 * generate_help_from_template:
 * @object: an XML pointer for a flow or program
 *
 * Generates the template help string, inserting the title, description, etc. fetched from @object. See
 * help_subst_fields().
 *
 * Returns: a newly allocated string containing the help.
 */
static gchar * generate_help_from_template(GebrGeoXmlObject * object)
{
	gchar buffer[1000];
	FILE * template;
	GString * prepared_html;
	GebrGeoXmlProgram * program;

	program = NULL;
	prepared_html = g_string_new (NULL);
	template = fopen (DEBR_DATA_DIR "help-template.html", "r");

	if (!template) {
		debr_message (GEBR_LOG_ERROR, _("Unable to open template. Please check your installation."));
		return g_strdup("");
	}

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		program = GEBR_GEOXML_PROGRAM(object);

	while (fgets(buffer, sizeof(buffer), template))
		g_string_append(prepared_html, buffer);

	/* Substitute title, description and categories */
	help_subst_fields(prepared_html, program, FALSE);

	fclose(template);

	return g_string_free (prepared_html, FALSE);
}

static void on_preview_toggled(GtkToggleAction * action, GtkUIManager * manager)
{
	gboolean toggled;
	GtkAction * refresh;
	GtkAction * revert;

	toggled = gtk_toggle_action_get_active (action);
	refresh = gtk_ui_manager_get_action (manager, "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "/RefreshAction");
	revert = gtk_ui_manager_get_action (manager, "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "/RevertAction");
	gtk_action_set_sensitive (refresh, !toggled);
	gtk_action_set_sensitive (revert, !toggled);
}

static void on_title_ready (GebrGuiHtmlViewerWidget * widget, const gchar * title, GtkWindow * window)
{
	gtk_window_set_title (window, title);
}

//==============================================================================
// PUBLIC METHODS							       =
//==============================================================================
void debr_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title)
{
	const gchar * html;
	GtkWidget * window;
	GebrGuiHtmlViewerWidget * html_viewer_widget;

	window = gebr_gui_html_viewer_window_new(title); 
	html_viewer_widget = gebr_gui_html_viewer_window_get_widget(GEBR_GUI_HTML_VIEWER_WINDOW(window));
	g_signal_connect (html_viewer_widget, "title-ready", G_CALLBACK (on_title_ready), window);

	if (menu)
		gebr_gui_html_viewer_widget_generate_links(html_viewer_widget, object);

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		html = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
	else
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));

	gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);

	gtk_dialog_run(GTK_DIALOG(window));
}

void debr_help_edit(GebrGeoXmlObject * object)
{
	gchar * help;
	GebrGeoXmlProgram * program;

	program = NULL;

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		program = GEBR_GEOXML_PROGRAM(object);
		help = g_strdup(gebr_geoxml_program_get_help(program));
	} else
		help = g_strdup(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object)));

	if (strlen(help) <= 1)
		help = generate_help_from_template(object);

	/* EDIT IT */
	if (debr.config.native_editor || !debr.config.htmleditor->len) {
		GtkWidget * help_edit_window;

		help_edit_window = g_hash_table_lookup(debr.help_edit_windows, object);

		if (help_edit_window != NULL)
			gtk_window_present(GTK_WINDOW(help_edit_window));
		else
			create_help_edit_window(object, help);
	} else {
		FILE * htmlfp;
		GString * html_path = gebr_make_temp_filename("debr_XXXXXX.html");

		/* Create a temporary file and write the help into it */
		htmlfp = fopen(html_path->str, "w");
		if (htmlfp == NULL) {
			debr_message(GEBR_LOG_ERROR, _("Unable to create temporary file."));
			goto out;
		}
		fputs(help, htmlfp);
		fclose(htmlfp);

		gchar *quote1 = g_shell_quote(debr.config.htmleditor->str);
		gchar *quote2 = g_shell_quote(html_path->str);
		gint exitstatus = WEXITSTATUS(gebr_system("%s %s", quote1, quote2));
		g_free(quote1);
		g_free(quote2);
		if (exitstatus) {
			debr_message(GEBR_LOG_ERROR, _("Error during editor execution."));
			goto out;
		}

		/* Add file to list of files to be removed */
		debr.tmpfiles = g_slist_append(debr.tmpfiles, html_path->str);

		/* open temporary file with help from XML */
		htmlfp = fopen(html_path->str, "r");
		if (htmlfp == NULL) {
			debr_message(GEBR_LOG_ERROR, _("Unable to create temporary file."));
			goto out;
		}
		GString * html_content = g_string_new (NULL);
		gchar buffer[1000];
		while (fgets(buffer, sizeof(buffer), htmlfp))
			g_string_append(html_content, buffer);
		if (program)
			help_edit_on_finished(GEBR_GEOXML_OBJECT(program), html_content->str);
		else
			help_edit_on_finished(GEBR_GEOXML_OBJECT(debr.menu), html_content->str);
		fclose(htmlfp);
		g_string_free(html_content, TRUE);

out:		g_string_free(html_path, TRUE);
		g_free(help);
	}
}
