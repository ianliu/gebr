/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../intl.h"

#include "programedit.h"
#include "parameter.h"
#include "utils.h"

/*
 * Prototypes
 */

static GtkWidget *
gebr_gui_program_edit_load(GebrGuiProgramEdit *program_edit, GebrGeoXmlParameters * parameters);

static GtkWidget *
gebr_gui_program_edit_load_parameter(GebrGuiProgramEdit *program_edit, GebrGeoXmlParameter * parameter, GSList ** radio_group);

static void
gebr_gui_program_edit_change_selected(GtkToggleButton * toggle_button, struct gebr_gui_parameter_widget *widget);

static void
gebr_gui_program_edit_instanciate(GtkButton * button, GebrGuiProgramEdit *program_edit);

static void
gebr_gui_program_edit_deinstanciate(GtkButton * button, GebrGuiProgramEdit *program_edit);

static gboolean on_group_expander_mnemonic_activate(GtkExpander * expander, gboolean cycle, GebrGuiProgramEdit *program_edit);

static void on_arrow_up_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence);

static void on_arrow_down_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence);

static void on_delete_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence);

/*
 * Public functions.
 */

GebrGuiProgramEdit *
gebr_gui_program_edit_setup_ui(GebrGeoXmlProgram * program, gpointer parameter_widget_data, gboolean use_default)
{
	GebrGuiProgramEdit *program_edit;
	GtkWidget *vbox;
	GtkWidget *title_label;
	GtkWidget *hbox;
	GtkWidget *scrolled_window;

	program_edit = g_new(GebrGuiProgramEdit, 1);
	program_edit->program = program;
	program_edit->parameter_widget_data = parameter_widget_data;
	program_edit->use_default = use_default;
	program_edit->widget = vbox = gtk_vbox_new(FALSE, 0);
	program_edit->dicts.project = NULL;
	program_edit->dicts.line = NULL;
	program_edit->dicts.flow = NULL;
	gtk_widget_show(vbox);
	program_edit->title_label = title_label = gtk_label_new(NULL);
	gtk_widget_show(title_label);
	gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(title_label), 0.5, 0);
	program_edit->hbox = hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);

	program_edit->scrolled_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gebr_gui_program_edit_reload(program_edit, NULL);

	return program_edit;
}

void gebr_gui_program_edit_destroy(GebrGuiProgramEdit *program_edit)
{
	gtk_widget_destroy(program_edit->widget);
	g_free(program_edit);
}

void gebr_gui_program_edit_reload(GebrGuiProgramEdit *program_edit, GebrGeoXmlProgram * program)
{
	GtkWidget *label;
	GtkWidget *widget;
	gchar *markup;
	const gchar *uri;

	if (program != NULL)
		program_edit->program = program;

	markup = g_markup_printf_escaped("<big><b>%s</b></big>", gebr_geoxml_program_get_title(program_edit->program));
	gtk_label_set_markup(GTK_LABEL(program_edit->title_label), markup);
	g_free(markup);

	gtk_container_foreach(GTK_CONTAINER(program_edit->hbox), (GtkCallback) gtk_widget_destroy, NULL);
	label = gtk_label_new(gebr_geoxml_program_get_description(program_edit->program));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(program_edit->hbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	uri = gebr_geoxml_program_get_url(program_edit->program);
	if (strlen(uri)) {
		GtkWidget *alignment;
		GtkWidget *button;
		GString *full_uri;

		alignment = gtk_alignment_new(1, 0, 0, 0);
		gtk_box_pack_start(GTK_BOX(program_edit->hbox), alignment, TRUE, TRUE, 5);

		full_uri = g_string_new(NULL);
		if (g_str_has_prefix(uri, "http://"))
			g_string_assign(full_uri, uri);
		else
			g_string_printf(full_uri, "http://%s", uri);

		button = gtk_link_button_new_with_label(full_uri->str, _("Link"));
		gtk_container_add(GTK_CONTAINER(alignment), button);
		gtk_widget_show_all(alignment);
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
		g_string_free(full_uri, TRUE);
	}

	gtk_container_foreach(GTK_CONTAINER(program_edit->scrolled_window), (GtkCallback) gtk_widget_destroy, NULL);
	widget = gebr_gui_program_edit_load(program_edit, gebr_geoxml_program_get_parameters(program_edit->program));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(program_edit->scrolled_window), widget);
}

/*
 * Private functions.
 */

/**
 * \internal
 */
