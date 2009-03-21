/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <gui/valuesequenceedit.h>

#include "parameter.h"
#include "debr.h"
#include "callbacks.h"
#include "enumoptionedit.h"
#include "interface.h"
#include "menu.h"
#include "parametergroup.h"

/*
 * File: parameter.c
 * Construct interfaces for parameter
 */

/*
 * Declarations
 */

/* bidirecional relation of combo box index and type enumerations */
struct {
	enum GEOXML_PARAMETERTYPE	type;
	guint				index;
	gchar *				title;
} combo_type_map [] = {
	{GEOXML_PARAMETERTYPE_FLOAT, 0, _("real")},
	{GEOXML_PARAMETERTYPE_INT, 1, _("integer")},
	{GEOXML_PARAMETERTYPE_RANGE, 2, _("range")},
	{GEOXML_PARAMETERTYPE_FLAG, 3, _("flag")},
	{GEOXML_PARAMETERTYPE_STRING, 4, _("text")},
	{GEOXML_PARAMETERTYPE_ENUM, 5, _("enum")},
	{GEOXML_PARAMETERTYPE_FILE, 6, _("file")},
	{GEOXML_PARAMETERTYPE_GROUP, 7, _("group")},
};
const gsize	combo_type_map_size = 8;

int
combo_type_map_get_index(enum GEOXML_PARAMETERTYPE type)
{
	int i;
	for (i = 0; i < combo_type_map_size; ++i)
		if (combo_type_map[i].type == type)
			return combo_type_map[i].index;
	return -1;
}

#define combo_type_map_get_type(index) \
	combo_type_map[index].type

#define combo_type_map_get_title(type) \
	combo_type_map[combo_type_map_get_index(type)].title

/* same order as combo_box_map */
const GtkRadioActionEntry parameter_type_radio_actions_entries [] = {
	{"parameter_type_real", NULL, _("real"), NULL, NULL, GEOXML_PARAMETERTYPE_FLOAT},
	{"parameter_type_integer", NULL, _("integer"), NULL, NULL, GEOXML_PARAMETERTYPE_INT},
	{"parameter_type_range", NULL, _("range"), NULL, NULL, GEOXML_PARAMETERTYPE_RANGE},
	{"parameter_type_flag", NULL, _("flag"), NULL, NULL, GEOXML_PARAMETERTYPE_FLAG},
	{"parameter_type_text", NULL, _("text"), NULL, NULL, GEOXML_PARAMETERTYPE_STRING},
	{"parameter_type_enum", NULL, _("enum"), NULL, NULL, GEOXML_PARAMETERTYPE_ENUM},
	{"parameter_type_file", NULL, _("file"), NULL, NULL, GEOXML_PARAMETERTYPE_FILE},
	{"parameter_type_group", NULL, _("group"), NULL, NULL, GEOXML_PARAMETERTYPE_GROUP},
};

static GtkTreeIter
parameter_append_to_ui(GeoXmlParameter * parameter, GtkTreeIter * parent);
static void
parameter_load_iter(GtkTreeIter * iter);
static void
parameter_select_iter(GtkTreeIter iter);
static gboolean
parameter_check_selected(gboolean show_warning);
static gboolean
parameter_get_selected(GtkTreeIter * iter, gboolean show_warning);
static void
parameter_selected(void);
static void
parameter_activated(void);
static GtkMenu *
parameter_popup_menu(GtkWidget * tree_view);
static gboolean
parameter_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position);
static gboolean
parameter_can_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position);

static void
parameter_default_widget_changed(struct parameter_widget * widget);
static void
parameter_required_toggled(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui);
static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui);
static void
parameter_separator_changed(GtkEntry * entry, struct ui_parameter_dialog * ui);
static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_widget * widget);
static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_widget * widget);
static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_widget * widget);
static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_widget * widget);
static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_widget * widget);
static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct ui_parameter_dialog * ui);

/*
 * Section: Public
 */

/*
 * Function: parameter_setup_ui
 * Set interface and its callbacks
 */
