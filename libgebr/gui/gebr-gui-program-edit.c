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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "../libgebr-gettext.h"
#include <glib/gi18n-lib.h>

#include "gebr-gui-program-edit.h"
#include "gebr-gui-param.h"
#include "gebr-gui-utils.h"
#include "../utils.h"

/*
 * Prototypes
 */

struct _GebrGuiProgramEditPriv
{
	GList *widgets;
	GebrGuiCompleteVariables *complete_var;
};

typedef struct {
	GtkBox * group_vbox;
	GList * instances_list;
} GebrGroupReorderData;

static void gebr_gui_program_edit_set_complete_variables(GebrGuiProgramEdit *program_edit,
							 GebrGuiCompleteVariables *complete_var);
static GtkWidget *
gebr_gui_program_edit_load(GebrGuiProgramEdit *program_edit, GebrGeoXmlParameters * parameters);

static GtkWidget *
gebr_gui_program_edit_load_parameter(GebrGuiProgramEdit *program_edit, GebrGeoXmlParameter * parameter, GSList ** radio_group);

static void
gebr_gui_program_edit_change_selected(GtkToggleButton * toggle_button, GebrGuiParam *widget);

static void
gebr_gui_program_edit_instanciate(GtkButton * button, GebrGuiProgramEdit *program_edit);

static void
gebr_gui_program_edit_deinstanciate(GtkButton * button, GebrGuiProgramEdit *program_edit);

static gboolean on_group_expander_mnemonic_activate(GtkExpander * expander, gboolean cycle, GebrGuiProgramEdit *program_edit);

static void on_arrow_up_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group);

static void on_arrow_down_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group);

static void on_delete_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group);

static void update_frame(GtkWidget * frame);

static void on_instance_destroy(GebrGroupReorderData * data);

/*
 * Public functions.
 */

GebrGuiProgramEdit *
gebr_gui_program_edit_setup_ui(GebrGeoXmlProgram * program,
			       gpointer parameter_widget_data,
			       gboolean use_default,
			       GebrValidator *validator,
			       GebrMaestroInfo *info,
			       GebrGuiCompleteVariables *complete_var,
			       gboolean add_title)
{
	GebrGuiProgramEdit *program_edit;
	GebrGuiProgramEditPriv *priv = g_new0(GebrGuiProgramEditPriv, 1);

	GtkWidget *vbox;
	GtkWidget *title_label;
	GtkWidget *hbox;
	GtkWidget *scrolled_window;

	program_edit = g_new(GebrGuiProgramEdit, 1);
	program_edit->priv = priv;
	program_edit->program = program;
	program_edit->parameter_widget_data = parameter_widget_data;
	program_edit->use_default = use_default;
	program_edit->widget = vbox = gtk_vbox_new(FALSE, 0);
	program_edit->validator = validator;
	program_edit->group_warning_widget = NULL;
	gebr_gui_program_edit_set_complete_variables(program_edit, complete_var);

	//MPI Program
	const gchar *mpi = gebr_geoxml_program_get_mpi(program);
	if (*mpi)
		program_edit->mpi_params = gebr_geoxml_program_mpi_get_parameters(program);
	else
		program_edit->mpi_params = NULL;

	program_edit->info = info;

	if (add_title) {
		program_edit->title_label = title_label = gtk_label_new(NULL);
		gtk_widget_show(title_label);
		gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, TRUE, 5);
		gtk_misc_set_alignment(GTK_MISC(title_label), 0.5, 0);

		gchar *markup = g_markup_printf_escaped("<big><b>%s</b></big>", gebr_geoxml_program_get_title(program));
		gtk_label_set_markup(GTK_LABEL(title_label), markup);
		g_free(markup);

		GtkWidget *description_label = gtk_label_new(NULL);
		gtk_widget_show(description_label);
		gtk_box_pack_start(GTK_BOX(vbox), description_label, FALSE, TRUE, 5);
		gtk_misc_set_alignment(GTK_MISC(description_label), 0.5, 0);

		gtk_label_set_text(GTK_LABEL(description_label), gebr_geoxml_program_get_description(program));
	} else {
		program_edit->title_label = NULL;
	}

	program_edit->hbox = hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	program_edit->scrolled_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show_all(scrolled_window);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_widget_show_all(vbox);
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 5);
	GtkWidget *widget;

	if (program_edit->mpi_params) {
		widget = gebr_gui_program_edit_load(program_edit, program_edit->mpi_params);
		gtk_box_pack_start(GTK_BOX(vbox1), widget, FALSE, TRUE, 0);
	}
	widget = gebr_gui_program_edit_load(program_edit, gebr_geoxml_program_get_parameters(program_edit->program));
	gtk_box_pack_start(GTK_BOX(vbox1), widget, TRUE, TRUE, 0);
	gtk_widget_show(vbox1);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(program_edit->scrolled_window), vbox1);

	return program_edit;
}

