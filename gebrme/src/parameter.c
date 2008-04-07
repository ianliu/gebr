/*   G�BR ME - G�BR Menu Editor
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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
#include <string.h>

#include <gui/gtkfileentry.h>
#include <gui/gtkenhancedentry.h>
#include <gui/utils.h>

#include "parameter.h"
#include "support.h"
#include "gebrme.h"
#include "groupparameters.h"
#include "interface.h"
#include "menu.h"

/*
 * Section: Private
 */

static void
parameter_create_ui_type_general(GtkWidget * table, struct parameter_data * data);
static void
parameter_data_free(GtkObject * expander, struct parameter_data * data);
static void
parameter_up(GtkButton * button, struct parameter_data * data);
static void
parameter_down(GtkButton * button, struct parameter_data * data);
static void
parameter_remove(GtkButton * button, struct parameter_data * data);
static void
parameter_type_changed(GtkComboBox * combo, struct parameter_data * data);
static void
parameter_required_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_separator_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_default_widget_changed(struct parameter_widget * widget, struct parameter_data * data);
static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data);
static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct parameter_data * data);
static void
parameter_change_exclusive(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_exclusive_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_expanded_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_multiple_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static gboolean
parameter_group_instances_changed(GtkSpinButton * spinbutton, struct parameter_data * data);
static void
parameter_uilabel_update(struct parameter_data * data);

/*
 * Public functions
 */

GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, struct parameters_data * parameters_data, gboolean hidden)
{
	struct parameter_data *		data;
	enum GEOXML_PARAMETERTYPE	type;

	GtkWidget *			frame;

	GtkWidget *			parameter_expander;
	GtkWidget *			parameter_label_widget;
	GtkWidget *			parameter_label;
	GtkWidget *			depth_hbox;
	GtkWidget *			parameter_vbox;
	GtkWidget *			parameter_table;

	GtkWidget *			type_label;
	GtkWidget *			type_combo;
	GtkWidget *			label_label;
	GtkWidget *			label_entry;
	GtkWidget *			general_table;

	GtkWidget *			widget;
	GtkWidget *			button_hbox;
	GtkWidget *			align;

	data = g_malloc(sizeof(struct parameter_data));
	data->parameter = parameter;
	data->parameters_data = parameters_data;
	type = geoxml_parameter_get_type(parameter);

	frame = gtk_frame_new("");
	gtk_widget_show(frame);
	g_object_set(G_OBJECT(frame), "shadow-type", GTK_SHADOW_OUT, NULL);

	parameter_expander = gtk_expander_new("");
	gtk_container_add(GTK_CONTAINER(frame), parameter_expander);
	gtk_expander_set_expanded(GTK_EXPANDER(parameter_expander), !hidden);
	gtk_widget_show(parameter_expander);
	depth_hbox = gtk_container_add_depth_hbox(parameter_expander);
	g_signal_connect(parameter_expander, "destroy",
		GTK_SIGNAL_FUNC(parameter_data_free), data);

	parameter_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(parameter_expander), parameter_label_widget);
	gtk_expander_hacked_define(parameter_expander, parameter_label_widget);

	if (parameters_data->is_group == TRUE) {
		GtkWidget *	radio_button;

		radio_button = gtk_radio_button_new(
			((struct group_parameters_data *)parameters_data)->radio_group);
		data->radio_button = radio_button;
		((struct group_parameters_data *)parameters_data)->radio_group =
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), radio_button, FALSE, FALSE, 0);
		g_signal_connect(radio_button, "toggled",
			(GCallback)parameter_change_exclusive, data);

		parameter_label = gtk_label_new("");
		data->label = parameter_label;
		gtk_widget_show(parameter_label);
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), parameter_label, FALSE, FALSE, 0);
		g_object_set(G_OBJECT(parameter_label), "user-data", radio_button, NULL);

		gtk_widget_show_all(parameter_label_widget);
	} else {
		data->radio_button = NULL;

		parameter_label_widget = gtk_hbox_new(FALSE, 0);
		gtk_expander_set_label_widget(GTK_EXPANDER(parameter_expander), parameter_label_widget);
		gtk_widget_show(parameter_label_widget);
		gtk_expander_hacked_define(parameter_expander, parameter_label_widget);

		parameter_label = gtk_label_new("");
		data->label = parameter_label;
		gtk_widget_show(parameter_label);
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), parameter_label, FALSE, FALSE, 0);
	}

	parameter_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), parameter_vbox, TRUE, TRUE, 0);
	gtk_widget_show(parameter_vbox);

	parameter_table = gtk_table_new (3, 2, FALSE);
	gtk_widget_show (parameter_table);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), parameter_table, FALSE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (parameter_table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (parameter_table), 5);

	type_label = gtk_label_new (_("Type:"));
	gtk_widget_show (type_label);
	gtk_table_attach (GTK_TABLE (parameter_table), type_label, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (type_label), 0, 0.5);

	type_combo = gtk_combo_box_new_text ();
	gtk_widget_show (type_combo);
	gtk_table_attach (GTK_TABLE (parameter_table), type_combo, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	/* TODO: map indexes to GEOXML_PARAMETERTYPE */
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("string"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("integer"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("file"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("flag"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("real number"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("range"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("enumeration"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("group"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), type);
	g_signal_connect(type_combo, "changed",
		(GCallback)parameter_type_changed, data);

	/*
	 * Label
	 */
	label_label = gtk_label_new(_("Label:"));
	gtk_widget_show(label_label);
	gtk_table_attach(GTK_TABLE(parameter_table), label_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);

	label_entry = gtk_entry_new();
	gtk_widget_show(label_entry);
	gtk_table_attach(GTK_TABLE(parameter_table), label_entry, 1, 2, 1, 2,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(label_entry), geoxml_parameter_get_label(parameter));
	/* signal */
	g_signal_connect(label_entry, "changed",
		(GCallback)parameter_label_changed, data);

	/*
	 * General parameters fields
	 */
	general_table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(general_table);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), general_table, FALSE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(general_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(general_table), 5);
	g_object_set(G_OBJECT(type_combo), "user-data", general_table, NULL);

	/* create parameter fields on GUI */
	parameter_create_ui_type_general(general_table, data);
	parameter_uilabel_update(data);

	/* finishing...
	 * Buttons Up, Down and Remove
	 */
	button_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(button_hbox);
	gtk_box_pack_end(GTK_BOX(parameter_vbox), button_hbox, TRUE, FALSE, 0);

	widget = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(button_hbox), widget, FALSE, FALSE, 5);
	g_signal_connect(widget, "clicked",
		GTK_SIGNAL_FUNC (parameter_up), data);
	g_object_set(G_OBJECT(widget), "user-data", frame,
		"relief", GTK_RELIEF_NONE, NULL);

	widget = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(button_hbox), widget, FALSE, FALSE, 5);
	g_signal_connect(widget, "clicked",
		GTK_SIGNAL_FUNC (parameter_down),
		data);
	g_object_set(G_OBJECT(widget), "user-data", frame,
		"relief", GTK_RELIEF_NONE, NULL);

	align = gtk_alignment_new(1, 0, 0, 1);
	gtk_widget_show(align);
	widget = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(button_hbox), align, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(align), widget);
	g_signal_connect(widget, "clicked",
		GTK_SIGNAL_FUNC (parameter_remove), data);
	g_object_set(G_OBJECT(widget), "user-data", frame,
		"relief", GTK_RELIEF_NONE, NULL);

	return frame;
}

void
parameter_create_ui_type_specific(GtkWidget * table, struct parameter_data * data)
{
	GtkWidget *			default_label;
	GtkWidget *			default_widget_hbox;
	GtkWidget *			default_widget;
	GtkWidget *			widget;
	GeoXmlProgramParameter *	program_parameter;
	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(data->parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *	parameters;

		if (geoxml_parameter_group_get_can_instanciate(GEOXML_PARAMETER_GROUP(data->parameter)) == TRUE) {
			GtkWidget *	instances_label;
			GtkWidget *	instances_spin;

			instances_label = gtk_label_new(_("Instances:"));
			gtk_widget_show(instances_label);
			gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, 0, 1,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(GTK_FILL), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

			instances_spin = gtk_spin_button_new_with_range(1, 999999999, 1);
			gtk_widget_show(instances_spin);
			gtk_table_attach(GTK_TABLE(table), instances_spin, 1, 2, 0, 1,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin),
				geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(data->parameter)));
			g_signal_connect(instances_spin, "output",
				(GCallback)parameter_group_instances_changed, data);
		}

		parameters = group_parameters_create_ui(data, TRUE);
		gtk_table_attach(GTK_TABLE(table), parameters, 0, 2, 1, 2,
			(GtkAttachOptions)(GTK_FILL|GTK_EXPAND),
			(GtkAttachOptions)(GTK_FILL|GTK_EXPAND), 0, 0);

		data->widget = NULL;
		return;
	}

	default_label = gtk_label_new(_("Default:"));
	gtk_widget_show(default_label);
	widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(widget), default_label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(default_label), 0, 0.5);

	default_widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(default_widget_hbox);

	program_parameter = GEOXML_PROGRAM_PARAMETER(data->parameter);
	switch (type) {
	case GEOXML_PARAMETERTYPE_STRING:
		data->widget = parameter_widget_new_string(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_INT:
		data->widget = parameter_widget_new_int(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		data->widget = parameter_widget_new_float(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		data->widget = parameter_widget_new_flag(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		type_label;
		GtkWidget *		type_combo;

		data->widget = parameter_widget_new_file(data->parameter, NULL, TRUE);

		type_label = gtk_label_new (_("Type:"));
		gtk_widget_show (type_label);
		gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 1, 2,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (type_label), 0, 0.5);

		type_combo = gtk_combo_box_new_text ();
		gtk_widget_show (type_combo);
		gtk_table_attach (GTK_TABLE (table), type_combo, 1, 2, 1, 2,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("File"));
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("Directory"));
		g_signal_connect(type_combo, "changed",
				(GCallback)parameter_file_type_changed, data);

		/* file or directory? */
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo),
			geoxml_program_parameter_get_file_be_directory(program_parameter) == TRUE ? 1 : 0);
		break;
	} case GEOXML_PARAMETERTYPE_RANGE: {
		GtkWidget *	min_label;
		GtkWidget *	min_entry;
		GtkWidget *	max_label;
		GtkWidget *	max_entry;
		GtkWidget *	inc_label;
		GtkWidget *	inc_entry;
		GtkWidget *	digits_label;
		GtkWidget *	digits_entry;
		gchar *		min_str;
		gchar *		max_str;
		gchar *		inc_str;
		gchar *		digits_str;

		geoxml_program_parameter_get_range_properties(program_parameter,
			&min_str, &max_str, &inc_str, &digits_str);
		data->widget = parameter_widget_new_range(data->parameter, TRUE);

		min_label = gtk_label_new(_("Minimum:"));
		gtk_widget_show(min_label);
		gtk_table_attach(GTK_TABLE(table), min_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(min_label), 0, 0.5);
		min_entry = gtk_entry_new();
		gtk_widget_show(min_entry);
		gtk_table_attach(GTK_TABLE(table), min_entry, 1, 2, 1, 2,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(min_entry), min_str);
		g_signal_connect(min_entry, "changed",
			(GCallback)parameter_range_min_changed, data);

		max_label = gtk_label_new(_("Maximum:"));
		gtk_widget_show(max_label);
		gtk_table_attach(GTK_TABLE(table), max_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(max_label), 0, 0.5);
		max_entry = gtk_entry_new();
		gtk_widget_show(max_entry);
		gtk_table_attach(GTK_TABLE(table), max_entry, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(max_entry), max_str);
		g_signal_connect(max_entry, "changed",
			(GCallback)parameter_range_max_changed, data);

		inc_label = gtk_label_new(_("Increment:"));
		gtk_widget_show(inc_label);
		gtk_table_attach(GTK_TABLE(table), inc_label, 0, 1, 4, 5,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(inc_label), 0, 0.5);
		inc_entry = gtk_entry_new();
		gtk_widget_show(inc_entry);
		gtk_table_attach(GTK_TABLE(table), inc_entry, 1, 2, 4, 5,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(inc_entry), inc_str);
		g_signal_connect(inc_entry, "changed",
			(GCallback)parameter_range_inc_changed, data);

		digits_label = gtk_label_new(_("Digits:"));
		gtk_widget_show(digits_label);
		gtk_table_attach(GTK_TABLE(table), digits_label, 0, 1, 5, 6,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(digits_label), 0, 0.5);
		digits_entry = gtk_entry_new();
		gtk_widget_show(digits_entry);
		gtk_table_attach(GTK_TABLE(table), digits_entry, 1, 2, 5, 6,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(digits_entry), digits_str);
		g_signal_connect(digits_entry, "changed",
			(GCallback)parameter_range_digits_changed, data);

		break;
	} case GEOXML_PARAMETERTYPE_ENUM: {
		GtkWidget *		enum_option_edit;
		GtkWidget *		options_label;
		GtkWidget *		vbox;

		GeoXmlSequence *	option;

		data->widget = parameter_widget_new_enum(data->parameter, TRUE);

		options_label = gtk_label_new (_("Options:"));
		gtk_widget_show(options_label);
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_box_pack_start(GTK_BOX(vbox), options_label, FALSE, FALSE, 0);
		gtk_table_attach(GTK_TABLE (table), vbox, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(options_label), 0, 0.5);

		geoxml_program_parameter_get_enum_option(GEOXML_PROGRAM_PARAMETER(data->parameter), &option, 0);
		enum_option_edit = enum_option_edit_new(GEOXML_ENUM_OPTION(option),
			GEOXML_PROGRAM_PARAMETER(data->parameter));
		gtk_widget_show(enum_option_edit);
		gtk_table_attach (GTK_TABLE (table), enum_option_edit, 1, 2, 1, 2,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		g_signal_connect(GTK_OBJECT(enum_option_edit), "changed",
			GTK_SIGNAL_FUNC(parameter_enum_options_changed), data);
		g_object_set(G_OBJECT(enum_option_edit), "user-data", default_widget_hbox, NULL);

		break;
	} default:
		data->widget = NULL;
		return;
	}

	if (data->parameters_data->is_group == TRUE &&
	geoxml_parameter_group_get_exclusive(((struct group_parameters_data *)data->parameters_data)->group) == TRUE) {
		GtkToggleButton *	radio_button;

		g_object_get(G_OBJECT(data->label), "user-data", &radio_button, NULL);
		gtk_toggle_button_set_active(radio_button, (gboolean)
			strlen(geoxml_program_parameter_get_default(program_parameter)));
		g_signal_emit_by_name(radio_button, "toggled");
	}

	default_widget = data->widget->widget;
	gtk_widget_show(default_widget);
	parameter_widget_set_auto_submit_callback(data->widget,
		(changed_callback)parameter_default_widget_changed, data);

	gtk_box_pack_start(GTK_BOX(default_widget_hbox), default_widget, TRUE, TRUE, 0);
	gtk_table_attach(GTK_TABLE(table), default_widget_hbox, 1, 2, 0, 1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
}