void
parameter_setup_ui(void)
{
	GtkWidget *		scrolled_window;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	debr.ui_parameter.tree_store = gtk_tree_store_new(PARAMETER_N_COLUMN,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_POINTER);
	debr.ui_parameter.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_parameter.tree_store));
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(debr.ui_parameter.tree_view),
		(GtkPopupCallback)parameter_popup_menu, NULL);
	gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(debr.ui_parameter.tree_view),
		(GtkTreeViewReorderCallback)parameter_reorder,
		(GtkTreeViewReorderCallback)parameter_can_reorder, NULL);
	gtk_widget_show(debr.ui_parameter.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_parameter.tree_view);
	g_signal_connect(debr.ui_parameter.tree_view, "cursor-changed",
		(GCallback)parameter_selected, NULL);
	g_signal_connect(debr.ui_parameter.tree_view, "row-activated",
		GTK_SIGNAL_FUNC(parameter_activated), NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PARAMETER_TYPE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_parameter.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Keyword"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PARAMETER_KEYWORD);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_parameter.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Label"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PARAMETER_LABEL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_parameter.tree_view), col);

	debr.ui_parameter.widget = scrolled_window;
	gtk_widget_show_all(debr.ui_parameter.widget);
}

/*
 * Function: parameter_load_program
 * Load current program parameters' to the UI
 */
void
parameter_load_program(void)
{
	GeoXmlSequence *		parameter;

	gtk_tree_store_clear(debr.ui_parameter.tree_store);

	geoxml_parameters_get_parameter(geoxml_program_get_parameters(debr.program), &parameter, 0);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		GtkTreeIter		iter;

		iter = parameter_append_to_ui(GEOXML_PARAMETER(parameter), NULL);

		if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == FALSE) {
			GeoXmlSequence *	first_instance;
			GeoXmlSequence *	group_parameter;

			geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &first_instance, 0);
			geoxml_parameters_get_parameter(GEOXML_PARAMETERS(first_instance), &group_parameter, 0);
			for (; group_parameter != NULL; geoxml_sequence_next(&group_parameter))
				parameter_append_to_ui(GEOXML_PARAMETER(group_parameter), &iter);
		}
	}

	debr.parameter = NULL;
}

/*
 * Function: parameter_load_selected
 * Load selected parameter contents to its iter
 */
void
parameter_load_selected(void)
{
	GtkTreeIter	iter;

	parameter_get_selected(&iter, FALSE);
	parameter_load_iter(&iter);
}

/*
 * Function: parameter_new
 * Append a new parameter
 */
void
parameter_new(void)
{
	GtkTreeIter		iter;

	if (parameter_get_selected(&iter, FALSE) &&
	(geoxml_parameter_get_is_program_parameter(debr.parameter) == FALSE ||
	geoxml_parameter_get_is_in_group(debr.parameter) == TRUE)) {
		GeoXmlParameterGroup *	parameter_group;
		GeoXmlSequence *	first_instance;
		GtkTreeIter		parent;
		GtkTreePath *		tree_path;

		if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, &iter)) {
			parameter_group = GEOXML_PARAMETER_GROUP(debr.parameter);
			parent = iter;
		} else
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent,
				PARAMETER_XMLPOINTER, &parameter_group,
				-1);

		geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
		iter = parameter_append_to_ui(
			geoxml_parameters_append_parameter(GEOXML_PARAMETERS(first_instance),
				GEOXML_PARAMETERTYPE_FLOAT),
			&parent);
		parameter_load_iter(&parent);
		tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(debr.ui_parameter.tree_view), tree_path, FALSE);
		gtk_tree_path_free(tree_path);
	} else {
		iter = parameter_append_to_ui(
			geoxml_parameters_append_parameter(
				geoxml_program_get_parameters(debr.program),
				GEOXML_PARAMETERTYPE_FLOAT),
			NULL);
	}

	parameter_select_iter(iter);
	parameter_change_type_setup_ui();
	parameter_activated();
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_remove
 * Confirm action and if confirmed removed selected parameter from XML and UI
 */
void
parameter_remove(void)
{
	GtkTreeIter		parent;
	GtkTreeIter		iter;
	gboolean		in_group;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return;
	if (confirm_action_dialog(_("Delete parameter"), _("Are you sure you want to delete parameter '%s'?"),
	geoxml_parameter_get_label(debr.parameter)) == FALSE)
		return;

	in_group = gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, &iter);

	geoxml_sequence_remove(GEOXML_SEQUENCE(debr.parameter));
	gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_parameter.tree_view));
	gtk_tree_store_remove(debr.ui_parameter.tree_store, &iter);
	debr.parameter = NULL;

	if (in_group)
		parameter_load_iter(&parent);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_duplicate
 * Append a duplicated parameter
 */
