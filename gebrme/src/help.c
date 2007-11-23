/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "help.h"
#include "gebrme.h"
#include "support.h"

void
help_fix_css(GString * help)
{
	gchar *		gebrcsspos;

	/* Change CSS's path to an absolute one. */
	if ((gebrcsspos = strstr(help->str, "\"gebr.css")) != NULL){
		int	pos;

		pos = (gebrcsspos - help->str)/sizeof(char);
		g_string_erase(help, pos, 9);
		g_string_insert(help, pos, "\"file://" GEBRME_DATA_DIR "gebr.css");
	}
}

void
help_subst_fields(GString * help)
{
	gchar *	content;
	gchar *	ptr;
	gsize	pos;

	/* Title replacement */
	content = (gchar*)geoxml_document_get_title(GEOXML_DOC(gebrme.current));
	if (strlen(content) > 0) {
		ptr = strstr(help->str, "Flow/Program Title");

		while (ptr != NULL){
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 18);
			g_string_insert(help, pos, content);
			ptr = strstr(help->str, "Flow/Program Title");
		}
	}

	/* Description replacement */
	content = (gchar*)geoxml_document_get_description(GEOXML_DOC(gebrme.current));
	if (strlen(content) > 0){
		ptr = strstr(help->str, "Put here an one-line description");

		while (ptr != NULL){
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 32);
			g_string_insert(help, pos, content);
			ptr = strstr(help->str, "Put here an one-line description");
		}
	}

	/* Categories replacement */
	if (geoxml_flow_get_categories_number(gebrme.current) > 0){
		GeoXmlCategory *category;
		GString * catstr;

		geoxml_flow_get_category(gebrme.current, &category, 0);
		catstr = g_string_new(geoxml_category_get_name(category));
		geoxml_category_next(&category);

		while (category != NULL) {
			g_string_append(catstr, " | ");
			g_string_append(catstr, geoxml_category_get_name(category));
			geoxml_category_next(&category);
		}

		ptr = strstr(help->str, "First category | Second category | ...");

		while (ptr != NULL){
			pos = (ptr - help->str)/sizeof(char);
			g_string_erase(help, pos, 38);
			g_string_insert(help, pos, catstr->str);
			ptr = strstr(help->str, "First category | Second category | ...");
		}

		g_string_free(catstr, TRUE);
	}
}



void
help_show(const gchar * help)
{
	FILE *		htmlfp;
	GString *	htmlpath;
	GString *	cmdline;
	GString *	prepared_html;

	prepared_html = g_string_new(help);
	help_fix_css(prepared_html);

	/* create temporary filename */
	htmlpath = g_string_new(tmpfile_template);
	mktemp(htmlpath->str);
	g_string_append(htmlpath, ".html");

	/* open temporary file with help from XML */
	htmlfp = fopen(htmlpath->str, "w");
	if (htmlfp == NULL) {
		gtk_statusbar_push(GTK_STATUSBAR(gebrme.statusbar), 0,
			_("Could not create an temporary file."));
		goto out;
	}
	fputs(prepared_html->str, htmlfp);
	fclose(htmlfp);

	/* Add file to list of files to be removed */
	gebrme.tmpfiles = g_slist_append(gebrme.tmpfiles, htmlpath->str);

	/* Launch an external browser */
	cmdline = g_string_new (gebrme.config.browser->str);
	g_string_append(cmdline, " file://");
	g_string_append(cmdline, htmlpath->str);
	g_string_append(cmdline, " &");
	system(cmdline->str);

	g_string_free(cmdline, TRUE);
out:	g_string_free(htmlpath, FALSE);
	g_string_free(prepared_html, TRUE);
}

GString *
help_edit(const gchar * help)
{
	FILE *		fp;
	GString *	htmlpath;
	GString *	prepared_html;
	GString *	cmdline;
	gchar		buffer[100];

	prepared_html = g_string_new(help);

	/* help empty; create from template. */
	if (strlen(prepared_html->str) == 0) {
		/* Read back the help from file */
		fp = fopen(GEBRME_DATA_DIR "help-template.html", "r");
		if (fp == NULL) {
			gtk_statusbar_push(GTK_STATUSBAR(gebrme.statusbar), 0,
				_("Could not open template. Check your installation."));
			return prepared_html;
		}

		while (fgets(buffer, sizeof(buffer), fp))
			g_string_append(prepared_html, buffer);

		fclose(fp);

		/* Substitute title, description and categories */
		help_subst_fields(prepared_html);
	}

	/* CSS fix */
	help_fix_css(prepared_html);

	/* create temporary filename */
	htmlpath = g_string_new(tmpfile_template);
	mktemp(htmlpath->str);
	g_string_append(htmlpath, ".html");

	/* load html into a temporary file */
	fp = fopen(htmlpath->str, "w");
	if (fp == NULL) {
		gtk_statusbar_push(GTK_STATUSBAR(gebrme.statusbar), 0,
				_("Could not create a temporary file."));
		goto out;
	}
	fputs(prepared_html->str, fp);
	fclose(fp);

	/* Add file to list of files to be removed */
	gebrme.tmpfiles = g_slist_append(gebrme.tmpfiles, htmlpath->str);

	/* run editor */
	cmdline = g_string_new("");
	g_string_printf(cmdline, "%s %s", gebrme.config.htmleditor->str, htmlpath->str);
	if (system(cmdline->str)) {
		gtk_statusbar_push(GTK_STATUSBAR(gebrme.statusbar), 0,
			_("Could not launch editor"));
		goto out;
	}
	g_string_free(cmdline, TRUE);

	/* read back the help from file */
	fp = fopen(htmlpath->str, "r");
	if (fp == NULL) {
		gtk_statusbar_push(GTK_STATUSBAR(gebrme.statusbar), 0,
			_("Could not read created temporary file."));
		goto out;
	}
	g_string_assign(prepared_html, "");
	while (fgets(buffer, sizeof(buffer), fp))
		g_string_append(prepared_html, buffer);
	fclose(fp);

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

	unlink(htmlpath->str);
out:	g_string_free(htmlpath, FALSE);
	return prepared_html;
}
