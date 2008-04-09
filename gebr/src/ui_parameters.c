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
parameters_load_program(struct ui_parameters * ui_parameters, gboolean use_default);

static struct parameter_data *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter,
	gboolean exclusive, GSList ** radio_group, gboolean use_default,
	GtkWidget ** widget);

static void
parameters_submit(struct ui_parameters * ui_parameters, GList * parameters);

static void
parameters_actions(GtkDialog *dialog, gint arg1, struct ui_parameters * ui_parameters);

static void
parameters_change_exclusive(GtkToggleButton * toggle_button, struct parameter_widget * widget);

static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters);

static void
on_link_button_clicked(GtkButton * button, GeoXmlProgram * program);

static void
parameters_free(GList * parameters);

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
	gtk_dialog_add_button(GTK_DIALOG(dialog), "Default", GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	gtk_widget_set_size_request(dialog, 630, 400);
	gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->vbox), FALSE);

	/* take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response",
		G_CALLBACK(parameters_actions), ui_parameters);

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
			G_CALLBACK(on_link_button_clicked), program);
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
		.program = GEOXML_PROGRAM(program),
		.program_index = program_index,
		.root_vbox = vbox,
		.to_free_list = NULL
	};
	/* load programs parameters into UI */
	parameters_load_program(ui_parameters, FALSE);

	gtk_widget_show(dialog);

	return ui_parameters;
}


/*
 * Section: Private
 * Private functions.
 */
static void
parameters_load_program(struct ui_parameters * ui_parameters, gboolean use_default)
{
	GeoXmlSequence *		parameter;

	ui_parameters->parameters = NULL;
	parameter = geoxml_parameters_get_first_parameter(
		geoxml_program_get_parameters(ui_parameters->program));
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		struct parameter_data *	data;
		GtkWidget *		widget;

		data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(parameter),
			FALSE, NULL, use_default, &widget);
		if (data == NULL)
			continue;
		ui_parameters->parameters = g_list_prepend(ui_parameters->parameters, data);
		gtk_box_pack_start(GTK_BOX(ui_parameters->root_vbox), widget, FALSE, TRUE, 0);
	}

	ui_parameters->to_free_list = g_list_prepend(ui_parameters->to_free_list, ui_parameters->parameters);
	ui_parameters->parameters = g_list_reverse(ui_parameters->parameters);
}

/*
 * Function: parameters_load_parameter
 *
 */
