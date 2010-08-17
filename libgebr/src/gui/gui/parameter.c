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

#include <string.h>
#include <stdlib.h>

#include "../../intl.h"
#include "../../utils.h"

#include "parameter.h"
#include "utils.h"

#define DOUBLE_MAX +999999999
#define DOUBLE_MIN -999999999

/*
 * Prototypes
 */

static void gebr_gui_parameter_widget_find_dict_parameter(struct gebr_gui_parameter_widget *widget);

static GtkWidget *gebr_gui_parameter_widget_dict_popup_menu(struct gebr_gui_parameter_widget *widget);

static void
on_dict_clicked(GtkEntry * entry, GtkEntryIconPosition icon_pos, GdkEventButton * event,
		struct gebr_gui_parameter_widget *widget);

static void
gebr_gui_parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
							struct gebr_gui_parameter_widget *widget);

static gboolean gebr_gui_parameter_widget_can_use_dict(struct gebr_gui_parameter_widget *widget);

static void on_dict_parameter_toggled(GtkMenuItem * menu_item, struct gebr_gui_parameter_widget *widget);

static gboolean on_list_widget_mnemonic_activate(GtkBox * box, gboolean cycle,
						 struct gebr_gui_parameter_widget *widget);

/*
 * Internal stuff
 */

/**
 * \internal
 */
static void
enum_value_to_label_set(GebrGeoXmlSequence * sequence, const gchar * label,
			struct gebr_gui_parameter_widget *parameter_widget)
{
	GebrGeoXmlSequence *enum_option;

	gebr_geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter, &enum_option, 0);
	for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option))
		if (!strcmp(label, gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)))) {
			gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(sequence),
						       gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION
											 (enum_option)));
			return;
		}
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(sequence), "");
}

/**
 * \internal
 */
static const gchar *enum_value_to_label_get(GebrGeoXmlSequence * sequence,
					    struct gebr_gui_parameter_widget *parameter_widget)
{
	GebrGeoXmlSequence *enum_option;

	gebr_geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter, &enum_option, 0);
	for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option))
		if (!strcmp(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(sequence)),
			    gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option))))
			return gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option));
	return gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(sequence));
}

/**
 * \internal
 */
static void
gebr_gui_parameter_widget_file_entry_customize_function(GtkFileChooser * file_chooser,
							struct gebr_gui_parameter_widget *parameter_widget)
{
	const gchar *filter_name;
	const gchar *filter_pattern;

	gebr_geoxml_program_parameter_get_file_filter(parameter_widget->program_parameter,
						      &filter_name, &filter_pattern);
	if (filter_name != NULL && strlen(filter_name)
	    && filter_pattern != NULL && strlen(filter_pattern)) {
		GString *name;
		GtkFileFilter *file_filter;
		gchar **patterns;

		file_filter = gtk_file_filter_new();
		name = g_string_new(NULL);
		patterns = g_strsplit_set(filter_pattern, " ,", -1);

		g_string_printf(name, "%s (%s)", filter_name, filter_pattern);
		gtk_file_filter_set_name(file_filter, name->str);
		for (int i = 0; patterns[i] != NULL; ++i)
			gtk_file_filter_add_pattern(file_filter, patterns[i]);

		gtk_file_chooser_add_filter(file_chooser, file_filter);
		file_filter = gtk_file_filter_new();
		gtk_file_filter_set_name(file_filter, _("All"));
		gtk_file_filter_add_pattern(file_filter, "*");
		gtk_file_chooser_add_filter(file_chooser, file_filter);

		g_string_free(name, TRUE);
		g_strfreev(patterns);
	}

	if (parameter_widget->data != NULL)
		((GebrGuiGtkFileEntryCustomize) parameter_widget->data) (file_chooser, NULL);
}

/**
 * \internal
 */
