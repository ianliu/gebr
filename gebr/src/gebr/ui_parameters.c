/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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
#include <gui/utils.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "support.h"
#include "menu.h"
#include "flow.h"
#include "ui_help.h"
#include "ui_flow.h"

#define GTK_RESPONSE_DEFAULT	GTK_RESPONSE_APPLY

/*
 * Prototypes
 */

static void
parameters_load_program(struct ui_parameters * ui_parameters);

static struct parameter_data *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter,
	GeoXmlParameter * selected, GSList ** radio_group);

static void
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters);

static void
parameters_change_selected(GtkToggleButton * toggle_button, struct parameter_data * data);

static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
parameters_on_link_button_clicked(GtkButton * button, GeoXmlProgram * program);

static gboolean
parameters_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_parameters * ui_parameters);

static void
parameters_free_parameter_data(GtkWidget * widget, struct parameter_data * data);

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

	GtkWidget *			dialog;
	GtkWidget *			label;
	GtkWidget *			vbox;
	GtkWidget *			hbox;
	GtkWidget *			scrolledwin;
	GtkWidget *			viewport;

	GeoXmlSequence *		program;
	int				program_index;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("No program selected"));
		return NULL;
	}

	/* alloc struct */
	ui_parameters = g_malloc(sizeof(struct ui_parameters));

	/* get program index and load it */
	path = gtk_tree_model_get_path(model, &iter);
	program_index = (int)atoi(gtk_tree_path_to_string(path));
	gtk_tree_path_free(path);
	geoxml_flow_get_program(gebr.flow, &program, program_index);

	dialog = gtk_dialog_new_with_buttons(_("Parameters"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Default"), GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	gtk_widget_set_size_request(dialog, 630, 400);
	gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->vbox), FALSE);

	/* take the apropriate action when a button is pressed */
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
			geoxml_program_get_title(GEOXML_PROGRAM(program)));
		gtk_label_set_markup(GTK_LABEL (label), markup);
		g_free(markup);
	}

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 5);
	/* program description */
	label = gtk_label_new(geoxml_program_get_description(GEOXML_PROGRAM(program)));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* program URL */
	if (strlen(geoxml_program_get_url(GEOXML_PROGRAM(program)))) {
		GtkWidget *	alignment;
		GtkWidget *	button;

		alignment = gtk_alignment_new(1, 0, 0, 0);
		gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 5);
		button = gtk_button_new_with_label(_("Link"));
		gtk_widget_show(button);
		gtk_container_add(GTK_CONTAINER(alignment), button);
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

		g_signal_connect(button, "clicked",
			G_CALLBACK(parameters_on_link_button_clicked), program);
	}

	/* scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwin);
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolledwin), viewport);
	gtk_container_add(GTK_CONTAINER(viewport), vbox);

	/* fill struct */
	*ui_parameters = (struct ui_parameters) {
		.dialog = dialog,
		.vbox = vbox,
		.program = GEOXML_PROGRAM(geoxml_sequence_append_clone(program)),
		.program_index = program_index,
	};
	/* load programs parameters into UI */
	parameters_load_program(ui_parameters);

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
static void
parameters_load_program(struct ui_parameters * ui_parameters)
{
	GeoXmlSequence *		parameter;

	parameter = geoxml_parameters_get_first_parameter(
		geoxml_program_get_parameters(ui_parameters->program));
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		struct parameter_data *	data;

		data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(parameter));
		if (data == NULL)
			continue;

		gtk_box_pack_start(GTK_BOX(ui_parameters->vbox), data->widget, FALSE, TRUE, 0);
	}
}

/*
 * Function: parameters_load_parameter
 *
 */
static struct parameter_data *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter)
{
	struct parameter_data * 	data;
	enum GEOXML_PARAMETERTYPE	type;

	GtkWidget *			label_widget;

	data = g_malloc(sizeof(struct parameter_data));
	data->parameter = parameter;
	geoxml_object_set_user_data(GEOXML_OBJECT(parameter), data);
	type = geoxml_parameter_get_type(parameter);

