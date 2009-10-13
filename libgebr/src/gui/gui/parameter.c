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

static void
parameter_widget_find_dict_parameter(struct parameter_widget * widget);
static GtkWidget *
parameter_widget_dict_popup_menu(struct parameter_widget * widget);
static void
on_dict_clicked(GtkEntry * entry, GtkEntryIconPosition icon_pos, GdkEventButton * event,
struct parameter_widget * widget);
static void
parameter_widget_value_entry_on_populate_popup(GtkEntry *entry, GtkMenu  *menu,
struct parameter_widget * widget);
static gboolean
parameter_widget_can_use_dict(struct parameter_widget * widget);
static void
on_dict_parameter_toggled(GtkMenuItem * menu_item, struct parameter_widget * widget);

/*
 * Section: Private
 * Internal stuff
 */

static void
enum_value_to_label_set(GeoXmlSequence * sequence, const gchar * label, struct parameter_widget * parameter_widget)
{
	GeoXmlSequence *	enum_option;

	geoxml_program_parameter_get_enum_option(
		parameter_widget->program_parameter, &enum_option, 0);
	for (; enum_option != NULL; geoxml_sequence_next(&enum_option))
		if (!strcmp(label, geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(enum_option)))) {
			geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(sequence),
				geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(enum_option)));
			return;
		}
	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(sequence), "");
}

static const gchar *
enum_value_to_label_get(GeoXmlSequence * sequence, struct parameter_widget * parameter_widget)
{
	GeoXmlSequence *	enum_option;

	geoxml_program_parameter_get_enum_option(
		parameter_widget->program_parameter, &enum_option, 0);
	for (; enum_option != NULL; geoxml_sequence_next(&enum_option))
		if (!strcmp(geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(sequence)),
		geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(enum_option))))
			return geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(enum_option));
	return geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(sequence));
}

static void
__parameter_widget_set_non_list_widget_value(struct parameter_widget * parameter_widget, const gchar * value)
{
	if (parameter_widget->dict_parameter != NULL) {
		GString *			value;

		value = g_string_new(NULL);

		g_string_printf(value, "%s=%s",
			geoxml_program_parameter_get_keyword(parameter_widget->dict_parameter),
			geoxml_program_parameter_get_first_value(parameter_widget->dict_parameter,
				parameter_widget->use_default_value));
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value->str);
		gtk_editable_set_editable(GTK_EDITABLE(parameter_widget->value_widget), FALSE);

		g_string_free(value, TRUE);

		return;
	}

	if (parameter_widget_can_use_dict(parameter_widget))
		gtk_editable_set_editable(GTK_EDITABLE(parameter_widget->value_widget), TRUE);
	switch (parameter_widget->parameter_type) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_RANGE:
		if (!strlen(value))
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget), 0);
		else {
			gchar *	endptr;
			double	number_value;

			number_value = strtod(value, &endptr);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget),
				(endptr != value) ? number_value : 0);
		}
		break;
	case GEOXML_PARAMETERTYPE_FILE:
		gtk_file_entry_set_path(GTK_FILE_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_ENUM: {
		GeoXmlSequence *	option;
		int			i;

		geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter, &option, 0);
		for (i = 0; option != NULL; ++i, geoxml_sequence_next(&option))
			if (strcmp(value, geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(option))) == 0) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget),
					geoxml_program_parameter_get_required(
					parameter_widget->program_parameter) ? i : i+1);
				return;
			}

		gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget), 0);
		break;
	} case GEOXML_PARAMETERTYPE_FLAG:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget),
			!strcmp(value, "on"));
		break;
	default:
		break;
	}
}

