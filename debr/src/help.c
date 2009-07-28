/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <regex.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/help.h>

#include "help.h"
#include "debr.h"
#include "defines.h"

void
add_program_parameter_item(GString *str, GeoXmlParameter *par);


void
help_fix_css(GString * help)
{
	gchar *		gebrcsspos;

	/* Change CSS's path to an absolute one. */
	if ((gebrcsspos = strstr(help->str, "\"gebr.css")) != NULL) {
		int	pos;

		pos = (gebrcsspos - help->str)/sizeof(char);
		g_string_erase(help, pos, 9);
		g_string_insert(help, pos, "\"file://" DEBR_DATA_DIR "gebr.css");
	}
}

void
add_program_parameter_item(GString *str, GeoXmlParameter *par)
{
        if (geoxml_program_parameter_get_required(GEOXML_PROGRAM_PARAMETER(par))){
                g_string_append_printf(str, "              "
                                       "<li class=\"req\"><span class=\"reqlabel\">%s</span>"
                                       " detailed description comes here.</li>\n\n",
                                       geoxml_parameter_get_label(par));
        }else{
                g_string_append_printf(str, "              "
                                       "<li><span class=\"label\">%s</span>"
                                       " detailed description comes here.</li>\n\n",
                                       geoxml_parameter_get_label(par));
        }
}


void
help_subst_fields(GString * help, GeoXmlProgram * program)
{
	gchar *		        content;
	gchar *		        ptr;
	gsize		        pos;
	GeoXmlParameters *	parameters;
	GeoXmlSequence *	parameter;

	/* Title replacement */
	if (program != NULL)
		content = (gchar*)geoxml_program_get_title(program);
	else
		content = (gchar*)geoxml_document_get_title(GEOXML_DOC(debr.menu));
	if (strlen(content)) {
		ptr = strstr(help->str, "Flow/Program Title");

		while (ptr != NULL){
			pos = (ptr - help->str)/sizeof(gchar);
			g_string_erase(help, pos, 18);
			g_string_insert(help, pos, content);
			ptr = strstr(help->str, "Flow/Program Title");
		}
	}

	/* Description replacement */
	if (program != NULL)
		content = (gchar*)geoxml_program_get_description(program);
	else
		content = (gchar*)geoxml_document_get_description(GEOXML_DOC(debr.menu));
	if (strlen(content)) {
		ptr = strstr(help->str, "Put here an one-line description");

		while (ptr != NULL){
			pos = (ptr - help->str)/sizeof(gchar);
			g_string_erase(help, pos, 32);
			g_string_insert(help, pos, content);
			ptr = strstr(help->str, "Put here an one-line description");
		}
	}

	/* Categories replacement */
	if (geoxml_flow_get_categories_number(debr.menu)) {
		GeoXmlSequence *	category;
		GString *		catstr;

		geoxml_flow_get_category(debr.menu, &category, 0);
		catstr = g_string_new(geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(category)));
		geoxml_sequence_next(&category);
		while (category != NULL) {
			g_string_append(catstr, " | ");
			g_string_append(catstr, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(category)));

			geoxml_sequence_next(&category);
		}

		while ((ptr = strstr(help->str, "First category | Second category | ...")) != NULL) {
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 38);
			g_string_insert(help, pos, catstr->str);
		}

		g_string_free(catstr, TRUE);
	}

	/* Parameter's description replacement */
	if (program != NULL) {
		GString *      label;

		parameters = geoxml_program_get_parameters(program);
		parameter = geoxml_parameters_get_first_parameter(parameters);
		
		label = g_string_new(NULL);
		while (parameter != NULL) {
                        
                        if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == TRUE){
                                add_program_parameter_item(label, GEOXML_PARAMETER(parameter));
                        }else{
                                GeoXmlSequence  *	instance;
                                GeoXmlSequence  *	subpar;

                                g_string_append_printf(label, "              "
                                                       "<li class=\"group\"><span class=\"grouplabel\">%s</span>"
                                                       " detailed description comes here.\n\n",
                                                       geoxml_parameter_get_label(GEOXML_PARAMETER(parameter))); 

                                g_string_append_printf(label, "              <ul>\n");

                                geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
                                subpar = geoxml_parameters_get_first_parameter(GEOXML_PARAMETERS(instance));
                                while (subpar != NULL) {
                                        add_program_parameter_item(label, GEOXML_PARAMETER(subpar));
                                        geoxml_sequence_next(&subpar);
                                }
                                g_string_append_printf(label, "              </ul></li>\n\n");

                                
                        }

			geoxml_sequence_next(&parameter);
		}

		if ((ptr = strstr(help->str, "              "
				  "<li class=\"req\">")) != NULL) {
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 338);
			g_string_insert(help, pos, label->str);
		}
		g_string_free(label, TRUE);
	}
	else { /* strip parameter section for flow help */
		if ((ptr = strstr(help->str, "          <div class=\"parameters\">")) != NULL) {
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 1350);
		}
		if ((ptr = strstr(help->str, "            <li><a href=\"#par\">Parameters</a></li>")) != NULL) {
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 52);
		}
	}
	
}

