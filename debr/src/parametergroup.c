/*   DeBR - GeBR Designer
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

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>

#include "debr.h"
#include "callbacks.h"
#include "parameter.h"
#include "parametergroup.h"

/*
 * Declarations
 */

static GtkWidget* parameter_group_instances_setup_ui_foreach(struct ui_parameter_group_dialog *ui, GebrGeoXmlSequence *instance, gint index, gboolean template);
static void parameter_group_instances_setup_ui(struct ui_parameter_group_dialog *ui);
static gboolean on_parameter_group_instances_changed(GtkSpinButton * spin_button, struct ui_parameter_group_dialog *ui);
static void
on_parameter_group_is_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog *ui);
static void on_parameter_group_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog *ui);

/*
 * Public functions
 */

gboolean parameter_group_dialog_setup_ui(gboolean new_parameter)
{
	GtkWidget *dialog;
	GtkWidget *scrolled_window;
	GtkWidget *table;
	int row;
	GtkWidget *instances_edit_vbox;

	GtkWidget *label_label;
	GtkWidget *label_entry;
	GtkWidget *instantiable_label;
	GtkWidget *instanciable_check_button;
	GtkWidget *instances_label;
	GtkWidget *instances_spin_button;
	GtkWidget *exclusive_label;
	GtkWidget *exclusive_check_button;
	GtkWidget *expanded_label;
	GtkWidget *expanded_check_button;

	GebrGeoXmlParameterGroup *parameter_group;
	GebrGeoXmlSequence *instance;
	GebrGeoXmlSequence *parameter;
	guint i;
	gboolean ret = TRUE;

	struct ui_parameter_group_dialog *ui;

	menu_archive();

	ui = g_new(struct ui_parameter_group_dialog, 1);
	ui->parameter_group = parameter_group = GEBR_GEOXML_PARAMETER_GROUP(debr.parameter);
	ui->dialog = dialog = gtk_dialog_new_with_buttons(_("Edit parameter (group)"),
							  GTK_WINDOW(debr.window),
							  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							  GTK_STOCK_OK, GTK_RESPONSE_OK,NULL);
	gtk_widget_set_size_request(dialog, -1, 500);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	table = gtk_table_new(11, 2, FALSE);
	gtk_widget_show(table);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	row = 0;

	/*
	 * Label
	 */
	label_label = gtk_label_new(_("Label:"));
	gtk_widget_show(label_label);
	gtk_table_attach(GTK_TABLE(table), label_label, 0, 1, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);

	label_entry = gtk_entry_new();
	gtk_widget_show(label_entry);
	gtk_table_attach(GTK_TABLE(table), label_entry, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;

	/*
	 * Expanded by default
	 */
	expanded_label = gtk_label_new(_("Expanded by default:"));
	gtk_widget_show(expanded_label);
	gtk_table_attach(GTK_TABLE(table), expanded_label, 0, 1, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(expanded_label), 0, 0.5);

	expanded_check_button = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expanded_check_button), gebr_geoxml_parameter_group_get_expand(parameter_group));
	gtk_widget_show(expanded_check_button);
	gtk_table_attach(GTK_TABLE(table), expanded_check_button, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;

	/*
	 * Instantiable
	 */
	instantiable_label = gtk_label_new(_("Instantiable:"));
	gtk_widget_show(instantiable_label);
	gtk_table_attach(GTK_TABLE(table), instantiable_label, 0, 1, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instantiable_label), 0, 0.5);

	instanciable_check_button = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(instanciable_check_button), gebr_geoxml_parameter_group_get_is_instanciable(parameter_group));
	gtk_widget_show(instanciable_check_button);
	gtk_table_attach(GTK_TABLE(table), instanciable_check_button, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;

	/*
	 * Exclusive
	 */
	exclusive_label = gtk_label_new(_("Exclusive:"));
	gtk_widget_show(exclusive_label);
	gtk_table_attach(GTK_TABLE(table), exclusive_label, 0, 1, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(exclusive_label), 0, 0.5);

	exclusive_check_button = gtk_check_button_new();
	gtk_widget_show(exclusive_check_button);
	gtk_table_attach(GTK_TABLE(table), exclusive_check_button, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;

	instance = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameter_group_get_template(ui->parameter_group));
	GtkWidget * template_frame = parameter_group_instances_setup_ui_foreach(ui, instance, 0, TRUE);
	gtk_table_attach(GTK_TABLE(table), template_frame, 0, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;

	/*
	 * Instances
	 */
	instances_label = gtk_label_new(_("Instances:"));
	gtk_widget_show(instances_label);
	gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

	instances_spin_button = gtk_spin_button_new_with_range(1, 999999999, 1);
	gtk_widget_show(instances_spin_button);
	gtk_table_attach(GTK_TABLE(table), instances_spin_button, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), ++row;
	/* if we don't any parameter we cannot instanciate */
	GebrGeoXmlParameters *template;
	template = gebr_geoxml_parameter_group_get_template(parameter_group);
	gtk_widget_set_sensitive(instances_spin_button,
				 gebr_geoxml_parameters_get_number(template) > 0 ? TRUE : FALSE);

	ui->instances_edit_vbox = instances_edit_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(ui->instances_edit_vbox);
	gtk_table_attach(GTK_TABLE(table), instances_edit_vbox, 0, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) (0), 0, 0), ++row;

	/* 
	 * XML group data -> UI
	 * */
	gtk_entry_set_text(GTK_ENTRY(label_entry), gebr_geoxml_parameter_get_label(debr.parameter));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin_button),
				  gebr_geoxml_parameter_group_get_instances_number(parameter_group));

	/* scan for an exclusive instance */
	gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button), TRUE);
	for (i = 0, parameter = NULL; instance != NULL; ++i, gebr_geoxml_sequence_next(&instance)) {
		parameter =
		    GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_default_selection(GEBR_GEOXML_PARAMETERS(instance)));
		if (parameter == NULL) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button), FALSE);
			break;
		}
	}
	parameter_group_instances_setup_ui(ui);

	GebrValidateCase *validate_case;
	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PARAMETER_LABEL);
	g_signal_connect(label_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_parameter)
		on_entry_focus_out(GTK_ENTRY(label_entry), NULL, validate_case);

	/* signals */
	g_signal_connect(instances_spin_button, "output", G_CALLBACK(on_parameter_group_instances_changed), ui);
	g_signal_connect(exclusive_check_button, "toggled", G_CALLBACK(on_parameter_group_is_exclusive_toggled), ui);

	/* For DeBR it doesn't matter if it's not instantiable, because we need to instantiate it
	 * (gebr_geoxml_parameter_group_instanciate doesn't work for non-instatiable groups).
	 * 'Instantiable' makes sense only for menus added in GeBR. */
	gebr_geoxml_parameter_group_set_is_instanciable(parameter_group, TRUE);
	/* let the user interact... */
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
		ret = FALSE;
		menu_replace();
		goto out;
	}

	/* things not automatically synced to XML are synced here */
	gebr_geoxml_parameter_set_label(debr.parameter, gtk_entry_get_text(GTK_ENTRY(label_entry)));
	gebr_geoxml_parameter_group_set_expand(parameter_group,
					       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(expanded_check_button)));
	gebr_geoxml_parameter_group_set_is_instanciable(parameter_group,
							gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
										     (instanciable_check_button)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	parameter_load_selected();

out:
	gtk_widget_destroy(dialog);
	g_free(ui);

	return ret;
}