void
gebr_gui_program_edit_set_validated_callback(GebrGuiProgramEdit *program_edit,
					     GebrGuiParameterValidatedFunc callback,
					     gpointer user_data)
{
	for (GList *i = program_edit->priv->widgets; i; i = i->next)
		gebr_gui_param_set_validated_callback(i->data, callback, user_data);
}

static void
gebr_gui_program_edit_set_complete_variables(GebrGuiProgramEdit *program_edit,
					     GebrGuiCompleteVariables *complete_var)
{
	if (program_edit->priv->complete_var)
		g_object_unref(program_edit->priv->complete_var);
	program_edit->priv->complete_var = g_object_ref(complete_var);
}

void gebr_gui_program_edit_destroy(GebrGuiProgramEdit *program_edit)
{
	gtk_widget_destroy(program_edit->widget);
	g_list_free(program_edit->priv->widgets);
	g_free(program_edit->priv);
	g_free(program_edit);
}

void gebr_gui_program_edit_reload(GebrGuiProgramEdit *program_edit, GebrGeoXmlProgram * program)
{
	GtkWidget *widget;

	if (program != NULL)
		program_edit->program = program;

	g_list_free(program_edit->priv->widgets);
	program_edit->priv->widgets = NULL;

	gtk_container_foreach(GTK_CONTAINER(program_edit->hbox), (GtkCallback) gtk_widget_destroy, NULL);

	gtk_container_foreach(GTK_CONTAINER(program_edit->scrolled_window), (GtkCallback) gtk_widget_destroy, NULL);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);

	if (program_edit->mpi_params) {
		widget = gebr_gui_program_edit_load(program_edit, program_edit->mpi_params);
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}
	widget = gebr_gui_program_edit_load(program_edit, gebr_geoxml_program_get_parameters(program_edit->program));
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_widget_show(vbox);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(program_edit->scrolled_window), vbox);
}

/*
 * Private functions.
 */

