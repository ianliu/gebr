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
#include <regex.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/help.h>

#include "help.h"
#include "debr.h"
#include "defines.h"

void
help_insert_parameters_list(GString *help, GebrGeoXmlProgram * program, gboolean refresh);

void
add_program_parameter_item(GString *str, GebrGeoXmlParameter *par);

gsize
strip_block(GString *buffer, const gchar *tag);

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
add_program_parameter_item(GString *str, GebrGeoXmlParameter *par)
{
        if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(par))){
                g_string_append_printf(str, "              "
                                       "<li class=\"req\"><span class=\"reqlabel\">%s</span><br/>"
                                       " detailed description comes here.</li>\n\n",
                                       gebr_geoxml_parameter_get_label(par));
        }else{
                g_string_append_printf(str, "              "
                                       "<li><span class=\"label\">%s</span><br/>"
                                       " detailed description comes here.</li>\n\n",
                                       gebr_geoxml_parameter_get_label(par));
        }
}

void
help_insert_parameters_list(GString *help, GebrGeoXmlProgram * program, gboolean refresh)
{
        GString          *      label;
	GebrGeoXmlParameters *	parameters;
	GebrGeoXmlSequence *	parameter;
        gchar *                 ptr;
        gsize                   pos;

        if (program == NULL)
                return;
        
        parameters = gebr_geoxml_program_get_parameters(program);
        parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	
        label = g_string_new(NULL);
        g_string_assign(label,"<ul>\n");
        while (parameter != NULL) {
                
                if (gebr_geoxml_parameter_get_is_program_parameter(GEBR_GEOXML_PARAMETER(parameter)) == TRUE){
                        add_program_parameter_item(label, GEBR_GEOXML_PARAMETER(parameter));
                }else{
                        GebrGeoXmlSequence  *	instance;
                        GebrGeoXmlSequence  *	subpar;
                        
                        g_string_append_printf(label, "              "
                                               "<li class=\"group\"><span class=\"grouplabel\">%s</span><br/>"
                                                       " detailed description comes here.\n\n",
                                               gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter))); 
                        
                        g_string_append_printf(label, "              <ul>\n");
                        
                        gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
                        subpar = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));
                        while (subpar != NULL) {
                                add_program_parameter_item(label, GEBR_GEOXML_PARAMETER(subpar));
                                gebr_geoxml_sequence_next(&subpar);
                        }
                        g_string_append_printf(label, "              </ul></li>\n\n");
                        
                }
                
                gebr_geoxml_sequence_next(&parameter);
        }
        g_string_append(label,"            </ul>\n            ");

        if (refresh){
                ptr = strstr(help->str, "<!-- end lst -->");
                pos = (ptr != NULL) ? (ptr - help->str)/sizeof(gchar) : 0;
        }
        else{
                pos = strip_block(help, "lst");
        }

        if (pos > 0){
                g_string_insert(help, pos, label->str);
        }
        else {
                /* Try to find a parameter's block, for */
                /* helps generated before this function */
                GString       *mark;
                gchar         *ptr1;
                gchar         *ptr2;
                gsize          pos;
                
                mark = g_string_new(NULL);
                
                g_string_printf(mark, "<div class=\"parameters\">");
                ptr1 = strstr(help->str, mark->str);
                
                if (ptr1 != NULL){
                        g_string_printf(mark, "</div>");
                        ptr2 = strstr(ptr1, mark->str);
                        pos = (ptr2 - help->str)/sizeof(gchar);
                        g_string_insert(help, pos, label->str);
                }
                else{
                        debr_message(LOG_WARNING, "Unable to reinsert parameter's list");
                }
        }

        g_string_free(label, TRUE);
}