void
parameter_duplicate(void)
{
	GeoXmlParameter *		parameter;

	if (debr.parameter != NULL && geoxml_parameter_get_is_program_parameter(debr.parameter) == FALSE) {
		GeoXmlSequence *	first_instance;

		geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(debr.parameter), &first_instance, 0);
		parameter = geoxml_parameters_append_parameter(GEOXML_PARAMETERS(first_instance),
			GEOXML_PARAMETERTYPE_STRING);
	} else {
		parameter = GEOXML_PARAMETER(geoxml_sequence_append_clone(GEOXML_SEQUENCE(debr.parameter)));
	}
	parameter_select_iter(parameter_append_to_ui(parameter, NULL));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_top
 * Move the current parameter to top
 */
void
parameter_top(void)
{
	GtkTreeIter	iter;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return;

	geoxml_sequence_move_after(GEOXML_SEQUENCE(debr.parameter), NULL);
	gtk_tree_store_move_after(debr.ui_parameter.tree_store, &iter, NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_bottom
 * Move the current parameter to bottom
 */
void
parameter_bottom(void)
{
	GtkTreeIter	iter;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return;

	geoxml_sequence_move_before(GEOXML_SEQUENCE(debr.parameter), NULL);
	gtk_tree_store_move_before(debr.ui_parameter.tree_store, &iter, NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_change_type_setup_ui
 * Open dialog to change type of current selected parameter
 */
void
parameter_change_type_setup_ui(void)
{
	GtkWidget *	dialog;
	GtkWidget *	table;
	GtkWidget *	type_label;
	GtkWidget *	type_combo;

	GtkTreeIter	iter;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return;

	dialog = gtk_dialog_new_with_buttons(_("Parameter's type"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 300, 100);

	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	type_label = gtk_label_new(_("Type:"));
	gtk_widget_show(type_label);
	gtk_table_attach(GTK_TABLE(table), type_label, 0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(type_label), 0, 0.5);

	type_combo = gtk_combo_box_new_text();
	gtk_widget_show(type_combo);
	gtk_table_attach(GTK_TABLE(table), type_combo, 1, 2, 0, 1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[0].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[1].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[2].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[3].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[4].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[5].title);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[6].title);
	/* does not allow group-in-group */
	if (geoxml_parameter_get_is_program_parameter(debr.parameter) == TRUE &&
	geoxml_parameter_get_is_in_group(debr.parameter) == FALSE) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[7].title);
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), combo_type_map_get_index(
			geoxml_parameter_get_type(debr.parameter)));
	} else
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);

	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	parameter_change_type(combo_type_map_get_type(gtk_combo_box_get_active(GTK_COMBO_BOX(type_combo))));
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	gtk_widget_destroy(dialog);
}

/*
 * Function: parameter_paste
 * Paste debr.clipboard
 */
void
parameter_paste(void)
{
	if (geoxml_object_get_type(GEOXML_OBJECT(debr.clipboard)) != GEOXML_OBJECT_TYPE_PARAMETER) {
		debr_message(LOG_ERROR, _("Clipboard doesn't keep a parameter"));
		return;
	}

	GeoXmlSequence *	copy;
	GtkTreeIter		copy_iter;
	GtkTreeIter		iter;

	copy = geoxml_sequence_copy(GEOXML_SEQUENCE(debr.parameter), debr.clipboard);
	if (copy == NULL) {
		debr_message(LOG_ERROR, _("Could not paste parameter"));
		return;
	}

	if (debr.parameter == NULL) {
		gint		n_children;

		if (!(n_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_parameter.tree_store), NULL))) {
			copy_iter = parameter_append_to_ui(GEOXML_PARAMETER(copy), NULL);
			goto out;
		}

		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(debr.ui_parameter.tree_store),
			&iter, NULL, n_children-1);
	} else
		parameter_get_selected(&iter, FALSE);

	gtk_tree_store_insert_after(debr.ui_parameter.tree_store, &copy_iter, NULL, &iter);
	gtk_tree_store_set(debr.ui_parameter.tree_store, &copy_iter,
		PARAMETER_XMLPOINTER, copy,
		-1);
	parameter_load_iter(&copy_iter);

out:	parameter_select_iter(copy_iter);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_change_type
 * Change type of selected parameter to _type_
 */