static GString *
__parameter_widget_get_widget_value(struct parameter_widget * parameter_widget, gboolean check_list)
{
	GString *			value;

	value = g_string_new(NULL);

	if (parameter_widget->dict_parameter != NULL) {
		g_string_assign(value, geoxml_program_parameter_get_first_value(
			parameter_widget->dict_parameter, parameter_widget->use_default_value));
		return value;
	}
	if (check_list && geoxml_program_parameter_get_is_list(parameter_widget->program_parameter)) {
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->list_value_widget)));
		return value;
	}

	switch (parameter_widget->parameter_type) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_RANGE: {
		guint	digits;
		digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(parameter_widget->value_widget));
		if (digits == 0)
			g_string_printf(value, "%d",
				gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(parameter_widget->value_widget)));
		else {
			GString *	mask;

			mask = g_string_new(NULL);
			g_string_printf(mask, "%%.%if", digits);
			g_string_printf(value, mask->str,
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(parameter_widget->value_widget)));

			g_string_free(mask, TRUE);
		}
		break;
	} case GEOXML_PARAMETERTYPE_FILE:
		g_string_assign(value, gtk_file_entry_get_path(GTK_FILE_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_ENUM: {
		gint	index;

		index = gtk_combo_box_get_active(GTK_COMBO_BOX(parameter_widget->value_widget));
		if (index == -1)
			g_string_assign(value, "");
		else {
			GeoXmlSequence *	enum_option;

			/* minus one to skip the first empty value */
			geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter,
				&enum_option, geoxml_program_parameter_get_required(
				parameter_widget->program_parameter) ? index : index-1);
			g_string_assign(value, geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(enum_option)));
		}

		break;
	} case GEOXML_PARAMETERTYPE_FLAG:
		g_string_assign(value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			parameter_widget->value_widget)) == TRUE ? "on" : "off");
		break;
	default:
		break;
	}

	return value;
}

static void
__parameter_widget_report_change(struct parameter_widget * parameter_widget)
{
	if (parameter_widget->callback != NULL)
		parameter_widget->callback(parameter_widget, parameter_widget->user_data);
}

/*
 * Function: __parameter_on_list_value_widget_changed
 * Update XML when user is manually editing the list
 */
static void
__parameter_on_list_value_widget_changed(GtkEntry * entry, struct parameter_widget * parameter_widget)
{
	geoxml_program_parameter_set_parse_list_value(
		parameter_widget->program_parameter,
		parameter_widget->use_default_value,
		gtk_entry_get_text(entry));

	__parameter_widget_report_change(parameter_widget);
}

/*
 * Function: __parameter_widget_sync_non_list
 * Syncronize input widget value with the parameter
 */
static void
__parameter_widget_sync_non_list(struct parameter_widget * parameter_widget)
{
	GString *	value;

	value = __parameter_widget_get_widget_value(parameter_widget, FALSE);
	geoxml_program_parameter_set_first_value(parameter_widget->program_parameter,
		parameter_widget->use_default_value, value->str);

	g_string_free(value, TRUE);
}

static void
__parameter_widget_on_value_widget_changed(GtkWidget * widget, struct parameter_widget * parameter_widget)
{
	if (parameter_widget->dict_parameter == NULL)
		return;

	__parameter_widget_sync_non_list(parameter_widget);
	__parameter_widget_report_change(parameter_widget);
}

static gboolean
__parameter_widget_on_range_changed(GtkSpinButton * spinbutton, struct parameter_widget * parameter_widget)
{
	__parameter_widget_on_value_widget_changed(GTK_WIDGET(spinbutton), parameter_widget);
	return FALSE;
}

/*
 * Function: __parameter_widget_weak_ref
 * Free struct before widget deletion
 */
static void
__parameter_widget_weak_ref(struct parameter_widget * parameter_widget, GtkWidget * widget)
{
	g_free(parameter_widget);
}

static void
__parameter_on_list_value_widget_changed(GtkEntry * entry, struct parameter_widget * parameter_widget);

/*
 * Function: __parameter_list_value_widget_update
 * Update from the XML
 */