static struct parameter_data *
parameters_load_parameter(struct ui_parameters * ui_parameters, GeoXmlParameter * parameter,
	gboolean exclusive, GSList ** radio_group, gboolean use_default,
	GtkWidget ** _widget)
{
	struct parameter_data * 	data;
	enum GEOXML_PARAMETERTYPE	type;

	data = g_malloc(sizeof(struct parameter_data));
	data->parameter = parameter;
	type = geoxml_parameter_get_type(parameter);

	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *		expander;
		GtkWidget *		depth_hbox;
		GtkWidget *		group_vbox;
		GtkWidget *		label_widget;
		GtkWidget *		label;

		GeoXmlSequence *	i;

		expander = gtk_expander_new("");
		gtk_widget_show(expander);
		gtk_expander_set_expanded(GTK_EXPANDER(expander),
			geoxml_parameter_group_get_expand(GEOXML_PARAMETER_GROUP(parameter)));
		depth_hbox = gtk_container_add_depth_hbox(expander);
		gtk_widget_show(depth_hbox);

		group_vbox = gtk_vbox_new(FALSE, 3);
		gtk_widget_show(group_vbox);
		gtk_container_add(GTK_CONTAINER(depth_hbox), group_vbox);
		data->data.group.vbox = group_vbox;

		label_widget = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(label_widget);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
		gtk_widget_show(label_widget);
		gtk_expander_hacked_define(expander, label_widget);
		label = gtk_label_new(geoxml_parameter_get_label(parameter));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(label_widget), label, FALSE, TRUE, 0);

		if (geoxml_parameter_group_get_can_instanciate(GEOXML_PARAMETER_GROUP(parameter))) {
			GtkWidget *	instanciate_button;
			GtkWidget *	deinstanciate_button;

			instanciate_button = gtk_button_new();
			gtk_widget_show(instanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), instanciate_button, FALSE, TRUE, 5);
			g_signal_connect(instanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_instanciate), ui_parameters);
			g_object_set(G_OBJECT(instanciate_button),
// 				"label", _("Instanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", data,
				NULL);

			deinstanciate_button = gtk_button_new();
			ui_parameters->deinstanciate_button = deinstanciate_button;
			gtk_box_pack_start(GTK_BOX(label_widget), deinstanciate_button, FALSE, TRUE, 5);
			g_signal_connect(deinstanciate_button, "clicked",
				GTK_SIGNAL_FUNC(parameters_deinstanciate), ui_parameters);
			g_object_set(G_OBJECT(deinstanciate_button),
// 				"label", _("Deinstanciate"),
				"image", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR),
				"relief", GTK_RELIEF_NONE,
				"user-data", data,
				NULL);

			if (geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(parameter)) > 1)
				gtk_widget_show(deinstanciate_button);
		}

		/* iterate list */
		data->data.group.radio_group = NULL;
		data->data.group.parameters = NULL;
		i = geoxml_parameters_get_first_parameter(
			geoxml_parameter_group_get_parameters(GEOXML_PARAMETER_GROUP(parameter)));
		for (; i != NULL; geoxml_sequence_next(&i)) {
			struct parameter_data *	i_data;
			GtkWidget *		widget;

			i_data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(i),
				geoxml_parameter_group_get_exclusive(GEOXML_PARAMETER_GROUP(parameter)),
				&data->data.group.radio_group, use_default,
				&widget);
			if (i_data == NULL)
				continue;
			data->data.group.parameters = g_list_prepend(data->data.group.parameters, i_data);
			gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);
		}

		ui_parameters->to_free_list = g_list_prepend(ui_parameters->to_free_list, data->data.group.parameters);
		data->data.group.parameters = g_list_reverse(data->data.group.parameters);

		*_widget = expander;
	} else {
		GeoXmlProgramParameter *	program_parameter;
		GtkWidget *			hbox;
		struct parameter_widget *	widget;

		program_parameter = GEOXML_PROGRAM_PARAMETER(parameter);
		switch (type) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			widget = parameter_widget_new_float(parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_INT:
			widget = parameter_widget_new_int(parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_STRING:
			widget = parameter_widget_new_string(parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			widget = parameter_widget_new_range(parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_FLAG:
			widget = parameter_widget_new_flag(parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			widget = parameter_widget_new_file(parameter,
				flow_io_customized_paths_from_line, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_ENUM:
			widget = parameter_widget_new_enum(parameter, FALSE);
			break;
		default:
			g_free(data);
			return NULL;
		}
		data->data.widget = widget;
		gtk_widget_show(widget->widget);
		if (use_default == TRUE)
			parameter_widget_set_widget_value(widget,
				geoxml_program_parameter_get_default(program_parameter));

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);
		if (exclusive == TRUE) {
			GtkWidget *	radio_button;
			GString *	value;

			radio_button = gtk_radio_button_new(*radio_group);
			gtk_widget_show(radio_button);
			*radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

			value = parameter_widget_get_widget_value(widget);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), value->len);
			g_signal_connect(radio_button, "toggled",
				(GCallback)parameters_change_exclusive, widget);
			g_signal_emit_by_name(radio_button, "toggled");

			gtk_box_pack_start(GTK_BOX(hbox), radio_button, FALSE, FALSE, 15);

			g_string_free(value, TRUE);
		}
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
			gtk_box_pack_start(GTK_BOX(hbox), align_vbox, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), widget->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(widget->value_widget), "label",
				geoxml_parameter_get_label(parameter), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), widget->widget, FALSE, FALSE, 0);
		}

		*_widget = hbox;
	}

	return data;
}

/*
 * Function: parameters_submit
 * Call parameter_widget_submit for each _parameters_
 *
 */
static void
parameters_submit(struct ui_parameters * ui_parameters, GList * parameters)
{
	GList *	i;


	for (i = g_list_first(parameters); i != NULL; i = g_list_next(i)) {
		struct parameter_data *	data;
		enum GEOXML_PARAMETERTYPE	type;

		data = (struct parameter_data *)i->data;
		type = geoxml_parameter_get_type(data->parameter);
		if (type == GEOXML_PARAMETERTYPE_GROUP) {
			parameters_submit(ui_parameters, data->data.group.parameters);
			continue;
		}

		parameter_widget_submit(data->data.widget);
	}
}

