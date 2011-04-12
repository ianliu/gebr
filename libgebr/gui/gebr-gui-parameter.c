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

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_parameter_widget_find_dict_parameter(struct gebr_gui_parameter_widget *widget);

static GtkWidget *gebr_gui_parameter_widget_dict_popup_menu(struct gebr_gui_parameter_widget *widget);

static GtkWidget *gebr_gui_parameter_widget_variable_popup_menu(struct gebr_gui_parameter_widget *widget,
								GtkEntry *entry);

static void gebr_gui_parameter_widget_value_entry_on_populate_popup(GtkEntry * entry, GtkMenu * menu,
								    struct gebr_gui_parameter_widget *widget);

static gboolean gebr_gui_parameter_widget_can_use_dict(struct gebr_gui_parameter_widget *widget);

static void on_dict_parameter_toggled(GtkMenuItem * menu_item, struct gebr_gui_parameter_widget *widget);

static void on_variable_parameter_activate(GtkMenuItem * menu_item, struct gebr_gui_parameter_widget *widget);

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

static GList *gebr_gui_parameter_widget_get_compatible_dicts(struct gebr_gui_parameter_widget *widget);
static gboolean on_entry_completion_matched (GtkEntryCompletion *completion,

					     GtkTreeModel       *model,
					     GtkTreeIter        *iter,
					     struct gebr_gui_parameter_widget *parameter_widget);

static gboolean completion_number_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data);

static gboolean completion_string_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data);

static GtkTreeModel *generate_completion_model(struct gebr_gui_parameter_widget *parameter_widget);

static void setup_entry_completion(GtkEntry *entry,
				   GtkTreeModel *model,
				   GtkEntryCompletionMatchFunc func,
				   GCallback match_selected_cb,
				   struct gebr_gui_parameter_widget *parameter_widget);

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

static gboolean __parameter_accepts_expression(struct gebr_gui_parameter_widget *parameter_widget)
{
	return parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_INT ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
		parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_STRING;
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

static void gebr_gui_parameter_widget_set_non_list_widget_value(struct gebr_gui_parameter_widget *parameter_widget,
								gchar * value)
{
	if (parameter_widget->dict_parameter != NULL) {
		/* only range parameter still use the old dictionary interface */
		if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
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
		} else {
			/* now parameters don't use an value from dictionary; instead, the dict keyword is transformed
			 * as the parameter value as an expression */
			value = (gchar*)gebr_geoxml_program_parameter_get_keyword(parameter_widget->dict_parameter);
			gebr_geoxml_program_parameter_set_string_value(parameter_widget->program_parameter,
								       parameter_widget->use_default_value, value); 
		}
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
		gebr_gui_file_entry_set_path(GEBR_GUI_FILE_ENTRY(parameter_widget->value_widget),
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

static void gebr_gui_parameter_widget_on_value_widget_changed(GtkWidget * widget,
							      struct gebr_gui_parameter_widget *parameter_widget)
{
	if (parameter_widget->dict_parameter != NULL)
		return;

	gebr_gui_parameter_widget_sync_non_list(parameter_widget);
	gebr_gui_parameter_widget_report_change(parameter_widget);
}

static gboolean gebr_gui_parameter_widget_on_range_changed(GtkSpinButton * spinbutton,
							   struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_on_value_widget_changed(GTK_WIDGET(spinbutton), parameter_widget);
	return FALSE;
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

static void __on_sequence_edit_add_request(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
					   struct gebr_gui_parameter_widget *parameter_widget)
{
	GString *value;
	GebrGeoXmlSequence *sequence;
	GtkListStore *list_store;
	GtkWidget *entry;

	entry = g_object_get_data (G_OBJECT (parameter_widget->gebr_gui_value_sequence_edit), "activatable-entry");
	if (entry) {
		gtk_widget_grab_focus (entry);
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	}

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

static void __on_sequence_edit_changed(GebrGuiSequenceEdit * sequence_edit,
				       struct gebr_gui_parameter_widget *parameter_widget)
{
	__parameter_list_value_widget_update(parameter_widget);
}

/**
 * for parameter that accepts an expression (int and float)
 */
static void __set_type_icon(struct gebr_gui_parameter_widget *parameter_widget)
{
	gboolean has_focus = FALSE;
	g_object_get(parameter_widget->value_widget, "has-focus", &has_focus, NULL);

	if (!has_focus && __parameter_accepts_expression(parameter_widget))
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, NULL);
	else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_INT) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, "integer-icon");
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, 
						_("This parameter uses an integer value."));
	} else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_ZOOM_IN);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, 
						_("This parameter uses a real value."));
	} else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_STRING) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_ZOOM_OUT);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, 
						_("This parameter uses a text value."));
	} else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE && parameter_widget->dict_parameter != NULL) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, "accessories-dictionary");
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), 
						GTK_ENTRY_ICON_SECONDARY, NULL);
	} else
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, NULL);

	if (parameter_widget->dicts == NULL)
		return;

	GError * validation_error = NULL;
	GString *value = gebr_gui_parameter_widget_get_value(parameter_widget);
	gboolean success = gebr_geoxml_document_validate_expr(value->str, 
							      parameter_widget->dicts->flow, 
							      parameter_widget->dicts->line, 
							      parameter_widget->dicts->project, 
							      &validation_error);

	if (!success) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(parameter_widget->value_widget), 
					      GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
		if (validation_error == NULL)
			gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, 
							_("This expression is invalid"));
		else
			gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), GTK_ENTRY_ICON_SECONDARY, 
							(const gchar *)validation_error->message);
	}
}