void
parameter_change_type(enum GEOXML_PARAMETERTYPE type)
{
	if (parameter_check_selected(TRUE) == FALSE)
		return;

	const gchar *	keyword;

	/* save keyword */
	keyword = (geoxml_parameter_get_is_program_parameter(debr.parameter))
		? geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(debr.parameter)) : NULL;

	geoxml_parameter_set_type(debr.parameter, type);

	/* restore keyword */
	if (geoxml_parameter_get_is_program_parameter(debr.parameter))
		geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(debr.parameter), keyword);

	parameter_load_selected();
}

/*
 * Function: parameter_dialog_setup_ui
 * Create an dialog to configure the current selected parameter (not a group)
 */
void
parameter_dialog_setup_ui(void)
{
	struct ui_parameter_dialog *	ui;
	enum GEOXML_PARAMETERTYPE	type;

	GtkWidget *			dialog;
	GtkWidget *			scrolled_window;
	GtkWidget *			table;
	int				row = 0;

	GtkWidget *			label_label;
	GtkWidget *			label_entry;
	GtkWidget *			keyword_label;
	GtkWidget *			keyword_entry;
	GtkWidget *			required_label;
	GtkWidget *			required_check_button;
	GtkWidget *			is_list_label;
	GtkWidget *			is_list_check_button;
	GtkWidget *			separator_label;
	GtkWidget *			separator_entry;
	GtkWidget *			default_label;
	GtkWidget *			default_widget;
	GtkWidget *			default_widget_hbox;

	GeoXmlProgramParameter *	program_parameter;
	struct parameter_widget *	parameter_widget;

	if (parameter_check_selected(TRUE) == FALSE)
		return;

	program_parameter = GEOXML_PROGRAM_PARAMETER(debr.parameter);
	type = geoxml_parameter_get_type(debr.parameter);
	ui = g_malloc(sizeof(struct ui_parameter_dialog));

	/*
	 * Dialog and table
	 */
	ui->dialog = dialog = gtk_dialog_new_with_buttons(_("Edit parameter"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 530, 400);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	table = gtk_table_new(15, 2, FALSE);
	gtk_widget_show(table);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/*
	 * Label
	 */
	label_label = gtk_label_new(_("Label:"));
	gtk_widget_show(label_label);
	gtk_table_attach(GTK_TABLE(table), label_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);

	label_entry = gtk_entry_new();
	gtk_widget_show(label_entry);
	gtk_table_attach(GTK_TABLE(table), label_entry, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	g_signal_connect(label_entry, "changed",
		(GCallback)menu_saved_status_set_unsaved, NULL);

	/*
	 * Keyword
	 */
	keyword_label = gtk_label_new(_("Keyword:"));
	gtk_widget_show(keyword_label);
	gtk_table_attach(GTK_TABLE(table), keyword_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(keyword_label), 0, 0.5);

	keyword_entry = gtk_entry_new();
	gtk_widget_show(keyword_entry);
	gtk_table_attach(GTK_TABLE(table), keyword_entry, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	g_signal_connect(keyword_entry, "changed",
		(GCallback)menu_saved_status_set_unsaved, NULL);

	/* skip default */
	++row;

	/*
	 * Required
	 */
	required_label = gtk_label_new(_("Required:"));
	gtk_widget_show(required_label);
	gtk_table_attach(GTK_TABLE(table), required_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(required_label), 0, 0.5);

	required_check_button = gtk_check_button_new();
	gtk_widget_show(required_check_button);
	gtk_table_attach(GTK_TABLE(table), required_check_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	g_signal_connect(required_check_button, "toggled",
		(GCallback)parameter_required_toggled, ui);

	/*
	 * Is List
	 */
	is_list_label = gtk_label_new(_("Is list?:"));
	gtk_widget_show(is_list_label);
	gtk_table_attach(GTK_TABLE(table), is_list_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(is_list_label), 0, 0.5);

	is_list_check_button = gtk_check_button_new();
	gtk_widget_show(is_list_check_button);
	gtk_table_attach(GTK_TABLE(table), is_list_check_button, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	g_signal_connect(is_list_check_button, "toggled",
		(GCallback)parameter_is_list_changed, ui);

	/*
	 * List separator
	 */
	separator_label = gtk_label_new(_("List separator:"));
	gtk_widget_show(separator_label);
	gtk_table_attach(GTK_TABLE(table), separator_label, 0, 1, row, row+1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(separator_label), 0, 0.5);

	ui->separator_entry = separator_entry = gtk_entry_new();
	gtk_widget_show(separator_entry);
	gtk_table_attach(GTK_TABLE(table), separator_entry, 1, 2, row, row+1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0), ++row;
	g_signal_connect(separator_entry, "changed",
		(GCallback)parameter_separator_changed, ui);

	parameter_widget = parameter_widget_new(debr.parameter, TRUE, NULL);
	switch (type) {
	case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		type_label;
		GtkWidget *		type_combo;

		/*
		 * Type
		 */
		type_label = gtk_label_new(_("Type:"));
		gtk_widget_show(type_label);
		gtk_table_attach(GTK_TABLE(table), type_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(type_label), 0, 0.5);

		type_combo = gtk_combo_box_new_text();
		gtk_widget_show(type_combo);
		gtk_table_attach(GTK_TABLE(table), type_combo, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("File"));
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("Directory"));
		g_signal_connect(type_combo, "changed",
			(GCallback)parameter_file_type_changed, parameter_widget);

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

		/*
		 * Minimum
		 */
		min_label = gtk_label_new(_("Minimum:"));
		gtk_widget_show(min_label);
		gtk_table_attach(GTK_TABLE(table), min_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(min_label), 0, 0.5);
		min_entry = gtk_entry_new();
		gtk_widget_show(min_entry);
		gtk_table_attach(GTK_TABLE(table), min_entry, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(min_entry, "changed",
			(GCallback)parameter_range_min_changed, parameter_widget);

		/*
		 * Maximum
		 */
		max_label = gtk_label_new(_("Maximum:"));
		gtk_widget_show(max_label);
		gtk_table_attach(GTK_TABLE(table), max_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(max_label), 0, 0.5);
		max_entry = gtk_entry_new();
		gtk_widget_show(max_entry);
		gtk_table_attach(GTK_TABLE(table), max_entry, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(max_entry, "changed",
			(GCallback)parameter_range_max_changed, parameter_widget);

		/*
		 * Increment
		 */
		inc_label = gtk_label_new(_("Increment:"));
		gtk_widget_show(inc_label);
		gtk_table_attach(GTK_TABLE(table), inc_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(inc_label), 0, 0.5);
		inc_entry = gtk_entry_new();
		gtk_widget_show(inc_entry);
		gtk_table_attach(GTK_TABLE(table), inc_entry, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(inc_entry, "changed",
			(GCallback)parameter_range_inc_changed, parameter_widget);

		/*
		 * Digits
		 */
		digits_label = gtk_label_new(_("Digits:"));
		gtk_widget_show(digits_label);
		gtk_table_attach(GTK_TABLE(table), digits_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(digits_label), 0, 0.5);
		digits_entry = gtk_entry_new();
		gtk_widget_show(digits_entry);
		gtk_table_attach(GTK_TABLE(table), digits_entry, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(digits_entry, "changed",
			(GCallback)parameter_range_digits_changed, parameter_widget);

		gtk_entry_set_text(GTK_ENTRY(min_entry), min_str);
		gtk_entry_set_text(GTK_ENTRY(max_entry), max_str);
		gtk_entry_set_text(GTK_ENTRY(inc_entry), inc_str);
		gtk_entry_set_text(GTK_ENTRY(digits_entry), digits_str);

		break;
	} case GEOXML_PARAMETERTYPE_ENUM: {
		GtkWidget *		enum_option_edit;
		GtkWidget *		options_label;
		GtkWidget *		vbox;

		GeoXmlSequence *	option;

		/*
		 * Options
		 */
		options_label = gtk_label_new(_("Options:"));
		gtk_widget_show(options_label);
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_box_pack_start(GTK_BOX(vbox), options_label, FALSE, FALSE, 0);
		gtk_table_attach(GTK_TABLE(table), vbox, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(options_label), 0, 0.5);

		geoxml_program_parameter_get_enum_option(GEOXML_PROGRAM_PARAMETER(debr.parameter), &option, 0);
		enum_option_edit = enum_option_edit_new(GEOXML_ENUM_OPTION(option),
			GEOXML_PROGRAM_PARAMETER(debr.parameter));
		gtk_widget_show(enum_option_edit);
		gtk_table_attach(GTK_TABLE(table), enum_option_edit, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(GTK_OBJECT(enum_option_edit), "changed",
			GTK_SIGNAL_FUNC(parameter_enum_options_changed), ui);

		break;
	} default: break;
	}

	ui->parameter_widget = parameter_widget;

	/*
	 * Default value
	 */
	default_label = gtk_label_new(_("Default value:"));
	gtk_widget_show(default_label);
	gtk_table_attach(GTK_TABLE(table), default_label, 0, 1, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(default_label), 0, 0.5);

	/* the hbox is used to make possible to replace the widget inside, keeping it
	 * on the same position on the table */
	ui->default_widget_hbox = default_widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(default_widget_hbox);
	default_widget = parameter_widget->widget;
	gtk_widget_show(default_widget);
	gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), default_widget, TRUE, TRUE, 0);
	gtk_table_attach(GTK_TABLE(table), default_widget_hbox, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	parameter_widget_set_auto_submit_callback(parameter_widget,
		(changed_callback)parameter_default_widget_changed, NULL);

	/* parameter -> UI */
	gtk_entry_set_text(GTK_ENTRY(label_entry), geoxml_parameter_get_label(debr.parameter));
	gtk_entry_set_text(GTK_ENTRY(keyword_entry),
		geoxml_program_parameter_get_keyword(program_parameter));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(required_check_button),
		geoxml_program_parameter_get_required(program_parameter));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(is_list_check_button),
		geoxml_program_parameter_get_is_list(program_parameter));
	gtk_widget_set_sensitive(separator_entry, geoxml_program_parameter_get_is_list(program_parameter));
	if (geoxml_program_parameter_get_is_list(program_parameter) == TRUE)
		gtk_entry_set_text(GTK_ENTRY(separator_entry),
			geoxml_program_parameter_get_list_separator(program_parameter));

	gtk_dialog_run(GTK_DIALOG(dialog));

	/* save not automatically synced changes */
	geoxml_parameter_set_label(debr.parameter, gtk_entry_get_text(GTK_ENTRY(label_entry)));
	geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_entry_get_text(GTK_ENTRY(keyword_entry)));
	geoxml_program_parameter_set_required(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(required_check_button)));
	geoxml_program_parameter_set_be_list(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(is_list_check_button)));
	if (geoxml_program_parameter_get_is_list(program_parameter) == TRUE)
		geoxml_program_parameter_set_list_separator(GEOXML_PROGRAM_PARAMETER(debr.parameter),
			gtk_entry_get_text(GTK_ENTRY(separator_entry)));

	parameter_load_selected();

	/* frees */
	gtk_widget_destroy(ui->dialog);
	g_free(ui);
}

/*
 * Section: Private
 */

/*
 * Function: parameter_append_to_ui
 * Create an item for _parameter_ on parameter tree view
 */
static GtkTreeIter
parameter_append_to_ui(GeoXmlParameter * parameter, GtkTreeIter * parent)
{
	GtkTreeIter	iter;

	gtk_tree_store_append(debr.ui_parameter.tree_store, &iter, parent);
	gtk_tree_store_set(debr.ui_parameter.tree_store, &iter,
		PARAMETER_XMLPOINTER, parameter,
		-1);
	parameter_load_iter(&iter);

	return iter;
}

/*
 * Function: parameter_load_iter
 * Load _iter_ columns from its parameter
 */
static void
parameter_load_iter(GtkTreeIter * iter)
{
	GeoXmlParameter *	parameter;
	GString *		keyword_label;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), iter,
		PARAMETER_XMLPOINTER, &parameter,
		-1);

	if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == TRUE) {
		GString *		keyword;

		keyword = g_string_new(NULL);
		g_string_assign(keyword, geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter)));

		if (keyword->str[keyword->len-1] ==  ' ') {
			keyword_label = g_string_new_len(keyword->str, keyword->len - 1);
			g_string_append_printf(keyword_label, "%lc", 0x02FD);
		} else
			keyword_label = g_string_new(keyword->str);

		g_string_free(keyword, TRUE);
	} else {
		GeoXmlSequence *	instance;

		keyword_label = g_string_new(NULL);
		geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		g_string_printf(keyword_label, "%lu parameter(s) and %lu instance(s)",
			geoxml_parameters_get_number(GEOXML_PARAMETERS(instance)),
			geoxml_parameter_group_get_instances_number(GEOXML_PARAMETER_GROUP(parameter)));
	}

	gtk_tree_store_set(debr.ui_parameter.tree_store, iter,
		PARAMETER_TYPE, combo_type_map_get_title(geoxml_parameter_get_type(parameter)),
		PARAMETER_KEYWORD, keyword_label->str,
		PARAMETER_LABEL, geoxml_parameter_get_label(parameter),
		-1);

	g_string_free(keyword_label, TRUE);
}