static void
__parameter_list_value_widget_update(struct parameter_widget * parameter_widget)
{
	GString *	value;

	value = geoxml_program_parameter_get_string_value(
		parameter_widget->program_parameter,
		parameter_widget->use_default_value);

	g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)__parameter_on_list_value_widget_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(parameter_widget->list_value_widget), value->str);
	g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)__parameter_on_list_value_widget_changed, NULL);

	g_string_free(value, TRUE);
}

static void
__on_sequence_edit_changed(GtkSequenceEdit * sequence_edit, struct parameter_widget * parameter_widget);
static void
__parameter_widget_on_value_widget_changed(GtkWidget * widget, struct parameter_widget * parameter_widget);

/*
 * Function: __on_edit_list_toggled
 * Take action to start/finish editing list of parameter's values
 */
static void
__on_edit_list_toggled(GtkToggleButton * toggle_button, struct parameter_widget * parameter_widget)
{
	gboolean			toggled;

	toggled = gtk_toggle_button_get_active(toggle_button);
	if (toggled == TRUE) {
		GeoXmlSequence *	first_value;

		geoxml_program_parameter_get_value(parameter_widget->program_parameter,
			parameter_widget->use_default_value, &first_value, 0);
		if (parameter_widget->parameter_type != GEOXML_PARAMETERTYPE_ENUM) {
			g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)__parameter_on_list_value_widget_changed,
				NULL);
			g_signal_handlers_block_matched(G_OBJECT(parameter_widget->value_sequence_edit),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)__on_sequence_edit_changed,
				NULL);
			value_sequence_edit_load(parameter_widget->value_sequence_edit, first_value,
				(ValueSequenceSetFunction)geoxml_value_sequence_set,
				(ValueSequenceGetFunction)geoxml_value_sequence_get, NULL);
			g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)__parameter_on_list_value_widget_changed,
				NULL);
			g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->value_sequence_edit),
				G_SIGNAL_MATCH_FUNC,
				0, 0, NULL,
				(GCallback)__on_sequence_edit_changed,
				NULL);

			gtk_widget_show(GTK_WIDGET(parameter_widget->value_sequence_edit));
		} else {
			value_sequence_edit_load(parameter_widget->value_sequence_edit, first_value,
				(ValueSequenceSetFunction)enum_value_to_label_set,
				(ValueSequenceGetFunction)enum_value_to_label_get, parameter_widget);
		}
	} else if (parameter_widget->parameter_type != GEOXML_PARAMETERTYPE_ENUM)
		gtk_widget_hide(GTK_WIDGET(parameter_widget->value_sequence_edit));

	gtk_widget_set_sensitive(parameter_widget->list_value_widget, !toggled);
}

static void
__on_sequence_edit_add_request(ValueSequenceEdit * value_sequence_edit, struct parameter_widget * parameter_widget)
{
	GString *			value;
	GeoXmlSequence *		sequence;
	GtkListStore *			list_store;

	g_object_get(value_sequence_edit, "list-store", &list_store, NULL);
	value = __parameter_widget_get_widget_value(parameter_widget, FALSE);
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL) == 0) {
		GeoXmlSequence *	first_sequence;

		geoxml_program_parameter_get_value(parameter_widget->program_parameter,
			parameter_widget->use_default_value, &first_sequence, 0);
		sequence = first_sequence;
	} else
		sequence = GEOXML_SEQUENCE(geoxml_program_parameter_append_value(
			parameter_widget->program_parameter, parameter_widget->use_default_value));

	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(sequence), value->str);
	value_sequence_edit_add(value_sequence_edit, sequence);

	__parameter_list_value_widget_update(parameter_widget);
	__parameter_widget_set_non_list_widget_value(parameter_widget, "");

	g_string_free(value, TRUE);
}

