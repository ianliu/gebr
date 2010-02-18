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

	if (!gebr.config.browser->len) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No editor defined. Choose one at Configure/Preferences"));
		return;
	}

	/* initialization */
	html_path = gebr_make_temp_filename("gebr_XXXXXX.html");

	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to write help in temporary file"));
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
	document_save(document);
}

void help_edit(GtkButton * button, GebrGeoXmlDocument * document)
{
	gebr_gui_help_edit(document, (GebrGuiHelpEdited)help_edit_on_edited);
}