/*
 * Section: Private
 */

static void
parameter_create_ui_type_general(GtkWidget * table, struct parameter_data * data)
{
	GtkWidget *			specific_table;
	enum GEOXML_PARAMETERTYPE	type;

	specific_table = gtk_table_new(6, 2, FALSE);
	data->specific_table = specific_table;
	gtk_widget_show(specific_table);
	gtk_table_attach(GTK_TABLE(table), specific_table, 0, 2, 3, 4,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_table_set_row_spacings(GTK_TABLE(specific_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(specific_table), 5);

	type = geoxml_parameter_get_type(data->parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *		exclusive_label;
		GtkWidget *		exclusive_checkbox;
		GtkWidget *		expanded_label;
		GtkWidget *		expanded_checkbox;
		GtkWidget *		multiple_label;
		GtkWidget *		multiple_checkbox;

		GeoXmlParameterGroup *	parameter_group;

		parameter_group = GEOXML_PARAMETER_GROUP(data->parameter);

		exclusive_label = gtk_label_new(_("Exclusive:"));
		gtk_widget_show(exclusive_label);
		gtk_table_attach(GTK_TABLE(table), exclusive_label, 0, 1, 0, 1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(exclusive_label), 0, 0.5);

		exclusive_checkbox = gtk_check_button_new();
		gtk_widget_show(exclusive_checkbox);
		gtk_table_attach(GTK_TABLE(table), exclusive_checkbox, 1, 2, 0, 1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_checkbox),
			geoxml_parameter_group_get_exclusive(parameter_group));
		g_signal_connect(exclusive_checkbox, "toggled",
			(GCallback)parameter_group_exclusive_changed, data);

		expanded_label = gtk_label_new(_("Expanded by default:"));
		gtk_widget_show(expanded_label);
		gtk_table_attach(GTK_TABLE(table), expanded_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(expanded_label), 0, 0.5);

		expanded_checkbox = gtk_check_button_new();
		gtk_widget_show(expanded_checkbox);
		gtk_table_attach(GTK_TABLE(table), expanded_checkbox, 1, 2, 1, 2,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		g_signal_connect(expanded_checkbox, "toggled",
			(GCallback)parameter_group_expanded_changed, data);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expanded_checkbox),
			geoxml_parameter_group_get_expand(parameter_group));

		multiple_label = gtk_label_new(_("Instanciable:"));
		gtk_widget_show(multiple_label);
		gtk_table_attach(GTK_TABLE(table), multiple_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(multiple_label), 0, 0.5);

		multiple_checkbox = gtk_check_button_new();
		gtk_widget_show(multiple_checkbox);
		gtk_table_attach(GTK_TABLE(table), multiple_checkbox, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multiple_checkbox),
			geoxml_parameter_group_get_can_instanciate(parameter_group));
		g_signal_connect(multiple_checkbox, "toggled",
			(GCallback)parameter_group_multiple_changed, data);
	} else {
		GtkWidget *			keyword_label;
		GtkWidget *			keyword_entry;
		GeoXmlProgramParameter *	program_parameter;

		program_parameter = GEOXML_PROGRAM_PARAMETER(data->parameter);

		/*
 		 * Keyword
 		 */
		keyword_label = gtk_label_new(_("Keyword:"));
		gtk_widget_show(keyword_label);
		gtk_table_attach(GTK_TABLE(table), keyword_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(keyword_label), 0, 0.5);

		keyword_entry = gtk_entry_new();
		gtk_widget_show(keyword_entry);
		gtk_table_attach(GTK_TABLE(table), keyword_entry, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		/* read */
		gtk_entry_set_text(GTK_ENTRY(keyword_entry),
			geoxml_program_parameter_get_keyword(program_parameter));
		/* signal */
		g_signal_connect(keyword_entry, "changed",
			(GCallback)parameter_keyword_changed, data);
		g_object_set(G_OBJECT(keyword_entry), "user-data", data->label, NULL);

		if (type != GEOXML_PARAMETERTYPE_FLAG) {
			GtkWidget *	required_label;
			GtkWidget *	required_checkbox;
			GtkWidget *	is_list_label;
			GtkWidget *	is_list_checkbox;
			gboolean	is_list;

			required_label = gtk_label_new(_("Required:"));
			gtk_widget_show(required_label);
			gtk_table_attach(GTK_TABLE(table), required_label, 0, 1, 0, 1,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(required_label), 0, 0.5);

			required_checkbox = gtk_check_button_new();
			gtk_widget_show(required_checkbox);
			gtk_table_attach(GTK_TABLE(table), required_checkbox, 1, 2, 0, 1,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			g_signal_connect(required_checkbox, "toggled",
				(GCallback)parameter_required_changed, data);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(required_checkbox),
				geoxml_program_parameter_get_required(program_parameter));

			is_list_label = gtk_label_new(_("Is list?:"));
			gtk_widget_show(is_list_label);
			gtk_table_attach(GTK_TABLE(table), is_list_label, 0, 1, 1, 2,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(is_list_label), 0, 0.5);

			is_list_checkbox = gtk_check_button_new();
			gtk_widget_show(is_list_checkbox);
			gtk_table_attach(GTK_TABLE(table), is_list_checkbox, 1, 2, 1, 2,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			is_list = geoxml_program_parameter_get_is_list(program_parameter);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(is_list_checkbox), is_list);
			g_signal_connect(is_list_checkbox, "toggled",
				(GCallback)parameter_is_list_changed, data);
			g_object_set(G_OBJECT(is_list_checkbox), "user-data", table, NULL);

			if (is_list == TRUE) {
				GtkWidget *	separator_label;
				GtkWidget *	separator_entry;

				separator_label = gtk_label_new(_("List separator:"));
				gtk_widget_show(separator_label);
				gtk_table_attach(GTK_TABLE(table), separator_label, 0, 1, 2, 3,
					(GtkAttachOptions)(GTK_FILL),
					(GtkAttachOptions)(0), 0, 0);
				gtk_misc_set_alignment(GTK_MISC(separator_label), 0, 0.5);

				separator_entry = gtk_entry_new();
				gtk_widget_show(separator_entry);
				gtk_table_attach(GTK_TABLE(table), separator_entry, 1, 2, 2, 3,
					(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions)(0), 0, 0);
				gtk_entry_set_text(GTK_ENTRY(separator_entry),
					geoxml_program_parameter_get_list_separator(program_parameter));
				g_signal_connect(separator_entry, "changed",
					(GCallback)parameter_separator_changed, data);
				g_object_set(G_OBJECT(separator_entry), "user-data", specific_table, NULL);
			}
		}
	}

	if (data->parameters_data->is_group == TRUE)
		g_object_set(data->radio_button,
			"visible", geoxml_parameter_group_get_exclusive(
				((struct group_parameters_data *)data->parameters_data)->group) == TRUE &&
				geoxml_parameter_get_is_program_parameter(data->parameter) == TRUE,
			NULL);

	parameter_create_ui_type_specific(specific_table, data);
}