static void
on_mpi_parameters_help_clicked()
{
	GtkBuilder *builder = gtk_builder_new();

	if (!gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/mpi-parameters-help.glade", NULL))
		return;

	GObject *dialog = gtk_builder_get_object(builder, "main");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

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
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_widget_show(frame);

	parameter_group = gebr_geoxml_parameters_get_group(parameters);
	if (parameter_group != NULL) {
		GtkWidget *hbox;
		GtkWidget *button;
		GtkRcStyle *style;

		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

		hbox = gtk_hbox_new(FALSE, 0);

		if (gebr_geoxml_parameter_group_get_is_instanciable(parameter_group)) {
			button = gtk_button_new();
			style = gtk_rc_style_new();
			style->xthickness = style->ythickness = 0;
			gtk_widget_modify_style(button, style);
			g_object_unref(style);
			g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
			g_object_set_data(G_OBJECT(button), "frame", frame);
			gtk_container_add(GTK_CONTAINER(button),
					  gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_MENU));
			gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
			g_signal_connect(button, "clicked", G_CALLBACK(on_delete_clicked), parameter_group);
			g_object_set_data(G_OBJECT(frame), "delete", button);
		}

		if (program_edit->mpi_params) {
			button = gtk_button_new();
			style = gtk_rc_style_new();
			style->xthickness = style->ythickness = 0;
			gtk_widget_modify_style(button, style);
			g_object_unref(style);
			g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
			gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
			gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
			g_signal_connect(button, "clicked", G_CALLBACK(on_mpi_parameters_help_clicked), NULL);
		}

		button = gtk_button_new();
		style = gtk_rc_style_new();
		style->xthickness = style->ythickness = 0;
		gtk_widget_modify_style(button, style);
		g_object_unref(style);
		g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
		g_object_set_data(G_OBJECT(button), "frame", frame);
		g_object_set_data(G_OBJECT(frame), "arrow-down", button);
		g_signal_connect(button, "clicked", G_CALLBACK(on_arrow_down_clicked), parameter_group);

		gtk_container_add(GTK_CONTAINER(button), gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE));
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);

		button = gtk_button_new();
		style = gtk_rc_style_new();
		style->xthickness = style->ythickness = 0;
		gtk_widget_modify_style(button, style);
		g_object_unref(style);
		g_object_set(button, "relief", GTK_RELIEF_NONE, NULL);
		g_object_set_data(G_OBJECT(button), "frame", frame);
		g_object_set_data(G_OBJECT(frame), "arrow-up", button);
		g_signal_connect(button, "clicked", G_CALLBACK(on_arrow_up_clicked), parameter_group);

		gtk_container_add(GTK_CONTAINER(button), gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE));
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	}

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	GList *groups = NULL;
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

		if (gebr_geoxml_parameter_get_group(GEBR_GEOXML_PARAMETER(parameter))) {
			gebr_geoxml_object_ref(parameter);
			groups = g_list_prepend(groups, GEBR_GEOXML_PARAMETER(parameter));
		}

		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}

	for (GList *i = groups; i; i = i->next)
		gebr_gui_group_validate(program_edit->validator,
		                        i->data,
		                        program_edit->group_warning_widget);

	g_list_foreach(groups, (GFunc)gebr_geoxml_object_unref, NULL);
	g_list_free(groups);

	return frame;
}

static GebrGuiParam *
create_parameter_widget(GebrGuiProgramEdit *program_edit,
			GebrGeoXmlParameter *parameter,
			GebrGeoXmlParameterType type)
{
	GebrGuiParam *widget;

	if (type != GEBR_GEOXML_PARAMETER_TYPE_FILE) {
		widget = gebr_gui_param_new(parameter,
					    program_edit->validator,
					    program_edit->info,
					    program_edit->use_default,
					    program_edit->priv->complete_var,
					    NULL);
	} else {
		widget = gebr_gui_param_new(parameter,
					    program_edit->validator,
					    program_edit->info,
					    program_edit->use_default,
					    program_edit->priv->complete_var,
					    program_edit->parameter_widget_data);

		GebrGeoXmlDocument *line;
		gebr_validator_get_documents(widget->validator, NULL, &line, NULL);

		const gchar *value = gtk_entry_get_text(GTK_ENTRY(GEBR_GUI_FILE_ENTRY(widget->value_widget)->entry));

		gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(line));
		gchar *mount_point;
		if (widget->info)
			mount_point = gebr_maestro_info_get_home_mount_point(widget->info);
		else
			mount_point = NULL;
		gchar *path = gebr_relativise_path(value, mount_point, paths);

		gtk_entry_set_text(GTK_ENTRY(GEBR_GUI_FILE_ENTRY(widget->value_widget)->entry), path);

		g_free(path);
		g_free(mount_point);
		gebr_pairstrfreev(paths);
	}

	program_edit->priv->widgets = g_list_prepend(program_edit->priv->widgets, widget);

	return widget;
}