static void
gebr_gui_parameter_widget_set_non_list_widget_value(struct gebr_gui_parameter_widget *parameter_widget,
						    const gchar * value)
{
	if (parameter_widget->dict_parameter != NULL) {
		GString *value;

		value = g_string_new(NULL);

		g_string_printf(value, "%s=%s",
				gebr_geoxml_program_parameter_get_keyword(parameter_widget->dict_parameter),
				gebr_geoxml_program_parameter_get_first_value(parameter_widget->dict_parameter,
									      parameter_widget->
									      use_default_value));
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value->str);
		gtk_editable_set_editable(GTK_EDITABLE(parameter_widget->value_widget), FALSE);

		g_string_free(value, TRUE);

		return;
	}

	if (gebr_gui_parameter_widget_can_use_dict(parameter_widget))
		gtk_editable_set_editable(GTK_EDITABLE(parameter_widget->value_widget), TRUE);
	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		if (!strlen(value))
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget), 0);
		else {
			gchar *endptr;
			double number_value;

			number_value = strtod(value, &endptr);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget),
						  (endptr != value) ? number_value : 0);
		}
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(parameter_widget->value_widget),
						 value);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM: {
			GebrGeoXmlSequence *option;
			int i;

			gebr_geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter,
								      &option, 0);
			for (i = 0; option != NULL; ++i, gebr_geoxml_sequence_next(&option))
				if (strcmp(value, gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(option))) ==
				    0) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget),
								 gebr_geoxml_program_parameter_get_required
								 (parameter_widget->program_parameter) ? i : i
								 + 1);
					return;
				}

			gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget), 0);
			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget),
					     !strcmp(value, "on"));
		break;
	default:
		break;
	}
}

/**
 * \internal
 */
static GString *gebr_gui_parameter_widget_get_widget_value_full(struct gebr_gui_parameter_widget *parameter_widget,
								gboolean check_list)
{
	GString *value;

	value = g_string_new(NULL);

	if (parameter_widget->dict_parameter != NULL) {
		g_string_assign(value,
				gebr_geoxml_program_parameter_get_first_value(parameter_widget->dict_parameter,
									      parameter_widget->
									      use_default_value));
		return value;
	}
	if (check_list && gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter)) {
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->list_value_widget)));
		return value;
	}

	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->value_widget)));
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:{
			guint digits;
			digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(parameter_widget->value_widget));
			if (digits == 0)
				g_string_printf(value, "%d",
						gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
										 (parameter_widget->
										  value_widget)));
			else {
				GString *mask;

				mask = g_string_new(NULL);
				g_string_printf(mask, "%%.%if", digits);
				g_string_printf(value, mask->str,
						gtk_spin_button_get_value(GTK_SPIN_BUTTON
									  (parameter_widget->value_widget)));

				g_string_free(mask, TRUE);
			}
			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		g_string_assign(value,
				gebr_gui_gtk_file_entry_get_path(GEBR_GUI_GTK_FILE_ENTRY
								 (parameter_widget->value_widget)));
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM:{
			gint index;

			index = gtk_combo_box_get_active(GTK_COMBO_BOX(parameter_widget->value_widget));
			if (index == -1)
				g_string_assign(value, "");
			else {
				GebrGeoXmlSequence *enum_option;

				/* minus one to skip the first empty value */
				gebr_geoxml_program_parameter_get_enum_option
				    (parameter_widget->program_parameter, &enum_option,
				     gebr_geoxml_program_parameter_get_required
				     (parameter_widget->program_parameter) ? index : index - 1);
				g_string_assign(value, (enum_option == NULL) ? "" :
						gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION
										  (enum_option)));
			}

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
		g_string_assign(value,
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget))
				== TRUE ? "on" : "off");
		break;
	default:
		break;
	}

	return value;
}

/**
 * \internal
 */
static void gebr_gui_parameter_widget_report_change(struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->callback != NULL)
		parameter_widget->callback(parameter_widget, parameter_widget->user_data);
}

/**
 * \internal
 * Update XML when user is manually editing the list
 */
static void
gebr_gui_parameter_on_list_value_widget_changed(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_geoxml_program_parameter_set_parse_list_value(parameter_widget->program_parameter,
							   parameter_widget->use_default_value,
							   gtk_entry_get_text(entry));

	gebr_gui_parameter_widget_report_change(parameter_widget);
}