/*
 * Private functions
 */

/**
 * \internal
 *
 */
static GtkWidget* parameter_group_instances_setup_ui_foreach(struct ui_parameter_group_dialog *ui,
							     GebrGeoXmlSequence *instance, gint index, gboolean template)
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label_widget;

	GebrGeoXmlSequence *parameter;
	GebrGeoXmlSequence *exclusive;

	table = gtk_table_new(gebr_geoxml_parameters_get_number(GEBR_GEOXML_PARAMETERS(instance)), 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	label_widget = NULL;
	exclusive = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_default_selection(GEBR_GEOXML_PARAMETERS(instance)));
	gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &parameter, 0);
	for (gint j = 0; parameter != NULL; ++j, gebr_geoxml_sequence_next(&parameter)) {
		struct gebr_gui_parameter_widget *widget;

		if (template || exclusive == NULL) {
			label_widget = gtk_label_new(gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)));
			gtk_misc_set_alignment(GTK_MISC(label_widget), 0, 0.5);
		} else {
			label_widget =
				gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label_widget),
									    gebr_geoxml_parameter_get_label
									    (GEBR_GEOXML_PARAMETER(parameter)));
			if (exclusive == parameter)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label_widget), TRUE);
			g_signal_connect(label_widget, "toggled",
					 G_CALLBACK(on_parameter_group_exclusive_toggled), ui);
			g_object_set(label_widget, "user-data", parameter, NULL);
		}
		gtk_widget_show(label_widget);
		gtk_table_attach(GTK_TABLE(table), label_widget, 0, 1, j, j + 1,
				 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

		widget = gebr_gui_parameter_widget_new(GEBR_GEOXML_PARAMETER(parameter), TRUE, NULL);
		if (template)
			gebr_gui_parameter_widget_set_readonly(widget, TRUE);
		gtk_widget_show(widget->widget);
		gtk_table_attach(GTK_TABLE(table), widget->widget, 1, 2, j, j + 1,
				 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	}

	GString *string = g_string_new(NULL);
	if (!template)
		g_string_printf(string, _("Defaults for instance #%d"), index);
	else
		g_string_printf(string, _("Defaults for a new instance"));
	frame = gtk_frame_new(string->str);
	g_string_free(string, TRUE);

	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_widget_show(frame);
	
	return frame;
}