static GtkWidget *
gebr_gui_program_edit_load(GebrGuiProgramEdit *program_edit, GebrGeoXmlParameters * parameters)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GebrGeoXmlSequence *parameter;
	GebrGeoXmlParameterGroup *parameter_group;
	GSList *radio_group;

	frame = gtk_frame_new(NULL);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_widget_show(frame);

	parameter_group = gebr_geoxml_parameters_get_group(parameters);
	if (parameter_group != NULL) {
		GtkWidget *hbox;
		GtkWidget *button;

		hbox = gtk_hbox_new(FALSE, 0);

		button = gtk_button_new();
		g_object_set_data(G_OBJECT(button), "frame", frame);
		gtk_container_add(GTK_CONTAINER(button), gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE));
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
		gtk_widget_set_sensitive(button, gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(parameters)) != 1);
		g_signal_connect(button, "clicked", G_CALLBACK(on_arrow_down_clicked), parameters);
		g_object_set_data(G_OBJECT(frame), "arrow-down", button);

		button = gtk_button_new();
		g_object_set_data(G_OBJECT(button), "frame", frame);
		gtk_container_add(GTK_CONTAINER(button), gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE));
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
		parameter = GEBR_GEOXML_SEQUENCE(parameters);
		gebr_geoxml_sequence_next(&parameter);
		gtk_widget_set_sensitive(button, parameter != NULL);
		g_signal_connect(button, "clicked", G_CALLBACK(on_arrow_up_clicked), parameters);
		g_object_set_data(G_OBJECT(frame), "arrow-up", button);

		if (!gebr_geoxml_parameter_group_get_is_instanciable(parameter_group))
			gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
		else {
			button = gtk_button_new();
			g_object_set_data(G_OBJECT(button), "frame", frame);
			gtk_container_add(GTK_CONTAINER(button),
					  gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON));
			gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
			g_signal_connect(button, "clicked", G_CALLBACK(on_delete_clicked), parameters);
			g_object_set_data(G_OBJECT(frame), "delete", button);
		}

		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	}

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	radio_group = NULL;
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (gboolean first_parameter = TRUE; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
		GtkWidget * widget;

		widget = gebr_gui_program_edit_load_parameter(program_edit, GEBR_GEOXML_PARAMETER(parameter), &radio_group);
		if (first_parameter) {
			/* used in on_group_expander_mnemonic_activate */
			g_object_set_data(G_OBJECT(frame), "first-parameter", parameter);
			g_object_set_data(G_OBJECT(frame), "first-parameter-widget", widget);
			first_parameter = FALSE;
		}
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}

	return frame;
}

/**
 * \internal
 */