/**
 * \internal
 * Syncronize input widget value with the parameter
 */
static void gebr_gui_parameter_widget_sync_non_list(struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;

	value = gebr_gui_parameter_widget_get_widget_value_full(parameter_widget, FALSE);
	gebr_geoxml_program_parameter_set_first_value(parameter_widget->program_parameter,
						      parameter_widget->use_default_value, value->str);

	g_string_free(value, TRUE);
}

/**
 * \internal
 */
static void
gebr_gui_parameter_widget_on_value_widget_changed(GtkWidget * widget,
						  struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	gebr_gui_parameter_widget_sync_non_list(parameter_widget);
	gebr_gui_parameter_widget_report_change(parameter_widget);
}

/**
 * \internal
 */
static gboolean
gebr_gui_parameter_widget_on_range_changed(GtkSpinButton * spinbutton,
					   struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_on_value_widget_changed(GTK_WIDGET(spinbutton), parameter_widget);
	return FALSE;
}

/**
 * \internal
 * Free struct before widget deletion
 */
static void
gebr_gui_parameter_widget_weak_ref(struct gebr_gui_parameter_widget *parameter_widget, GtkWidget * widget)
{
	g_free(parameter_widget);
}

static void
gebr_gui_parameter_on_list_value_widget_changed(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget);

/**
 * \internal
 * Update from the XML
 */
static void __parameter_list_value_widget_update(struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;

	value = gebr_geoxml_program_parameter_get_string_value(parameter_widget->program_parameter,
							       parameter_widget->use_default_value);

	g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
					G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed), NULL);
	gtk_entry_set_text(GTK_ENTRY(parameter_widget->list_value_widget), value->str);
	g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
					  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					  G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed), NULL);

	g_string_free(value, TRUE);
}

static void
__on_sequence_edit_changed(GtkSequenceEdit * sequence_edit,
			   struct gebr_gui_parameter_widget *parameter_widget);
static void gebr_gui_parameter_widget_on_value_widget_changed(GtkWidget * widget, struct gebr_gui_parameter_widget
							      *parameter_widget);

/**
 * \internal
 * Take action to start/finish editing list of parameter's values.
 */
static void
__on_edit_list_toggled(GtkToggleButton * toggle_button, struct gebr_gui_parameter_widget *parameter_widget)
{
	gboolean toggled;

	toggled = gtk_toggle_button_get_active(toggle_button);
	if (toggled == TRUE) {
		GebrGeoXmlSequence *first_value;

		gebr_geoxml_program_parameter_get_value(parameter_widget->program_parameter,
							parameter_widget->use_default_value, &first_value, 0);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM) {
			g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
							G_SIGNAL_MATCH_FUNC,
							0, 0, NULL,
							G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed), NULL);
			g_signal_handlers_block_matched(G_OBJECT
							(parameter_widget->gebr_gui_value_sequence_edit),
							G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
							G_CALLBACK(__on_sequence_edit_changed), NULL);
			gebr_gui_value_sequence_edit_load(parameter_widget->gebr_gui_value_sequence_edit,
							  first_value,
							  (ValueSequenceSetFunction) gebr_geoxml_value_sequence_set,
							  (ValueSequenceGetFunction) gebr_geoxml_value_sequence_get,
							  NULL);
			g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
							  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
							  G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed), NULL);
			g_signal_handlers_unblock_matched(G_OBJECT
							  (parameter_widget->gebr_gui_value_sequence_edit),
							  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
							  G_CALLBACK(__on_sequence_edit_changed), NULL);

			gtk_widget_show(GTK_WIDGET(parameter_widget->gebr_gui_value_sequence_edit));
		} else {
			gebr_gui_value_sequence_edit_load(parameter_widget->gebr_gui_value_sequence_edit,
							  first_value,
							  (ValueSequenceSetFunction) enum_value_to_label_set,
							  (ValueSequenceGetFunction) enum_value_to_label_get,
							  parameter_widget);
		}
	} else if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		gtk_widget_hide(GTK_WIDGET(parameter_widget->gebr_gui_value_sequence_edit));

	gtk_widget_set_sensitive(parameter_widget->list_value_widget, !toggled);
}

