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
#include <config.h>

#include <glib/gi18n-lib.h>

#include "../utils.h"

#include "gebr-gui-parameter.h"
#include "gebr-gui-utils.h"

#define DOUBLE_MAX +999999999
#define DOUBLE_MIN -999999999

/* Prototypes {{{1 */
static GtkWidget *gebr_gui_parameter_widget_variable_popup_menu(struct gebr_gui_parameter_widget *widget,
								GtkEntry *entry);

static void gebr_gui_parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
								    struct gebr_gui_parameter_widget *widget);

static void on_variable_parameter_activate(GtkMenuItem * menu_item);

static gboolean on_mnemonic_activate(GtkBox * box, gboolean cycle,
						 struct gebr_gui_parameter_widget *widget);

static void gebr_gui_parameter_on_list_value_widget_changed(GtkEntry * entry,
							    struct gebr_gui_parameter_widget *parameter_widget);

static void __on_sequence_edit_changed(GebrGuiSequenceEdit * sequence_edit,
				       struct gebr_gui_parameter_widget *parameter_widget);

static void gebr_gui_parameter_widget_on_value_widget_changed(GtkWidget * widget, struct gebr_gui_parameter_widget
							      *parameter_widget);

static void on_entry_activate_add (GtkEntry *entry, struct gebr_gui_parameter_widget *parameter_widget);

static void __on_destroy_menu_unblock_handler(GtkMenuShell *menushell,
					      GtkEntry     *entry);

static gboolean on_entry_completion_matched (GtkEntryCompletion *completion,
					     GtkTreeModel       *model,
					     GtkTreeIter        *iter,
					     gpointer            data);

static gboolean completion_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data);

static GtkTreeModel *generate_completion_model(struct gebr_gui_parameter_widget *parameter_widget);

static void setup_entry_completion(GtkEntry *entry,
				   GtkTreeModel *model,
				   GtkEntryCompletionMatchFunc func,
				   GCallback match_selected_cb,
				   gpointer data);

static GList * get_compatible_variables(GebrGeoXmlParameterType type,
					GebrGeoXmlDocument *flow,
					GebrGeoXmlDocument *line,
					GebrGeoXmlDocument *proj);

static gboolean on_spin_button_output(GtkSpinButton *spin,
				      struct gebr_gui_parameter_widget *widget);

static gint on_spin_button_input(GtkSpinButton *spin,
				 gdouble *rval,
				 struct gebr_gui_parameter_widget *widget);

static GString *gebr_gui_parameter_widget_get_widget_value_full(struct gebr_gui_parameter_widget *parameter_widget);

static gboolean on_list_value_widget_focus_out(GtkEntry *entry, GdkEventFocus *event, GebrGuiParameterWidget *self);

static void on_list_value_widget_validate(GtkEntry *entry,
					  struct gebr_gui_parameter_widget *parameter_widget);

static void __set_type_icon(struct gebr_gui_parameter_widget *parameter_widget);

static void validate_list_value_widget(GebrGuiParameterWidget *self);

/* Implementation of GebrGuiValidatableWidget interface {{{1 */
static void parameter_widget_set_icon(GebrGuiValidatableWidget *widget,
				      GebrGeoXmlParameter *param,
				      GError *error)
{
	GtkEntry *entry;
	GebrGuiParameterWidget *self = (GebrGuiParameterWidget*)widget;

	if (self->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE)
		entry = GTK_ENTRY(GEBR_GUI_FILE_ENTRY(self->value_widget)->entry);
	else
		entry = GTK_ENTRY(self->value_widget);

	if (error) {
		if (self->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE)
			gebr_gui_file_entry_set_warning(GEBR_GUI_FILE_ENTRY(self->value_widget), error->message);
		else {
			gtk_entry_set_icon_from_stock(entry,
						      GTK_ENTRY_ICON_SECONDARY,
						      GTK_STOCK_DIALOG_WARNING);
			gtk_entry_set_icon_tooltip_text(entry,
							GTK_ENTRY_ICON_SECONDARY,
							error->message);
		}
	} else
		__set_type_icon(self);
}

static void parameter_widget_set_value(GebrGuiValidatableWidget *widget,
				   const gchar *value)
{
	GtkEntry *entry;
	GebrGuiParameterWidget *self = (GebrGuiParameterWidget*)widget;

	if (self->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE)
		entry = GTK_ENTRY(GEBR_GUI_FILE_ENTRY(self->value_widget)->entry);
	else
		entry = GTK_ENTRY(self->value_widget);

	switch (self->parameter_type)
	{
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		gtk_entry_set_text(entry, value);
		break;
	default:
		break;
	}
}

static gchar *parameter_widget_get_value(GebrGuiValidatableWidget *widget)
{
	GString *value;
	GebrGuiParameterWidget *self = (GebrGuiParameterWidget*)widget;
	value = gebr_gui_parameter_widget_get_widget_value_full(self);
	return g_string_free(value, FALSE);
}

/* Private Functions {{{1 */
static gboolean __parameter_accepts_expression(struct gebr_gui_parameter_widget *parameter_widget)
{
	return parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_INT ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE;
}