static void __on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget *parameter_widget)
{
	gebr_gui_parameter_widget_validate(parameter_widget);
}


static gboolean __on_focus_out_event(GtkWidget * widget, GdkEventFocus * event,
				     struct gebr_gui_parameter_widget *parameter_widget)
{
	__set_type_icon(parameter_widget);
	gebr_gui_parameter_widget_validate(parameter_widget);
	return FALSE;
}

static gboolean __on_focus_in_event(GtkWidget * widget, GdkEventFocus * event,
				    struct gebr_gui_parameter_widget *parameter_widget)
{
	__set_type_icon(parameter_widget);
	return FALSE;
}

/*
 * gebr_gui_parameter_widget_configure:
 * Create UI.
 */
static void gebr_gui_parameter_widget_configure(struct gebr_gui_parameter_widget *parameter_widget)
{
	GtkEntry *activatable_entry = NULL;

	gtk_container_foreach(GTK_CONTAINER(parameter_widget->widget), (GtkCallback)gtk_widget_destroy, NULL);

	gboolean may_use_dict = parameter_widget->dicts != NULL && gebr_gui_parameter_widget_can_use_dict(parameter_widget);
	if (may_use_dict) {
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
	}

	switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:{
			GtkWidget *entry;
			GtkTreeModel *completion_model;

			parameter_widget->value_widget = entry = gtk_entry_new();
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_number_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       parameter_widget);
			g_object_unref (completion_model);

			gtk_widget_set_size_request(entry, 120, 30);
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
	case GEBR_GEOXML_PARAMETER_TYPE_INT:{
			GtkWidget *entry;
			GtkTreeModel *completion_model;

			parameter_widget->value_widget = entry = gtk_entry_new();
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_number_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       parameter_widget);
			g_object_unref (completion_model);

			gtk_widget_set_size_request(entry, 120, 30);
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
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:{
			GtkWidget *entry;
			GtkTreeModel *completion_model;

			parameter_widget->value_widget = entry = gtk_entry_new();
			completion_model = generate_completion_model(parameter_widget);
			setup_entry_completion(GTK_ENTRY(entry), completion_model,
					       completion_string_match_func,
					       G_CALLBACK(on_entry_completion_matched),
					       parameter_widget);
			g_object_unref (completion_model);

			activatable_entry = GTK_ENTRY (entry);
			gtk_widget_set_size_request(parameter_widget->value_widget, 140, 30);

			g_signal_connect (parameter_widget->value_widget, "activate",
					  G_CALLBACK (on_entry_activate_add), parameter_widget);
			g_signal_connect (entry, "focus-in-event",
					  G_CALLBACK (__on_focus_in_event), parameter_widget);
			g_signal_connect (entry, "focus-out-event",
					  G_CALLBACK(__on_focus_out_event), parameter_widget);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:{
			if (parameter_widget->dict_parameter == NULL) {
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
				activatable_entry = GTK_ENTRY (spin);
				g_signal_connect (spin, "activate",
						  G_CALLBACK (on_entry_activate_add), parameter_widget);
				gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), atoi(digits_str));
			} else {
				parameter_widget->value_widget = gtk_entry_new();
			}
			gtk_widget_set_size_request(parameter_widget->value_widget, 90, 30);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:{
			GtkWidget *file_entry;

			/* file entry */
			parameter_widget->value_widget = file_entry =
			    gebr_gui_file_entry_new((GebrGuiFileEntryCustomize)
							gebr_gui_parameter_widget_file_entry_customize_function,
							parameter_widget);
			activatable_entry = GTK_ENTRY (GEBR_GUI_FILE_ENTRY (file_entry)->entry);
			g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "activate",
					  G_CALLBACK (on_entry_activate_add), parameter_widget);
			gtk_widget_set_size_request(file_entry, 220, 30);

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

	if (gebr_geoxml_program_parameter_get_is_list(parameter_widget->program_parameter) == TRUE) {
		GtkWidget *hbox;
		GtkWidget *button;
		GtkWidget *sequence_edit;

		hbox = gtk_hbox_new(FALSE, 10);
		if (parameter_widget->parameter_type != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
			gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(parameter_widget->widget), hbox, TRUE, TRUE, 0);

		parameter_widget->list_value_widget = gtk_entry_new();
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
			g_signal_connect(parameter_widget->value_widget, "changed",
					 G_CALLBACK(gebr_gui_parameter_widget_on_value_widget_changed),
					 parameter_widget);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			if (parameter_widget->dict_parameter == NULL)
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

	gebr_gui_parameter_widget_update(parameter_widget);
	gebr_gui_parameter_widget_set_auto_submit_callback(parameter_widget,
							   parameter_widget->callback,
							   parameter_widget->user_data);

	if (may_use_dict) {
		gebr_gui_parameter_widget_find_dict_parameter(parameter_widget);
		g_signal_connect(parameter_widget->value_widget, "populate-popup",
				 G_CALLBACK(gebr_gui_parameter_widget_value_entry_on_populate_popup),
				 parameter_widget);
	}
}

/*
 * gebr_gui_parameter_widget_find_dict_parameter:
 * Find in documents' dictionaries for the associated dictionary parameter
 */
static void gebr_gui_parameter_widget_find_dict_parameter(struct gebr_gui_parameter_widget *widget)
{
	gboolean changed_state = (widget->dict_enabled && widget->dict_parameter == NULL) || (!widget->dict_enabled && widget->dict_parameter != NULL);
	if (changed_state && widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
		widget->dict_enabled = !widget->dict_enabled;
		gebr_gui_parameter_widget_reconfigure(widget);
		return;
	}

	__set_type_icon(widget);
}

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
 * gebr_gui_parameter_widget_dict_popup_menu:
 * Read dictionaries and build a popup menu
 */
static GtkWidget *gebr_gui_parameter_widget_dict_popup_menu(struct gebr_gui_parameter_widget *widget)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GList *cp = NULL;
	GSList *group = NULL;
	GList *compatible_parameters = NULL;

	compatible_parameters = gebr_gui_parameter_widget_get_compatible_dicts(widget);
	menu = gtk_menu_new();

	menu_item = gtk_radio_menu_item_new_with_label(NULL, _("Do not use dictionary"));
	g_object_set(menu_item, "user-data", NULL, NULL);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_dict_parameter_toggled), widget);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp)) {
		GString *label;
		const gchar * keyword;
		const gchar * first_value;
		const gchar * param_label;

		keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data));
		first_value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data), FALSE);
		param_label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(cp->data));

		label = g_string_new(NULL);
		g_string_printf(label, "%s=%s", keyword, first_value);
		if (param_label != NULL && strlen(param_label) > 0)
			g_string_append_printf(label, " (%s)", param_label);

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