/**
 * \internal
 * Build the user interface for a group.
 */
static void parameter_group_instances_setup_ui(struct ui_parameter_group_dialog *ui)
{
	gtk_container_foreach(GTK_CONTAINER(ui->instances_edit_vbox), (GtkCallback) gtk_widget_destroy, NULL);

	GebrGeoXmlSequence *instance;
	gebr_geoxml_parameter_group_get_instance(ui->parameter_group, &instance, 0);
	for (gint i = 1; instance != NULL; i++, gebr_geoxml_sequence_next(&instance)) {
		GtkWidget * frame = parameter_group_instances_setup_ui_foreach(ui, instance, i, FALSE);
		gtk_box_pack_start(GTK_BOX(ui->instances_edit_vbox), frame, TRUE, TRUE, 5);
	}
}

/**
 * \internal
 * Called when the number of instances is changed.
 */
static gboolean on_parameter_group_instances_changed(GtkSpinButton * spin_button, struct ui_parameter_group_dialog *ui)
{
	gint i, instanciate;

	instanciate = gtk_spin_button_get_value(spin_button) -
	    gebr_geoxml_parameter_group_get_instances_number(ui->parameter_group);
	if (instanciate == 0)
		return FALSE;

	if (instanciate > 0)
		for (i = 0; i < instanciate; ++i)
			gebr_geoxml_parameter_group_instanciate(ui->parameter_group);
	else
		for (i = instanciate; i < 0; ++i)
			gebr_geoxml_parameter_group_deinstanciate(ui->parameter_group);

	parameter_group_instances_setup_ui(ui);

	return FALSE;
}

/**
 * \internal
 * Called when the 'Exclusive' toggle button is pressed.
 */
static void
on_parameter_group_is_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog *ui)
{
	gboolean toggled;
	GebrGeoXmlSequence *instance;
	GebrGeoXmlParameters *template;
	GebrGeoXmlSequence *first_parameter;

	toggled = gtk_toggle_button_get_active(toggle_button);
	template = gebr_geoxml_parameter_group_get_template(ui->parameter_group);
	if (!toggled)
		gebr_geoxml_parameters_set_default_selection(template, NULL);
	else {
		gebr_geoxml_parameters_get_parameter(template, &first_parameter, 0);
		gebr_geoxml_parameters_set_default_selection(template, GEBR_GEOXML_PARAMETER(first_parameter));
	}

	gebr_geoxml_parameter_group_get_instance(ui->parameter_group, &instance, 0);
	for (; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
		if (!toggled)
			gebr_geoxml_parameters_set_default_selection(GEBR_GEOXML_PARAMETERS(instance), NULL);
		else {
			gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &first_parameter, 0);
			gebr_geoxml_parameters_set_default_selection(GEBR_GEOXML_PARAMETERS(instance),
								     GEBR_GEOXML_PARAMETER(first_parameter));
		}
	}

	parameter_group_instances_setup_ui(ui);
}

/**
 * \internal
 */
static void on_parameter_group_exclusive_toggled(GtkToggleButton * toggle_button, struct ui_parameter_group_dialog *ui)
{
	if (gtk_toggle_button_get_active(toggle_button) == FALSE)
		return;

	GebrGeoXmlParameter *parameter;

	g_object_get(toggle_button, "user-data", &parameter, NULL);
	gebr_geoxml_parameters_set_default_selection(gebr_geoxml_parameter_get_parameters(parameter), parameter);
}
