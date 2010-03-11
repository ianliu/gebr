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
#include <libgebr/gui/help.h>

#include "ui_help.h"
#include "gebr.h"
#include "document.h"
#include "../defines.h"
#include "menu.h"

void program_help_show(void)
{
	if (!flow_edition_get_selected_component(NULL, TRUE))
		return;

	help_show(gebr_geoxml_program_get_help(gebr.program), _("Program help"));
}

void help_show(const gchar * help, const gchar * title)
{
	GString *prepared_html;
	FILE *html_fp;
	GString *html_path;

	/* initialization */
	prepared_html = g_string_new(NULL);
	/* CSS to absolute path */
	{
		gchar *gebrcsspos;
		int pos;

		g_string_assign(prepared_html, help);

		if ((gebrcsspos = strstr(prepared_html->str, "gebr.css")) != NULL) {
			pos = (gebrcsspos - prepared_html->str) / sizeof(char);
			g_string_erase(prepared_html, pos, 8);
			g_string_insert(prepared_html, pos, "file://" GEBR_DATA_DIR "/gebr.css");
		}
	}

	/* initialization */
	html_path = gebr_make_temp_filename("gebr_XXXXXX.html");

	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to write help in temporary file."));
		goto out;
	}
	fwrite(prepared_html->str, sizeof(char), strlen(prepared_html->str), html_fp);
	fclose(html_fp);

	/* Add file to list of files to be removed */
	gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_path->str);

	g_string_prepend(html_path, "file://");
	gebr_gui_help_show(html_path->str, title);

	/* frees */
 out:	g_string_free(html_path, FALSE);
	g_string_free(prepared_html, TRUE);
}

void help_show_callback(GtkButton * button, GebrGeoXmlDocument * document)
{
	help_show(gebr_geoxml_document_get_help(document), gebr_geoxml_document_get_title(document));
}

/**
 * \internal
 * Save document upon help change.
 */
static void help_edit_on_edited(GebrGeoXmlDocument * document, const gchar * help)
{
	document_save(document, TRUE);
}

void help_edit(GtkButton * button, GebrGeoXmlDocument * document)
{
	if (gebr.config.editor->len == 0) {
		gebr_gui_help_edit(document, (GebrGuiHelpEdited)help_edit_on_edited, NULL, FALSE);
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
