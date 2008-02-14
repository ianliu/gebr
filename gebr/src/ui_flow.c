/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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
 * File: ui_flow.c
 */

#include <gui/gtkfileentry.h>

#include "ui_flow.h"
#include "gebr.h"
#include "support.h"
#include "flow.h"
#include "ui_flow_browse.h"

extern gchar * no_flow_selected_error;

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: flow_io_actions
 * Actions for Flow IO files edition dialog
 */
static void
flow_io_actions(GtkDialog * dialog, gint arg1, struct ui_flow_io * ui_flow_io)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		geoxml_flow_io_set_input(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(ui_flow_io->input)));
		geoxml_flow_io_set_output(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(ui_flow_io->output)));
		geoxml_flow_io_set_error(gebr.flow,
			gtk_file_entry_get_path(GTK_FILE_ENTRY(ui_flow_io->error)));

		flow_save();
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(ui_flow_io->dialog));
	g_free(ui_flow_io);
}

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: flow_io_setup_ui
 * A dialog for user selection of the flow IO files
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
struct ui_flow_io *
flow_io_setup_ui(void)
{
	struct ui_flow_io *	ui_flow_io;

	GtkWidget *		dialog;
	GtkWidget *		table;
	GtkWidget *		label;

	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return NULL;
	}

	/* alloc */
	ui_flow_io = g_malloc(sizeof(struct ui_flow_io));

	dialog = gtk_dialog_new_with_buttons(_("Flow input/output"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	ui_flow_io->dialog = dialog;
	gtk_widget_set_size_request(dialog, 390, 160);

	g_signal_connect(dialog, "response",
			G_CALLBACK(flow_io_actions), ui_flow_io);
	g_signal_connect(GTK_OBJECT(dialog), "delete_event",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Input */
	label = gtk_label_new(_("Input file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	ui_flow_io->input = gtk_file_entry_new(flow_io_customized_paths_from_line);
	gtk_widget_set_size_request(ui_flow_io->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io->input, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL,  3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io->input), geoxml_flow_io_get_input(gebr.flow));
	gtk_file_entry_set_do_overwrite_confirmation(GTK_FILE_ENTRY(ui_flow_io->input), FALSE);

	/* Output */
	label = gtk_label_new(_("Output file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	ui_flow_io->output = gtk_file_entry_new(flow_io_customized_paths_from_line);
	gtk_widget_set_size_request(ui_flow_io->output, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io->output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io->output), geoxml_flow_io_get_output(gebr.flow));

	/* Error */
	label = gtk_label_new(_("Error log file"));
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	ui_flow_io->error = gtk_file_entry_new(flow_io_customized_paths_from_line);
	gtk_widget_set_size_request(ui_flow_io->error, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_flow_io->error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_file_entry_set_path(GTK_FILE_ENTRY(ui_flow_io->error), geoxml_flow_io_get_error(gebr.flow));

	gtk_widget_show_all(dialog);

	return ui_flow_io;
}

/*
 * Function: customize_paths_from_line
 * Set line's path to input/output/error files
 *
 */
void
flow_io_customized_paths_from_line(GtkFileChooser * chooser)
{
	GError *		error;
	GeoXmlSequence *	path_sequence;

	if(gebr.line == NULL)
		return;

	error = NULL;
	geoxml_line_get_path(gebr.line, &path_sequence, 0);
	if (path_sequence != NULL) {
		gtk_file_chooser_set_current_folder(
			chooser, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(path_sequence)));

		do {
			gtk_file_chooser_add_shortcut_folder(
				chooser, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(path_sequence)),
				&error);
			geoxml_sequence_next(&path_sequence);
		} while (path_sequence != NULL);
	}
}

/*
 * Function: flow_add_programs_to_view
 * Add _flow_'s programs to flow sequence view
 *
 */
void
flow_add_programs_to_view(GeoXmlFlow * flow)
{
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	geoxml_flow_get_program(flow, &program, 0);
	while (program != NULL) {
		gchar *			menu;
		gulong			prog_index;
		const gchar *		status;

		GdkPixbuf *		pixbuf;

		geoxml_program_get_menu(GEOXML_PROGRAM(program), &menu, &prog_index);
		status = geoxml_program_get_status(GEOXML_PROGRAM(program));

		if (g_ascii_strcasecmp(status, "unconfigured") == 0)
			pixbuf = gebr.pixmaps.stock_warning;
		else if (g_ascii_strcasecmp(status, "configured") == 0)
			pixbuf = gebr.pixmaps.stock_apply;
		else if (g_ascii_strcasecmp(status, "disabled") == 0)
			pixbuf = gebr.pixmaps.stock_cancel;
		else {
			gebr_message(LOG_WARNING, TRUE, TRUE, _("Unknown flow program '%s' status"),
				geoxml_program_get_title(GEOXML_PROGRAM(program)));
			pixbuf = NULL;
		}

		/* Add to the GUI */
		gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &iter);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
				FSEQ_TITLE_COLUMN, geoxml_program_get_title(GEOXML_PROGRAM(program)),
				FSEQ_STATUS_COLUMN, pixbuf,
				FSEQ_MENU_FILE_NAME_COLUMN, menu,
				FSEQ_MENU_INDEX, prog_index,
				-1);

		geoxml_sequence_next(&program);
	}
}
