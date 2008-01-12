/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <gui/parameter.h>

#include "parameter.h"
#include "support.h"
#include "gebrme.h"
#include "program.h"
#include "interface.h"
#include "menu.h"

/* Persintant pointer of GeoXmlParameter. As
 * it may change (because of geoxml_parameter_set_type)
 * we must keep a container for it and share this container beetween signals.
 */
struct parameter_data {
	GeoXmlParameter *		parameter;

	struct parameter_widget *	widget;
	GtkWidget *			label;
};

GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, gboolean hidden)
{
	struct parameter_data *		data;

	GtkWidget *			frame;

	GtkWidget *			parameter_expander;
	GtkWidget *			parameter_label_widget;
	GtkWidget *			parameter_label;
	GtkWidget *			depth_hbox;
	GtkWidget *			parameter_vbox;
	GtkWidget *			parameter_table;

	GtkWidget *			type_label;
	GtkWidget *			type_combo;
	GtkWidget *			keyword_label;
	GtkWidget *			keyword_entry;
	GtkWidget *			label_label;
	GtkWidget *			label_entry;
	GtkWidget *			specific_table;

	GtkWidget *			widget;
	GtkWidget *			button_hbox;
	GtkWidget *			align;

	data = g_malloc(sizeof(struct parameter_data));
	data->parameter = parameter;

	frame = gtk_frame_new("");
	gtk_widget_show(frame);
	g_object_set(G_OBJECT(frame), "shadow-type", GTK_SHADOW_OUT, NULL);

	parameter_expander = gtk_expander_new("");
	gtk_container_add(GTK_CONTAINER(frame), parameter_expander);
	gtk_expander_set_expanded(GTK_EXPANDER(parameter_expander), hidden);
	gtk_widget_show(parameter_expander);
	depth_hbox = create_depth(parameter_expander);
	g_signal_connect(parameter_expander, "destroy",
		GTK_SIGNAL_FUNC (parameter_data_free),
		data);

	parameter_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), parameter_vbox, TRUE, TRUE, 0);
	gtk_widget_show(parameter_vbox);

	parameter_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(parameter_expander), parameter_label_widget);
	gtk_widget_show(parameter_label_widget);
	gtk_expander_hacked_define(parameter_expander, parameter_label_widget);
	parameter_label = gtk_label_new("");
	data->label = parameter_label;
	gtk_widget_show(parameter_label);
	gtk_box_pack_start(GTK_BOX(parameter_label_widget), parameter_label, FALSE, FALSE, 0);

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
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("string"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("integer"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("file"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("flag"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("real number"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("range"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), geoxml_parameter_get_type(parameter));
	g_signal_connect(type_combo, "changed",
			(GCallback)parameter_type_changed, data);

	/*
	 * Keyword
	 */
	keyword_label = gtk_label_new (_("Keyword:"));
	gtk_widget_show (keyword_label);
	gtk_table_attach (GTK_TABLE (parameter_table), keyword_label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (keyword_label), 0, 0.5);

	keyword_entry = gtk_entry_new ();
	gtk_widget_show (keyword_entry);
	gtk_table_attach (GTK_TABLE (parameter_table), keyword_entry, 1, 2, 1, 2,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(keyword_entry),
		geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter)));
	gtk_label_set_text(GTK_LABEL(parameter_label),
		geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter)));
	/* signal */
	g_signal_connect(keyword_entry, "changed",
			(GCallback)parameter_keyword_changed, data);
	g_object_set(G_OBJECT(keyword_entry), "user-data", parameter_label, NULL);

	/*
	 * Label
	 */
	label_label = gtk_label_new (_("Label:"));
	gtk_widget_show (label_label);
	gtk_table_attach (GTK_TABLE (parameter_table), label_label, 0, 1, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_label), 0, 0.5);

	label_entry = gtk_entry_new ();
	gtk_widget_show (label_entry);
	gtk_table_attach (GTK_TABLE (parameter_table), label_entry, 1, 2, 2, 3,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(label_entry), geoxml_parameter_get_label(parameter));
	/* signal */
	g_signal_connect(label_entry, "changed",
			(GCallback)parameter_label_changed, data);

	/*
	 * Specific parameters fields
	 */
	specific_table = gtk_table_new (5, 2, FALSE);
	gtk_widget_show (specific_table);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), specific_table, FALSE, TRUE, 5);
	gtk_table_set_row_spacings (GTK_TABLE (specific_table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (specific_table), 5);
	g_object_set(G_OBJECT(type_combo), "user-data", specific_table, NULL);

	parameter_create_ui_type_specific(specific_table, data);

	/*
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

	parameter_uilabel_update(data);

	return frame;
}

void
parameter_create_ui_type_specific(GtkWidget * table, struct parameter_data * data)
{
	GtkWidget *			default_label;
	GtkWidget *			default_widget;

	GeoXmlProgramParameter *	program_parameter;
	enum GEOXML_PARAMETERTYPE	type;

	program_parameter = GEOXML_PROGRAM_PARAMETER(data->parameter);
	type = geoxml_parameter_get_type(data->parameter);
	if (type != GEOXML_PARAMETERTYPE_FLAG) {
		GtkWidget *	required_label;
		GtkWidget *	required_checkbox;

		required_label = gtk_label_new (_("Required:"));
		gtk_widget_show (required_label);
		gtk_table_attach (GTK_TABLE (table), required_label, 0, 1, 0, 1,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (required_label), 0, 0.5);

		required_checkbox = gtk_check_button_new ();
		gtk_widget_show (required_checkbox);
		gtk_table_attach (GTK_TABLE (table), required_checkbox, 1, 2, 0, 1,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(required_checkbox),
			geoxml_program_parameter_get_required(program_parameter));
		g_signal_connect(required_checkbox, "toggled",
				(GCallback)parameter_required_changed, data);
	}

	default_label = gtk_label_new (_("Default:"));
	gtk_widget_show (default_label);
	gtk_table_attach (GTK_TABLE (table), default_label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (default_label), 0, 0.5);

	switch (type) {
	case GEOXML_PARAMETERTYPE_STRING:
		data->widget = parameter_widget_new_string(data->parameter, TRUE);
		default_widget = data->widget->widget;
		g_signal_connect(default_widget, "changed",
				(GCallback)parameter_default_changed, data);

		break;
	case GEOXML_PARAMETERTYPE_INT:
		data->widget = parameter_widget_new_int(data->parameter, TRUE);
		default_widget = data->widget->widget;
		g_signal_connect(default_widget, "changed",
				(GCallback)parameter_default_changed, data);

		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		data->widget = parameter_widget_new_float(data->parameter, TRUE);
		default_widget = data->widget->widget;
		g_signal_connect(default_widget, "changed",
				(GCallback)parameter_default_changed, data);

		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		data->widget = NULL;
		default_widget = gtk_check_button_new();
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(default_widget),
			geoxml_program_parameter_get_flag_default(program_parameter));
		g_signal_connect(default_widget, "toggled",
				(GCallback)parameter_flag_default_changed, data);

		break;
	case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		type_label;
		GtkWidget *		type_combo;

		data->widget = parameter_widget_new_file(data->parameter, TRUE);
		default_widget = data->widget->widget;
		g_signal_connect(default_widget, "path-changed",
				(GCallback)parameter_file_default_changed, data);

		type_label = gtk_label_new (_("Type:"));
		gtk_widget_show (type_label);
		gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 2, 3,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (type_label), 0, 0.5);

		type_combo = gtk_combo_box_new_text ();
		gtk_widget_show (type_combo);
		gtk_table_attach (GTK_TABLE (table), type_combo, 1, 2, 2, 3,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("File"));
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("Directory"));
		g_signal_connect(type_combo, "changed",
				(GCallback)parameter_file_type_changed, data);
		g_object_set(G_OBJECT(type_combo),
			"user-data", default_widget, NULL);

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
		gchar *		min_str;
		gchar *		max_str;
		gchar *		inc_str;

		geoxml_program_parameter_get_range_properties(program_parameter, &min_str, &max_str, &inc_str);

		data->widget = parameter_widget_new_range(data->parameter, TRUE);
		default_widget = data->widget->widget;
		g_signal_connect(default_widget, "output",
				(GCallback)parameter_range_default_changed, data);

		min_label = gtk_label_new (_("Minimum:"));
		gtk_widget_show (min_label);
		gtk_table_attach (GTK_TABLE (table), min_label, 0, 1, 2, 3,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (min_label), 0, 0.5);
		min_entry = gtk_entry_new();
		gtk_widget_show (min_entry);
		gtk_table_attach (GTK_TABLE (table), min_entry, 1, 2, 2, 3,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(min_entry), min_str);
		g_object_set(G_OBJECT(min_entry), "user-data", default_widget, NULL);
		g_signal_connect(min_entry, "changed",
				(GCallback)parameter_range_min_changed, data);

		max_label = gtk_label_new (_("Maximum:"));
		gtk_widget_show (max_label);
		gtk_table_attach (GTK_TABLE (table), max_label, 0, 1, 3, 4,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (max_label), 0, 0.5);
		max_entry = gtk_entry_new();
		gtk_widget_show (max_entry);
		gtk_table_attach (GTK_TABLE (table), max_entry, 1, 2, 3, 4,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(max_entry), max_str);
		g_object_set(G_OBJECT(max_entry), "user-data", default_widget, NULL);
		g_signal_connect(max_entry, "changed",
				(GCallback)parameter_range_max_changed, data);

		inc_label = gtk_label_new (_("Increment:"));
		gtk_widget_show (inc_label);
		gtk_table_attach (GTK_TABLE (table), inc_label, 0, 1, 4, 5,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (inc_label), 0, 0.5);
		inc_entry = gtk_entry_new();
		gtk_widget_show (inc_entry);
		gtk_table_attach (GTK_TABLE (table), inc_entry, 1, 2, 4, 5,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(inc_entry), inc_str);
		g_object_set(G_OBJECT(inc_entry), "user-data", default_widget, NULL);
		g_signal_connect(inc_entry, "changed",
				(GCallback)parameter_range_inc_changed, data);

		break;
	} default:
		return;
	}

	gtk_widget_show(default_widget);
	gtk_table_attach(GTK_TABLE (table), default_widget, 1, 2, 1, 2,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
}

void
parameter_add(GtkButton * button, GeoXmlProgram * program)
{
	GtkWidget *		parameter_widget;
	GtkWidget *		parameters_vbox;

	GeoXmlParameter *	parameter;

	g_object_get(G_OBJECT(button), "user-data", &parameters_vbox, NULL);

	parameter = geoxml_parameters_append_parameter(geoxml_program_get_parameters(program), GEOXML_PARAMETERTYPE_STRING);
	parameter_widget = parameter_create_ui(parameter, TRUE);
	gtk_box_pack_start(GTK_BOX(parameters_vbox), parameter_widget, FALSE, TRUE, 0);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
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

void
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

void
parameter_remove(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	parameter_expander;

	g_object_get(G_OBJECT(button), "user-data", &parameter_expander, NULL);

	geoxml_sequence_remove(GEOXML_SEQUENCE(data->parameter));
	gtk_widget_destroy(parameter_expander);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_data_free(GtkObject * expander, struct parameter_data * data)
{
	g_free(data);
}

void
parameter_type_changed(GtkComboBox * combo, struct parameter_data * data)
{
	GtkWidget *	table;

	g_object_get(G_OBJECT(combo), "user-data", &table, NULL);

	/* clear specific data */
	gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback)gtk_widget_destroy, NULL);

	/* change its type and recreate UI */
	geoxml_parameter_set_type(&data->parameter, (enum GEOXML_PARAMETERTYPE)gtk_combo_box_get_active(combo));
	if (data->parameter == NULL)
		puts("parameter_type_changed err");
	parameter_create_ui_type_specific(table, data);

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_required_changed(GtkToggleButton * togglebutton, struct parameter_data * data)
{
	geoxml_program_parameter_set_required(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data)
{
	GtkWidget *	parameter_label;

	g_object_get(G_OBJECT(entry), "user-data", &parameter_label, NULL);
	geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data)
{
	geoxml_parameter_set_label(data->parameter, gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_default_changed(GtkEntry * entry, struct parameter_data * data)
{
	geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data)
{
	GtkWidget *	file_entry;
	gboolean	is_directory;

	is_directory = !gtk_combo_box_get_active(combo) ? FALSE : TRUE;

	g_object_get(G_OBJECT(combo), "user-data", &file_entry, NULL);
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry), is_directory);

	geoxml_program_parameter_set_file_be_directory(GEOXML_PROGRAM_PARAMETER(data->parameter), is_directory);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_flag_default_changed(GtkToggleButton * togglebutton, struct parameter_data * data)
{
	geoxml_program_parameter_set_flag_default(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(togglebutton));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_file_default_changed(GtkFileEntry * file_entry, struct parameter_data * data)
{
	geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_file_entry_get_path(file_entry));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

gboolean
parameter_range_default_changed(GtkSpinButton * spinbutton, struct parameter_data * data)
{
	gchar *		str;

	str = g_strdup_printf("%lf", gtk_spin_button_get_value(spinbutton));
	geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(data->parameter), str);

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	g_free(str);
	return FALSE;
}

void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gdouble		min, max;
	GtkWidget *	default_widget;

	g_object_get(G_OBJECT(entry), "user-data", &default_widget, NULL);

	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str);
	gtk_spin_button_get_range(GTK_SPIN_BUTTON(default_widget), &min, &max);

	min_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	min = atof(min_str);

	gtk_spin_button_set_range(GTK_SPIN_BUTTON(default_widget), min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gdouble		min, max;
	GtkWidget *	default_widget;

	g_object_get(G_OBJECT(entry), "user-data", &default_widget, NULL);

	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str);
	gtk_spin_button_get_range(GTK_SPIN_BUTTON(default_widget), &min, &max);

	max_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	max = atof(max_str);

	gtk_spin_button_set_range(GTK_SPIN_BUTTON(default_widget), min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gdouble		inc;
	GtkWidget *	default_widget;

	g_object_get(G_OBJECT(entry), "user-data", &default_widget, NULL);

	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str);

	inc_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	inc = atof(inc_str);

	gtk_spin_button_set_increments(GTK_SPIN_BUTTON(default_widget), inc, 0);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
parameter_uilabel_update(struct parameter_data * data)
{
	gchar *		markup;
	GString *	uilabel;

	/* initialization*/
	uilabel = g_string_new(NULL);

	switch (geoxml_parameter_get_type(data->parameter)) {
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
	default:
		markup = g_markup_printf_escaped("<i>%s</i> ", _("unknown"));
	}

	g_string_assign(uilabel, markup);
	/* keyword */
	g_string_append(uilabel, geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(data->parameter)));
	/* default */
	g_string_append(uilabel, " [");
	g_string_append(uilabel, geoxml_program_parameter_get_default(GEOXML_PROGRAM_PARAMETER(data->parameter)));
	g_string_append(uilabel, "]");
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