/*
 * gebr_gui_parameter_widget_variable_popup_menu:
 * Read dictionaries variables and build a popup menu to build expressions
 */
static GtkWidget *gebr_gui_parameter_widget_variable_popup_menu(struct gebr_gui_parameter_widget *widget,
								GtkEntry *entry)
{
	GList *cp;
	GList *compatible_parameters;
	GtkWidget *menu;
	GtkWidget *menu_item;

	compatible_parameters = gebr_gui_parameter_widget_get_compatible_dicts(widget);
	menu = gtk_menu_new();

	menu_item = gtk_menu_item_new();
	g_object_set(menu_item, "user-data", NULL, NULL);

	compatible_parameters = g_list_sort(compatible_parameters, (GCompareFunc) compare_parameters_by_keyword);
	for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp)) {
		GString *label;
		const gchar * keyword;
		const gchar * first_value;
		const gchar * param_label;

		keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data));
		first_value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data), FALSE);
		param_label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(cp->data));

		label = g_string_new(NULL);
		g_string_printf(label, "%s=%s", keyword, first_value);
		if (param_label != NULL && strlen(param_label) > 0)
			g_string_append_printf(label, " (%s)", param_label);

		menu_item = gtk_menu_item_new_with_label(label->str);
		g_object_set(menu_item, "user-data", cp->data, NULL);
		g_object_set_data (G_OBJECT (menu_item), "entry-widget", entry);
		g_signal_connect(menu_item, "activate", G_CALLBACK(on_variable_parameter_activate), widget);
		gtk_container_add(GTK_CONTAINER(menu), menu_item);

		g_string_free(label, TRUE);
	}

	gtk_widget_show_all(menu);
	g_list_free(compatible_parameters);

	return menu;
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

	if (widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
		menu_item = gtk_menu_item_new_with_label(_("Dictionary"));
		gtk_widget_show(menu_item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), gebr_gui_parameter_widget_dict_popup_menu(widget));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
	} else if (__parameter_accepts_expression(widget)) {
		menu_item = gtk_separator_menu_item_new();
		gtk_widget_show(menu_item);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(_("Insert Variable"));
		gtk_widget_show(menu_item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), gebr_gui_parameter_widget_variable_popup_menu(widget, entry));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
	}

}
/*
 * gebr_gui_parameter_widget_can_use_dict:
 * Return TRUE if parameter is of an compatible type to use an dictionary value
 */