static void enum_value_to_label_set(GebrGeoXmlSequence * sequence, const gchar * label,
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

static void gebr_gui_parameter_widget_file_entry_customize_function(GtkFileChooser * file_chooser,
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
		((GebrGuiFileEntryCustomize) parameter_widget->data) (file_chooser, NULL);
}

static void
gebr_gui_parameter_widget_set_non_list_widget_value(GebrGuiParameterWidget *parameter_widget,
						    const gchar * value)
{
	if (__parameter_accepts_expression(parameter_widget)) {
		if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE)
			gtk_editable_set_editable(GTK_EDITABLE(GTK_ENTRY (GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget)->entry)), TRUE);
		else
			gtk_editable_set_editable(GTK_EDITABLE(parameter_widget->value_widget), TRUE);
	}

	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		gtk_spin_button_update(GTK_SPIN_BUTTON(parameter_widget->value_widget));
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		gebr_gui_file_entry_set_path(GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget),
						 value);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM: {
			GebrGeoXmlSequence *option;
			int i;

			gebr_geoxml_program_parameter_get_enum_option(parameter_widget->program_parameter,
								      &option, 0);
			for (i = 0; option != NULL; ++i, gebr_geoxml_sequence_next(&option))
				if (strcmp(value, gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(option))) == 0) {
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

static GString *gebr_gui_parameter_widget_get_widget_value_full(struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;

	value = g_string_new(NULL);

	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->value_widget)));
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		g_string_assign(value,
				gebr_gui_file_entry_get_path(GEBR_GUI_FILE_ENTRY
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

static void gebr_gui_parameter_widget_report_change(struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->callback != NULL)
		parameter_widget->callback(parameter_widget, parameter_widget->user_data);
}

/*
 * gebr_gui_parameter_on_list_value_widget_changed:
 * Update XML when user is manually editing the list
 */
static void gebr_gui_parameter_on_list_value_widget_changed(GtkEntry * entry,
							    struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_geoxml_program_parameter_set_parse_list_value(parameter_widget->program_parameter,
							   parameter_widget->use_default_value,
							   gtk_entry_get_text(entry));

	gebr_gui_parameter_widget_report_change(parameter_widget);
}

/*
 * gebr_gui_parameter_widget_sync_non_list:
 *
 * Writes the input widget value into the parameter.
 */
static void gebr_gui_parameter_widget_sync_non_list(struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;

	value = gebr_gui_parameter_widget_get_widget_value_full(parameter_widget);
	gebr_geoxml_program_parameter_set_first_value(parameter_widget->program_parameter,
						      parameter_widget->use_default_value, value->str);

	g_string_free(value, TRUE);
}

static void gebr_gui_parameter_widget_on_value_widget_changed(GtkWidget * widget,
							      struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_sync_non_list(parameter_widget);
	gebr_gui_parameter_widget_report_change(parameter_widget);
}

/*
 * __parameter_list_value_widget_update:
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

/*
 * __on_edit_list_toggled:
 * Take action to start/finish editing list of parameter's values.
 */
static void __on_edit_list_toggled(GtkToggleButton * toggle_button, struct gebr_gui_parameter_widget *parameter_widget)
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

static void __on_sequence_edit_add_request(GebrGuiValueSequenceEdit *gebr_gui_value_sequence_edit,
					   GebrGuiParameterWidget *self)
{
	GString *value;
	GebrGeoXmlSequence *sequence;
	GtkListStore *list_store;
	GtkWidget *entry;

	entry = g_object_get_data (G_OBJECT (self->gebr_gui_value_sequence_edit), "activatable-entry");
	if (entry) {
		gtk_widget_grab_focus (entry);
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	}

	g_object_get(gebr_gui_value_sequence_edit, "list-store", &list_store, NULL);
	value = gebr_gui_parameter_widget_get_widget_value_full(self);

	if (!gebr_gui_parameter_widget_validate(self)){
		g_string_free(value, TRUE);
		return;
	}

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL) == 0) {
		GebrGeoXmlSequence *first_sequence;

		gebr_geoxml_program_parameter_get_value(self->program_parameter,
							self->use_default_value, &first_sequence,
							0);
		sequence = first_sequence;
	} else
		sequence =
		    GEBR_GEOXML_SEQUENCE(gebr_geoxml_program_parameter_append_value
					 (self->program_parameter,
					  self->use_default_value));

	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(sequence), value->str);
	gebr_gui_value_sequence_edit_add(gebr_gui_value_sequence_edit, sequence);

	__parameter_list_value_widget_update(self);
	gebr_gui_parameter_widget_set_non_list_widget_value(self, "");

	g_string_free(value, TRUE);
}

static void __on_sequence_edit_changed(GebrGuiSequenceEdit * sequence_edit,
				       struct gebr_gui_parameter_widget *parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

/*
 * for parameter that accepts an expression (int and float)
 */
static void __set_type_icon(GebrGuiParameterWidget *parameter_widget)
{
	gchar *result = NULL;
	const gchar *value;
	gboolean has_focus = FALSE;
	GtkEntry *entry;
	GError *error = NULL;
	GebrGeoXmlDocumentType scope = GEBR_GEOXML_DOCUMENT_TYPE_FLOW;

	value = gebr_geoxml_program_parameter_get_first_value(parameter_widget->program_parameter, FALSE);

	if (gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(parameter_widget->parameter)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
		scope = GEBR_GEOXML_DOCUMENT_TYPE_LINE;

	gebr_validator_evaluate(parameter_widget->validator, value,
	                        parameter_widget->parameter_type,
	                        scope, &result, &error);

	if (error) {
		// This call won't recurse since 'error' is non-NULL.
		// Execution should never reach here anyway...
		gebr_gui_validatable_widget_set_icon(GEBR_GUI_VALIDATABLE_WIDGET(parameter_widget),
						     parameter_widget->parameter,
						     error);
		g_clear_error(&error);
		return;
	}

	if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
		gebr_gui_file_entry_unset_warning(GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget),
						  result);
		goto free;
	}

	entry = GTK_ENTRY(parameter_widget->value_widget);
	g_object_get(entry, "has-focus", &has_focus, NULL);

	if (!has_focus && __parameter_accepts_expression(parameter_widget)) {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		goto free;
	}

	if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_INT)
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, "integer-icon");
	else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, "real-icon");
	else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_STRING)
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, "string-icon");
	else {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		goto free;
	}

	gtk_entry_set_icon_tooltip_text(entry, GTK_ENTRY_ICON_SECONDARY, result);

