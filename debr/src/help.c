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

/*
 * Prototypes
 */

static void help_edit_on_refresh(GebrGuiHelpEditWindow * window, GebrGeoXmlObject * object);

static void add_program_parameter_item(GString * str, GebrGeoXmlParameter * par);

static void help_insert_parameters_list(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_subst_fields(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_edit_on_finished(GebrGeoXmlObject * object, const gchar * _help);

static gsize strip_block(GString * buffer, const gchar * tag);

static GtkMenuBar * create_menu_bar(GebrGeoXmlObject * object, GebrGuiHelpEditWindow * window);

static void help_edit_on_commit_request(GebrGuiHelpEditWidget * self, GtkTreeIter * iter);

/*
 * Public functions.
 */

void debr_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title)
{
	const gchar * html;
	GtkWidget * window;
	GebrGuiHtmlViewerWidget * html_viewer_widget;

	window = gebr_gui_html_viewer_window_new(title); 
	html_viewer_widget = gebr_gui_html_viewer_window_get_widget(GEBR_GUI_HTML_VIEWER_WINDOW(window));

	if (menu) {
		gebr_gui_html_viewer_window_set_geoxml_object(GEBR_GUI_HTML_VIEWER_WINDOW(window), object);
		gebr_gui_html_viewer_widget_set_generate_links(html_viewer_widget, TRUE);
	}

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		html = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
	else
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));

	gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);

	gtk_dialog_run(GTK_DIALOG(window));
}

void debr_help_edit(GebrGeoXmlObject * object)
{
	GString *prepared_html;
	GString *cmd_line;
	FILE *html_fp;
	GString *html_path;
	const gchar * help;
	GebrGeoXmlProgram * program = NULL;

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		program = GEBR_GEOXML_PROGRAM(object);
		help = gebr_geoxml_program_get_help(program);
	} else
		help = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));

	prepared_html = g_string_new(help);

	// If help is empty, create from template.
	if (prepared_html->len <= 1) {
		FILE *fp;
		gchar buffer[1000];

		/* Read back the help from file */
		fp = fopen(DEBR_DATA_DIR "help-template.html", "r");
		if (fp == NULL) {
			debr_message(GEBR_LOG_ERROR, _("Unable to open template. Please check your installation."));
			return;
		}

		while (fgets(buffer, sizeof(buffer), fp))
			g_string_append(prepared_html, buffer);

		fclose(fp);

		/* Substitute title, description and categories */
		help_subst_fields(prepared_html, program, FALSE);
	}

	/* EDIT IT */
	if (debr.config.native_editor || !debr.config.htmleditor->len) {
		GtkWidget * help_edit_window;

		help_edit_window = g_hash_table_lookup(debr.help_edit_windows, object);

		if (help_edit_window != NULL) {
			gtk_window_present(GTK_WINDOW(help_edit_window));
		} else {
			gchar * title;
			gboolean is_menu_selected;
			const gchar * object_title;
			GtkMenuBar * menu_bar;
			GtkTreeIter iter;
			GebrGuiHelpEditWidget * help_edit_widget;
			GebrGeoXmlObject * object;

			if (program != NULL) {
				object = GEBR_GEOXML_OBJECT(program);
				object_title = gebr_geoxml_program_get_title(program);
				title = g_strdup_printf(_("Program: %s"), object_title);
			} else {
				object = GEBR_GEOXML_OBJECT(debr.menu);
				object_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(debr.menu));
				title = g_strdup_printf(_("Menu: %s"), object_title);
			}

			is_menu_selected = menu_get_selected(&iter, TRUE);
			help_edit_widget = debr_help_edit_widget_new(object, prepared_html->str);
			help_edit_window = gebr_gui_help_edit_window_new_with_refresh(help_edit_widget);
			menu_bar = create_menu_bar(object, GEBR_GUI_HELP_EDIT_WINDOW(help_edit_window));
			gebr_gui_help_edit_window_set_menu_bar(GEBR_GUI_HELP_EDIT_WINDOW(help_edit_window), menu_bar);

			g_signal_connect_swapped(help_edit_window, "destroy",
						 G_CALLBACK(debr_remove_help_edit_window), object);
			g_signal_connect(help_edit_window, "refresh-requested",
					 G_CALLBACK(help_edit_on_refresh), object);
			g_signal_connect(help_edit_widget, "commit-request",
					 G_CALLBACK(help_edit_on_commit_request),
					 is_menu_selected ? gtk_tree_iter_copy(&iter):NULL);

			g_hash_table_insert(debr.help_edit_windows, object, help_edit_window);
			gtk_window_set_title(GTK_WINDOW(help_edit_window), title);
			gtk_window_set_default_size(GTK_WINDOW(help_edit_window), 600, 500);
			g_free(title);
			gtk_widget_show(help_edit_window);
		}
	} else {
		/* create temporary filename */
		html_path = gebr_make_temp_filename("debr_XXXXXX.html");

		/* open temporary file with help from XML */
		html_fp = fopen(html_path->str, "w");
		if (html_fp == NULL) {
			debr_message(GEBR_LOG_ERROR, _("Unable to create temporary file."));
			goto out;
		}
		fputs(prepared_html->str, html_fp);
		fclose(html_fp);

		cmd_line = g_string_new(NULL);
		g_string_printf(cmd_line, "%s  %s", debr.config.htmleditor->str, html_path->str);

		if (WEXITSTATUS(system(cmd_line->str)))
			debr_message(GEBR_LOG_ERROR, _("Error during editor execution."));

		g_string_free(cmd_line, TRUE);

		/* Add file to list of files to be removed */
		debr.tmpfiles = g_slist_append(debr.tmpfiles, html_path->str);

		/* open temporary file with help from XML */
		html_fp = fopen(html_path->str, "r");
		if (html_fp == NULL) {
			debr_message(GEBR_LOG_ERROR, _("Unable to create temporary file."));
			goto out;
		}
		g_string_assign(prepared_html, "");

		gchar buffer[1000];
		while (fgets(buffer, sizeof(buffer), html_fp))
			g_string_append(prepared_html, buffer);

		fclose(html_fp);
		if (program)
			help_edit_on_finished(GEBR_GEOXML_OBJECT(program), prepared_html->str);
		else
			help_edit_on_finished(GEBR_GEOXML_OBJECT(debr.menu), prepared_html->str);

	out:
		g_string_free(html_path, FALSE);
		g_string_free(prepared_html, TRUE);
	}
}