static gboolean gebr_gui_parameter_widget_can_use_dict(struct gebr_gui_parameter_widget *widget)
{
	if (gebr_geoxml_program_get_control(gebr_geoxml_parameter_get_program(widget->parameter)) != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return FALSE;

	switch (widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		return !gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter));
	default:
		return FALSE;
	}
}

/*
 * on_dict_parameter_toggled:
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

/*
 * on_variable_parameter_activate:
 * Use value of dictionary parameter corresponding to menu_item in parameter at _widget_
 */
static void on_variable_parameter_activate(GtkMenuItem * menu_item, struct gebr_gui_parameter_widget *widget)
{
	GtkEntry *entry;
	GebrGeoXmlProgramParameter *dict_parameter;
	gint position;

	g_object_get(menu_item, "user-data", &dict_parameter, NULL);
	entry = g_object_get_data (G_OBJECT (menu_item), "entry-widget");
	position = gtk_editable_get_position(GTK_EDITABLE(entry));
	gtk_editable_insert_text(GTK_EDITABLE(entry), gebr_geoxml_program_parameter_get_keyword(dict_parameter), -1, &position);
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

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

struct gebr_gui_parameter_widget *gebr_gui_parameter_widget_new(GebrGeoXmlParameter * parameter,
								struct gebr_gui_gebr_gui_program_edit_dicts *dicts,
								gboolean use_default_value,
								gpointer data)
{
	struct gebr_gui_parameter_widget *parameter_widget;

	parameter_widget = g_new(struct gebr_gui_parameter_widget, 1);
	parameter_widget->parameter = parameter;
	parameter_widget->program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	parameter_widget->parameter_type = gebr_geoxml_parameter_get_type(parameter);
	parameter_widget->use_default_value = use_default_value;
	parameter_widget->readonly = FALSE;
	parameter_widget->data = data;
	parameter_widget->dict_enabled = FALSE;
	parameter_widget->dict_parameter = NULL;
	parameter_widget->dicts = dicts;
	parameter_widget->callback = NULL;
	parameter_widget->user_data = NULL;
	parameter_widget->widget = gtk_vbox_new(FALSE, 10);
	g_object_weak_ref(G_OBJECT(parameter_widget->widget), (GWeakNotify) g_free, parameter_widget);
	g_signal_connect(parameter_widget->widget, "mnemonic-activate",
			 G_CALLBACK(on_mnemonic_activate), parameter_widget);

	gebr_gui_parameter_widget_configure(parameter_widget);

	return parameter_widget;
}

void gebr_gui_parameter_widget_set_dicts(struct gebr_gui_parameter_widget *parameter_widget,
					 struct gebr_gui_gebr_gui_program_edit_dicts *dicts)
{
	parameter_widget->dicts = dicts;
	gebr_gui_parameter_widget_reconfigure(parameter_widget);
}

GString *gebr_gui_parameter_widget_get_value(struct gebr_gui_parameter_widget *parameter_widget)
{
	return gebr_gui_parameter_widget_get_widget_value_full(parameter_widget, TRUE);
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
	else
		gebr_gui_parameter_widget_set_non_list_widget_value(parameter_widget,
								    (gchar*)gebr_geoxml_program_parameter_get_first_value
								    (parameter_widget->program_parameter,
								     parameter_widget->use_default_value));
}

void gebr_gui_parameter_widget_validate(struct gebr_gui_parameter_widget *parameter_widget)
{
	if (__parameter_accepts_expression(parameter_widget)) {
		if (parameter_widget->dicts) {
			GError * error = NULL;
			GString *value = gebr_gui_parameter_widget_get_value(parameter_widget);
			gboolean success = gebr_geoxml_document_validate_expr(value->str, 
									      parameter_widget->dicts->flow, 
									      parameter_widget->dicts->line, 
									      parameter_widget->dicts->project, 
									      &error);

			if (!success)
			{
				if (error != NULL)
				{
					gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), 
									GTK_ENTRY_ICON_SECONDARY, 
									error->message);
					g_error_free(error);
				}
				else
					gtk_entry_set_icon_tooltip_text(GTK_ENTRY(parameter_widget->value_widget), 
									GTK_ENTRY_ICON_SECONDARY, 
									"This expression is invalid");
			}

			g_string_free(value, TRUE);
		}
	} else if (parameter_widget->dict_parameter != NULL)
		return;
	else switch (parameter_widget->parameter_type) {
	case GEBR_GEOXML_PARAMETER_TYPE_INT: {
		const gchar *min, *max;
		GtkEntry *entry = GTK_ENTRY(parameter_widget->value_widget);

		gebr_geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
		gtk_entry_set_text(entry, gebr_validate_int(gtk_entry_get_text(entry), min, max));
		break;
	} case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: {
		const gchar *min, *max;
		GtkEntry *entry = GTK_ENTRY(parameter_widget->value_widget);

		gebr_geoxml_program_parameter_get_number_min_max(parameter_widget->program_parameter, &min, &max);
		gtk_entry_set_text(entry, gebr_validate_float(gtk_entry_get_text(entry), min, max));
		break;
	} default:
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

	gebr_gui_parameter_widget_configure(parameter_widget);
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
					     struct gebr_gui_parameter_widget *parameter_widget)
{
	GtkWidget * entry;
	const gchar * entry_text;
	gint position;
	gchar * var;
	gchar * word;
	gint ini;