static void
__on_sequence_edit_changed(GtkSequenceEdit * sequence_edit, struct parameter_widget * parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

/* Function: __validate_int
 * Validate an int parameter
 */
static void
__validate_int(GtkEntry * entry, struct parameter_widget * parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	gchar *	min, * max;

	geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
	gtk_entry_set_text(entry, libgebr_validate_int(
		gtk_entry_get_text(entry), min, max));
}

/* Function: __validate_float
 * Validate a float parameter
 */
static void
__validate_float(GtkEntry * entry, struct parameter_widget * parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	gchar *	min, * max;

	geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
	gtk_entry_set_text(entry, libgebr_validate_float(
		gtk_entry_get_text(entry), min, max));
}

/* Function: __validate_on_leaving
 * Call a validation function
 */
static gboolean
__validate_on_leaving(GtkWidget * widget, GdkEventFocus * event,
struct parameter_widget * parameter_widget)
{
	parameter_widget_validate(parameter_widget);
	return FALSE;
}

/* Function: __parameter_widget_configure
 * Create UI
 */
static void
__parameter_widget_configure(struct parameter_widget * parameter_widget)
{
	switch (parameter_widget->parameter_type) {
	case GEOXML_PARAMETERTYPE_FLOAT: {
		GtkWidget *			entry;

		parameter_widget->value_widget = entry = gtk_entry_new();
		gtk_widget_set_size_request(entry, 90, 30);

		g_signal_connect(entry, "activate",
			GTK_SIGNAL_FUNC(__validate_float), parameter_widget);
		g_signal_connect(entry, "focus-out-event",
			GTK_SIGNAL_FUNC(__validate_on_leaving), parameter_widget);

		break;
	} case GEOXML_PARAMETERTYPE_INT: {
		GtkWidget *			entry;

		parameter_widget->value_widget = entry = gtk_entry_new();
		gtk_widget_set_size_request(entry, 90, 30);

		g_signal_connect(entry, "activate",
			GTK_SIGNAL_FUNC(__validate_int), parameter_widget);
		g_signal_connect(entry, "focus-out-event",
			GTK_SIGNAL_FUNC(__validate_on_leaving), parameter_widget);

		break;
	} case GEOXML_PARAMETERTYPE_STRING: {
		parameter_widget->value_widget = gtk_entry_new();
		gtk_widget_set_size_request(parameter_widget->value_widget, 140, 30);
	
		break;
	} case GEOXML_PARAMETERTYPE_RANGE: {
		GtkWidget *			spin;

		gchar *				min_str;
		gchar *				max_str;
		gchar *				inc_str;
		gchar *				digits_str;
		double				min, max, inc;

		geoxml_program_parameter_get_range_properties(parameter_widget->program_parameter,
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
	} case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		file_entry;

		/* file entry */
		parameter_widget->value_widget =
			file_entry = gtk_file_entry_new((GtkFileEntryCustomize)parameter_widget->data);
		gtk_widget_set_size_request(file_entry, 220, 30);

		gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry),
			geoxml_program_parameter_get_file_be_directory(
				parameter_widget->program_parameter));
		gtk_file_entry_set_do_overwrite_confirmation(GTK_FILE_ENTRY(file_entry), FALSE);

		break;
	} case GEOXML_PARAMETERTYPE_ENUM: {
		GtkWidget *		combo_box;
		GeoXmlSequence *	enum_option;

		parameter_widget->value_widget = combo_box = gtk_combo_box_new_text();
		if (!geoxml_program_parameter_get_required(parameter_widget->program_parameter))
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), "");
		geoxml_program_parameter_get_enum_option(
			parameter_widget->program_parameter, &enum_option, 0);
		for (; enum_option != NULL; geoxml_sequence_next(&enum_option)) {
			const gchar *		text;

			text = strlen(geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(enum_option)))
				? geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(enum_option))
				: geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(enum_option));
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text);
		}

		break;
	} case GEOXML_PARAMETERTYPE_FLAG: {
		parameter_widget->value_widget = gtk_check_button_new();
		break;
	} default:
		return;
	}

	if (geoxml_program_parameter_get_is_list(parameter_widget->program_parameter) == TRUE) {
		GtkWidget *		vbox;
		GtkWidget *		hbox;
		GtkWidget *		button;
		GtkWidget *		sequence_edit;

		vbox = gtk_vbox_new(FALSE, 10);
		hbox = gtk_hbox_new(FALSE, 10);
		if (parameter_widget->parameter_type != GEOXML_PARAMETERTYPE_ENUM)
			gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

		parameter_widget->list_value_widget = gtk_entry_new();
		gtk_widget_show(parameter_widget->list_value_widget);
		gtk_box_pack_start(GTK_BOX(hbox), parameter_widget->list_value_widget, TRUE, TRUE, 0);
		if (parameter_widget->parameter_type != GEOXML_PARAMETERTYPE_ENUM)
			g_signal_connect(parameter_widget->list_value_widget, "changed",
				(GCallback)__parameter_on_list_value_widget_changed, parameter_widget);

		button = gtk_toggle_button_new_with_label(_("Edit list"));
		gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
		g_signal_connect(button, "toggled",
			GTK_SIGNAL_FUNC(__on_edit_list_toggled), parameter_widget);

		__parameter_widget_set_non_list_widget_value(parameter_widget, "");
		gtk_widget_show(parameter_widget->value_widget);

		sequence_edit = value_sequence_edit_new(parameter_widget->value_widget);
		gtk_box_pack_start(GTK_BOX(vbox), sequence_edit, TRUE, TRUE, 0);
		parameter_widget->value_sequence_edit = VALUE_SEQUENCE_EDIT(sequence_edit);
		g_object_set(G_OBJECT(sequence_edit), "minimum-one", TRUE, NULL);
		g_signal_connect(sequence_edit, "add-request",
			GTK_SIGNAL_FUNC(__on_sequence_edit_add_request), parameter_widget);
		if (parameter_widget->parameter_type != GEOXML_PARAMETERTYPE_ENUM)
			g_signal_connect(sequence_edit, "changed",
				GTK_SIGNAL_FUNC(__on_sequence_edit_changed), parameter_widget);
		else {
			gtk_widget_show(sequence_edit);
			g_object_set(sequence_edit, "may-rename", FALSE, NULL);
			gtk_button_clicked(GTK_BUTTON(button));
			gtk_widget_hide(hbox);
		}

		parameter_widget->widget = vbox;
	} else {
		parameter_widget->widget = parameter_widget->value_widget;
		parameter_widget->value_sequence_edit = NULL;

		switch (parameter_widget->parameter_type) {
		case GEOXML_PARAMETERTYPE_FLOAT:
		case GEOXML_PARAMETERTYPE_INT:
		case GEOXML_PARAMETERTYPE_STRING:
		case GEOXML_PARAMETERTYPE_ENUM:
			g_signal_connect(parameter_widget->value_widget, "changed",
				(GCallback)__parameter_widget_on_value_widget_changed, parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			g_signal_connect(parameter_widget->value_widget, "output",
				(GCallback)__parameter_widget_on_range_changed, parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			g_signal_connect(parameter_widget->value_widget, "path-changed",
				(GCallback)__parameter_widget_on_value_widget_changed, parameter_widget);
			break;
		case GEOXML_PARAMETERTYPE_FLAG:
			g_signal_connect(parameter_widget->value_widget, "toggled",
				(GCallback)__parameter_widget_on_value_widget_changed, parameter_widget);
			break;
		default:
			break;
		}
	}
	if (parameter_widget->dicts != NULL && parameter_widget_can_use_dict(parameter_widget)) {
		GeoXmlDocument *		documents [] = {
			parameter_widget->dicts->project, parameter_widget->dicts->line,
			parameter_widget->dicts->flow, NULL};

		parameter_widget->dict_parameter = NULL;
		for (int i = 0; documents[i] != NULL; i++) {
			GeoXmlProgramParameter *	dict_parameter;

			dict_parameter = geoxml_program_parameter_find_dict_parameter(
				parameter_widget->program_parameter, documents[i]);
			if (dict_parameter != NULL)
				parameter_widget->dict_parameter = dict_parameter;
		}

		parameter_widget_find_dict_parameter(parameter_widget);
		g_signal_connect(parameter_widget->value_widget, "populate-popup",
			(GCallback)parameter_widget_value_entry_on_populate_popup, parameter_widget);
	}

	parameter_widget_update(parameter_widget);
	parameter_widget_set_auto_submit_callback(parameter_widget,
		parameter_widget->callback, parameter_widget->user_data);

	/* delete struct */
	g_object_weak_ref(G_OBJECT(parameter_widget->widget), (GWeakNotify)__parameter_widget_weak_ref, parameter_widget);
}

/* Function: compare_parameters_by_keyword
 * Find in documents' dictionaries for the associated dictionary parameter
 */
static void
parameter_widget_find_dict_parameter(struct parameter_widget * widget)
{
	g_signal_handlers_disconnect_matched(G_OBJECT(widget->value_widget),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)on_dict_clicked, NULL);
	if (widget->dict_parameter != NULL) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(widget->value_widget),
			GTK_ENTRY_ICON_SECONDARY, "accessories-dictionary");
		g_signal_connect(widget->value_widget, "icon-press",
			(GCallback)on_dict_clicked, widget);
	} else {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(widget->value_widget),
			GTK_ENTRY_ICON_SECONDARY, NULL);
	}
}