/*
 * Function: parameter_select_iter
 * Select _iter_ loading its pointer
 */
static void
parameter_select_iter(GtkTreeIter iter)
{
	GtkTreeSelection *	tree_selection;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter,
		PARAMETER_XMLPOINTER, &debr.parameter,
		-1);
	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_parameter.tree_view));
	gtk_tree_selection_select_iter(tree_selection, &iter);
	libgebr_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(debr.ui_parameter.tree_view), &iter);

	parameter_selected();
}

/*
 * Function: parameter_check_selected
 * Returns true if there is a parameter selected. Othewise,
 * returns false and show a message on the status bar.
 */
static gboolean
parameter_check_selected(gboolean show_warning)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_parameter.tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		if (show_warning)
			debr_message(LOG_ERROR, _("No parameter is selected"));
		return FALSE;
	}
	return TRUE;
}

/*
 * Function: parameter_get_selected
 * Return true if there is a parameter selected and write it to _iter_
 */
static gboolean
parameter_get_selected(GtkTreeIter * iter, gboolean show_warning)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	if (parameter_check_selected(show_warning) == FALSE)
		return FALSE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_parameter.tree_view));
	return gtk_tree_selection_get_selected(selection, &model, iter);
}

/*
 * Function: parameter_selected
 * Set debr.parameter to the current selected parameter
 */