static GtkWidget *gebr_gui_program_edit_load_parameter(GebrGuiProgramEdit *program_edit,
						       GebrGeoXmlParameter * parameter, GSList ** radio_group)
{
	GebrGeoXmlParameterType type;

	type = gebr_geoxml_parameter_get_type(parameter);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GtkWidget *expander;
		GtkWidget *label_widget;
		GtkWidget *label;

		GtkWidget *depth_hbox;
		GtkWidget *group_vbox;
		GtkWidget *instanciate_button;
		GtkWidget *deinstanciate_button;

		GebrGeoXmlParameterGroup *parameter_group;
		GebrGeoXmlSequence *instance;
		GebrGeoXmlSequence *param;
		gboolean required;

		parameter_group = GEBR_GEOXML_PARAMETER_GROUP(parameter);

		expander = gtk_expander_new("");
		gtk_widget_show(expander);
		gtk_expander_set_expanded(GTK_EXPANDER(expander),
					  gebr_geoxml_parameter_group_get_expand(parameter_group));

		label_widget = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(label_widget);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
		gebr_gui_gtk_expander_hacked_define(expander, label_widget);

		required = FALSE;
		gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
		gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &param, 0);
		for (; param != NULL; gebr_geoxml_sequence_next(&param)) {
			if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param))) {
				required = TRUE;
				break;
			}
		}

		if (required) {
			gchar *markup;
			markup = g_markup_printf_escaped("<b>%s*</b>",
							 gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)));
			label = gtk_label_new("");
			gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), markup);
			g_free(markup);
		} else 
			label = gtk_label_new_with_mnemonic(gebr_geoxml_parameter_get_label(parameter));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(label_widget), label, FALSE, TRUE, 0);

		if (gebr_geoxml_parameter_group_get_is_instanciable(GEBR_GEOXML_PARAMETER_GROUP(parameter))) {
			instanciate_button = gtk_button_new();
			gtk_widget_show(instanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), instanciate_button, FALSE, TRUE, 2);
			g_signal_connect(instanciate_button, "clicked",
					 G_CALLBACK(gebr_gui_program_edit_instanciate), program_edit);
			g_object_set(G_OBJECT(instanciate_button),
				     "image", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR),
				     "relief", GTK_RELIEF_NONE, "user-data", parameter_group, NULL);

			deinstanciate_button = gtk_button_new();
			gtk_widget_show(deinstanciate_button);
			gtk_box_pack_start(GTK_BOX(label_widget), deinstanciate_button, FALSE, TRUE, 2);
			g_signal_connect(deinstanciate_button, "clicked",
					 G_CALLBACK(gebr_gui_program_edit_deinstanciate), program_edit);
			g_object_set(G_OBJECT(deinstanciate_button),
				     "image", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR),
				     "relief", GTK_RELIEF_NONE, "user-data", parameter_group, NULL);

			gtk_widget_set_sensitive(deinstanciate_button,
						 gebr_geoxml_parameter_group_get_instances_number
						 (GEBR_GEOXML_PARAMETER_GROUP(parameter)) > 1);
		} else
			deinstanciate_button = NULL;

		depth_hbox = gebr_gui_gtk_container_add_depth_hbox(expander);
		gtk_widget_show(depth_hbox);
		group_vbox = gtk_vbox_new(FALSE, 3);
		gtk_widget_show(group_vbox);
		gtk_container_add(GTK_CONTAINER(depth_hbox), group_vbox);

		gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(parameter_group), group_vbox);
		g_object_set(G_OBJECT(group_vbox), "user-data", deinstanciate_button, NULL);

		gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
		for (gboolean first_instance = TRUE; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
			GtkWidget *widget;

			widget = gebr_gui_program_edit_load(program_edit, GEBR_GEOXML_PARAMETERS(instance));
			if (first_instance) {
				g_signal_connect(expander, "mnemonic-activate",
						 G_CALLBACK(on_group_expander_mnemonic_activate), program_edit);
				g_object_set_data(G_OBJECT(expander), "first-instance-widget", widget);
				first_instance = FALSE;
			}
			gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(instance), widget);
			gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);
		}

		return expander;
	} else {
		GtkWidget *hbox;
		struct gebr_gui_parameter_widget *gebr_gui_parameter_widget;

		GebrGeoXmlParameter *selected;

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);

		/* input widget */
		if (type != GEBR_GEOXML_PARAMETER_TYPE_FILE)
			gebr_gui_parameter_widget =
			    gebr_gui_parameter_widget_new(parameter, program_edit->use_default, NULL);
		else
			gebr_gui_parameter_widget = gebr_gui_parameter_widget_new(parameter, program_edit->use_default,
										  program_edit->parameter_widget_data);
		if (program_edit->dicts.project != NULL)
			gebr_gui_parameter_widget_set_dicts(gebr_gui_parameter_widget, &program_edit->dicts);
		gtk_widget_show(gebr_gui_parameter_widget->widget);

		/* exclusive? */
		selected = gebr_geoxml_parameters_get_selection(gebr_geoxml_parameter_get_parameters(parameter));
		if (selected != NULL) {
			GtkWidget *radio_button;

			radio_button = gtk_radio_button_new(*radio_group);
			gtk_widget_show(radio_button);
			*radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), selected == parameter);
			g_signal_connect(radio_button, "toggled",
					 G_CALLBACK(gebr_gui_program_edit_change_selected), gebr_gui_parameter_widget);

			gtk_box_pack_start(GTK_BOX(hbox), radio_button, FALSE, FALSE, 15);
			gtk_widget_set_sensitive(gebr_gui_parameter_widget->widget,
						 selected == parameter ? TRUE : FALSE);
		}
		if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
			GtkWidget *label;
			gchar *label_str;
			GtkWidget *align_vbox;

			label_str = (gchar *) gebr_geoxml_parameter_get_label(parameter);
			label = gtk_label_new("");
			gtk_widget_show(label);

			if (gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameter)) ==
			    TRUE) {
				gchar *markup;

				markup = g_markup_printf_escaped("<b>%s</b><sup>*</sup>", label_str);
				gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), markup);
				g_free(markup);
			} else
				gtk_label_set_text_with_mnemonic(GTK_LABEL(label), label_str);
			gtk_label_set_mnemonic_widget(GTK_LABEL(label), gebr_gui_parameter_widget->widget);

			align_vbox = gtk_vbox_new(FALSE, 0);
			gtk_widget_show(align_vbox);
			gtk_box_pack_start(GTK_BOX(align_vbox), label, FALSE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), align_vbox, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), gebr_gui_parameter_widget->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(gebr_gui_parameter_widget->value_widget),
				     "label", gebr_geoxml_parameter_get_label(parameter), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), gebr_gui_parameter_widget->widget, FALSE, FALSE, 0);
		}

		g_object_set_data(G_OBJECT(hbox), "parameter-widget", gebr_gui_parameter_widget->widget);

		return hbox;
	}
}