static GtkWidget *gebr_gui_program_edit_load_parameter(GebrGuiProgramEdit  *program_edit,
						       GebrGeoXmlParameter *parameter,
						       GSList             **radio_group)
{
	GebrGeoXmlParameterType type;

	type = gebr_geoxml_parameter_get_type(parameter);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GtkWidget *expander;
		GtkWidget *label_widget;
		GtkWidget *label;
		GtkWidget *image_widget;

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
		image_widget = gtk_image_new();
		program_edit->group_warning_widget = image_widget;

		gtk_widget_show(expander);
		gtk_expander_set_expanded(GTK_EXPANDER(expander),
					  gebr_geoxml_parameter_group_get_expand(parameter_group));

		label_widget = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(label_widget);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
		gebr_gui_gtk_expander_hacked_define(expander, label_widget);

		GebrGeoXmlParameters *template;
		required = FALSE;
		template = gebr_geoxml_parameter_group_get_template(parameter_group);
		gebr_geoxml_parameters_get_parameter(template, &param, 0);
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
		gtk_widget_show(image_widget);
		gtk_box_pack_start(GTK_BOX(label_widget), label, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(label_widget), image_widget, FALSE, TRUE, 5);

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

		GebrGroupReorderData * data = g_new0(GebrGroupReorderData, 1);
		data->group_vbox = GTK_BOX(group_vbox);
		gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(parameter_group), data);
		g_object_set(G_OBJECT(group_vbox), "user-data", deinstanciate_button, NULL);
		g_object_weak_ref(G_OBJECT(group_vbox), (GWeakNotify)on_instance_destroy, data);

		gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
		for (gboolean first_instance = TRUE, i = 0; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
			GtkWidget *widget;
			widget = gebr_gui_program_edit_load(program_edit, GEBR_GEOXML_PARAMETERS(instance));
			data->instances_list = g_list_prepend(data->instances_list, widget);
			g_object_set_data(G_OBJECT(widget), "list-node", data->instances_list);
			g_object_set_data(G_OBJECT(widget), "instance", instance);
			g_object_set_data(G_OBJECT(widget), "index", GUINT_TO_POINTER(i++));
			if (first_instance) {
				g_signal_connect(expander, "mnemonic-activate",
						 G_CALLBACK(on_group_expander_mnemonic_activate), program_edit);
				g_object_set_data(G_OBJECT(expander), "first-instance-widget", widget);
				first_instance = FALSE;
			}
			gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(instance), widget);
			gtk_box_pack_start(GTK_BOX(group_vbox), widget, FALSE, TRUE, 0);
		}

		data->instances_list = g_list_reverse(data->instances_list);

		/* Updates the arrow and delete buttons */
		GList * tmp;
		GtkWidget * first_frame;
		GtkWidget * last_frame;

		tmp = g_list_last(data->instances_list);

		first_frame = data->instances_list? data->instances_list->data : NULL;
		last_frame = tmp? tmp->data : NULL;
		update_frame(first_frame);
		update_frame(last_frame);

		return expander;
	} else {
		GtkWidget *hbox;
		GebrGuiParam *gebr_gui_param;

		GebrGeoXmlParameter *selected;

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);

		/* input widget */
		gebr_gui_param = create_parameter_widget(program_edit, parameter, type);

		gebr_gui_param->group_warning_widget = program_edit->group_warning_widget;
		gtk_widget_show(gebr_gui_param->widget);

		GebrGeoXmlParameters *params = gebr_geoxml_parameter_get_parameters(gebr_gui_param->parameter);
		gboolean is_mpi = gebr_geoxml_parameters_is_mpi(params);
		if (is_mpi)
			gtk_widget_set_sensitive(gebr_gui_param->value_widget, FALSE);
		gebr_geoxml_object_unref(params);

		/* exclusive? */
		selected = gebr_geoxml_parameters_get_selection(gebr_geoxml_parameter_get_parameters(parameter));
		if (selected != NULL) {
			GtkWidget *radio_button;

			radio_button = gtk_radio_button_new(*radio_group);
			gtk_widget_show(radio_button);
			*radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), selected == parameter);
			g_signal_connect(radio_button, "toggled",
					 G_CALLBACK(gebr_gui_program_edit_change_selected), gebr_gui_param);

			gtk_box_pack_start(GTK_BOX(hbox), radio_button, FALSE, FALSE, 15);
			gtk_widget_set_sensitive(gebr_gui_param->widget,
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
			gtk_label_set_mnemonic_widget(GTK_LABEL(label), gebr_gui_param->widget);

			align_vbox = gtk_vbox_new(FALSE, 0);
			gtk_widget_show(align_vbox);
			gtk_box_pack_start(GTK_BOX(align_vbox), label, FALSE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), align_vbox, FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(hbox), gebr_gui_param->widget, FALSE, TRUE, 0);
		} else {
			g_object_set(G_OBJECT(gebr_gui_param->value_widget),
				     "label", gebr_geoxml_parameter_get_label(parameter), NULL);

			gtk_box_pack_start(GTK_BOX(hbox), gebr_gui_param->widget, FALSE, FALSE, 0);
		}

		g_object_set_data(G_OBJECT(hbox), "parameter-widget", gebr_gui_param->widget);

		return hbox;
	}
}