	label_widget = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(label_widget);
	if (selected != NULL) {
		GtkWidget *	radio_button;

		radio_button = gtk_radio_button_new(*radio_group);
		data->radio_button = radio_button;
		gtk_widget_show(radio_button);
		*radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), selected == parameter);
		g_signal_connect(radio_button, "toggled",
			(GCallback)parameters_change_selected, data);

		gtk_box_pack_start(GTK_BOX(label_widget), radio_button, FALSE, FALSE, 15);
	} else
		data->radio_button = NULL;

	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *		expander;
		GtkWidget *		depth_hbox;
		GtkWidget *		group_vbox;
		GtkWidget *		label;

		GeoXmlSequence *	i;

		expander = gtk_expander_new("");
		gtk_widget_show(expander);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
		gtk_expander_hacked_define(expander, label_widget);
		label = gtk_label_new(geoxml_parameter_get_label(parameter));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(label_widget), label, FALSE, TRUE, 0);
		gtk_expander_set_expanded(GTK_EXPANDER(expander),
			geoxml_parameter_group_get_expand(GEOXML_PARAMETER_GROUP(parameter)));

		depth_hbox = gtk_container_add_depth_hbox(expander);
		gtk_widget_show(depth_hbox);
		group_vbox = gtk_vbox_new(FALSE, 3);
		gtk_widget_show(group_vbox);
		gtk_container_add(GTK_CONTAINER(depth_hbox), group_vbox);
		data->specific.group.vbox = group_vbox;

		if (geoxml_parameter_group_get_is_instanciable(GEOXML_PARAMETER_GROUP(parameter))) {
			GtkWidget *	instanciate_button;
			GtkWidget *	deinstanciate_button;

			instanciate_button = gtk_button_new();
			gtk_widget_show(instanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), instanciate_button, FALSE, TRUE, 2);
			g_signal_connect(instanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_instanciate), ui_parameters);
			g_object_set(G_OBJECT(instanciate_button),
// 				"label", _("Instanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", data,
				NULL);

			deinstanciate_button = gtk_button_new();
			gtk_widget_show(deinstanciate_button);
			data->specific.group.deinstanciate_button = deinstanciate_button;
			gtk_box_pack_start(GTK_BOX(label_widget), deinstanciate_button, FALSE, TRUE, 2);
			g_signal_connect(deinstanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_deinstanciate), ui_parameters);
			g_object_set(G_OBJECT(deinstanciate_button),
// 				"label", _("Deinstanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", data,
				NULL);

			gtk_widget_set_sensitive(deinstanciate_button,
				geoxml_parameter_group_get_instances_number(GEOXML_PARAMETER_GROUP(parameter)) > 1);
		}

		GeoXmlSequence *	instance;

		/* iterate list */
		geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		data->specific.group.radio_group = NULL;
		for (; instance != NULL; geoxml_sequence_next(&instance)) {
			i = geoxml_parameters_get_first_parameter(GEOXML_PARAMETERS(instance));
			for (; i != NULL; geoxml_sequence_next(&i)) {
				struct parameter_data *	i_data;

				i_data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(i),
					geoxml_parameters_get_selected(GEOXML_PARAMETERS(instance)),
					&data->specific.group.radio_group);
				if (i_data == NULL)
					continue;

				gtk_box_pack_start(GTK_BOX(group_vbox), i_data->widget, FALSE, TRUE, 0);
			}
		}
		data->widget = expander;
	} else {
		GeoXmlProgramParameter *	program_parameter;
		GtkWidget *			hbox;

		/* create the input widget */
		program_parameter = GEOXML_PROGRAM_PARAMETER(parameter);
		switch (type) {
		case GEOXML_PARAMETERTYPE_FLOAT:
		case GEOXML_PARAMETERTYPE_INT:
		case GEOXML_PARAMETERTYPE_STRING:
		case GEOXML_PARAMETERTYPE_RANGE:
		case GEOXML_PARAMETERTYPE_FLAG:
		case GEOXML_PARAMETERTYPE_ENUM:
			data->specific.widget = parameter_widget_new(parameter, FALSE, NULL);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			data->specific.widget = parameter_widget_new(parameter, FALSE,
				flow_io_customized_paths_from_line);
			break;
		default:
			g_free(data);
			return NULL;
		}
		gtk_widget_show(data->specific.widget->widget);

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(hbox), label_widget, FALSE, FALSE, 0);

		/* create the label widget */
		if (type != GEOXML_PARAMETERTYPE_FLAG) {
			GtkWidget *	label;
			gchar *		label_str;
			GtkWidget *	align_vbox;

			label_str = (gchar*)geoxml_parameter_get_label(parameter);
			label = gtk_label_new("");
			gtk_widget_show(label);

			if (geoxml_program_parameter_get_required(GEOXML_PROGRAM_PARAMETER(parameter)) == TRUE) {
				gchar *	markup;

				markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
				gtk_label_set_markup(GTK_LABEL(label), markup);
				g_free(markup);
			} else
				gtk_label_set_text(GTK_LABEL(label), label_str);

			align_vbox = gtk_vbox_new(FALSE, 0);
			gtk_widget_show(align_vbox);
			gtk_box_pack_start(GTK_BOX(align_vbox), label, FALSE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(label_widget), align_vbox, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), data->specific.widget->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(data->specific.widget->value_widget), "label",
				geoxml_parameter_get_label(parameter), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), data->specific.widget->widget, FALSE, FALSE, 0);
		}

		data->widget = hbox;
	}

	if (selected != NULL)
		g_signal_emit_by_name(data->radio_button, "toggled");
	g_signal_connect(data->widget, "destroy",
		GTK_SIGNAL_FUNC(parameters_free_parameter_data), data);

	return data;
}

