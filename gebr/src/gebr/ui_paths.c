/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_paths.c
 * Line paths UI.
 *
 * Assembly line's paths dialog.
 */

#include <string.h>
#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/gui/valuesequenceedit.h>
#include <libgebr/gui/gtkfileentry.h>

#include "ui_paths.h"
#include "gebr.h"
#include "document.h"

/*
 * Prototypes
 */



/*
 * Section: Private
 * Private functions.
 */

void
path_add(ValueSequenceEdit * sequence_edit)
{
	GtkFileEntry *      file_entry;
	gchar *	            path;


	g_object_get(G_OBJECT(sequence_edit),  "value-widget", &file_entry, NULL);
	path = gtk_file_entry_get_path(file_entry);

	if (!strlen(path))
		return;

	value_sequence_edit_add(sequence_edit, GEOXML_SEQUENCE(geoxml_line_append_path(gebr.line, path)));
}

gboolean
path_save(void)
{
	document_save(GEOXML_DOCUMENT(gebr.line));
	project_line_info_update();
	return TRUE;
}

/*
 * Section: Public
 * Private functions.
 */

void
path_list_setup_ui(void)
{

	GtkWidget *		dialog;
	GtkWidget *		file_entry;
	GtkWidget *		path_sequence_edit;
	GeoXmlSequence *	path_sequence;
	GString *		dialog_title;

	if (gebr.line == NULL) {
		gebr_message(LOG_WARNING, TRUE, FALSE, _("No line selected"));
		return;
	}

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Path list for line '%s'"), geoxml_document_get_title(GEOXML_DOCUMENT(gebr.line)));

	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	gtk_widget_set_size_request(dialog, 380, 260);

	file_entry = gtk_file_entry_new(NULL);
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry), TRUE);
	gtk_widget_set_size_request(file_entry, 220, 30);

	geoxml_line_get_path(gebr.line, &path_sequence, 0);
	path_sequence_edit = value_sequence_edit_new(file_entry);
	value_sequence_edit_load(VALUE_SEQUENCE_EDIT(path_sequence_edit), path_sequence,
		(ValueSequenceSetFunction)geoxml_value_sequence_set,
		(ValueSequenceGetFunction)geoxml_value_sequence_get, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), path_sequence_edit, TRUE, TRUE, 0);

	g_signal_connect(GTK_OBJECT(path_sequence_edit), "add-request",
		GTK_SIGNAL_FUNC(path_add), NULL);
	g_signal_connect(GTK_OBJECT(dialog), "delete-event",
		GTK_SIGNAL_FUNC(path_save), NULL);

  	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	document_save(GEOXML_DOCUMENT(gebr.line));
	project_line_info_update();

	/* frees */
	gtk_widget_destroy(dialog);
	g_string_free(dialog_title, TRUE);
}