/**
 * \internal
 */
static void
__on_sequence_edit_add_request(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
			       struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;
	GebrGeoXmlSequence *sequence;
	GtkListStore *list_store;

	g_object_get(gebr_gui_value_sequence_edit, "list-store", &list_store, NULL);
	value = gebr_gui_parameter_widget_get_widget_value_full(parameter_widget, FALSE);
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL) == 0) {
		GebrGeoXmlSequence *first_sequence;

		gebr_geoxml_program_parameter_get_value(parameter_widget->program_parameter,
							parameter_widget->use_default_value, &first_sequence,
							0);
		sequence = first_sequence;
	} else
		sequence =
		    GEBR_GEOXML_SEQUENCE(gebr_geoxml_program_parameter_append_value
					 (parameter_widget->program_parameter,
					  parameter_widget->use_default_value));

	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(sequence), value->str);
	gebr_gui_value_sequence_edit_add(gebr_gui_value_sequence_edit, sequence);

	__parameter_list_value_widget_update(parameter_widget);
	gebr_gui_parameter_widget_set_non_list_widget_value(parameter_widget, "");

	g_string_free(value, TRUE);
}

/**
 * \internal
 */
static void
__on_sequence_edit_changed(GtkSequenceEdit * sequence_edit, struct gebr_gui_parameter_widget *parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

/**
 * \internal
 * Validate an int parameter
 */
static void __validate_int(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	const gchar *min, *max;

	gebr_geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
	gtk_entry_set_text(entry, gebr_validate_int(gtk_entry_get_text(entry), min, max));
}

/**
 * \internal
 * Validate a float parameter
 */
static void __validate_float(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	const gchar *min, *max;

	gebr_geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
	gtk_entry_set_text(entry, gebr_validate_float(gtk_entry_get_text(entry), min, max));
}

/**
 * \internal
 * Call a validation function
 */
static gboolean
__validate_on_leaving(GtkWidget * widget, GdkEventFocus * event,
		      struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_validate(parameter_widget);
	return FALSE;
}

/**
 * \internal
 * Create UI.
 */
static void gebr_gui_parameter_widget_configure(struct gebr_gui_parameter_widget *parameter_widget)
{
	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:{
			GtkWidget *entry;

			parameter_widget->value_widget = entry = gtk_entry_new();
			gtk_widget_set_size_request(entry, 90, 30);

			g_signal_connect(entry, "activate", G_CALLBACK(__validate_float), parameter_widget);
			g_signal_connect(entry, "focus-out-event",
					 G_CALLBACK(__validate_on_leaving), parameter_widget);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_INT:{
			GtkWidget *entry;

			parameter_widget->value_widget = entry = gtk_entry_new();
			gtk_widget_set_size_request(entry, 90, 30);

			g_signal_connect(entry, "activate", G_CALLBACK(__validate_int), parameter_widget);
			g_signal_connect(entry, "focus-out-event",
					 G_CALLBACK(__validate_on_leaving), parameter_widget);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:{
			parameter_widget->value_widget = gtk_entry_new();
			gtk_widget_set_size_request(parameter_widget->value_widget, 140, 30);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:{
			GtkWidget *spin;

			const gchar *min_str;
			const gchar *max_str;
			const gchar *inc_str;
			const gchar *digits_str;
			double min, max, inc;

			gebr_geoxml_program_parameter_get_range_properties(parameter_widget->program_parameter,
									   &min_str, &max_str, &inc_str, &digits_str);
			min = !strlen(min_str) ? DOUBLE_MIN : atof(min_str);
			max = !strlen(max_str) ? DOUBLE_MAX : atof(max_str);
			inc = !strlen(inc_str) ? 1.0 : atof(inc_str);
			if (inc == 0)
				inc = 1.0;

			parameter_widget->value_widget = spin = gtk_spin_button_new_with_range(min, max, inc);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), atoi(digits_str));
			gtk_widget_set_size_request(spin, 90, 30);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:{
			GtkWidget *file_entry;

			/* file entry */
			parameter_widget->value_widget = file_entry =
			    gebr_gui_gtk_file_entry_new((GebrGuiGtkFileEntryCustomize)
							gebr_gui_parameter_widget_file_entry_customize_function,
							parameter_widget);
			gtk_widget_set_size_request(file_entry, 220, 30);

			gebr_gui_gtk_file_entry_set_choose_directory(GEBR_GUI_GTK_FILE_ENTRY(file_entry),
								     gebr_geoxml_program_parameter_get_file_be_directory
								     (parameter_widget->program_parameter));
			gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(file_entry),
									      FALSE);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM: {
			GtkWidget *combo_box;
			GebrGeoXmlSequence *enum_option;

			parameter_widget->value_widget = combo_box = gtk_combo_box_new_text();
			if (!gebr_geoxml_program_parameter_get_required(parameter_widget->program_parameter))
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), "");
			gebr_geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter,
								      &enum_option, 0);
			for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option)) {
				const gchar *text;

				text = strlen(gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)))
				    ? gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option))
				    : gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option));
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text);
			}

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG: {
			parameter_widget->value_widget = gtk_check_button_new();
			gtk_button_set_use_underline(GTK_BUTTON(parameter_widget->value_widget), TRUE);
			break;
		}
	default:
		return;
	}
	if (parameter_widget->readonly)
		gtk_widget_set_sensitive(parameter_widget->value_widget, FALSE);

	if (gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter) == TRUE) {
		GtkWidget *vbox;
		GtkWidget *hbox;
		GtkWidget *button;
		GtkWidget *sequence_edit;

		vbox = gtk_vbox_new(FALSE, 10);
		hbox = gtk_hbox_new(FALSE, 10);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
			gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

		parameter_widget->list_value_widget = gtk_entry_new();
		if (parameter_widget->readonly)
			gtk_widget_set_sensitive(parameter_widget->list_value_widget, FALSE);
		gtk_widget_show(parameter_widget->list_value_widget);
		gtk_box_pack_start(GTK_BOX(hbox), parameter_widget->list_value_widget, TRUE, TRUE, 0);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
			g_signal_connect(parameter_widget->list_value_widget, "changed",
					 G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed),
					 parameter_widget);

		button = gtk_toggle_button_new_with_label(_("Edit list"));
		if (parameter_widget->readonly)
			gtk_widget_set_sensitive(button, FALSE);
		gtk_widget_set_sensitive(parameter_widget->value_widget, TRUE);
		gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
		g_signal_connect(button, "toggled", G_CALLBACK(__on_edit_list_toggled), parameter_widget);

		gebr_gui_parameter_widget_set_non_list_widget_value(parameter_widget, "");
		gtk_widget_show(parameter_widget->value_widget);

		sequence_edit = gebr_gui_value_sequence_edit_new(parameter_widget->value_widget);
		gtk_box_pack_start(GTK_BOX(vbox), sequence_edit, TRUE, TRUE, 0);
		parameter_widget->gebr_gui_value_sequence_edit = GEBR_GUI_VALUE_SEQUENCE_EDIT(sequence_edit);
		g_object_set(G_OBJECT(sequence_edit), "minimum-one", TRUE, NULL);
		g_signal_connect(sequence_edit, "add-request",
				 G_CALLBACK(__on_sequence_edit_add_request), parameter_widget);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
			g_signal_connect(sequence_edit, "changed",
					 G_CALLBACK(__on_sequence_edit_changed), parameter_widget);
		else {
			gtk_widget_show(sequence_edit);
			g_object_set(sequence_edit, "may-rename", FALSE, NULL);
			gtk_button_clicked(GTK_BUTTON(button));
			gtk_widget_hide(hbox);
		}

		parameter_widget->widget = vbox;
		g_signal_connect(vbox, "mnemonic-activate",
				 G_CALLBACK(on_list_widget_mnemonic_activate), parameter_widget);
	} else {
		parameter_widget->widget = parameter_widget->value_widget;
		parameter_widget->gebr_gui_value_sequence_edit = NULL;

		switch (parameter_widget->parameter_type) {
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
		case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		case GEBR_GEOXML_PARAMETER_TYPE_ENUM:
			g_signal_connect(parameter_widget->value_widget, "changed",
					 G_CALLBACK(gebr_gui_parameter_widget_on_value_widget_changed),
					 parameter_widget);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			g_signal_connect(parameter_widget->value_widget, "output",
					 G_CALLBACK(gebr_gui_parameter_widget_on_range_changed),
					 parameter_widget);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FILE:
			g_signal_connect(parameter_widget->value_widget, "path-changed",
					 G_CALLBACK(gebr_gui_parameter_widget_on_value_widget_changed),
					 parameter_widget);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
			g_signal_connect(parameter_widget->value_widget, "toggled",
					 G_CALLBACK(gebr_gui_parameter_widget_on_value_widget_changed),
					 parameter_widget);
			break;
		default:
			break;
		}
	}
	if (parameter_widget->dicts != NULL
	    && gebr_gui_parameter_widget_can_use_dict(parameter_widget)) {
		GebrGeoXmlDocument *documents[] = {
			parameter_widget->dicts->project, parameter_widget->dicts->line,
			parameter_widget->dicts->flow, NULL
		};

		parameter_widget->dict_parameter = NULL;
		for (int i = 0; documents[i] != NULL; i++) {
			GebrGeoXmlProgramParameter *dict_parameter;

			dict_parameter =
			    gebr_geoxml_program_parameter_find_dict_parameter
			    (parameter_widget->program_parameter, documents[i]);
			if (dict_parameter != NULL)
				parameter_widget->dict_parameter = dict_parameter;
		}

		gebr_gui_parameter_widget_find_dict_parameter(parameter_widget);
		g_signal_connect(parameter_widget->value_widget, "populate-popup",
				 G_CALLBACK(gebr_gui_parameter_widget_value_entry_on_populate_popup),
				 parameter_widget);
	}

	gebr_gui_parameter_widget_update(parameter_widget);
	gebr_gui_parameter_widget_set_auto_submit_callback(parameter_widget,
							   parameter_widget->callback,
							   parameter_widget->user_data);

	/* delete struct */
	g_object_weak_ref(G_OBJECT(parameter_widget->widget), (GWeakNotify) gebr_gui_parameter_widget_weak_ref,
			  parameter_widget);
}