free:
	g_free(result);
}

static void __on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_validate(parameter_widget);
}

static gboolean __on_focus_out_event(GtkWidget * widget, GdkEventFocus * event,
				     struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_validate(parameter_widget);
	return FALSE;
}

static gboolean __on_focus_in_event(GtkWidget * widget, GdkEventFocus * event,
				    struct gebr_gui_parameter_widget *parameter_widget)
{
	GtkEntry *entry;
	if (GEBR_GUI_IS_FILE_ENTRY(parameter_widget->value_widget)) {
		entry = GTK_ENTRY(GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget)->entry);
	} else {
		entry = GTK_ENTRY(parameter_widget->value_widget);
	}
	if (!gtk_entry_get_icon_stock(entry, GTK_ENTRY_ICON_SECONDARY))
		__set_type_icon(parameter_widget);
	return FALSE;
}

/*
 * gebr_gui_parameter_widget_configure:
 * Create UI.
 */
/* Widget construction function {{{ */
static void gebr_gui_parameter_widget_configure(struct gebr_gui_parameter_widget *parameter_widget)
{
	gboolean may_complete;
	GtkEntry *activatable_entry = NULL;
	GebrGeoXmlProgram *program;

	gtk_container_foreach(GTK_CONTAINER(parameter_widget->widget), (GtkCallback)gtk_widget_destroy, NULL);

	program = gebr_geoxml_parameter_get_program(parameter_widget->parameter);
	if (parameter_widget->validator != NULL
	    && __parameter_accepts_expression(parameter_widget))
		may_complete = TRUE;
	else
		may_complete = FALSE;

	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE: {
		GtkWidget *spin;

		const gchar *min_str;
		const gchar *max_str;
		const gchar *inc_str;
		const gchar *digits_str;
		double min, max, inc;
		GtkTreeModel *completion_model;

		gebr_geoxml_program_parameter_get_range_properties(parameter_widget->program_parameter,
								   &min_str, &max_str, &inc_str, &digits_str);
		min = !strlen(min_str) ? DOUBLE_MIN : atof(min_str);
		max = !strlen(max_str) ? DOUBLE_MAX : atof(max_str);
		inc = !strlen(inc_str) ? 1.0 : atof(inc_str);
		if (inc == 0)
			inc = 1.0;

		parameter_widget->value_widget = spin = gtk_spin_button_new_with_range(min, max, inc);
		activatable_entry = GTK_ENTRY (spin);

		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(activatable_entry, completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);
		}

		g_signal_connect (spin, "activate",
				  G_CALLBACK (on_entry_activate_add), parameter_widget);
		g_signal_connect (spin, "input",
				  G_CALLBACK (on_spin_button_input), parameter_widget);
		g_signal_connect (spin, "output",
				  G_CALLBACK (on_spin_button_output), parameter_widget);
		g_signal_connect (spin, "focus-in-event",
				  G_CALLBACK (__on_focus_in_event), parameter_widget);
		g_signal_connect (spin, "focus-out-event",
				  G_CALLBACK (__on_focus_out_event), parameter_widget);
		g_signal_connect (spin, "activate",
				  G_CALLBACK (__on_activate), parameter_widget);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), FALSE);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), atoi(digits_str));
		gtk_widget_set_size_request(parameter_widget->value_widget, 220, -1);
		break;
	}
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: {
		GtkWidget *entry;
		GtkTreeModel *completion_model;

		parameter_widget->value_widget = entry = gtk_entry_new();

		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);
		}

		gtk_widget_set_size_request(entry, 220, -1);
		activatable_entry = GTK_ENTRY (entry);

		g_signal_connect (entry, "focus-in-event",
				  G_CALLBACK (__on_focus_in_event), parameter_widget);
		g_signal_connect (entry, "focus-out-event",
				  G_CALLBACK (__on_focus_out_event), parameter_widget);
		g_signal_connect (entry, "activate",
				  G_CALLBACK (on_entry_activate_add), parameter_widget);
		/* validation */
		g_signal_connect (entry, "activate",
				  G_CALLBACK (__on_activate), parameter_widget);

		break;
	}
	case GEBR_GEOXML_PARAMETER_TYPE_INT: {
		GtkWidget *entry;
		GtkTreeModel *completion_model;

		parameter_widget->value_widget = entry = gtk_entry_new();

		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);
		}

		gtk_widget_set_size_request(entry, 220, -1);
		activatable_entry = GTK_ENTRY (entry);

		g_signal_connect (entry, "focus-in-event",
				  G_CALLBACK (__on_focus_in_event), parameter_widget);
		g_signal_connect (entry, "focus-out-event",
				  G_CALLBACK(__on_focus_out_event), parameter_widget);
		g_signal_connect (entry, "activate",
				  G_CALLBACK (on_entry_activate_add), parameter_widget);
		/* validation */
		g_signal_connect (entry, "activate",
				  G_CALLBACK(__on_activate), parameter_widget);

		break;
	}
	case GEBR_GEOXML_PARAMETER_TYPE_STRING: {
		GtkWidget *entry;
		GtkTreeModel *completion_model;

		parameter_widget->value_widget = entry = gtk_entry_new();

		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);
		}

		activatable_entry = GTK_ENTRY (entry);
		gtk_widget_set_size_request(parameter_widget->value_widget, 220, -1);

		g_signal_connect (parameter_widget->value_widget, "activate",
				  G_CALLBACK (on_entry_activate_add), parameter_widget);
		g_signal_connect (entry, "focus-in-event",
				  G_CALLBACK (__on_focus_in_event), parameter_widget);
		g_signal_connect (entry, "focus-out-event",
				  G_CALLBACK(__on_focus_out_event), parameter_widget);
		/* validation */
		g_signal_connect (entry, "activate",
				  G_CALLBACK(__on_activate), parameter_widget);

		break;
	}
	case GEBR_GEOXML_PARAMETER_TYPE_FILE: {
		GtkWidget *file_entry;
		GtkTreeModel *completion_model;

		/* file entry */
		parameter_widget->value_widget = file_entry =
			gebr_gui_file_entry_new((GebrGuiFileEntryCustomize)
						gebr_gui_parameter_widget_file_entry_customize_function,
						parameter_widget);
		activatable_entry = GTK_ENTRY (GEBR_GUI_FILE_ENTRY (file_entry)->entry);
		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(GEBR_GUI_FILE_ENTRY(file_entry)->entry), completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);
		}
		g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "activate",
				  G_CALLBACK (on_entry_activate_add), parameter_widget);
		g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "focus-in-event",
				  G_CALLBACK (__on_focus_in_event), parameter_widget);
		g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "focus-out-event",
				  G_CALLBACK(__on_focus_out_event), parameter_widget);
		/* validation */
		g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "activate",
				  G_CALLBACK (__on_activate), parameter_widget);
		gtk_widget_set_size_request(file_entry, 220, -1);

		gebr_gui_file_entry_set_choose_directory(GEBR_GUI_FILE_ENTRY(file_entry),
							 gebr_geoxml_program_parameter_get_file_be_directory
							 (parameter_widget->program_parameter));
		gebr_gui_file_entry_set_do_overwrite_confirmation(GEBR_GUI_FILE_ENTRY(file_entry),
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

	gboolean is_list = gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter);

	if (is_list) {
		GtkWidget *hbox;
		GtkWidget *button;
		GtkWidget *sequence_edit;
		GtkTreeModel *completion_model;

		hbox = gtk_hbox_new(FALSE, 10);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
			gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(parameter_widget->widget), hbox, TRUE, TRUE, 0);

		parameter_widget->list_value_widget = gtk_entry_new();

		/* Connects this signal to validate the list widget */
		g_signal_connect(parameter_widget->list_value_widget, "activate",
				 G_CALLBACK(on_list_value_widget_validate), parameter_widget);
		g_signal_connect(parameter_widget->list_value_widget, "focus-out-event",
				 G_CALLBACK(on_list_value_widget_focus_out), parameter_widget);

		if (parameter_widget->readonly)
			gtk_widget_set_sensitive(parameter_widget->list_value_widget, FALSE);
		gtk_widget_show(parameter_widget->list_value_widget);
		gtk_box_pack_start(GTK_BOX(hbox), parameter_widget->list_value_widget, TRUE, TRUE, 0);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		{
			g_signal_connect(parameter_widget->list_value_widget, "changed",
					 G_CALLBACK(gebr_gui_parameter_on_list_value_widget_changed),
					 parameter_widget);
		}
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
		g_object_set(sequence_edit, "has-scroll", FALSE, NULL);

		if (may_complete) {
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(parameter_widget->list_value_widget), completion_model,
					       completion_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       GINT_TO_POINTER(parameter_widget->parameter_type));
			g_object_unref (completion_model);

			GebrGeoXmlDocument *flow, *line, *proj;
			gebr_validator_get_documents(parameter_widget->validator, &flow, &line, &proj);
			gebr_gui_value_sequence_edit_set_autocomplete(GEBR_GUI_VALUE_SEQUENCE_EDIT(sequence_edit),
								      flow, line, proj, parameter_widget->parameter_type);
		}

		gtk_box_pack_start(GTK_BOX(parameter_widget->widget), sequence_edit, TRUE, TRUE, 0);
		parameter_widget->gebr_gui_value_sequence_edit = GEBR_GUI_VALUE_SEQUENCE_EDIT(sequence_edit);
		g_object_set_data (G_OBJECT (sequence_edit), "activatable-entry", activatable_entry);
		g_object_set(G_OBJECT(sequence_edit), "minimum-one", TRUE, NULL);
		g_signal_connect(sequence_edit, "add-request",
				 G_CALLBACK(__on_sequence_edit_add_request), parameter_widget);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		{
			g_signal_connect(sequence_edit, "changed",
					 G_CALLBACK(__on_sequence_edit_changed), parameter_widget);
		}
		else {
			gtk_widget_show(sequence_edit);
			g_object_set(sequence_edit, "may-rename", FALSE, NULL);
			gtk_button_clicked(GTK_BUTTON(button));
			gtk_widget_hide(hbox);
		}
	} else {
		gtk_box_pack_start(GTK_BOX(parameter_widget->widget), parameter_widget->value_widget, TRUE, TRUE, 0);
		gtk_widget_show(parameter_widget->value_widget);
		parameter_widget->gebr_gui_value_sequence_edit = NULL;

		switch (parameter_widget->parameter_type) {
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
		case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		case GEBR_GEOXML_PARAMETER_TYPE_ENUM:
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			g_signal_connect(parameter_widget->value_widget, "changed",
					 G_CALLBACK(gebr_gui_parameter_widget_on_value_widget_changed),
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

	gebr_gui_parameter_widget_update(parameter_widget);
	gebr_gui_parameter_widget_set_auto_submit_callback(parameter_widget,
							   parameter_widget->callback,
							   parameter_widget->user_data);

	if (may_complete) {
		GtkEntry * entry;

		if (is_list)
			validate_list_value_widget(parameter_widget);
		gebr_gui_parameter_widget_validate(parameter_widget);

		if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FILE)
			entry = GTK_ENTRY(GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget)->entry);
		else
			entry = GTK_ENTRY(parameter_widget->value_widget);

		g_signal_connect(entry, "populate-popup",
				 G_CALLBACK(gebr_gui_parameter_widget_value_entry_on_populate_popup),
				 parameter_widget);
	}
}
/* }}} */

