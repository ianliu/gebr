/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <glib/gi18n.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gebr-expr.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "menu.h"
#include "flow.h"
#include "document.h"
#include "ui_help.h"
#include "ui_flow.h"

/*
 * Prototypes
 */

static void parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters *ui_parameters);
static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters *ui_parameters);

/*
 * Public functions.
 */

struct ui_parameters *parameters_configure_setup_ui(void)
{
	struct ui_parameters *ui_parameters;
	GtkWidget *dialog;
	GtkWidget *button;

	if (flow_edition_get_selected_component(NULL, FALSE) == FALSE)
		return NULL;

	ui_parameters = g_new(struct ui_parameters, 1);
	dialog = gtk_dialog_new_with_buttons(_("Parameters"), GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);
	g_object_set(dialog, "type-hint", GDK_WINDOW_TYPE_HINT_NORMAL, NULL);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	if (strlen(gebr_geoxml_program_get_help(gebr.program)) == 0)	
		gtk_widget_set_sensitive(button, FALSE);

	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Default"), GTK_RESPONSE_APPLY);
	g_object_set(G_OBJECT(button), "image",
		     gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_BUTTON), NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	g_signal_connect(dialog, "response", G_CALLBACK(parameters_actions), ui_parameters);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(parameters_on_delete_event), ui_parameters);

	ui_parameters->dialog = dialog;
	ui_parameters->program_edit =
		gebr_gui_program_edit_setup_ui(GEBR_GEOXML_PROGRAM(gebr_geoxml_sequence_append_clone(GEBR_GEOXML_SEQUENCE(gebr.program))),
					       flow_io_customized_paths_from_line, FALSE,
					       gebr.validator);
	gebr_gui_program_edit_reload(ui_parameters->program_edit, NULL);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_parameters->program_edit->widget, TRUE, TRUE, 0);
	gtk_widget_show(dialog);

	/* adjust window size to fit parameters horizontally */
	gdouble width;
	width = GTK_BIN(GTK_BIN(ui_parameters->program_edit->scrolled_window)->child)->child->allocation.width + 30;
	if (width >= gdk_screen_get_width(gdk_screen_get_default()))
		gtk_window_maximize(GTK_WINDOW(dialog));
	else
		gtk_widget_set_size_request(dialog, width, 500);

	return ui_parameters;
}

gboolean validate_selected_program(GError **error)
{
	GtkTreeIter iter;
	flow_edition_get_selected_component(&iter, FALSE);
	return validate_program_iter(&iter, error);
}

gboolean validate_program_iter(GtkTreeIter *iter, GError **error)
{
	GebrGeoXmlProgram *program;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), iter,
			   FSEQ_GEBR_GEOXML_POINTER, &program, -1);

	if (gebr_geoxml_program_is_valid(program,
					 gebr.validator,
					 error))
		return TRUE;

	return FALSE;
}

/*
 * Private functions.
 */

/*
 * parameters_actions:
 * @dialog: the dialog that emitted the response signal
 * @arg1: the response id
 * @ui_parameters: a #ui_parameter's structure
 *
 * Take the appropriate action when the parameter dialog emmits a response signal.
 */
static void parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters *ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:{
		GtkTreeIter iter;
		const gchar * icon;

		gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(ui_parameters->program_edit->program),
						 GEBR_GEOXML_SEQUENCE(gebr.program));
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(gebr.program));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(ui_parameters->program_edit->program));

		/* Update interface */
		flow_edition_get_selected_component(&iter, FALSE);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
				   FSEQ_GEBR_GEOXML_POINTER, ui_parameters->program_edit->program,
				   FSEQ_ICON_COLUMN, icon,
				   -1);
		flow_edition_select_component_iter(&iter);

		if (validate_selected_program(NULL))
			flow_edition_status_changed (GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
		else
			flow_edition_status_changed (GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);

		break;
	}
	case GTK_RESPONSE_APPLY:
		gebr_geoxml_parameters_reset_to_default(gebr_geoxml_program_get_parameters(ui_parameters->program_edit->program));
		gebr_gui_program_edit_reload(ui_parameters->program_edit, NULL);
		return;
	case GTK_RESPONSE_HELP:
		gebr_help_show_selected_program_help();
		return;
	case GTK_RESPONSE_CANCEL:
	default:
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(ui_parameters->program_edit->program));
		break;
	}

	/* gui free */
	gebr_gui_program_edit_destroy(ui_parameters->program_edit);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters);
}

/**
 * \internal
 */
static gboolean parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters *ui_parameters)
{
	parameters_actions(dialog, GTK_RESPONSE_CANCEL, ui_parameters);
	return FALSE;
}
