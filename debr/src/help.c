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
#include <libgebr/gui/help.h>

#include "help.h"
#include "debr.h"
#include "defines.h"

/*
 * Prototypes
 */

static void help_edit_on_refresh(GString * help, GebrGeoXmlObject * object);

static void add_program_parameter_item(GString * str, GebrGeoXmlParameter * par);

static void help_insert_parameters_list(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_subst_fields(GString * help, GebrGeoXmlProgram * program, gboolean refresh);

static void help_edit_on_finished(GebrGeoXmlObject * object, const gchar * _help);

static gsize strip_block(GString * buffer, const gchar * tag);

/*
 * Public functions.
 */

void help_show(GebrGeoXmlObject *object, const gchar * title)
{
	GString *html = g_string_new("");
	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		g_string_assign(html, gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object)));
	else
		g_string_assign(html, gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object)));

	gebr_gui_help_show(object, TRUE, html->str, title);

	g_string_free(html, TRUE);
}

void debr_help_edit(const gchar * help, GebrGeoXmlProgram * program)
{
	GString *prepared_html;
	GString *cmd_line;
	FILE *html_fp;
	GString *html_path;

	/* initialization */
	prepared_html = g_string_new(NULL);

	/* help empty; create from template. */
	g_string_assign(prepared_html, help);
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
		if (program != NULL)
			gebr_gui_program_help_edit(program, prepared_html, help_edit_on_finished, help_edit_on_refresh);
		else
			gebr_gui_help_edit(GEBR_GEOXML_DOCUMENT(debr.menu), prepared_html, help_edit_on_finished,
					   help_edit_on_refresh, TRUE);
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

static void help_edit_on_refresh(GString * help, GebrGeoXmlObject * object)
{
	if (gebr_geoxml_object_get_type(object) != GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		object = NULL;
	help_subst_fields(help, GEBR_GEOXML_PROGRAM(object), TRUE);
}

/**
 * \internal
 */
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

/**
 * \internal
 */
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
	validate_image_set_check_help(debr.ui_menu.help_validate_image, _help);

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
