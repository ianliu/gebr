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
	GString *html = g_string_new("");

	if (gebr_geoxml_object_get_type(object) == GEBR_GEOXML_OBJECT_TYPE_PROGRAM)
		g_string_assign(html, gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(object)));
	else
		g_string_assign(html, gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(object)));

	/* CSS to absolute path */
	gchar *gebrcsspos;
	if ((gebrcsspos = strstr(html->str, "gebr.css")) != NULL) {
		int pos = (gebrcsspos - html->str) / sizeof(gchar);
		g_string_erase(html, pos, 8);
		g_string_insert(html, pos, "file://" GEBR_DATA_DIR "/gebr.css");
	}

	gebr_gui_help_show(object, menu, html->str, title);

	/* frees */
	g_string_free(html, TRUE);
}

void help_show_callback(GtkButton * button, GebrGeoXmlDocument * document)
{
	help_show(GEBR_GEOXML_OBJECT(document), FALSE, gebr_geoxml_document_get_title(document));
}

void on_edit_preview_toggle(GtkToggleToolButton * button, GebrGuiHelpEditWidget * help_edit)
{
	gboolean active = gtk_toggle_tool_button_get_active(button);
	gebr_gui_help_edit_widget_set_editing(help_edit, active);
}

void help_edit(GtkButton * button, GebrGeoXmlDocument * document)
{
	if (gebr.config.native_editor || gebr.config.editor->len == 0) {
		const gchar * html;
		GtkWidget * vbox;
		GtkWidget * dialog;
		GtkWidget * toolbar;
		GtkToolItem * tool_item;
		GtkWidget * help_edit;

		vbox = gtk_vbox_new(FALSE, 0);
		html = gebr_geoxml_document_get_help(document);
		dialog = gtk_dialog_new();
		toolbar = gtk_toolbar_new();
		tool_item = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_EDIT);
		help_edit = gebr_help_edit_widget_new(GEBR_GEOXML_FLOW(document), html);

		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool_item), TRUE);
		gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
		g_signal_connect(tool_item, "toggled",
				 G_CALLBACK(on_edit_preview_toggle), help_edit);

		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
		gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), help_edit, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox, TRUE, TRUE, 0);

		gtk_widget_show(vbox);
		gtk_widget_show_all(toolbar);
		gtk_widget_show(help_edit);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
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
