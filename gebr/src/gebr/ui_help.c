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
#include <libgebr/gui.h>

#include "gebr-help-edit-widget.c"
#include "ui_help.h"
#include "gebr.h"
#include "document.h"
#include "../defines.h"
#include "menu.h"

void program_help_show(void)
{
	if (!flow_edition_get_selected_component(NULL, TRUE))
		return;

	help_show(GEBR_GEOXML_OBJECT(gebr.program), FALSE, _("Program help"));
}

void help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title)
{
	const gchar * html;
	GtkWidget * window;

	window = gebr_gui_html_viewer_window_new(title); 

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		html = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object));
		gebr_gui_html_viewer_window_set_geoxml_object(GEBR_GUI_HTML_VIEWER_WINDOW(window), object);
	} else
		html = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object));

	gebr_gui_html_viewer_window_show_html(GEBR_GUI_HTML_VIEWER_WINDOW(window), html);

	gtk_dialog_run(GTK_DIALOG(window));
}

void help_show_callback(GtkButton * button, GebrGeoXmlDocument * document)
{
	help_show(GEBR_GEOXML_OBJECT(document), FALSE, gebr_geoxml_document_get_title(document));
}

void help_edit(GtkButton * button, GebrGeoXmlDocument * document)
{
	if (gebr.config.native_editor || gebr.config.editor->len == 0) {
		const gchar * help;
		GtkWidget * window;
		GtkWidget * help_edit_widget;

		help = gebr_geoxml_document_get_help(document);
		help_edit_widget = gebr_help_edit_widget_new(document, help);
		window = gebr_gui_help_edit_window_new(GEBR_GUI_HELP_EDIT_WIDGET(help_edit_widget));
		gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
		gtk_dialog_run(GTK_DIALOG(window));
		gtk_widget_destroy(window);
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
