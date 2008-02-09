/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/*
 * File: ui_help.c
 * Responsible for help/report exibition and edition
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <misc/utils.h>

#include "ui_help.h"
#include "gebr.h"
#include "defines.h"
#include "support.h"

#define BUFFER_SIZE 1024

gchar * unable_to_write_help_error = _("Unable to write help in temporary file");

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: help_show
 * Open user's browser with _help_
 */
void
help_show(const gchar * help, const gchar * title)
{
	FILE *		html_fp;
	GString *	html_path;

	GString *	ghelp;
	GString *	url;
	GString *	cmd_line;

	if (!gebr.config.browser->len) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("No editor defined. Choose one at Configure/Preferences"));
		return;
	}

	/* initialization */
	html_path = make_temp_filename("gebr_XXXXXX.html");
	ghelp = g_string_new(NULL);
	url = g_string_new(NULL);
	cmd_line = g_string_new(NULL);

	/* Gambiarra */
	{
		gchar *	gebrcsspos;
		int	pos;

		g_string_assign(ghelp, help);

		if ((gebrcsspos = strstr(ghelp->str, "gebr.css")) != NULL) {
			pos = (gebrcsspos - ghelp->str)/sizeof(char);
			g_string_erase(ghelp, pos, 8);
			g_string_insert(ghelp, pos, "file://" GEBR_DATA_DIR "/gebr.css");
		}
	}

	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(LOG_ERROR, TRUE, TRUE, unable_to_write_help_error);
		goto out;
	}
	fwrite(ghelp->str, sizeof(char), strlen(ghelp->str), html_fp);
	fclose(html_fp);

	/* Add file to list of files to be removed */
	gebr.tmpfiles = g_slist_append(gebr.tmpfiles, html_path->str);

	/* Launch an external browser */
	g_string_printf(url, "file://%s", html_path->str);
	g_string_printf(cmd_line, "%s %s &", gebr.config.browser->str, url->str);
	system(cmd_line->str);

	/* frees */
out:	g_string_free(html_path, FALSE);
	g_string_free(ghelp, TRUE);
	g_string_free(url, TRUE);
	g_string_free(cmd_line, TRUE);
}

/* Function: help_edit
 * Edit help in editor.
 *
 * Edit help in editor as reponse to button clicks.
 */
void
help_edit(GtkButton * button, GeoXmlDocument * document)
{
	FILE *		html_fp;
	GString *	html_path;

	gchar		buffer[BUFFER_SIZE];
	GString *	help;
	GString *	cmd_line;

	/* Check for editor */
	if (!gebr.config.editor->len) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("No editor defined. Choose one at Configure/Preferences"));
		return;
	}

	/* initialization */
	html_path = make_temp_filename("gebr_XXXXXX.html");
	cmd_line = g_string_new(NULL);
	help = g_string_new(NULL);

	/* Write current help to temporary file */
	html_fp = fopen(html_path->str, "w");
	if (html_fp == NULL) {
		gebr_message(LOG_ERROR, TRUE, TRUE, unable_to_write_help_error);
		goto out;
	}
	fputs(geoxml_document_get_help(document), html_fp);
	fclose(html_fp);

	/* Run editor and wait for user... */
	g_string_printf(cmd_line, "%s %s", gebr.config.editor->str, html_path->str);
	system(cmd_line->str);

	/* Read back the help from file */
	html_fp = fopen(html_path->str, "r");
	if (html_fp == NULL) {
		gebr_message(LOG_ERROR, TRUE, TRUE, _("Could not read created temporary file."));
		goto out;
	}
	while (fgets(buffer, BUFFER_SIZE, html_fp) != NULL)
		g_string_append(help, buffer);
	fclose(html_fp);
	g_unlink(html_path->str);

	/* ensure UTF-8 encoding */
	if (g_utf8_validate(help->str, -1, NULL) == FALSE) {
		gchar *		converted;
		gsize		bytes_read;
		gsize		bytes_written;
		GError *	error;

		error = NULL;
		converted = g_locale_to_utf8(help->str, -1, &bytes_read, &bytes_written, &error);
		/* TODO: what else should be tried? */
		if (converted == NULL) {
			g_free(converted);
			gebr_message(LOG_ERROR, TRUE, TRUE, _("Please change the report encoding to UTF-8"));
			goto out;
		}

		g_string_assign(help, converted);
		g_free(converted);
	}

	/* Finally, the edited help back to the document */
	geoxml_document_set_help(document, help->str);

	/* frees */
out:	g_string_free(html_path, TRUE);
	g_string_free(cmd_line, TRUE);
	g_string_free(help, TRUE);
}