/**
 * \internal
 */
static void
gebr_gui_program_edit_change_selected(GtkToggleButton * toggle_button, struct gebr_gui_parameter_widget *widget)
{
	gtk_widget_set_sensitive(widget->widget, gtk_toggle_button_get_active(toggle_button));
	if (gtk_toggle_button_get_active(toggle_button))
		gebr_geoxml_parameters_set_selection(gebr_geoxml_parameter_get_parameters(widget->parameter),
						     widget->parameter);
}

/**
 * \internal
 */
static void gebr_gui_program_edit_instanciate(GtkButton * button, GebrGuiProgramEdit *program_edit)
{
	GebrGeoXmlParameterGroup *parameter_group;
	GebrGeoXmlParameters *instance;
	GtkWidget *group_vbox;
	GtkWidget *deinstanciate_button;
	GtkWidget *widget;

	g_object_get(button, "user-data", &parameter_group, NULL);
	group_vbox = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	g_object_get(group_vbox, "user-data", &deinstanciate_button, NULL);

	instance = gebr_geoxml_parameter_group_instanciate(parameter_group);
	widget = gebr_gui_program_edit_load(program_edit, GEBR_GEOXML_PARAMETERS(instance));
	gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(instance), widget);
	gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);

	gtk_widget_set_sensitive(deinstanciate_button, TRUE);
}

/**
 * \internal
 * Group deinstanciation button callback.
 */
static void gebr_gui_program_edit_deinstanciate(GtkButton * button, GebrGuiProgramEdit *program_edit)
{
	GebrGeoXmlParameterGroup *parameter_group;
	GebrGeoXmlSequence *last_instance;
	GtkWidget *widget;

	g_object_get(button, "user-data", &parameter_group, NULL);
	gebr_geoxml_parameter_group_get_instance(parameter_group, &last_instance,
						 gebr_geoxml_parameter_group_get_instances_number(parameter_group) - 1);

	widget = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(last_instance));
	gtk_widget_destroy(widget);
	gebr_geoxml_parameter_group_deinstanciate(parameter_group);

	gtk_widget_set_sensitive(GTK_WIDGET(button),
				 gebr_geoxml_parameter_group_get_instances_number(parameter_group) > 1);
}

/**
 * \internal
 */
static gboolean on_group_expander_mnemonic_activate(GtkExpander * expander, gboolean cycle, GebrGuiProgramEdit *program_edit)
{
	if (!gtk_expander_get_expanded(expander)) {
		GtkWidget *first_instance_widget;
		GtkWidget *first_parameter_widget;
		GtkWidget *parameter_widget;

		first_instance_widget = g_object_get_data(G_OBJECT(expander), "first-instance-widget");
		first_parameter_widget = g_object_get_data(G_OBJECT(first_instance_widget), "first-parameter-widget");
		parameter_widget = g_object_get_data(G_OBJECT(first_parameter_widget), "parameter-widget");
		gtk_widget_mnemonic_activate(parameter_widget, cycle);
		gtk_expander_set_expanded(expander, TRUE);
	} else
		gtk_expander_set_expanded(expander, FALSE);
	
	return TRUE;
}

/**
 * \internal
 */
static void on_arrow_up_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence)
{
	gint index;
	GtkBox *box;
	GtkWidget *frame;

	frame = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "frame"));
	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(sequence));

	if (index == 1)
		return;

	gebr_geoxml_sequence_move_up(GEBR_GEOXML_SEQUENCE(sequence));

	box = GTK_BOX(gtk_widget_get_parent(frame));
	gtk_box_reorder_child(box, frame, index-1);

	gtk_widget_set_sensitive(button, index == 2);
}

/**
 * \internal
 */
static void on_arrow_down_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence)
{
	gint index;
	GtkBox *box;
	GtkWidget *frame;
	GebrGeoXmlSequence *next;

	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(sequence));
	next = GEBR_GEOXML_SEQUENCE(sequence);
	gebr_geoxml_sequence_next(&next);

	if (!next)
		return;

	gebr_geoxml_sequence_next(&next);

	gtk_widget_set_sensitive(button, next != NULL);

	frame = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "frame"));
	box = GTK_BOX(gtk_widget_get_parent(frame));
	gtk_box_reorder_child(box, frame, index+1);
}

static void on_delete_clicked(GtkWidget *button, GebrGeoXmlParameters * sequence)
{
}