/*
 * Function: help_show
 * Show _help_ using user's browser
 */
void
help_show(const gchar * help)
{
	GString *	prepared_html;
	FILE *		html_fp;
	GString *	html_path;

	prepared_html = g_string_new(help);
	help_fix_css(prepared_html);

	/* create temporary filename */
	html_path = make_temp_filename("debr_XXXXXX.html");

	/* open temporary file with help from XML */
	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		debr_message(LOG_ERROR, _("Could not create an temporary file."));
		goto out;
	}
	fputs(prepared_html->str, html_fp);
	fclose(html_fp);

	/* Add file to list of files to be removed */
	debr.tmpfiles = g_slist_append(debr.tmpfiles, html_path->str);

	g_string_prepend(html_path, "file://");
#ifdef WEBKIT_ENABLED
	libgebr_gui_help_show(html_path->str);
#else
	GString *	cmdline;

	cmdline = g_string_new(debr.config.browser->str);
	g_string_append(cmdline, html_path->str);
	g_string_append(cmdline, " &");
	system(cmdline->str);

	g_string_free(cmdline, TRUE);
#endif

out:	g_string_free(html_path, FALSE);
	g_string_free(prepared_html, TRUE);
}

GString *
help_edit(const gchar * help, GeoXmlProgram * program)
{
	FILE *		fp;
	GString *	html_path;
	GString *	prepared_html;
	GString *	cmdline;
	gchar		buffer[100];

        /* check if there is an html editor in preferences */
        if (!strlen(debr.config.htmleditor->str)){
                debr_message(LOG_ERROR, _("No HTML editor specified in preferences."));
                return NULL;
        }
        
	/* initialization */
	prepared_html = g_string_new(NULL);

	/* help empty; create from template. */
	g_string_assign(prepared_html, help);
	if (prepared_html->len <= 1) {
		/* Read back the help from file */
		fp = fopen(DEBR_DATA_DIR "help-template.html", "r");
		if (fp == NULL) {
			debr_message(LOG_ERROR, _("Could not open template. Please check your installation."));
			return prepared_html;
		}

		while (fgets(buffer, sizeof(buffer), fp))
			g_string_append(prepared_html, buffer);

		fclose(fp);

		/* Substitute title, description and categories */
		help_subst_fields(prepared_html, program);
	}

	/* CSS fix */
	help_fix_css(prepared_html);

	/* load html into a temporary file */
	html_path = make_temp_filename("debr_XXXXXX.html");
	fp = fopen(html_path->str, "w");
	if (fp == NULL) {
		debr_message(LOG_ERROR, _("Could not create a temporary file."));
		goto err2;
	}
	fputs(prepared_html->str, fp);
	fclose(fp);

	/* Add file to list of files to be removed */
	debr.tmpfiles = g_slist_append(debr.tmpfiles, html_path->str);

	/* run editor */
	cmdline = g_string_new("");
	g_string_printf(cmdline, "%s %s", debr.config.htmleditor->str, html_path->str);
	if (system(cmdline->str)) {
		debr_message(LOG_ERROR, _("Could not launch editor"));
		goto err;
	}
	g_string_free(cmdline, TRUE);

	/* read back the help from file */
	fp = fopen(html_path->str, "r");
	if (fp == NULL) {
		debr_message(LOG_ERROR, _("Could not read created temporary file."));
		goto err;
	}
	g_string_assign(prepared_html, "");
	while (fgets(buffer, sizeof(buffer), fp))
		g_string_append(prepared_html, buffer);
	fclose(fp);

	/* ensure UTF-8 encoding */
	if (g_utf8_validate(prepared_html->str, -1, NULL) == FALSE) {
		gchar *	converted;

		/* TODO: what else should be tried? */
		converted = g_simple_locale_to_utf8(prepared_html->str);
		if (converted == NULL) {
			g_free(converted);
			debr_message(LOG_ERROR, _("Please change the help encoding to UTF-8"));
			goto err;
		}

		g_string_assign(prepared_html, converted);
		g_free(converted);
	}

	/* transform css into a relative path back */
	{
		regex_t		regexp;
		regmatch_t	matchptr;

		regcomp(&regexp, "<link[^<]*>", REG_NEWLINE | REG_ICASE );
		if (!regexec (&regexp, prepared_html->str, 1, &matchptr, 0)) {
			g_string_erase(prepared_html, (gssize) matchptr.rm_so, (gssize) matchptr.rm_eo - matchptr.rm_so);
			g_string_insert(prepared_html, (gssize) matchptr.rm_so,
					"<link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />" );
		}
		else {
			regcomp (&regexp, "<head>", REG_NEWLINE | REG_ICASE );
			if(!regexec (&regexp, prepared_html->str, 1, &matchptr, 0))
			g_string_insert(prepared_html, (gssize) matchptr.rm_eo,
					"\n  <link rel=\"stylesheet\" type=\"text/css\" href=\"gebr.css\" />" );
		}
	}

	g_string_free(html_path, FALSE);
	return prepared_html;

err:	g_string_free(html_path, FALSE);
err2:	g_string_free(prepared_html, TRUE);
	return NULL;
}