	entry = gtk_entry_completion_get_entry(completion);
	entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
	position = gtk_editable_get_position(GTK_EDITABLE(entry)) - 1;
	ini = position;
	gtk_tree_model_get(model, iter, 0, &var, -1);
	word = gebr_str_word_before_pos(entry_text, (gsize *)&ini);


	if (!word)
		ini = position;
	if (ini <= position)
		gtk_editable_delete_text(GTK_EDITABLE(entry), ini, position + 1);

	if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
	    parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_INT){
		gtk_editable_insert_text(GTK_EDITABLE(entry), var, -1, &ini);
		gtk_editable_set_position(GTK_EDITABLE(entry), ini + strlen(var));
	}
	else if (parameter_widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_STRING){
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

static gboolean completion_number_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data)
{
	return TRUE;
}

static gboolean completion_string_match_func(GtkEntryCompletion *completion,
					     const gchar *key,
					     GtkTreeIter *iter,
					     gpointer user_data)
{
	return TRUE;
}

static GList *
gebr_gui_parameter_widget_get_compatible_dicts(struct gebr_gui_parameter_widget *widget)
{
	GList *compatible_parameters = NULL;
	GList *cp = NULL;

	GebrGeoXmlParameterType compatibles_types[5] = {
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN,
		GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN
	};

	GebrGeoXmlDocument *documents[4] = {
		widget->dicts->project,
		widget->dicts->line,
		widget->dicts->flow,
		NULL
	};

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
			for (cp = compatible_parameters; cp != NULL; cp = g_list_next(cp))
				if (!strcmp(keyword, gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(cp->data))))
					compatible_parameters = g_list_remove_link(compatible_parameters, cp);

			compatible_parameters = g_list_prepend(compatible_parameters, dict_parameter);
		}
	}

	return g_list_sort(compatible_parameters, (GCompareFunc) compare_parameters_by_keyword);
}