/**
 * Find in documents' dictionaries for the associated dictionary parameter
 */
static void gebr_gui_parameter_widget_find_dict_parameter(struct gebr_gui_parameter_widget *widget)
{
	g_signal_handlers_disconnect_matched(G_OBJECT(widget->value_widget),
					     G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(on_dict_clicked), NULL);
	if (widget->dict_parameter != NULL) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(widget->value_widget),
					      GTK_ENTRY_ICON_SECONDARY, "accessories-dictionary");
		g_signal_connect(widget->value_widget, "icon-press", G_CALLBACK(on_dict_clicked), widget);
	} else {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(widget->value_widget), GTK_ENTRY_ICON_SECONDARY, NULL);
	}
}

/**
 * For sorting parameters by keyword
 */
static gint
compare_parameters_by_keyword(GebrGeoXmlProgramParameter * parameter1, GebrGeoXmlProgramParameter * parameter2)
{
	return strcmp(gebr_geoxml_program_parameter_get_keyword(parameter1),
		      gebr_geoxml_program_parameter_get_keyword(parameter2));
}

/**
 * Read dictionaries and build a popup menu
 */
static GtkWidget *gebr_gui_parameter_widget_dict_popup_menu(struct gebr_gui_parameter_widget *widget)
{
	GebrGeoXmlParameterType compatibles_types[5] = {
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN, GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN, GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN
	};

	GList *compatible_parameters, *cp;
	GebrGeoXmlDocument *documents[4] = {
		widget->dicts->project, widget->dicts->line,
		widget->dicts->flow, NULL
	};

	GtkWidget *menu;
	GtkWidget *menu_item;
	GSList *group;

	compatibles_types[0] = widget->parameter_type;
	switch (widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		compatibles_types[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		compatibles_types[2] = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
		compatibles_types[3] = GEBR_GEOXML_PARAMETER_TYPE_RANGE;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		compatibles_types[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		compatibles_types[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		compatibles_types[2] = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
		break;
	default:
		break;
	}

	compatible_parameters = NULL;
	for (int i = 0; documents[i] != NULL; i++) {
		GebrGeoXmlSequence *dict_parameter;
		GebrGeoXmlParameterType dict_parameter_type;

		dict_parameter =
		    gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters(documents[i]));
		for (; dict_parameter != NULL; gebr_geoxml_sequence_next(&dict_parameter)) {
			gboolean compatible;
			const gchar *keyword;

			compatible = FALSE;
			dict_parameter_type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(dict_parameter));
			for (int j = 0; compatibles_types[j] != GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN; j++) {
				if (compatibles_types[j] == dict_parameter_type) {
					compatible = TRUE;
					break;
				}
			}
			if (!compatible)
				continue;

			keyword =
			    gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(dict_parameter));
			for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp))
				if (!strcmp
				    (keyword,
				     gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER
									       (cp->data))))
					compatible_parameters = g_list_remove_link(compatible_parameters, cp);

			compatible_parameters = g_list_prepend(compatible_parameters, dict_parameter);
		}
	}

	menu = gtk_menu_new();

	menu_item = gtk_radio_menu_item_new_with_label(NULL, _("Do not use dictionary"));
	g_object_set(menu_item, "user-data", NULL, NULL);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_dict_parameter_toggled), widget);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	compatible_parameters = g_list_sort(compatible_parameters, (GCompareFunc) compare_parameters_by_keyword);
	for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp)) {
		GString *label;

		label = g_string_new(NULL);

		g_string_printf(label, "%s=%s (%s)",
				gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data)),
				gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data),
									      FALSE),
				gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(cp->data)));
		menu_item = gtk_radio_menu_item_new_with_label(group, label->str);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
		g_object_set(menu_item, "user-data", cp->data, NULL);
		g_signal_connect(menu_item, "toggled", G_CALLBACK(on_dict_parameter_toggled), widget);
		gtk_container_add(GTK_CONTAINER(menu), menu_item);

		if ((void *)widget->dict_parameter == cp->data)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);

		g_string_free(label, TRUE);
	}

	gtk_widget_show_all(menu);
	g_list_free(compatible_parameters);

	return menu;
}