static void
parameter_selected(void)
{
	GtkTreeIter	iter;

	if (parameter_get_selected(&iter, FALSE) == FALSE) {
		debr.parameter = NULL;
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter,
		PARAMETER_XMLPOINTER, &debr.parameter,
		-1);
	do_navigation_bar_update();

	/* parameter type stuff */
	gtk_action_set_visible(gtk_action_group_get_action(debr.action_group,
		"parameter_type_group"), !geoxml_parameter_get_is_in_group(debr.parameter));
	g_signal_handlers_block_matched(
		G_OBJECT(gtk_action_group_get_action(debr.action_group, "parameter_type_real")),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)on_parameter_type_activate, NULL);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(debr.action_group,
		parameter_type_radio_actions_entries[
			combo_type_map_get_index(geoxml_parameter_get_type(debr.parameter))].name)),
		TRUE);
	g_signal_handlers_unblock_matched(
		G_OBJECT(gtk_action_group_get_action(debr.action_group, "parameter_type_real")),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)on_parameter_type_activate, NULL);
}

/*
 * Function: parameter_activated
 * Open a dialog to configure the parameter, according to its type.
 */
static void
parameter_activated(void)
{
	GtkTreeIter	iter;

	if (parameter_get_selected(&iter, FALSE) == FALSE)
		return;
	if (geoxml_parameter_get_is_program_parameter(debr.parameter) == TRUE)
		parameter_dialog_setup_ui();
	else
		parameter_group_dialog_setup_ui();
}