/* Function: compare_parameters_by_keyword
 * For sorting parameters by keyword
 */
static gint
compare_parameters_by_keyword(GeoXmlProgramParameter * parameter1, GeoXmlProgramParameter * parameter2)
{
	return strcmp(geoxml_program_parameter_get_keyword(parameter1),
		geoxml_program_parameter_get_keyword(parameter2));
}

/* Function: parameter_widget_dict_popup_menu
 * Read dictionaries and build a popup menu
 */
static GtkWidget *
parameter_widget_dict_popup_menu(struct parameter_widget * widget)
{
	enum GEOXML_PARAMETERTYPE	compatibles_types[5] = {
		GEOXML_PARAMETERTYPE_UNKNOWN, GEOXML_PARAMETERTYPE_UNKNOWN,
		GEOXML_PARAMETERTYPE_UNKNOWN, GEOXML_PARAMETERTYPE_UNKNOWN,
		GEOXML_PARAMETERTYPE_UNKNOWN
	};

	GList *				compatible_parameters, * cp;
	GeoXmlDocument *		documents[4] = {
		widget->dicts->project, widget->dicts->line,
		widget->dicts->flow, NULL
	};

	GtkWidget *			menu;
	GtkWidget *			menu_item;
	GSList *			group;
	
	compatibles_types[0] = widget->parameter_type;
	switch (widget->parameter_type) {
	case GEOXML_PARAMETERTYPE_STRING:
		compatibles_types[1] = GEOXML_PARAMETERTYPE_INT;
		compatibles_types[2] = GEOXML_PARAMETERTYPE_FLOAT;
		compatibles_types[3] = GEOXML_PARAMETERTYPE_RANGE;
	case GEOXML_PARAMETERTYPE_FLOAT:
		compatibles_types[1] = GEOXML_PARAMETERTYPE_INT;
		break;
	case GEOXML_PARAMETERTYPE_RANGE:
		compatibles_types[1] = GEOXML_PARAMETERTYPE_INT;
		compatibles_types[2] = GEOXML_PARAMETERTYPE_FLOAT;
		break;
	default:
		break;
	}

	compatible_parameters = NULL;
	for (int i = 0; documents[i] != NULL; i++) {
		GeoXmlSequence *		dict_parameter;
		enum GEOXML_PARAMETERTYPE	dict_parameter_type;

		dict_parameter = geoxml_parameters_get_first_parameter(
			geoxml_document_get_dict_parameters(documents[i]));
		for (; dict_parameter != NULL; geoxml_sequence_next(&dict_parameter)) {
			gboolean		compatible;
			const gchar *		keyword;

			compatible = FALSE;
			dict_parameter_type = geoxml_parameter_get_type(GEOXML_PARAMETER(dict_parameter));
			for (int j = 0; compatibles_types[j] != GEOXML_PARAMETERTYPE_UNKNOWN; j++) {
				if (compatibles_types[j] == dict_parameter_type) {
					compatible = TRUE;
					break;
				}
			}
			if (!compatible)
				continue;

			keyword = geoxml_program_parameter_get_keyword(
				GEOXML_PROGRAM_PARAMETER(dict_parameter));
			for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp))
				if (!strcmp(keyword, geoxml_program_parameter_get_keyword(
				GEOXML_PROGRAM_PARAMETER(cp->data))))
					compatible_parameters = g_list_remove_link(compatible_parameters, cp);

			compatible_parameters = g_list_prepend(compatible_parameters, dict_parameter);
		}
	}

	menu = gtk_menu_new();

	menu_item = gtk_radio_menu_item_new_with_label(NULL, _("Do not use dictionary"));
	g_object_set(menu_item, "user-data", NULL, NULL);
	g_signal_connect(menu_item, "activate",
		G_CALLBACK(on_dict_parameter_toggled), widget);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	compatible_parameters = g_list_sort(compatible_parameters, (GCompareFunc)compare_parameters_by_keyword);
	for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp)) {
		GString *	label;

		label = g_string_new(NULL);

		g_string_printf(label, "%s=%s (%s)",
			geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(cp->data)),
			geoxml_program_parameter_get_first_value(GEOXML_PROGRAM_PARAMETER(cp->data), FALSE),
			geoxml_parameter_get_label(GEOXML_PARAMETER(cp->data)));
		menu_item = gtk_radio_menu_item_new_with_label(group, label->str);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
		g_object_set(menu_item, "user-data", cp->data, NULL);
		g_signal_connect(menu_item, "toggled",
			G_CALLBACK(on_dict_parameter_toggled), widget);
		gtk_container_add(GTK_CONTAINER(menu), menu_item);

		if ((void*)widget->dict_parameter == cp->data)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);

		g_string_free(label, TRUE);
	}

	gtk_widget_show_all(menu);
	g_list_free(compatible_parameters);

	return menu;
}