/*
 * compare_parameters_by_keyword:
 * For sorting parameters by keyword
 */
static gint compare_parameters_by_keyword(GebrGeoXmlProgramParameter * parameter1,
					  GebrGeoXmlProgramParameter * parameter2)
{
	return strcmp(gebr_geoxml_program_parameter_get_keyword(parameter1),
		      gebr_geoxml_program_parameter_get_keyword(parameter2));
}

/*
 * gebr_gui_parameter_widget_variable_popup_menu:
 *
 * Read dictionaries variables and build a popup menu to build expressions
 */
static GtkWidget *gebr_gui_parameter_widget_variable_popup_menu(struct gebr_gui_parameter_widget *widget,
								GtkEntry *entry)
{
	GebrGeoXmlDocument *flow, *line, *proj;
	gebr_validator_get_documents(widget->validator, &flow, &line, &proj);

	return gebr_gui_parameter_add_variables_popup(entry, flow, line, proj,
						      widget->parameter_type);
}

/*
 * gebr_gui_parameter_widget_value_entry_on_populate_popup:
 * Add dictionary submenu into entry popup menu
 */
static void gebr_gui_parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
								    struct gebr_gui_parameter_widget *widget)
{
	GtkWidget *menu_item;

	g_signal_handlers_block_matched(G_OBJECT(entry),
					  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					  G_CALLBACK(__on_focus_out_event), widget);

