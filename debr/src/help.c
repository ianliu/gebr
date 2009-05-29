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

#ifdef WEBKIT_ENABLED
#include <webkit/webkit.h>
#endif

#include <libgebrintl.h>
#include <misc/utils.h>

#include "help.h"
#include "debr.h"
#include "defines.h"

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
help_subst_fields(GString * help, GeoXmlProgram * program)
{
	gchar *		        content;
	gchar *		        ptr;
	gsize		        pos;
	GeoXmlParameters *	parameters;
	GeoXmlSequence *	parameter;
	int                     npar;
        int                     ipar;

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
		npar = geoxml_parameters_get_number(parameters);
		
		label = g_string_new(NULL);
		for (ipar = 0; ipar < npar; ipar++){
			g_string_append_printf(label, "              "
					       "<li><span class=\"label\">[%s]</span>"
					       " detailed description comes here.</li>\n\n",
					       geoxml_parameter_get_label(GEOXML_PARAMETER(parameter)));
			geoxml_sequence_next(&parameter);
		}
		if ((ptr = strstr(help->str, "              "
				  "<li><span class=\"label\">[label for parameter]</span>")) != NULL){
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 96);
			g_string_insert(help, pos, label->str);
		}
		g_string_free(label, TRUE);
	}
	else{ /* strip parameter section for flow help */
		if( (ptr = strstr(help->str, "          <div class=\"parameters\">")) != NULL){
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 1317);
		}
		if( (ptr = strstr(help->str, "            <li><a href=\"#par\">Parameters</a></li>")) != NULL){
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

#ifdef WEBKIT_ENABLED
        GtkWidget *       window;
        GtkWidget *       scrolled_window;
        GtkWidget *       web_view;
        WebKitWebFrame*   frame;
	GString *	  prepared_html;

	prepared_html = g_string_new(help);
	help_fix_css(prepared_html);

        window = gtk_dialog_new ();
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        web_view = webkit_web_view_new ();
        frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(web_view));

        /* Place the WebKitWebView in the GtkScrolledWindow */
        gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG(window)->vbox), scrolled_window, TRUE, TRUE, 0);

        /* Open a webpage */
        webkit_web_view_load_html_string(WEBKIT_WEB_VIEW (web_view), prepared_html->str, NULL);

        /* Show the result */
        gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
        gtk_widget_show_all (window);

        g_string_free(prepared_html, TRUE);
#else
	FILE *		html_fp;
	GString *	html_path;
	GString *	cmdline;
	GString *	prepared_html;

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

	/* Launch an external browser */
	cmdline = g_string_new (debr.config.browser->str);
	g_string_append(cmdline, " file://");
	g_string_append(cmdline, html_path->str);
	g_string_append(cmdline, " &");
	system(cmdline->str);

	g_string_free(cmdline, TRUE);
out:	g_string_free(html_path, FALSE);
	g_string_free(prepared_html, TRUE);

#endif
}

GString *
help_edit(const gchar * help, GeoXmlProgram * program)
{
	FILE *		fp;
	GString *	html_path;
	GString *	prepared_html;
	GString *	cmdline;
	gchar		buffer[100];

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