/* Function: on_dict_clicked
 * Popup menu upon dictionary icon click
 */
static void
on_dict_clicked(GtkEntry * entry, GtkEntryIconPosition icon_pos, GdkEventButton * event,
struct parameter_widget * widget)
{
	gtk_menu_popup(GTK_MENU(parameter_widget_dict_popup_menu(widget)), NULL, NULL, NULL, NULL, event->button, event->time);
}

/* Function: parameter_widget_value_entry_on_populate_popup
 * Add dictionary submenu into entry popup menu
 */
static void
parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
struct parameter_widget * widget)
{
	GtkWidget *	menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Dictionary"));
	gtk_widget_show(menu_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), parameter_widget_dict_popup_menu(widget));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
}

/* Function: parameter_widget_can_use_dict
 * Return TRUE if parameter is of an compatible type to use an dictionary value
 */
static gboolean
parameter_widget_can_use_dict(struct parameter_widget * widget)
{
	switch (geoxml_parameter_get_type(widget->parameter)) {
	case GEOXML_PARAMETERTYPE_FLOAT: case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING: case GEOXML_PARAMETERTYPE_RANGE:
		return TRUE;
	default:
		return FALSE;
	}
}

/* Function: on_dict_parameter_toggled
 * Use value of dictionary parameter corresponding to menu_item in parameter at _widget_
 */