/**
 * \internal
 */
static void
gebr_gui_program_edit_change_selected(GtkToggleButton * toggle_button, GebrGuiParam *widget)
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
	GebrGroupReorderData *data;
	GtkWidget *deinstanciate_button;
	GtkWidget *widget;
	GList *node;

	g_object_get(button, "user-data", &parameter_group, NULL);
	data = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	g_object_get(data->group_vbox, "user-data", &deinstanciate_button, NULL);

	instance = gebr_geoxml_parameter_group_instanciate(parameter_group);
	widget = gebr_gui_program_edit_load(program_edit, GEBR_GEOXML_PARAMETERS(instance));
	data->instances_list = g_list_append(data->instances_list, widget);
	node = g_list_last(data->instances_list);
	g_object_set_data(G_OBJECT(widget), "list-node", node);
	g_object_set_data(G_OBJECT(widget), "instance", instance);
	g_object_set_data(G_OBJECT(widget), "index", GUINT_TO_POINTER(g_list_length(data->instances_list) - 1));
	gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(instance), widget);
	gtk_box_pack_start(data->group_vbox, widget, FALSE, TRUE, 0);
	update_frame(node->data);
	update_frame(node->prev->data);

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
	GebrGroupReorderData *data;
	GtkWidget *widget;
	GList *penultimate;

	g_object_get(button, "user-data", &parameter_group, NULL);
	data = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	penultimate = g_list_last(data->instances_list)->prev;
	data->instances_list = g_list_delete_link(data->instances_list, penultimate->next);
	gebr_geoxml_parameter_group_get_instance(parameter_group, &last_instance,
						 gebr_geoxml_parameter_group_get_instances_number(parameter_group) - 1);

	widget = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(last_instance));
	gtk_widget_destroy(widget);
	gebr_geoxml_parameter_group_deinstanciate(parameter_group);
	update_frame(penultimate->data);
	update_frame(penultimate->prev? penultimate->prev->data:NULL);
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
static void on_arrow_up_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group)
{
	GtkWidget * frame1;
	GtkWidget * frame2;
	GList * node;
	GList * prev;
	guint index1;
	guint index2;
	GebrGroupReorderData *data;
	GebrGeoXmlSequence *instance;

	frame1 = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "frame"));
	node = (GList*)(g_object_get_data(G_OBJECT(frame1), "list-node"));
	instance = GEBR_GEOXML_SEQUENCE(g_object_get_data(G_OBJECT(frame1), "instance"));

	g_return_if_fail(node->prev != NULL);

	gebr_geoxml_sequence_move_up(instance);

	frame2 = GTK_WIDGET(node->prev->data);
	data = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	prev = node->prev;

	data->instances_list = g_list_delete_link(data->instances_list, node);
	data->instances_list = g_list_insert_before(data->instances_list, prev, frame1);
	g_object_set_data(G_OBJECT(frame1), "list-node", prev->prev);

	index1 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(frame1), "index")) - 1;
	index2 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(frame2), "index")) + 1;
	g_object_set_data(G_OBJECT(frame1), "index", GUINT_TO_POINTER(index1));
	g_object_set_data(G_OBJECT(frame2), "index", GUINT_TO_POINTER(index2));

	gtk_box_reorder_child(data->group_vbox, frame1, index1);

	update_frame(frame1);
	update_frame(frame2);
}