	g_signal_connect(GTK_MENU_SHELL(menu), "deactivate", G_CALLBACK(__on_destroy_menu_unblock_handler), entry);

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	if (__parameter_accepts_expression(widget)) {
		menu_item = gtk_menu_item_new_with_label(_("Insert Variable"));
		gtk_widget_show(menu_item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), gebr_gui_parameter_widget_variable_popup_menu(widget, entry));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
	}
}

/*
 * on_variable_parameter_activate:
 * Use value of dictionary parameter corresponding to menu_item in parameter at _widget_
 */
static void on_variable_parameter_activate(GtkMenuItem * menu_item)
{
	gint position;
	GtkEntry *entry;
	GebrGeoXmlProgramParameter *param;
	GebrGeoXmlParameterType type;

	type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "param-type"));
	param = g_object_get_data (G_OBJECT (menu_item), "dict-param");
	entry = g_object_get_data (G_OBJECT (menu_item), "entry-widget");
	position = gtk_editable_get_position(GTK_EDITABLE(entry));

	if(type == GEBR_GEOXML_PARAMETER_TYPE_STRING || type == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
		gchar *str_keyword = g_strconcat("[", gebr_geoxml_program_parameter_get_keyword(param), "]", NULL);
		gtk_editable_insert_text(GTK_EDITABLE(entry), str_keyword, -1, &position);
	} else
		gtk_editable_insert_text(GTK_EDITABLE(entry), gebr_geoxml_program_parameter_get_keyword(param), -1, &position);
	gtk_editable_set_position(GTK_EDITABLE(entry), position);
}

static gboolean on_mnemonic_activate(GtkBox * box, gboolean cycle, struct gebr_gui_parameter_widget *widget)
{
	if (!gebr_geoxml_program_parameter_get_is_list(widget->program_parameter)) {
		gtk_widget_mnemonic_activate(widget->value_widget, cycle);
	} else {
		gboolean sensitive;
		g_object_get(G_OBJECT(widget->list_value_widget), "sensitive", &sensitive, NULL);
		gtk_widget_mnemonic_activate(sensitive? widget->list_value_widget:
					     GTK_WIDGET(widget->gebr_gui_value_sequence_edit), cycle);
	}
	return TRUE;
}

static void on_entry_activate_add (GtkEntry *entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	if (gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter))
		g_signal_emit_by_name (parameter_widget->gebr_gui_value_sequence_edit, "add-request");
}

static void __on_destroy_menu_unblock_handler(GtkMenuShell *menushell,
					      GtkEntry     *entry)
{
	g_signal_handlers_unblock_matched(G_OBJECT(entry),
					  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					  G_CALLBACK(__on_focus_out_event), menushell);
}