/*
 * Function: parameters_change_selected
 *
 */
static void
parameters_change_selected(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	if (geoxml_parameter_get_is_program_parameter(data->parameter) == TRUE)
		gtk_widget_set_sensitive(data->specific.widget->widget, gtk_toggle_button_get_active(toggle_button));
	else
		gtk_widget_set_sensitive(GTK_BIN(data->widget)->child,
			gtk_toggle_button_get_active(toggle_button));
}

/*
 * Function: parameters_instanciate
 *
 */
static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	struct parameter_data * group;
	GeoXmlSequence *	parameter;
	GeoXmlParameters *	instance;

	g_object_get(button, "user-data", &group, NULL);
	instance = geoxml_parameter_group_instanciate(GEOXML_PARAMETER_GROUP(group->parameter));
	geoxml_parameters_get_parameter(instance, &parameter, 0);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		struct parameter_data *	parameter_data;

		parameter_data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(parameter),
			geoxml_parameters_get_selected(instance),
			&group->specific.group.radio_group);
		if (parameter_data == NULL)
			continue;

		gtk_box_pack_start(GTK_BOX(group->specific.group.vbox), parameter_data->widget, FALSE, TRUE, 0);
	}

	gtk_widget_set_sensitive(group->specific.group.deinstanciate_button, TRUE);
}

/*
 * Function: parameters_deinstanciate
 *
 */
static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	struct parameter_data *	group;
	GeoXmlSequence *	parameter;
	GeoXmlSequence *	last_instance, * i;

	g_object_get(button, "user-data", &group, NULL);
	geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(group->parameter), &last_instance, 0);
	geoxml_sequence_next(&last_instance);
	/* has this group only one instance? */
	if (last_instance == NULL)
		return;
	for (i = last_instance; i != NULL; last_instance = i, geoxml_sequence_next(&i));

	geoxml_parameters_get_parameter(GEOXML_PARAMETERS(last_instance), &parameter, 0);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		struct parameter_data *	parameter_data;

		parameter_data = (struct parameter_data *)geoxml_object_get_user_data(GEOXML_OBJECT(parameter));
		if (parameter_data->radio_button != NULL &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parameter_data->radio_button)) == TRUE) {
			GeoXmlSequence *	first;
			struct parameter_data *	first_data;

			first = geoxml_parameters_get_first_parameter(GEOXML_PARAMETERS(last_instance));
			first_data = (struct parameter_data *)geoxml_object_get_user_data(GEOXML_OBJECT(first));

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first_data->radio_button), TRUE);
		}
		gtk_widget_destroy(GTK_WIDGET(parameter_data->widget));
	}

	geoxml_parameter_group_deinstanciate(GEOXML_PARAMETER_GROUP(group->parameter));
	gtk_widget_set_sensitive(GTK_WIDGET(button),
		geoxml_parameter_group_get_instances_number(GEOXML_PARAMETER_GROUP(group->parameter)) > 1);

	/* FIXME: restablish the exclusive case */
}

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
		GeoXmlSequence *	program;

		/* Set program state to configured */
		geoxml_program_set_status(GEOXML_PROGRAM(ui_parameters->program), "configured");
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
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gebr.actions.flow_edition.configured), TRUE);
		}

		/* clone substitute the original program */
		geoxml_flow_get_program(gebr.flow, &program, ui_parameters->program_index);
		geoxml_sequence_move_before(GEOXML_SEQUENCE(ui_parameters->program), program);
		geoxml_sequence_remove(program);

		flow_save();
		break;
	} case GTK_RESPONSE_DEFAULT: {
		parameters_reset_to_default(geoxml_program_get_parameters(ui_parameters->program));
		gtk_container_foreach(GTK_CONTAINER(ui_parameters->vbox), (GtkCallback)gtk_widget_destroy, NULL);
		parameters_load_program(ui_parameters);
		return;
	} case GTK_RESPONSE_HELP: {
		program_help_show();
		return;
	} case GTK_RESPONSE_CANCEL:
	default:
		geoxml_sequence_remove(GEOXML_SEQUENCE(ui_parameters->program));
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

static void
parameters_free_parameter_data(GtkWidget * widget, struct parameter_data * data)
{
	g_free(data);
}