static void
on_dict_parameter_toggled(GtkMenuItem * menu_item, struct parameter_widget * widget)
{
	GeoXmlProgramParameter *	dict_parameter;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
		return;

	g_object_get(menu_item, "user-data", &dict_parameter, NULL);

	geoxml_program_parameter_set_value_from_dict(widget->program_parameter, dict_parameter);
	widget->dict_parameter = dict_parameter;
	parameter_widget_find_dict_parameter(widget);

	parameter_widget_update(widget);
}

/*
 * Public functions
 */

/*
 * Function: parameter_widget_new
 * Create a new parameter widget
 */
struct parameter_widget *
parameter_widget_new(GeoXmlParameter * parameter, gboolean use_default_value, gpointer data)
{
	struct parameter_widget *	parameter_widget;

	parameter_widget = g_malloc(sizeof(struct parameter_widget));
	*parameter_widget = (struct parameter_widget) {
		.parameter = parameter,
		.program_parameter = GEOXML_PROGRAM_PARAMETER(parameter),
		.parameter_type = geoxml_parameter_get_type(parameter),
		.use_default_value = use_default_value,
		.data = data,
		.dict_parameter = NULL,
		.dicts = NULL,
		.callback = NULL,
		.user_data = NULL
	};

	__parameter_widget_configure(parameter_widget);

	return parameter_widget;
}