static gboolean on_entry_completion_matched (GtkEntryCompletion *completion,
					     GtkTreeModel       *model,
					     GtkTreeIter        *iter,
					     gpointer            data)
{
	GtkWidget *entry;
	const gchar *text;
	gint pos;
	gchar * var;
	gchar * word;
	gint ini;
	GebrGeoXmlParameterType type = GPOINTER_TO_INT(data);

	entry = gtk_entry_completion_get_entry(completion);
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	pos = gtk_editable_get_position(GTK_EDITABLE(entry)) - 1;
	ini = pos;
	gtk_tree_model_get(model, iter, 0, &var, -1);
	word = gebr_str_word_before_pos(text, &ini);

	if (!word)
		ini = pos;
	else if (ini - 1 >= 0 && text[ini-1] == '[')
		ini--;

	if (ini <= pos)
		gtk_editable_delete_text(GTK_EDITABLE(entry), ini, pos + 1);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
	    type == GEBR_GEOXML_PARAMETER_TYPE_INT ||
	    type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
		gtk_editable_insert_text(GTK_EDITABLE(entry), var, -1, &ini);
		gtk_editable_set_position(GTK_EDITABLE(entry), ini + strlen(var));
	}
	else if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING || 
		 type == GEBR_GEOXML_PARAMETER_TYPE_FILE){
		GString * value = g_string_new(NULL);

		g_string_printf(value, "[%s]", var);
		gtk_editable_insert_text(GTK_EDITABLE(entry), value->str, -1, &ini);
		gtk_editable_set_position(GTK_EDITABLE(entry), ini + value->len);
		g_string_free(value, TRUE);
	}
	g_free(word);
	g_free(var);

	return TRUE;
}

static gboolean completion_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data)
{
	GtkTreeModel *model;
	GtkWidget *entry;
	const gchar *text;
	gchar *compl;
	gchar *word;
	gint pos;
	gboolean retval;

	entry = gtk_entry_completion_get_entry(completion);
	text = gtk_entry_get_text(GTK_ENTRY(entry));

	// Subtract 1 from position so 0 means 'after first char'
	pos = gtk_editable_get_position(GTK_EDITABLE(entry)) - 1;

	// pos = -1 means caret is before first char
	if (pos == -1)
		return FALSE;

	// Start of variable name
	if (text[pos] == '[') {
		// This is an escaped bracket
		if (pos-1 >= 0 && text[pos-1] == '[')
			return FALSE;
		else
			return TRUE;
	}

	word = gebr_str_word_before_pos(text, &pos);

	// We could not find a word under the cursor
	if (!word)
		return FALSE;

	model = gtk_entry_completion_get_model(completion);
	gtk_tree_model_get(model, iter, 0, &compl, -1);
	retval = g_str_has_prefix(compl, word);

	g_free(word);
	g_free(compl);

	return retval;
}

static void fill_compatible_dicts(GebrGeoXmlParameterType type,
				  GebrGeoXmlParameterType compat[])
{
	compat[0] = type == GEBR_GEOXML_PARAMETER_TYPE_FILE? GEBR_GEOXML_PARAMETER_TYPE_STRING:type;
	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		compat[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		compat[2] = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
		compat[3] = GEBR_GEOXML_PARAMETER_TYPE_RANGE;
		return;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		compat[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		return;
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		compat[1] = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
		return;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		compat[1] = GEBR_GEOXML_PARAMETER_TYPE_INT;
		compat[2] = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
		return;
	default:
		return;
	}
}

static GList *
get_compatible_variables(GebrGeoXmlParameterType type,
			 GebrGeoXmlDocument *flow,
			 GebrGeoXmlDocument *line,
			 GebrGeoXmlDocument *proj)
{
	GList *compat = NULL;
	GList *cp = NULL;

	GebrGeoXmlParameterType compatibles_types[5] = {
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN
	};

	GebrGeoXmlDocument *documents[4] = { proj, line, flow, NULL };

	fill_compatible_dicts(type, compatibles_types);

	for (int i = 0; documents[i] != NULL; i++) {
		GebrGeoXmlSequence *dict_parameter;
		GebrGeoXmlParameterType dict_parameter_type;

		dict_parameter = gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters(documents[i]));
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

			/* Give preference for flow variables over project and line */
			keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(dict_parameter));
			for (cp = compat; cp != NULL; cp = g_list_next(cp))
				if (!strcmp(keyword, gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data))))
					compat = g_list_remove_link(compat, cp);

			compat = g_list_prepend(compat, dict_parameter);
		}
	}

	return g_list_sort(compat, (GCompareFunc) compare_parameters_by_keyword);
}

static GtkTreeModel *generate_completion_model(struct gebr_gui_parameter_widget *widget)
{
	GebrGeoXmlDocument *flow, *line, *proj;
	gebr_validator_get_documents(widget->validator, &flow, &line, &proj);
	if (gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(widget->parameter)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
		return gebr_gui_parameter_get_completion_model(NULL, line, proj, widget->parameter_type);
	return gebr_gui_parameter_get_completion_model(flow, line, proj, widget->parameter_type);
}

static void
setup_entry_completion(GtkEntry *entry,
		       GtkTreeModel *model,
		       GtkEntryCompletionMatchFunc func,
		       GCallback match_selected_cb,
		       gpointer data)
{
	GtkEntryCompletion *comp;
	GtkCellRenderer *cell;

	comp = gtk_entry_completion_new();
	gtk_entry_completion_set_model(comp, model);
	gtk_entry_completion_set_match_func(comp, func, NULL, NULL);
	g_signal_connect(comp, "match-selected", match_selected_cb, data);

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comp), cell, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comp), cell, "stock-id", 1);

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comp), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comp), cell, "text", 0);

	gtk_entry_set_completion(entry, comp);
	g_object_unref(comp);
}