/**
 * Popup menu upon dictionary icon click
 */
static void
on_dict_clicked(GtkEntry * entry, GtkEntryIconPosition icon_pos, GdkEventButton * event,
		struct gebr_gui_parameter_widget *widget)
{
	gtk_menu_popup(GTK_MENU(gebr_gui_parameter_widget_dict_popup_menu(widget)), NULL, NULL, NULL, NULL,
		       event->button, event->time);
}

/**
 * Add dictionary submenu into entry popup menu
 */
static void
gebr_gui_parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
							struct gebr_gui_parameter_widget *widget)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Dictionary"));
	gtk_widget_show(menu_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), gebr_gui_parameter_widget_dict_popup_menu(widget));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
}

/**
 * Return TRUE if parameter is of an compatible type to use an dictionary value
 */
static gboolean gebr_gui_parameter_widget_can_use_dict(struct gebr_gui_parameter_widget *widget)
{
	switch (gebr_geoxml_parameter_get_type(widget->parameter)) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		return TRUE;
	default:
		return FALSE;
	}
}

/**
 * Use value of dictionary parameter corresponding to menu_item in parameter at _widget_
 */
static void on_dict_parameter_toggled(GtkMenuItem * menu_item, struct gebr_gui_parameter_widget *widget)
{
	GebrGeoXmlProgramParameter *dict_parameter;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
		return;

	g_object_get(menu_item, "user-data", &dict_parameter, NULL);

	gebr_geoxml_program_parameter_set_value_from_dict(widget->program_parameter, dict_parameter);
	widget->dict_parameter = dict_parameter;
	gebr_gui_parameter_widget_find_dict_parameter(widget);

	gebr_gui_parameter_widget_update(widget);
}