/* Function: parameter_widget_set_dicts
 * Set dictionaries documents to find dictionaries parameters
 */
void
parameter_widget_set_dicts(struct parameter_widget * parameter_widget,
struct libgebr_gui_program_edit_dicts *	dicts)
{
	parameter_widget->dicts = dicts;
	parameter_widget_reconfigure(parameter_widget);
}

/* Function: parameter_widget_get_widget_value
 * Return the parameter's widget value
 */
GString *
parameter_widget_get_widget_value(struct parameter_widget * parameter_widget)
{
	return __parameter_widget_get_widget_value(parameter_widget, TRUE);
}

/*
 * Function: parameter_widget_set_auto_submit_callback
 *
 */
void
parameter_widget_set_auto_submit_callback(struct parameter_widget * parameter_widget,
	changed_callback callback, gpointer user_data)
{
	parameter_widget->callback = callback;
	parameter_widget->user_data = user_data;
}

/*
 * Function: parameter_widget_update
 * Update input widget value from the parameter's value
 */
void
parameter_widget_update(struct parameter_widget * parameter_widget)
{
	if (geoxml_program_parameter_get_is_list(parameter_widget->program_parameter) == TRUE)
		__parameter_list_value_widget_update(parameter_widget);
	else
		__parameter_widget_set_non_list_widget_value(parameter_widget,
			geoxml_program_parameter_get_first_value(
				parameter_widget->program_parameter,
				parameter_widget->use_default_value));
}

/* Function: parameter_widget_validate
 * Apply validations rules (if existent) to _parameter_widget_
 */
void
parameter_widget_validate(struct parameter_widget * parameter_widget)
{
	switch (parameter_widget->parameter_type) {
	case GEOXML_PARAMETERTYPE_INT:
		__validate_int(GTK_ENTRY(parameter_widget->value_widget), parameter_widget);
		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		__validate_float(GTK_ENTRY(parameter_widget->value_widget), parameter_widget);
		break;
	default:
		break;
	}
}

/* Function: parameter_widget_update_list_separator
 * Update UI of list with the new separator
 */
void
parameter_widget_update_list_separator(struct parameter_widget * parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

/* Function: parameter_widget_reconfigure
 * Rebuild the UI
 */
void
parameter_widget_reconfigure(struct parameter_widget * parameter_widget)
{
	parameter_widget->parameter_type = geoxml_parameter_get_type(parameter_widget->parameter);

	g_object_weak_unref(G_OBJECT(parameter_widget->widget),
		(GWeakNotify)__parameter_widget_weak_ref, parameter_widget);
	gtk_widget_destroy(parameter_widget->widget);

	__parameter_widget_configure(parameter_widget);
}