static gboolean on_spin_button_output(GtkSpinButton *spin,
				      GebrGuiParameterWidget *widget)
{
	gchar *err = NULL;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(spin));

	g_ascii_strtod(text, &err);
	if (*err || !*text)
		return TRUE;

	gdouble d;
	gchar *format;
	const gchar *dig;
	gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	gebr_geoxml_program_parameter_get_range_properties(widget->program_parameter,
							   NULL, NULL, NULL, &dig);

	d = gtk_spin_button_get_value(spin);
	format = g_strconcat("%.", dig, "f", NULL);

	gtk_entry_set_text(GTK_ENTRY(spin),
			   g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, format, d));
	return TRUE;
}

static gint on_spin_button_input(GtkSpinButton *spin,
				 gdouble *rval,
				 GebrGuiParameterWidget *widget)
{
	gchar *err = NULL;
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(spin));

	g_ascii_strtod(text, &err);
	if (*err || !*text) {
		*rval = 0;
		gtk_spin_button_set_range(spin, 1, 0);
		return TRUE;
	} else {
		const gchar *min, *max;
		gebr_geoxml_program_parameter_get_number_min_max(widget->program_parameter,
								 &min, &max);
		gtk_spin_button_set_range(spin, g_ascii_strtod(min, NULL), g_ascii_strtod(max, NULL));
		return FALSE;
	}
}

static gboolean on_list_value_widget_focus_out(GtkEntry *entry, GdkEventFocus *event, GebrGuiParameterWidget *self)
{
	validate_list_value_widget(self);
	return FALSE;
}

static void on_list_value_widget_validate(GtkEntry *entry, struct gebr_gui_parameter_widget *self)
{
	validate_list_value_widget(self);
}

static void
validate_list_value_widget(GebrGuiParameterWidget *self)
{
	GError *error = NULL;
	gchar **exprs;
	const gchar *text;
	const gchar *separator;
	GebrGeoXmlParameterType type;
	GtkEntry *entry;

	entry = GTK_ENTRY(self->list_value_widget);
	type = gebr_geoxml_parameter_get_type(self->parameter);
	separator = gebr_geoxml_program_parameter_get_list_separator(self->program_parameter);
	text = gtk_entry_get_text(entry);

	if (!separator)
		return;

	if (!strlen(separator)) {
		exprs = g_new(gchar *, 2);
		exprs[0] = g_strdup(text);
		exprs[1] = NULL;
	} else
		exprs = g_strsplit(text, separator, -1);

	if (self->group_warning_widget) {
		GebrGeoXmlParameterGroup *group;
		GebrGeoXmlSequence *instance;
		group = gebr_geoxml_parameter_get_group(self->parameter);
		gebr_geoxml_parameter_group_get_instance(group, &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			gebr_gui_group_instance_validate(self->validator, instance, self->group_warning_widget);
	}

	for (int i = 0; exprs[i]; i++) {
		gebr_validator_validate_expr(self->validator, exprs[i], type, &error);
		if (error) {
			gtk_entry_set_icon_from_stock(entry,
						      GTK_ENTRY_ICON_SECONDARY,
						      GTK_STOCK_DIALOG_WARNING);
			gtk_entry_set_icon_tooltip_text(entry,
							GTK_ENTRY_ICON_SECONDARY,
							error->message);
			g_clear_error(&error);
			return;
		}
	}

	gtk_entry_set_icon_from_stock(entry,
				      GTK_ENTRY_ICON_SECONDARY,
				      NULL);
	gtk_entry_set_icon_tooltip_text(entry,
					GTK_ENTRY_ICON_SECONDARY,
					NULL);

	g_strfreev(exprs);
}

/* Public functions {{{1 */
GtkWidget *gebr_gui_parameter_add_variables_popup(GtkEntry *entry,
						  GebrGeoXmlDocument *flow,
						  GebrGeoXmlDocument *line,
						  GebrGeoXmlDocument *proj,
						  GebrGeoXmlParameterType type)
{
	GList *compat;
	GtkWidget *menu;
	GtkWidget *menu_item;

	compat = get_compatible_variables(type, flow, line, proj);
	menu = gtk_menu_new();

	for (GList *i = compat; i; i = i->next)
	{
		GString *label;
		const gchar * keyword;
		const gchar * first_value;
		const gchar * param_label;
		GebrGeoXmlProgramParameter *param;

		param = i->data;
		keyword = gebr_geoxml_program_parameter_get_keyword(param);
		first_value = gebr_geoxml_program_parameter_get_first_value(param, FALSE);
		param_label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(param));

		label = g_string_new(NULL);
		g_string_printf(label, "%s=%s", keyword, first_value);
		if (param_label != NULL && strlen(param_label) > 0)
			g_string_append_printf(label, " (%s)", param_label);

		menu_item = gtk_menu_item_new_with_label(label->str);
		g_object_set_data (G_OBJECT (menu_item), "param-type", GINT_TO_POINTER(type));
		g_object_set_data (G_OBJECT (menu_item), "dict-param", param);
		g_object_set_data (G_OBJECT (menu_item), "entry-widget", entry);
		g_signal_connect(menu_item, "activate", G_CALLBACK(on_variable_parameter_activate), NULL);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

		g_string_free(label, TRUE);
	}

	gtk_widget_show_all(menu);

	return menu;
}