/**
 * \internal
 */
static gboolean on_list_widget_mnemonic_activate(GtkBox * box, gboolean cycle, struct gebr_gui_parameter_widget *widget)
{
	gboolean sensitive;
	g_object_get(G_OBJECT(widget->list_value_widget), "sensitive", &sensitive, NULL);
	gtk_widget_mnemonic_activate(sensitive? widget->list_value_widget:
				     GTK_WIDGET(widget->gebr_gui_value_sequence_edit), cycle);
	return TRUE;
}

/*
 * Public functions
 */

struct gebr_gui_parameter_widget *gebr_gui_parameter_widget_new(GebrGeoXmlParameter * parameter,
								gboolean use_default_value, gpointer data)
{
	struct gebr_gui_parameter_widget *parameter_widget;

	parameter_widget = g_new(struct gebr_gui_parameter_widget, 1);
	parameter_widget->parameter = parameter;
	parameter_widget->program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	parameter_widget->parameter_type = gebr_geoxml_parameter_get_type(parameter);
	parameter_widget->use_default_value = use_default_value;
	parameter_widget->readonly = FALSE;
	parameter_widget->data = data;
	parameter_widget->dict_parameter = NULL;
	parameter_widget->dicts = NULL;
	parameter_widget->callback = NULL;
	parameter_widget->user_data = NULL;

	gebr_gui_parameter_widget_configure(parameter_widget);

	return parameter_widget;
}

