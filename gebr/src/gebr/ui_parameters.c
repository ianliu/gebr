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

/*
 * File: ui_parameters.c
 * Program's parameter window stuff
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <libgebr/intl.h>
#include <libgebr/geoxml.h>
#include <libgebr/gui/utils.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "menu.h"
#include "flow.h"
#include "document.h"
#include "ui_help.h"
#include "ui_flow.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static void
parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters * ui_parameters);
static void
parameters_on_link_button_clicked(GtkButton * button, GeoXmlProgram * program);
static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters * ui_parameters);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: parameters_configure_setup_ui
 * Assembly a dialog to configure the current selected program's parameters
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
struct ui_parameters *
parameters_configure_setup_ui(void)
{
	struct ui_parameters *		ui_parameters;
	GtkWidget *			dialog;
	GtkWidget *			hbox;
	GtkWidget *			label;
	GtkWidget *			button;

	if (flow_edition_get_selected_component(NULL, FALSE) == FALSE)
		return NULL;

	ui_parameters = g_malloc(sizeof(struct ui_parameters));
	dialog = gtk_dialog_new_with_buttons(_("Parameters"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Default"), GTK_RESPONSE_DEFAULT);
	g_object_set(G_OBJECT(button),
		"image", gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_BUTTON), NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	gtk_widget_set_size_request(dialog, 630, 400);
	g_signal_connect(dialog, "response",
		G_CALLBACK(parameters_actions), ui_parameters);
	g_signal_connect(dialog, "delete-event",
		G_CALLBACK(parameters_on_delete_event), ui_parameters);

	/* program title in bold */
	label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0);
	{
		gchar *	markup;

		markup = g_markup_printf_escaped("<big><b>%s</b></big>",
			geoxml_program_get_title(gebr.program));
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);
	}

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 5);
	/* program description */
	label = gtk_label_new(geoxml_program_get_description(gebr.program));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* program URL */
	if (strlen(geoxml_program_get_url(gebr.program))) {
		GtkWidget *	alignment;
		GtkWidget *	button;

		alignment = gtk_alignment_new(1, 0, 0, 0);
		gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 5);
		button = gtk_button_new_with_label(_("Link"));
		gtk_container_add(GTK_CONTAINER(alignment), button);
		gtk_widget_show_all(alignment);
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

		g_signal_connect(button, "clicked",
			G_CALLBACK(parameters_on_link_button_clicked), gebr.program);
	}

	*ui_parameters = (struct ui_parameters) {
		.dialog = dialog,
		.program_edit = libgebr_gui_program_edit_setup_ui(
			GEOXML_PROGRAM(geoxml_sequence_append_clone(GEOXML_SEQUENCE(gebr.program))),
			flow_io_customized_paths_from_line, FALSE)
	};
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_parameters->program_edit.widget, TRUE, TRUE, 0);

	gtk_widget_show(dialog);

	return ui_parameters;
}

/*
 * Function: parameters_reset_to_default
 * Change all parameters' values from _parameters_ to their default value
 *
 */
void
parameters_reset_to_default(GeoXmlParameters * parameters)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		if (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter)) == GEOXML_PARAMETERTYPE_GROUP) {
			GeoXmlSequence *	instance;

			geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; geoxml_sequence_next(&instance)) {
				parameters_reset_to_default(GEOXML_PARAMETERS(instance));
				geoxml_parameters_set_selected(GEOXML_PARAMETERS(instance),
					geoxml_parameters_get_exclusive(GEOXML_PARAMETERS(instance)));
			}

			continue;
		}

		GeoXmlSequence *	value;
		GeoXmlSequence *	default_value;

		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
			FALSE, &value, 0);
		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
			TRUE, &default_value, 0);
		for (; default_value != NULL; geoxml_sequence_next(&default_value), geoxml_sequence_next(&value)) {
			if (value == NULL)
				value = GEOXML_SEQUENCE(geoxml_program_parameter_append_value(
					GEOXML_PROGRAM_PARAMETER(parameter), FALSE));
			geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(value),
				geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(default_value)));
		}

		/* remove extras values */
		while (value != NULL) {
			GeoXmlSequence *	tmp;

			tmp = value;
			geoxml_sequence_next(&tmp);
			geoxml_sequence_remove(value);
			value = tmp;
		}
	}
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: parameters_actions
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
static void
parameters_actions(GtkDialog * dialog, gint arg1, struct ui_parameters * ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter		iter;

		geoxml_program_set_status(GEOXML_PROGRAM(ui_parameters->program_edit.program), "configured");
		geoxml_sequence_move_before(GEOXML_SEQUENCE(ui_parameters->program_edit.program),
			GEOXML_SEQUENCE(gebr.program));
		geoxml_sequence_remove(GEOXML_SEQUENCE(gebr.program));
		document_save(GEOXML_DOCUMENT(gebr.flow));

		/* Update interface */
		flow_edition_get_selected_component(&iter, FALSE);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
			FSEQ_GEOXML_POINTER, ui_parameters->program_edit.program,
			FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_apply,
			-1);
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(
			gebr.action_group, "flow_edition_status_configured")), TRUE);

		break;
	} case GTK_RESPONSE_DEFAULT:
		parameters_reset_to_default(geoxml_program_get_parameters(ui_parameters->program_edit.program));
		libgebr_gui_program_edit_reload(&ui_parameters->program_edit, NULL);
		return;
	case GTK_RESPONSE_HELP:
		program_help_show();
		return;
	case GTK_RESPONSE_CANCEL:
	default:
		geoxml_sequence_remove(GEOXML_SEQUENCE(ui_parameters->program_edit.program));
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters);
}

static void
parameters_on_link_button_clicked(GtkButton * button, GeoXmlProgram * program)
{
	GString * cmd_line;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s %s &", gebr.config.browser->str, geoxml_program_get_url(program));
	system(cmd_line->str);

	g_string_free(cmd_line, TRUE);
}

static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters * ui_parameters)
{
	parameters_actions(dialog, GTK_RESPONSE_CANCEL, ui_parameters);
	return FALSE;
}