/**
 * \internal
 */
static void on_arrow_down_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group)
{
	GtkWidget * frame1;
	GtkWidget * frame2;
	GList * node;
	GList * next;
	guint index1;
	guint index2;
	GebrGroupReorderData *data;
	GebrGeoXmlSequence *instance;

	frame1 = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "frame"));
	node = (GList*)(g_object_get_data(G_OBJECT(frame1), "list-node"));
	instance = GEBR_GEOXML_SEQUENCE(g_object_get_data(G_OBJECT(frame1), "instance"));

	g_return_if_fail(node->next != NULL);

	gebr_geoxml_sequence_move_down(instance);

	frame2 = GTK_WIDGET(node->next->data);
	data = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	next = node->next;

	data->instances_list = g_list_delete_link(data->instances_list, node);
	data->instances_list = g_list_insert_before(data->instances_list, next->next, frame1);
	g_object_set_data(G_OBJECT(frame1), "list-node", next->next);

	index1 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(frame1), "index")) + 1;
	index2 = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(frame2), "index")) - 1;
	g_object_set_data(G_OBJECT(frame1), "index", GUINT_TO_POINTER(index1));
	g_object_set_data(G_OBJECT(frame2), "index", GUINT_TO_POINTER(index2));

	gtk_box_reorder_child(data->group_vbox, frame1, index1);

	update_frame(frame1);
	update_frame(frame2);
}

/**
 * \internal
 */
static void on_delete_clicked(GtkWidget *button, GebrGeoXmlParameterGroup * parameter_group)
{
	GList * iter;
	GList * node;
	gulong index;
	GtkWidget * frame;
	GtkWidget * frame_prev;
	GtkWidget * frame_next;
	GebrGeoXmlSequence * instance;
	GebrGroupReorderData * data;

	data = gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(parameter_group));
	frame = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "frame"));
	node = (GList*)(g_object_get_data(G_OBJECT(frame), "list-node"));
	iter = node->next;
	frame_prev = GTK_WIDGET(node->prev ? node->prev->data : NULL);
	frame_next = GTK_WIDGET(node->next ? node->next->data : NULL);

	instance = GEBR_GEOXML_SEQUENCE(g_object_get_data(G_OBJECT(frame), "instance"));
	data->instances_list = g_list_delete_link(data->instances_list, node);
	gebr_geoxml_sequence_remove(instance);
	update_frame(frame_prev);
	update_frame(frame_next);

	/* If there is only one item in the list, we must disable the 'Deinstanciate' button. */
	if (!data->instances_list->next) {
		GtkWidget * deinstanciate;
		g_object_get(data->group_vbox, "user-data", &deinstanciate, NULL);
		gtk_widget_set_sensitive(deinstanciate, FALSE);
	}

	index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(frame), "index"));
	while (iter) {
		g_object_set_data(G_OBJECT(iter->data), "index", GUINT_TO_POINTER(index++));
		iter = iter->next;
	}

	gtk_widget_destroy(frame);
}

/**
 * \internal
 */
static void update_frame(GtkWidget * frame)
{
	GList *node;
	GtkWidget *up;
	GtkWidget *down;
	GtkWidget *delbtn;

	if (frame) {
		node = (GList *)(g_object_get_data(G_OBJECT(frame), "list-node"));
		up = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "arrow-up"));
		down = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "arrow-down"));
		delbtn = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "delete"));
		gtk_widget_set_sensitive(up, node->prev != NULL);
		gtk_widget_set_sensitive(down, node->next != NULL);
		if (delbtn)
			gtk_widget_set_sensitive(delbtn, node->next || node->prev);
	}
}

static void on_instance_destroy(GebrGroupReorderData * data)
{
	g_list_free(data->instances_list);
	g_free(data);
}