GebrGuiParameterWidget *gebr_gui_parameter_widget_new(GebrGeoXmlParameter *parameter,
						      GebrValidator       *validator,
						      gboolean             use_default_value,
						      gpointer             data)
{
	GebrGuiParameterWidget *self;

	self = g_new(GebrGuiParameterWidget, 1);

	/* GebrGuiValidatable interface implementation */
	self->parent.set_icon = parameter_widget_set_icon;
	self->parent.set_value = parameter_widget_set_value;
	self->parent.get_value = parameter_widget_get_value;

	self->validator = validator;
	self->parameter = parameter;
	self->program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	self->parameter_type = gebr_geoxml_parameter_get_type(parameter);
	self->use_default_value = use_default_value;
	self->readonly = FALSE;
	self->data = data;
	self->callback = NULL;
	self->user_data = NULL;
	self->group_warning_widget = NULL;
	self->widget = gtk_vbox_new(FALSE, 10);
	g_object_weak_ref(G_OBJECT(self->widget), (GWeakNotify) g_free, self);
	g_signal_connect(self->widget, "mnemonic-activate",
			 G_CALLBACK(on_mnemonic_activate), self);

	gebr_gui_parameter_widget_configure(self);

	return self;
}

void gebr_gui_parameter_widget_set_auto_submit_callback(struct gebr_gui_parameter_widget *parameter_widget,
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
	else {
		const gchar *first;
		first = gebr_geoxml_program_parameter_get_first_value(parameter_widget->program_parameter, parameter_widget->use_default_value);
		gebr_gui_parameter_widget_set_non_list_widget_value(parameter_widget, first);
	}
}

gboolean gebr_gui_parameter_widget_validate(GebrGuiParameterWidget *self)
{
	if (self->group_warning_widget) {
		GebrGeoXmlParameterGroup *group;
		GebrGeoXmlSequence *instance;
		group = gebr_geoxml_parameter_get_group(self->parameter);
		gebr_geoxml_parameter_group_get_instance(group, &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			gebr_gui_group_instance_validate(self->validator, instance, self->group_warning_widget);
	}

	if (!__parameter_accepts_expression(self))
		return TRUE;

	return gebr_gui_validatable_widget_validate(GEBR_GUI_VALIDATABLE_WIDGET(self),
						    self->validator,
						    self->parameter);
}

void gebr_gui_parameter_widget_update_list_separator(struct gebr_gui_parameter_widget *parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

void gebr_gui_parameter_widget_reconfigure(struct gebr_gui_parameter_widget *parameter_widget)
{
	parameter_widget->parameter_type =
	    gebr_geoxml_parameter_get_type(parameter_widget->parameter);

	gebr_gui_parameter_widget_configure(parameter_widget);
}


GtkTreeModel *gebr_gui_parameter_get_completion_model(GebrGeoXmlDocument *flow,
						      GebrGeoXmlDocument *line,
						      GebrGeoXmlDocument *proj,
						      GebrGeoXmlParameterType type)
{
	const gchar *keyword;
	const gchar *icon;
	GList *compatible;
	GtkTreeIter iter;
	GtkListStore *store;
	GebrGeoXmlProgramParameter *ppar;

	store = gtk_list_store_new(2,
				   G_TYPE_STRING,
				   G_TYPE_STRING);
	compatible = get_compatible_variables(type, flow, line, proj);

	for (GList *i = compatible; i; i = i->next) {
		ppar = i->data;
		keyword = gebr_geoxml_program_parameter_get_keyword(ppar);
		switch(gebr_geoxml_parameter_get_type(i->data)) {
		case GEBR_GEOXML_PARAMETER_TYPE_STRING: icon = "string-icon"; break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: icon = "real-icon"; break;
		case GEBR_GEOXML_PARAMETER_TYPE_INT: icon = "int-icon"; break;
		default: icon = NULL; break;
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, keyword, 1, icon, -1);
	}

	g_list_free(compatible);

	return GTK_TREE_MODEL(store);
}

void gebr_gui_parameter_set_entry_completion(GtkEntry *entry,
					     GtkTreeModel *model,
					     GebrGeoXmlParameterType type)
{
	setup_entry_completion(entry, model, completion_match_func,
			       G_CALLBACK(on_entry_completion_matched),
			       GINT_TO_POINTER(type));
}

gboolean gebr_gui_group_instance_validate(GebrValidator *validator, GebrGeoXmlSequence *instance, GtkWidget *icon)
{
	gchar *validated;
	gboolean invalid = FALSE;
	GebrGeoXmlSequence *parameter;
	GebrGeoXmlParameter *selected;
	int i = 0;
	gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &parameter, 0);
	selected = gebr_geoxml_parameters_get_selection(GEBR_GEOXML_PARAMETERS(instance));
	if (selected) {
		if (!gebr_validator_validate_param(validator, GEBR_GEOXML_PARAMETER(selected), &validated, NULL))
			i++;
	} else {
		while (parameter) {
			if (!gebr_validator_validate_param(validator, GEBR_GEOXML_PARAMETER(parameter), &validated, NULL))
				i++;
			gebr_geoxml_sequence_next(&parameter);
		}
	}
	if (i) {
		gtk_image_set_from_stock(GTK_IMAGE(icon), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
		gchar *message;
		if (i == 1)
			message = g_strdup_printf(_("This group has %d error"), i);
		else
			message = g_strdup_printf(_("This group has %d errors"), i);
		gtk_widget_set_tooltip_text (icon, message);
	} else
		gtk_image_clear(GTK_IMAGE(icon));

	return invalid;
}
