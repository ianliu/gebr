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
#include <math.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <geoxml.h>
#include <gui/gtkfileentry.h>

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

static GtkWidget *
parameters_create_label(GeoXmlParameter * parameter);

static GtkWidget *
parameters_add_input_float(GeoXmlParameter * parameter, GtkWidget ** widget);

static GtkWidget *
parameters_add_input_int(GeoXmlParameter * parameter, GtkWidget ** widget);

static GtkWidget *
parameters_add_input_string(GeoXmlParameter * parameter, GtkWidget ** widget);

static GtkWidget *
parameters_add_input_range(GeoXmlParameter * parameter, GtkWidget ** widget);

static GtkWidget *
parameters_add_input_flag(GeoXmlParameter * parameter, GtkWidget ** widget);

static GtkWidget *
parameters_add_input_file(GeoXmlParameter * parameter, GtkWidget ** widget);

static void
validate_int(GtkEntry *entry);

static void
validate_float(GtkEntry *entry);

gboolean
validate_on_leaving(GtkWidget * widget, GdkEventFocus * event, void (*function)(GtkEntry*));

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
		GtkWidget *	widget;
		GtkWidget *	parameter_widget;

		switch (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter))) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			widget = parameters_add_input_float(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_INT:
			widget = parameters_add_input_int(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_STRING:
			widget = parameters_add_input_string(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			widget = parameters_add_input_range(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_FLAG:
			widget = parameters_add_input_flag(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			widget = parameters_add_input_file(GEOXML_PARAMETER(parameter), &parameter_widget);
			break;
		default:
			continue;
			break;
		}

		/* set and add */
		ui_parameters->widgets[i] = parameter_widget;
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);

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
		GeoXmlSequence *	parameter;
		int			i;

		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		parameter = geoxml_parameters_get_first_parameter(
			geoxml_program_get_parameters(GEOXML_PROGRAM(program)));

		/* Set program state to configured */
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
			GtkWidget *			widget;

			type = geoxml_parameter_get_type(GEOXML_PARAMETER(parameter));
			if (type == GEOXML_PARAMETERTYPE_GROUP) {
				/* TODO: call recursive function */
				continue;
			}

			widget = ui_parameters->widgets[i];

			switch (type) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter),
					gtk_entry_get_text(GTK_ENTRY(widget)));
				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				GString *	value;

				value = g_string_new(NULL);

				g_string_printf(value, "%f", gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
				geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter), value->str);

				g_string_free(value, TRUE);
				break;
			} case GEOXML_PARAMETERTYPE_FLAG:
				geoxml_program_parameter_set_flag_state(GEOXML_PROGRAM_PARAMETER(parameter),
					gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
				break;
			case GEOXML_PARAMETERTYPE_FILE:
				geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter),
					gtk_file_entry_get_path(GTK_FILE_ENTRY(widget)));
				break;
			default:
				break;
			}

			geoxml_sequence_next(&parameter);
		}

		flow_save();
		break;
	} case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_DEFAULT: {
		GeoXmlSequence *	program;
		GeoXmlSequence *	parameter;
		int		 	i;

		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		parameter = geoxml_parameters_get_first_parameter(
			geoxml_program_get_parameters(GEOXML_PROGRAM(program)));

		for (i = 0; i < ui_parameters->widgets_number; ++i) {
			enum GEOXML_PARAMETERTYPE	type;
			GtkWidget *			widget;
			const gchar *			default_value;

			type = geoxml_parameter_get_type(GEOXML_PARAMETER(parameter));
			if (type == GEOXML_PARAMETERTYPE_GROUP) {
				/* TODO: call recursive function */
				continue;
			}

			widget = ui_parameters->widgets[i];
			default_value = geoxml_program_parameter_get_default(GEOXML_PROGRAM_PARAMETER(parameter));

			switch (type) {
			case GEOXML_PARAMETERTYPE_FLOAT:
			case GEOXML_PARAMETERTYPE_INT:
			case GEOXML_PARAMETERTYPE_STRING:
				gtk_entry_set_text(GTK_ENTRY(widget), default_value);
				break;
			case GEOXML_PARAMETERTYPE_RANGE: {
				gchar *	endptr;
				double	value;

				value = strtod(default_value, &endptr);
				if (endptr != default_value)
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
				break;
			} case GEOXML_PARAMETERTYPE_FLAG:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
					geoxml_program_parameter_get_flag_default(GEOXML_PROGRAM_PARAMETER(parameter)));
				break;
			case GEOXML_PARAMETERTYPE_FILE:
				gtk_file_entry_set_path(GTK_FILE_ENTRY(widget), default_value);
				break;
			default:
				break;
			}

			geoxml_sequence_next(&parameter);
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

static GtkWidget *
parameters_create_label(GeoXmlParameter * parameter)
{
	GtkWidget *	label;
	gchar *		label_str;

	label_str = (gchar*)geoxml_parameter_get_label(parameter);
	label = gtk_label_new("");

	if (geoxml_program_parameter_get_required(GEOXML_PROGRAM_PARAMETER(parameter)) == TRUE) {
		gchar *	markup;

		markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);
	} else
		gtk_label_set_text(GTK_LABEL(label), label_str);

	return label;
}