/*
 * Private functions.
 */

static void help_edit_on_refresh(GebrGuiHelpEditWindow * window, GebrGeoXmlObject * object)
{
	gchar * help_content;
	GString * help_string;
	GebrGuiHelpEditWidget * help_edit_widget;

	if (gebr_geoxml_object_get_type(object) != GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		object = NULL;

	g_object_get(window, "help-edit-widget", &help_edit_widget, NULL);
	help_content = gebr_gui_help_edit_widget_get_content(help_edit_widget);
	help_string = g_string_new(help_content);
	help_subst_fields(help_string, GEBR_GEOXML_PROGRAM(object), TRUE);
	gebr_gui_help_edit_widget_set_content(help_edit_widget, help_string->str);
	g_free(help_content);
	g_string_free(help_string, TRUE);
}

static GtkMenuBar * create_menu_bar(GebrGeoXmlObject * object, GebrGuiHelpEditWindow * window)
{
	GtkWidget * menu;
	GtkWidget * menu_bar;
	GtkWidget * menu_item;
	GtkWidget * menu_bar_item;
	GtkAccelGroup * accel_group;
	GebrGuiHelpEditWidget * help_edit_widget;
	GebrGuiHtmlViewerWidget * html_viewer_widget;

	g_object_get(window, "help-edit-widget", &help_edit_widget, NULL);
	html_viewer_widget = gebr_gui_help_edit_widget_get_html_viewer(help_edit_widget);

	accel_group = gtk_accel_group_new();
	menu_bar = gtk_menu_bar_new();

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	// 'File' menu
	//
	menu = gtk_menu_new();
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, accel_group);
	g_signal_connect_swapped(menu_item, "activate",
				 G_CALLBACK(gebr_gui_html_viewer_widget_print), html_viewer_widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
	g_signal_connect_swapped(menu_item, "activate",
				 G_CALLBACK(gebr_gui_help_edit_window_quit), window);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Insert 'File' menu into menubar
	menu_bar_item = gtk_menu_item_new_with_mnemonic(_("_File"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_bar_item), menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), GTK_WIDGET(menu_bar_item));

	// 'Jump to' menu
	//
	menu = gtk_menu_new();

	// Insert 'Jump to' menu into menubar
	menu_bar_item = gtk_menu_item_new_with_mnemonic(_("_Jump to"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_bar_item), menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), GTK_WIDGET(menu_bar_item));

	GebrGeoXmlFlow * flow;
	GebrGeoXmlSequence * program;

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		flow = GEBR_GEOXML_FLOW(gebr_geoxml_object_get_owner_document(object));
	else
		flow = GEBR_GEOXML_FLOW(object);

	// Insert link for Flow
	gchar * label;
	const gchar * title;
	title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow));
	label = g_strdup_printf(_("Menu: %s"), title);
	menu_item = gtk_menu_item_new_with_label(label);
	g_free(label);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	if (GEBR_GEOXML_OBJECT(flow) != object)
		g_signal_connect_swapped(menu_item, "activate",
					 G_CALLBACK(debr_help_edit), flow);
	else
		gtk_widget_set_sensitive(menu_item, FALSE);

	gebr_geoxml_flow_get_program(flow, &program, 0);
	while (program) {
		title = gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program));
		label = g_strdup_printf(_("Program: %s"), title);
		menu_item = gtk_menu_item_new_with_label(label);
		g_free(label);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		if (GEBR_GEOXML_OBJECT(program) != object)
			g_signal_connect_swapped(menu_item, "activate",
						 G_CALLBACK(debr_help_edit), program);
		else
			gtk_widget_set_sensitive(menu_item, FALSE);
		gebr_geoxml_sequence_next(&program);
	}

	gtk_widget_show_all(menu_bar);
	return GTK_MENU_BAR(menu_bar);
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

/**
 * \internal
 */
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
       	original_locale = g_strdup(setlocale(LC_TIME, ""));
       	new_locale = g_strdup(setlocale(LC_TIME, "en_US.UTF-8"));

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

/**
 * \internal
 */
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

/**
 * \internal
 * Strips a block delimited by
 *      <!-- begin tag -->
 *      <!-- end tag -->
 *  and returns the position of the
 *  begining of the block, suitable for
 *  text insertion. "tag" must have 3 letters.
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

static void help_edit_on_commit_request(GebrGuiHelpEditWidget * self, GtkTreeIter * iter)
{
	GebrGeoXmlObject *object = NULL;

	if (iter != NULL)
		menu_status_set_from_iter(iter, MENU_STATUS_UNSAVED);
	
	g_object_get(self, "geoxml-object", &object, NULL);	

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		g_object_set(debr.ui_program.details.help_view,
			     "sensitive", strlen(gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(debr.program)))
			     ? TRUE : FALSE, NULL);
	else
		g_object_set(debr.ui_menu.details.help_view,
			     "sensitive", strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(debr.menu)))
			     ? TRUE : FALSE, NULL);
}