static void
parameter_data_free(GtkObject * expander, struct parameter_data * data)
{
	g_free(data);
}

static void
parameter_up(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	vbox;
	GtkWidget *	frame;
	GList *		parameters_frames;
	GList *		this;
	GList *		up;

	g_object_get(G_OBJECT(button), "user-data", &frame, NULL);
	vbox = gtk_widget_get_parent(frame);

	parameters_frames = gtk_container_get_children(GTK_CONTAINER(vbox));
	this = g_list_find(parameters_frames, frame);
	up = g_list_previous(this);
	if (up != NULL) {
		gtk_box_reorder_child(GTK_BOX(vbox), up->data, g_list_position(parameters_frames, this));
		geoxml_sequence_move_up(GEOXML_SEQUENCE(data->parameter));
	}

	g_list_free(parameters_frames);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_down(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	vbox;
	GtkWidget *	frame;
	GList *		parameters_frames;
	GList *		this;
	GList *		down;

	g_object_get(G_OBJECT(button), "user-data", &frame, NULL);
	vbox = gtk_widget_get_parent(frame);

	parameters_frames = gtk_container_get_children(GTK_CONTAINER(vbox));
	this = g_list_find(parameters_frames, frame);
	down = g_list_next(this);
	if (down != NULL) {
		gtk_box_reorder_child(GTK_BOX(vbox), down->data, g_list_position(parameters_frames, this));
		geoxml_sequence_move_down(GEOXML_SEQUENCE(data->parameter));
	}

	g_list_free(parameters_frames);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_remove(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	parameter_expander;

	if (confirm_action_dialog(_("Delete parameter"), _("Are you sure you want to delete this parameter?")) == FALSE)
		return;

	g_object_get(G_OBJECT(button), "user-data", &parameter_expander, NULL);

	geoxml_sequence_remove(GEOXML_SEQUENCE(data->parameter));
	gtk_widget_destroy(parameter_expander);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_type_changed(GtkComboBox * combo, struct parameter_data * data)
{
	GtkWidget *	table;

	g_object_get(G_OBJECT(combo), "user-data", &table, NULL);

	/* change its type */
	geoxml_parameter_set_type(&data->parameter, (enum GEOXML_PARAMETERTYPE)gtk_combo_box_get_active(combo));

	/* recreate UI */
	gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_general(table, data);

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_required_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_program_parameter_set_required(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	GtkWidget *	table;

	g_object_get(G_OBJECT(toggle_button), "user-data", &table, NULL);

	geoxml_program_parameter_set_be_list(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	/* rebuild ui */
	gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_general(table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_separator_changed(GtkEntry * entry, struct parameter_data * data)
{
	GtkWidget *	table;

	g_object_get(G_OBJECT(entry), "user-data", &table, NULL);

	geoxml_program_parameter_set_list_separator(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	/* rebuild specific ui */
	gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data)
{
	geoxml_parameter_set_label(data->parameter, gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data)
{
	GtkWidget *	parameter_label;

	g_object_get(G_OBJECT(entry), "user-data", &parameter_label, NULL);
	geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_default_widget_changed(struct parameter_widget * widget, struct parameter_data * data)
{
	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data)
{
	gboolean	is_directory;

	is_directory = gtk_combo_box_get_active(combo) == 0 ? FALSE : TRUE;

	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(data->widget->value_widget), is_directory);
	geoxml_program_parameter_set_file_be_directory(GEOXML_PROGRAM_PARAMETER(data->parameter), is_directory);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	min_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	min = atof(min_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	max_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	max = atof(max_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		inc;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	inc_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	inc = atof(inc_str);

	gtk_spin_button_set_increments(spin_button, inc, 0);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		digits;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	digits_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	digits = atof(digits_str);

	gtk_spin_button_set_digits(spin_button, digits);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct parameter_data * data)
{
	GtkWidget *	default_widget_hbox;

	g_object_get(G_OBJECT(enum_option_edit), "user-data", &default_widget_hbox, NULL);

	gtk_widget_destroy(data->widget->widget);
	data->widget = parameter_widget_new_enum(data->parameter, TRUE);
	parameter_widget_set_auto_submit_callback(data->widget,
		(changed_callback)parameter_default_widget_changed, data);
	gtk_widget_show(data->widget->widget);
	gtk_box_pack_start(GTK_BOX(default_widget_hbox), data->widget->widget, TRUE, TRUE, 0);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_change_exclusive(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	gboolean	active;

	active = gtk_toggle_button_get_active(toggle_button);
	if (active == FALSE)
		geoxml_parameter_reset(data->parameter, FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(data->widget->widget), active);
}

static void
parameter_group_exclusive_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	if (gtk_toggle_button_get_active(toggle_button) == TRUE) {
		if (confirm_action_dialog(_("Confirm change to an exclusive group"),
		_("By changing this group to be an exclusive one, all the paramters' values and defaults ones are going to be reset so that you can set only one of them.\nAre you sure you want to make it an exclusive group?")) == FALSE) {
			g_signal_handlers_block_matched(G_OBJECT(toggle_button),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)parameter_group_exclusive_changed,
				NULL);
			gtk_toggle_button_set_active(toggle_button, FALSE);
			g_signal_handlers_unblock_matched(G_OBJECT(toggle_button),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)parameter_group_exclusive_changed,
				NULL);
			return;
		}
	}
	geoxml_parameter_group_set_exclusive(GEOXML_PARAMETER_GROUP(data->parameter),
		gtk_toggle_button_get_active(toggle_button));
	((struct group_parameters_data *)data->parameters_data)->radio_group = NULL;

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_group_expanded_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_parameter_group_set_expand(GEOXML_PARAMETER_GROUP(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_group_multiple_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_parameter_group_set_can_instanciate(GEOXML_PARAMETER_GROUP(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_group_instances_changed(GtkSpinButton * spin_button, struct parameter_data * data)
{
	gint	i, instanciate;

	instanciate = gtk_spin_button_get_value(spin_button) -
	geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(data->parameter));
	if (instanciate == 0)
		return FALSE;

	if (instanciate > 0)
		for (i = 0; i < instanciate; ++i)
			geoxml_parameter_group_instanciate(GEOXML_PARAMETER_GROUP(data->parameter));
	else
		for (i = instanciate; i < 0; ++i)
			geoxml_parameter_group_deinstanciate(GEOXML_PARAMETER_GROUP(data->parameter));

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);

	return FALSE;
}

static void
parameter_uilabel_update(struct parameter_data * data)
{
	enum GEOXML_PARAMETERTYPE	type;
	gchar *				markup;
	GString *			uilabel;

	/* initialization*/
	uilabel = g_string_new("");
	type = geoxml_parameter_get_type(data->parameter);

	switch (type) {
	case GEOXML_PARAMETERTYPE_STRING:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("string"));
		break;
	case GEOXML_PARAMETERTYPE_INT:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("integer"));
		break;
	case GEOXML_PARAMETERTYPE_FILE:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("file"));
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("flag"));
		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("real number"));
		break;
	case GEOXML_PARAMETERTYPE_RANGE:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("range"));
		break;
	case GEOXML_PARAMETERTYPE_ENUM:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("enumeration"));
		break;
	case GEOXML_PARAMETERTYPE_GROUP:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("group"));
		break;
	default:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("unknown"));
	}

	g_string_append(uilabel, markup);
	if (type != GEOXML_PARAMETERTYPE_GROUP) {
		/* keyword */
		g_string_append(uilabel, geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(data->parameter)));
		/* default */
		g_string_append_printf(uilabel, " [%s]",
			type != GEOXML_PARAMETERTYPE_ENUM
			? geoxml_program_parameter_get_default(GEOXML_PROGRAM_PARAMETER(data->parameter))
			: gtk_combo_box_get_active_text(GTK_COMBO_BOX(data->widget->value_widget)));
	}
	/* label */
	if (strlen(geoxml_parameter_get_label(data->parameter))) {
		g_string_append(uilabel, ",   <i>");
		g_string_append(uilabel, geoxml_parameter_get_label(data->parameter));
		g_string_append(uilabel, "</i>");
	}

	gtk_label_set_markup(GTK_LABEL(data->label), uilabel->str);

	/* frees */
	g_free(markup);
	g_string_free(uilabel, TRUE);
}