/*
 * Funcion: parameter_popup_menu
 * Show a popup menu for parameter actions
 */
static GtkMenu *
parameter_popup_menu(GtkWidget * tree_view)
{
	GtkWidget *	menu;
	GtkWidget *	menu_item;

	GtkTreeIter	iter;

	menu = gtk_menu_new();

	if (parameter_get_selected(&iter, FALSE) == FALSE)
		goto out;

	if (gtk_tree_store_can_move_up(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, "parameter_top")));
	if (gtk_tree_store_can_move_down(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, "parameter_bottom")));
	if (gtk_tree_store_can_move_up(debr.ui_parameter.tree_store, &iter) == TRUE ||
	gtk_tree_store_can_move_down(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_new")));

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_duplicate")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_copy")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_paste")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_delete")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_properties")));

	menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "parameter_change_type"));
	gtk_action_block_activate_from(gtk_action_group_get_action(debr.action_group,
		"parameter_change_type"), menu_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), debr.parameter_type_menu);
	gtk_container_add(GTK_CONTAINER(menu), menu_item);


out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/*
 * Funcion: parameter_reorder
 * Parameter reordering callback. Also responsible for putting
 * parameters in or out of a group.
 */
static gboolean
parameter_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position)
{
	GeoXmlParameter *	parameter;
	GeoXmlParameter *	position_parameter;
	GtkTreeIter		parent;
	GtkTreeIter		position_parent;
	GtkTreeIter		new;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), iter,
		PARAMETER_XMLPOINTER, &parameter, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), position,
		PARAMETER_XMLPOINTER, &position_parameter, -1);
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, iter);
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &position_parent, position);

	if ((drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) &&
	geoxml_parameter_get_type(position_parameter) == GEOXML_PARAMETERTYPE_GROUP) {
		geoxml_sequence_move_into_group(GEOXML_SEQUENCE(parameter), GEOXML_PARAMETER_GROUP(position_parameter));

		gtk_tree_store_append(debr.ui_parameter.tree_store, &new, position);
		gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &new, iter);
		gtk_tree_store_remove(debr.ui_parameter.tree_store, iter);

		parameter_load_iter(position);
	} else {
		if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
			geoxml_sequence_move_after(GEOXML_SEQUENCE(parameter), GEOXML_SEQUENCE(position_parameter));
		else
			geoxml_sequence_move_before(GEOXML_SEQUENCE(parameter), GEOXML_SEQUENCE(position_parameter));

		if (gtk_tree_model_iter_equal_to(&parent, &position_parent)) {
			new = *iter;
			if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
				gtk_tree_store_move_after(debr.ui_parameter.tree_store, iter, position);
			else
				gtk_tree_store_move_before(debr.ui_parameter.tree_store, iter, position);
		} else {
			if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
				gtk_tree_store_insert_after(debr.ui_parameter.tree_store, &new, NULL, position);
			else
				gtk_tree_store_insert_before(debr.ui_parameter.tree_store, &new, NULL, position);
			gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &new, iter);
			gtk_tree_store_remove(debr.ui_parameter.tree_store, iter);
		}

		if (gtk_tree_model_iter_is_valid(&position_parent))
			parameter_load_iter(&position_parent);
	}
	if (gtk_tree_model_iter_is_valid(&parent))
		parameter_load_iter(&parent);

	parameter_select_iter(new);
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	return TRUE;
}