void
gebr_gui_parameter_widget_set_dicts(struct gebr_gui_parameter_widget *parameter_widget,
				    struct gebr_gui_gebr_gui_program_edit_dicts *dicts)
{
	parameter_widget->dicts = dicts;
	gebr_gui_parameter_widget_reconfigure(parameter_widget);
}

GString *parameter_widget_get_widget_value(struct gebr_gui_parameter_widget *parameter_widget)
{
	return gebr_gui_parameter_widget_get_widget_value_full(parameter_widget, TRUE);
}

void
gebr_gui_parameter_widget_set_auto_submit_callback(struct gebr_gui_parameter_widget *parameter_widget,
						   changed_callback callback, gpointer user_data)
{
	parameter_widget->callback = callback;
	parameter_widget->user_data = user_data;
}

void gebr_gui_parameter_widget_set_readonly(struct gebr_gui_parameter_widget *parameter_widget, gboolean readonly)
{
	parameter_widget->readonly = readonly;
	gebr_gui_parameter_widget_reconfigure(parameter_widget);
}

void gebr_gui_parameter_widget_update(struct gebr_gui_parameter_widget *parameter_widget)
{
	if (gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter) == TRUE)
		__parameter_list_value_widget_update(parameter_widget);
	else
		gebr_gui_parameter_widget_set_non_list_widget_value(parameter_widget,
								    gebr_geoxml_program_parameter_get_first_value
								    (parameter_widget->program_parameter,
								     parameter_widget->use_default_value));
}

void gebr_gui_parameter_widget_validate(struct gebr_gui_parameter_widget *parameter_widget)
{
	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		__validate_int(GTK_ENTRY(parameter_widget->value_widget), parameter_widget);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		__validate_float(GTK_ENTRY(parameter_widget->value_widget), parameter_widget);
		break;
	default:
		break;
	}
}

void gebr_gui_parameter_widget_update_list_separator(struct gebr_gui_parameter_widget *parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

void gebr_gui_parameter_widget_reconfigure(struct gebr_gui_parameter_widget *parameter_widget)
{
	parameter_widget->parameter_type =
	    gebr_geoxml_parameter_get_type(parameter_widget->parameter);

	g_object_weak_unref(G_OBJECT(parameter_widget->widget),
			    (GWeakNotify) gebr_gui_parameter_widget_weak_ref, parameter_widget);
	gtk_widget_destroy(parameter_widget->widget);

	gebr_gui_parameter_widget_configure(parameter_widget);
}

