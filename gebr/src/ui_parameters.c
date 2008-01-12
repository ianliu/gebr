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
 * File: ui_parameters.c
 * Program's parameter window stuff
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <geoxml.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "support.h"
#include "menu.h"
#include "flow.h"
#include "ui_help.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static void
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters);

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

	GtkTreeSelection *		selection;
	GtkTreeModel *			model;
	GtkTreeIter			iter;
	GtkTreePath *			path;

	GtkWidget *			label;
	GtkWidget *			vbox;
	GtkWidget *			scrolledwin;
	GtkWidget *			viewport;

	GeoXmlSequence *		program;
	GeoXmlParameters *		parameters;
	GeoXmlSequence *		parameter;
	gulong				i;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, _("No program selected"));
		return NULL;
	}

	/* alloc struct */
	ui_parameters = g_malloc(sizeof(struct ui_parameters));

	/* get program index */
	path = gtk_tree_model_get_path (model, &iter);
	ui_parameters->program_index = (int)atoi(gtk_tree_path_to_string(path));
	gtk_tree_path_free(path);

	ui_parameters->dialog = gtk_dialog_new_with_buttons(_("Parameters"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters->dialog), "Default", GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(ui_parameters->dialog), "Help", GTK_RESPONSE_HELP);
	gtk_widget_set_size_request(ui_parameters->dialog, 630, 400);
	gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(ui_parameters->dialog)->vbox), FALSE);

	/* take the apropriate action when a button is pressed */
	g_signal_connect(ui_parameters->dialog, "response",
			G_CALLBACK (parameters_actions), ui_parameters);

	/* flow title */
	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_parameters->dialog)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0);

	/* scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	viewport = gtk_viewport_new(NULL, NULL);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG (ui_parameters->dialog)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (scrolledwin), viewport);
	gtk_container_add(GTK_CONTAINER (viewport), vbox);

	/* starts reading program and its parameters */
	geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
	parameters = geoxml_program_get_parameters(GEOXML_PROGRAM(program));

	/* program title in bold */
	{
		gchar *	markup;

		markup = g_markup_printf_escaped("<big><b>%s</b></big>",
			geoxml_program_get_title(GEOXML_PROGRAM(program)));
		gtk_label_set_markup(GTK_LABEL (label), markup);
		g_free(markup);
	}

	ui_parameters->widgets_number = geoxml_parameters_get_number(parameters);
	ui_parameters->widgets = g_malloc(sizeof(GtkWidget*)*ui_parameters->widgets_number);

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (i = 0; i < ui_parameters->widgets_number; ++i) {
		enum GEOXML_PARAMETERTYPE	type;

		GtkWidget *			hbox;
		struct parameter_widget *	widget;

		type = geoxml_parameter_get_type(GEOXML_PARAMETER(parameter));
		switch (type) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			widget = parameter_widget_new_float(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_INT:
			widget = parameter_widget_new_int(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_STRING:
			widget = parameter_widget_new_string(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			widget = parameter_widget_new_range(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_FLAG:
			widget = parameter_widget_new_flag(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			widget = parameter_widget_new_file(GEOXML_PARAMETER(parameter), FALSE);
			break;
		case GEOXML_PARAMETERTYPE_ENUM:
			widget = parameter_widget_new_enum(GEOXML_PARAMETER(parameter), FALSE);
			break;
		default:
			continue;
			break;
		}

		hbox = gtk_hbox_new(FALSE, 10);
		if (type != GEOXML_PARAMETERTYPE_FLAG) {
			GtkWidget *	label;
			gchar *		label_str;

			label_str = (gchar*)geoxml_parameter_get_label(GEOXML_PARAMETER(parameter));
			label = gtk_label_new("");

			if (geoxml_program_parameter_get_required(GEOXML_PROGRAM_PARAMETER(parameter)) == TRUE) {
				gchar *	markup;

				markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
				gtk_label_set_markup(GTK_LABEL(label), markup);
				g_free(markup);
			} else
				gtk_label_set_text(GTK_LABEL(label), label_str);

			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), widget->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(widget->value_widget), "label",
				geoxml_program_parameter_get_label(GEOXML_PROGRAM_PARAMETER(parameter)), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), widget->widget, FALSE, FALSE, 0);
		}

		/* set and add */
		ui_parameters->widgets[i] = widget;
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

		geoxml_sequence_next(&parameter);
	}

	gtk_widget_show_all(ui_parameters->dialog);

	return ui_parameters;
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
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GeoXmlSequence *	program;
		int			i;

		/* Set program state to configured */
		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		geoxml_program_set_status(GEOXML_PROGRAM(program), "configured");
		/* Update interface */
		{
			GtkTreeSelection *	selection;
			GtkTreeModel *		model;
			GtkTreeIter		iter;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
			gtk_tree_selection_get_selected(selection, &model, &iter);
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
					FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_apply,
					-1);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gebr.configured_menuitem), TRUE);
		}

		for (i = 0; i < ui_parameters->widgets_number; ++i) {
			enum GEOXML_PARAMETERTYPE	type;
			struct parameter_widget *	widget;

			widget = ui_parameters->widgets[i];
			type = geoxml_parameter_get_type(widget->parameter);
			if (type == GEOXML_PARAMETERTYPE_GROUP) {
				/* TODO: call recursive function */
				continue;
			}

			parameter_widget_submit(widget);
		}

		flow_save();
		break;
	} case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_DEFAULT: {
		int	i;

		for (i = 0; i < ui_parameters->widgets_number; ++i) {
			enum GEOXML_PARAMETERTYPE	type;
			struct parameter_widget *	widget;

			widget = ui_parameters->widgets[i];
			type = geoxml_parameter_get_type(widget->parameter);
			if (type == GEOXML_PARAMETERTYPE_GROUP) {
				/* TODO: call recursive function */
				continue;
			}

			geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(widget->parameter),
				geoxml_program_parameter_get_default(GEOXML_PROGRAM_PARAMETER(widget->parameter)));
			parameter_widget_update(widget);
		}
		return;
	} case GTK_RESPONSE_HELP: {
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreeIter		iter;
		GtkTreePath *		path;

		GeoXmlFlow *		menu;
		GeoXmlSequence *	program;

		gulong			index;
		gchar *			menu_filename;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
		if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
			gebr_message(ERROR, TRUE, FALSE, _("No flow component selected"));
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		index = (gulong)atoi(gtk_tree_path_to_string(path));
		gtk_tree_path_free(path);

		/* get the program and its path on menu */
		geoxml_flow_get_program(gebr.flow, &program, index);
		geoxml_program_get_menu(GEOXML_PROGRAM(program), &menu_filename, &index);

		menu = menu_load(menu_filename);
		if (menu == NULL)
			break;

		/* go to menu's program index specified in flow */
		geoxml_flow_get_program(menu, &program, index);
		help_show(geoxml_program_get_help(GEOXML_PROGRAM(program)), _("Program help"));

		geoxml_document_free(GEOXML_DOC(menu));
		return;
	} default:
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(ui_parameters->widgets);
	g_free(ui_parameters);
}