/*
 * Function: parameters_add_input_float
 * Add an input entry to a float parameter
 */
static GtkWidget *
parameters_add_input_float(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;

	hbox = parameters_add_input_string(parameter, widget);
	gtk_widget_set_size_request(*widget, 90, 30);

	g_signal_connect(GTK_OBJECT(*widget), "activate",
		GTK_SIGNAL_FUNC(validate_float), NULL);
	g_signal_connect(GTK_OBJECT(*widget), "focus-out-event",
		GTK_SIGNAL_FUNC(validate_on_leaving), &validate_float);

	return hbox;
}

/*
 * Function: parameters_add_input_int
 * Add an input entry to an integer parameter
 */
static GtkWidget *
parameters_add_input_int(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;

	hbox = parameters_add_input_string(parameter, widget);
	gtk_widget_set_size_request(*widget, 90, 30);

	g_signal_connect(GTK_OBJECT(*widget), "activate",
		GTK_SIGNAL_FUNC(validate_int), NULL);
	g_signal_connect(GTK_OBJECT(*widget), "focus-out-event",
		GTK_SIGNAL_FUNC(validate_on_leaving), &validate_int);

	return hbox;
}

/*
 * Function: parameters_add_input_string
 * Add an input entry to a string parameter
 */
static GtkWidget *
parameters_add_input_string(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	entry;

	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_widget_set_size_request (entry, 140, 30);
	gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

	gtk_entry_set_text(GTK_ENTRY(entry),
		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter)));

	*widget = entry;
	return hbox;
}

/*
 * Function: parameters_add_input_range
 * Add an input entry to a range parameter
 */
static GtkWidget *
parameters_add_input_range(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	spin;

	gchar *		min;
	gchar *		max;
	gchar *		step;

	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	geoxml_program_parameter_get_range_properties(
		GEOXML_PROGRAM_PARAMETER(parameter), &min, &max, &step);

	spin = gtk_spin_button_new_with_range(atof(min), atof(max), atof(step));
	gtk_widget_set_size_request(spin, 90, 30);
	gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin),
		atof(geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter))));

	*widget = spin;
	return hbox;
}

/*
 * Function: parameters_add_input_flag
 * Add an input entry to a flag parameter
 */
static GtkWidget *
parameters_add_input_flag(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;
	GtkWidget *	check_button;

	hbox = gtk_hbox_new(FALSE, 10);
	check_button = gtk_check_button_new_with_label(
		geoxml_program_parameter_get_label(GEOXML_PROGRAM_PARAMETER(parameter)));
	gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, TRUE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
		geoxml_program_parameter_get_flag_status(GEOXML_PROGRAM_PARAMETER(parameter)));

	*widget = check_button;
	return hbox;
}

/*
 * Function: parameters_add_input_file
 * Add an input entry and button to browse for a file or directory
 */
static GtkWidget *
parameters_add_input_file(GeoXmlParameter * parameter, GtkWidget ** widget)
{
	GtkWidget *	hbox;
	GtkWidget *	label;
	GtkWidget *	file_entry;

	/* hbox */
	hbox = gtk_hbox_new(FALSE, 10);
	label = parameters_create_label(parameter);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	/* file entry */
	file_entry = gtk_file_entry_new();
	gtk_widget_set_size_request(file_entry, 220, 30);
	gtk_box_pack_end(GTK_BOX(hbox), file_entry, FALSE, FALSE, 0);

	gtk_file_entry_set_path(GTK_FILE_ENTRY(file_entry),
		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter)));
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry),
		geoxml_program_parameter_get_file_be_directory(GEOXML_PROGRAM_PARAMETER(parameter)));

	*widget = file_entry;
	return hbox;
}

/*
 * Function: validate_int
 * Validate an int parameter
 */
static void
validate_int(GtkEntry *entry)
{
	gchar		number[15];
	gchar *		valuestr;
	gdouble		value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	value = g_ascii_strtod(valuestr, NULL);
	gtk_entry_set_text(entry, g_ascii_dtostr(number, 15, round(value)));
}

/*
 * Function: validate_float
 * Validate a float parameter
 */
static void
validate_float(GtkEntry *entry)
{
	gchar *		valuestr;
	gchar *		last;
	GString *	value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	/* initialization */
	value = g_string_new(NULL);

	g_ascii_strtod(valuestr, &last);
	g_string_assign(value, valuestr);
	g_string_truncate(value, strlen(valuestr) - strlen(last));
	gtk_entry_set_text(entry, value->str);

	/* frees */
	g_string_free(value, TRUE);
}

/*
 * Function: validate_on_leaving
 * Call a validation function
 */
gboolean
validate_on_leaving(GtkWidget * widget, GdkEventFocus * event, void (*function)(GtkEntry*))
{
	function(GTK_ENTRY(widget));

	return FALSE;
}