/*
 * Funcion: parameter_can_reorder
 * Parameter reordering acceptance callback.
 */
static gboolean
parameter_can_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position)
{
	GeoXmlParameter *	parameter;
	GeoXmlParameter *	position_parameter;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), iter,
		PARAMETER_XMLPOINTER, &parameter, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), position,
		PARAMETER_XMLPOINTER, &position_parameter, -1);

	if (geoxml_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_GROUP)
		return TRUE;
	/* group inside another expanded group */
	if (geoxml_parameter_get_group(position_parameter) != NULL)
		return FALSE;
	/* group inside another group */
	if ((drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) &&
	geoxml_parameter_get_type(position_parameter) == GEOXML_PARAMETERTYPE_GROUP)
		return FALSE;

	return TRUE;
}

static void
parameter_reconfigure_default_widget(struct ui_parameter_dialog * ui)
{
	parameter_widget_reconfigure(ui->parameter_widget);
	gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), ui->parameter_widget->widget, TRUE, TRUE, 0);
	gtk_widget_show(ui->parameter_widget->widget);
}

static void
parameter_default_widget_changed(struct parameter_widget * widget)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_required_toggled(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui)
{
	geoxml_program_parameter_set_required(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_toggle_button_get_active(toggle_button));
	parameter_reconfigure_default_widget(ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui)
{
	geoxml_program_parameter_set_be_list(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_toggle_button_get_active(toggle_button));
	gtk_widget_set_sensitive(ui->separator_entry, gtk_toggle_button_get_active(toggle_button));

	parameter_reconfigure_default_widget(ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_separator_changed(GtkEntry * entry, struct ui_parameter_dialog * ui)
{
	geoxml_program_parameter_set_list_separator(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));
	parameter_widget_update_list_separator(ui->parameter_widget);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_widget * widget)
{
	gboolean	is_directory;

	is_directory = gtk_combo_box_get_active(combo) == 0 ? FALSE : TRUE;

	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(widget->value_widget), is_directory);
	geoxml_program_parameter_set_file_be_directory(GEOXML_PROGRAM_PARAMETER(debr.parameter), is_directory);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	min_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	min = atof(min_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	max_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	max = atof(max_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		inc;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	inc_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	inc = atof(inc_str);

	gtk_spin_button_set_increments(spin_button, inc, 0);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		digits;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	digits_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	digits = atof(digits_str);

	gtk_spin_button_set_digits(spin_button, digits);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(debr.parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct ui_parameter_dialog * ui)
{
	parameter_reconfigure_default_widget(ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