void
help_subst_fields(GString * help, GebrGeoXmlProgram * program, gboolean refresh)
{
	gchar *		        content;
        GString *               text;
        gsize                   pos;

        text = g_string_new (NULL);

	/* Title replacement */
	if (program != NULL)
		content = (gchar*)gebr_geoxml_program_get_title(program);
	else
		content = (gchar*)gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu));
	if (strlen(content)) {

                pos = strip_block(help, "ttl");
                if (pos){
                        g_string_printf(text,"\n  <title>G&ecirc;BR - %s</title>\n  ", content);
                        g_string_insert(help, pos, text->str);
                }

                pos = strip_block(help, "tt2");
                if (pos){
                        g_string_printf(text,"\n         <span class=\"flowtitle\">%s</span>\n         ",
                                        content);
                        g_string_insert(help, pos, text->str);
                }
	}

	/* Description replacement */
	if (program != NULL)
		content = (gchar*)gebr_geoxml_program_get_description(program);
	else
		content = (gchar*)gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(debr.menu));
	if (strlen(content)) {
                pos = strip_block(help, "des");
                if (pos){
                        g_string_printf(text,"\n            %s\n            ", content);
                        g_string_insert(help, pos, text->str);
                }
	}

	/* Categories replacement */
	if (gebr_geoxml_flow_get_categories_number(debr.menu)) {
		GebrGeoXmlSequence *	category;
		GString *		catstr;

                pos = strip_block(help, "cat");
                if (pos){
                        gebr_geoxml_flow_get_category(debr.menu, &category, 0);
                        
                        catstr = g_string_new("\n       "); 
                        g_string_append(catstr, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)));
                        gebr_geoxml_sequence_next(&category);
                        while (category != NULL) {
                                g_string_append(catstr, " | ");
                                g_string_append(catstr, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)));
                                
                                gebr_geoxml_sequence_next(&category);
                        }
                        g_string_append(catstr, "\n       ");
                        
                        
                        g_string_insert(help, pos, catstr->str);
                        g_string_free(catstr, TRUE);
                }
	}

	/* Parameter's description replacement */
	if (program != NULL) {
                help_insert_parameters_list(help, program, refresh);
	}
	else { /* strip parameter section for flow help */
                strip_block(help, "par");
                strip_block(help, "mpr");
	}

        /* Credits for menu */
	if (program == NULL) {
                GDate * date;
                gchar   buffer[13];
                gchar * ptr1;
                gchar * ptr2;
                gsize   pos;
                                
                date = g_date_new();
                g_date_set_time_t(date, time(NULL));
                g_date_strftime(buffer, 13, "%b %d, %Y", date);

                g_string_printf(text, "\n          <p>%s: written by %s &lt;%s&gt;</p>\n          ",
                                buffer,
                                gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu)),
                                gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu)));
                
                ptr1 = strstr(help->str, "<!-- begin cpy -->");
                ptr2 = strstr(help->str, "<!-- end cpy -->");

                if (ptr1 != NULL && ptr2 != NULL){
                        gsize len;
                        len = (ptr2-ptr1)/sizeof(gchar);
                        
                        if ((refresh) || (len < 40)){

                                pos = (ptr2 - help->str)/sizeof(gchar);
                                g_string_insert(help, pos, text->str);
                        }
                }
                
                g_date_free(date);
	}

        g_string_free(text, TRUE);
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
	gebr_gui_help_show(html_path->str, debr.config.browser->str);

out:	g_string_free(html_path, FALSE);
	g_string_free(prepared_html, TRUE);
}

GString *
help_edit(const gchar * help, GebrGeoXmlProgram * program, gboolean refresh)
{
	FILE *		fp;
	GString *	html_path;
	GString *	prepared_html;
	GString *	cmdline;
	gchar		buffer[100];

	/* check if there is an html editor in preferences */
	if (!strlen(debr.config.htmleditor->str)) {
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
		help_subst_fields(prepared_html, program, refresh);
	}
        else {
                if (refresh){
                        help_subst_fields(prepared_html, program, refresh);
                }
        }

        /* Always fix DTD version */
        {
                gsize pos;
                
                pos = strip_block(prepared_html, "dtd");
                if (pos)
                        g_string_insert(prepared_html, pos, gebr_geoxml_document_get_version(GEBR_GEOXML_DOCUMENT(debr.menu)));
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

/* Strips a block delimited by
       <!-- begin tag -->
       <!-- end tag -->
   and returns the position of the
   begining of the block, suitable for
   text insertion. "tag" must have 3 letters.
*/
gsize
strip_block(GString *buffer, const gchar *tag)
{

        static GString       *mark = NULL;
        gchar                *ptr;
        gsize                 pos;
        gsize                 len;

        if (mark == NULL)
                mark = g_string_new(NULL);

        g_string_printf(mark, "<!-- begin %s -->", tag);
        ptr = strstr(buffer->str, mark->str);

	if (ptr != NULL){
                pos = (ptr - buffer->str)/sizeof(gchar) + 18;
        } else {
                return 0;
        }

        g_string_printf(mark, "<!-- end %s -->", tag);
        ptr = strstr(buffer->str, mark->str);

        if (ptr != NULL){
                len = (ptr - buffer->str)/sizeof(gchar) - pos;
        } else {
                return 0;
        }

        g_string_erase(buffer, pos, len);
        
        return pos;
                        
}