static GtkTreeModel *generate_completion_model(struct gebr_gui_parameter_widget *widget)
{
	const gchar *keyword;
	GList *compatible;
	GtkTreeIter iter;
	GtkListStore *store;
	GebrGeoXmlProgramParameter *ppar;

	store = gtk_list_store_new(1, G_TYPE_STRING);
	compatible = gebr_gui_parameter_widget_get_compatible_dicts(widget);
	for (GList *i = compatible; i; i = i->next) {
		ppar = i->data;
		keyword = gebr_geoxml_program_parameter_get_keyword(ppar);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, ppar, -1);
	}

	g_list_free(compatible);

	return GTK_TREE_MODEL(store);
}

static void setup_entry_completion(GtkEntry *entry,
				   GtkTreeModel *model,
				   GtkEntryCompletionMatchFunc func,
				   GCallback match_selected_cb,
				   struct gebr_gui_parameter_widget *parameter_widget)
{
	GtkEntryCompletion *comp;

	comp = gtk_entry_completion_new();
	gtk_entry_completion_set_model(comp, model);
	gtk_entry_completion_set_text_column(comp, 0);
	gtk_entry_completion_set_match_func(comp, func, NULL, NULL);
	g_signal_connect(comp, "match-selected", match_selected_cb, NULL);
	gtk_entry_set_completion(entry, comp);
	g_object_unref(comp);
}