/*
 * Function: parameters_change_exclusive
 *
 */
static void
parameters_change_exclusive(GtkToggleButton * toggle_button, struct parameter_widget * widget)
{
	gboolean	active;

	active = gtk_toggle_button_get_active(toggle_button);
	if (active == FALSE)
		parameter_widget_set_widget_value(widget, "");
	gtk_widget_set_sensitive(widget->widget, active);
}

/*
 * Function: parameters_instanciate
 *
 */
static void
parameters_instanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	struct parameter_data *	group;
	GeoXmlSequence *	parameter;

	g_object_get(G_OBJECT(button), "user-data", &group, NULL);

	geoxml_parameter_group_instanciate(GEOXML_PARAMETER_GROUP(group->parameter));
	parameter = GEOXML_SEQUENCE(
		geoxml_parameter_group_last_instance_parameter(GEOXML_PARAMETER_GROUP(group->parameter)));
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		struct parameter_data *	parameter_data;
		GtkWidget *		widget;

		parameter_data = parameters_load_parameter(ui_parameters, GEOXML_PARAMETER(parameter),
			geoxml_parameter_group_get_exclusive(GEOXML_PARAMETER_GROUP(group->parameter)),
			&group->data.group.radio_group, FALSE,
			&widget);
		if (parameter_data == NULL)
			continue;
		group->data.group.parameters = g_list_append(group->data.group.parameters, parameter_data);
		gtk_box_pack_start(GTK_BOX(group->data.group.vbox), widget, FALSE, TRUE, 0);
	}

	gtk_widget_show(ui_parameters->deinstanciate_button);
}

/*
 * Function: parameters_deinstanciate
 *
 */
static void
parameters_deinstanciate(GtkButton * button, struct ui_parameters * ui_parameters)
{
	struct parameter_data *	group;
	GList *			children;
	GList *			i;
	GList *			j;
	gulong			index;

	g_object_get(G_OBJECT(button), "user-data", &group, NULL);

	geoxml_parameter_group_deinstanciate(GEOXML_PARAMETER_GROUP(group->parameter));
	if (geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(group->parameter)) == 1)
		gtk_widget_hide(GTK_WIDGET(button));

	index = geoxml_parameter_group_get_parameters_by_instance(GEOXML_PARAMETER_GROUP(group->parameter)) *
		geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(group->parameter));
	children = gtk_container_get_children(GTK_CONTAINER(group->data.group.vbox));
	i = g_list_nth(children, index);
	j = g_list_nth(group->data.group.parameters, index);
	while (i != NULL) {
		GList *		aux;

		gtk_widget_destroy(GTK_WIDGET(i->data));
		i = g_list_next(i);

		aux = j;
		j = g_list_next(j);
// 		g_free(aux->data); FIXME
		group->data.group.parameters = g_list_delete_link(group->data.group.parameters, aux);
	}

	/* FIXME: restablish the exclusive case */

	g_list_free(children);
}

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

			gtk_radio_action_set_current_value(gebr.actions.configured, 1<<0);
		}

		/* Write values from UI to XML */
		parameters_submit(ui_parameters, ui_parameters->parameters);

		flow_save();
		break;
	} case GTK_RESPONSE_CANCEL:
		break;
	case GTK_RESPONSE_DEFAULT: {
		gtk_container_foreach(GTK_CONTAINER(ui_parameters->root_vbox), (GtkCallback)gtk_widget_destroy, NULL);
		parameters_load_program(ui_parameters, TRUE);
		return;
	} case GTK_RESPONSE_HELP: {
		program_help_show();
		return;
	} default:
		break;
	}

	/* gui free */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_list_foreach(ui_parameters->to_free_list, (GFunc)parameters_free, NULL);
	g_list_free(ui_parameters->to_free_list);
	g_free(ui_parameters);
}

static void
on_link_button_clicked(GtkButton * button, GeoXmlProgram * program)
{
	GString * cmd_line;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s %s &", gebr.config.browser->str, geoxml_program_get_url(program));

	system(cmd_line->str);

	g_string_free(cmd_line, TRUE);
}

static void
parameters_free(GList * parameters)
{
	g_list_foreach(parameters, (GFunc)g_free, NULL);
	g_list_free(parameters);
}
