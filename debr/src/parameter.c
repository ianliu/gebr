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
#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gtkfileentry.h>
#include <libgebr/gui/gtkenhancedentry.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/valuesequenceedit.h>

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
	enum GEBR_GEOXML_PARAMETER_TYPE	type;
	guint				index;
	gchar *				title;
} combo_type_map [] = {
	{GEBR_GEOXML_PARAMETER_TYPE_FLOAT, 0, N_("real")},
	{GEBR_GEOXML_PARAMETER_TYPE_INT, 1, N_("integer")},
	{GEBR_GEOXML_PARAMETER_TYPE_RANGE, 2, N_("range")},
	{GEBR_GEOXML_PARAMETER_TYPE_FLAG, 3, N_("flag")},
	{GEBR_GEOXML_PARAMETER_TYPE_STRING, 4, N_("text")},
	{GEBR_GEOXML_PARAMETER_TYPE_ENUM, 5, N_("enum")},
	{GEBR_GEOXML_PARAMETER_TYPE_FILE, 6, N_("file")},
	{GEBR_GEOXML_PARAMETER_TYPE_GROUP, 7, N_("group")},
};
const gsize	combo_type_map_size = 8;

int
combo_type_map_get_index(enum GEBR_GEOXML_PARAMETER_TYPE type)
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
	{"parameter_type_real", NULL, N_("real"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FLOAT},
	{"parameter_type_integer", NULL, N_("integer"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_INT},
	{"parameter_type_range", NULL, N_("range"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_RANGE},
	{"parameter_type_flag", NULL, N_("flag"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FLAG},
	{"parameter_type_text", NULL, N_("text"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_STRING},
	{"parameter_type_enum", NULL, N_("enum"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_ENUM},
	{"parameter_type_file", NULL, N_("file"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_FILE},
	{"parameter_type_group", NULL, N_("group"), NULL, NULL, GEBR_GEOXML_PARAMETER_TYPE_GROUP},
};

static void
parameter_dialog_setup_ui(void);
static GtkTreeIter
parameter_append_to_ui(GebrGeoXmlParameter * parameter, GtkTreeIter * parent);
static void
parameter_load_iter(GtkTreeIter * iter, gboolean load_group);
static void
parameter_select_iter(GtkTreeIter iter);
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
parameter_default_widget_changed(struct gebr_gui_parameter_widget * widget);
static void
parameter_required_toggled(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui);
static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui);
static void
parameter_separator_changed(GtkEntry * entry, struct ui_parameter_dialog * ui);
static void
parameter_file_type_changed(GtkComboBox * combo, struct gebr_gui_parameter_widget * widget);
static void
parameter_file_filter_name_changed(GebrGuiGtkEnhancedEntry * entry, struct gebr_gui_parameter_widget * widget);
static void
parameter_file_filter_pattern_changed(GebrGuiGtkEnhancedEntry * entry, struct gebr_gui_parameter_widget * widget);
static void
parameter_number_min_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget);
static gboolean
parameter_number_min_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget);
static void
parameter_number_max_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget);
static gboolean
parameter_number_max_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget);
static void
parameter_range_inc_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget);
static gboolean
parameter_range_inc_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget);
static void
parameter_range_digits_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget);
static gboolean
parameter_range_digits_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget);
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
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_parameter.tree_view)),
		GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(debr.ui_parameter.tree_view),
		(GebrGuiGtkPopupCallback)parameter_popup_menu, NULL);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(debr.ui_parameter.tree_view),
		(GebrGuiGtkTreeViewReorderCallback)parameter_reorder,
		(GebrGuiGtkTreeViewReorderCallback)parameter_can_reorder, NULL);
	gtk_widget_show(debr.ui_parameter.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_parameter.tree_view);
	g_signal_connect(debr.ui_parameter.tree_view, "cursor-changed",
		(GCallback)parameter_selected, NULL);
	g_signal_connect(debr.ui_parameter.tree_view, "row-activated",
		G_CALLBACK(parameter_activated), NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", PARAMETER_TYPE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_parameter.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Keyword / default value"), renderer, NULL);
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
	GebrGeoXmlSequence *		parameter;

	gtk_tree_store_clear(debr.ui_parameter.tree_store);
	if (debr.program == NULL) {
		debr.parameter = NULL;
		return;
	}

	gebr_geoxml_parameters_get_parameter(gebr_geoxml_program_get_parameters(debr.program), &parameter, 0);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		parameter_append_to_ui(GEBR_GEOXML_PARAMETER(parameter), NULL);

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
	parameter_load_iter(&iter, FALSE);
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
	(gebr_geoxml_parameter_get_is_program_parameter(debr.parameter) == FALSE ||
	gebr_geoxml_parameter_get_is_in_group(debr.parameter) == TRUE)) {
		GebrGeoXmlParameterGroup *	parameter_group;
		GebrGeoXmlSequence *	first_instance;
		GtkTreeIter		parent;
		GtkTreePath *		tree_path;

		if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, &iter)) {
			parameter_group = GEBR_GEOXML_PARAMETER_GROUP(debr.parameter);
			parent = iter;
		} else
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent,
				PARAMETER_XMLPOINTER, &parameter_group,
				-1);

		gebr_geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
		iter = parameter_append_to_ui(
			gebr_geoxml_parameters_append_parameter(GEBR_GEOXML_PARAMETERS(first_instance),
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT),
			&parent);
		tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(debr.ui_parameter.tree_view), tree_path, FALSE);
		gtk_tree_path_free(tree_path);
	} else {
		iter = parameter_append_to_ui(
			gebr_geoxml_parameters_append_parameter(
				gebr_geoxml_program_get_parameters(debr.program),
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT),
			NULL);
	}

	parameter_select_iter(iter);
	do_navigation_bar_update();
	if (!parameter_change_type_setup_ui())
		parameter_remove(FALSE);
	else
		parameter_activated();

	do_navigation_bar_update();
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_remove
 * Confirm action and if confirmed removed selected parameter from XML and UI
 */
void
parameter_remove(gboolean confirm)
{
	GtkTreeIter		parent;
	GtkTreeIter		iter;
	gboolean		in_group;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return;
	if (confirm && gebr_gui_confirm_action_dialog(_("Delete parameter"),
	_("Are you sure you want to delete selected(s) parameter(s)?")) == FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_parameter.tree_view) {
		if (debr.parameter == NULL)
			continue;
		in_group = gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, &iter);

		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(debr.parameter));
		gtk_tree_store_remove(debr.ui_parameter.tree_store, &iter);

		if (in_group)
			parameter_load_iter(&parent, FALSE);

		g_signal_emit_by_name(debr.ui_parameter.tree_view, "cursor-changed");
	}
	debr.parameter = NULL;
	gebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_parameter.tree_view));

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

	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(debr.parameter), NULL);
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

	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(debr.parameter), NULL);
	gtk_tree_store_move_before(debr.ui_parameter.tree_store, &iter, NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_change_type_setup_ui
 * Open dialog to change type of current selected parameter
 */
gboolean
parameter_change_type_setup_ui(void)
{
	GtkWidget *	dialog;
	GtkWidget *	table;
	GtkWidget *	type_label;
	GtkWidget *	type_combo;

	GtkTreeIter	iter;
	gboolean	ret = TRUE;

	if (parameter_get_selected(&iter, TRUE) == FALSE)
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Parameter's type"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
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
	if (gebr_geoxml_parameter_get_is_program_parameter(debr.parameter) == TRUE &&
	gebr_geoxml_parameter_get_is_in_group(debr.parameter) == FALSE) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), combo_type_map[7].title);
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), combo_type_map_get_index(
			gebr_geoxml_parameter_get_type(debr.parameter)));
	} else
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);

	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
		ret = FALSE;
		goto out;
	}

	parameter_change_type(combo_type_map_get_type(gtk_combo_box_get_active(GTK_COMBO_BOX(type_combo))));
	menu_saved_status_set(MENU_STATUS_UNSAVED);

out:	gtk_widget_destroy(dialog);

	return ret;
}

/* Function: parameter_copy
 * Copy selected(s) parameter to clipboard
 */
void
parameter_copy(void)
{
	GtkTreeIter		iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_parameter.tree_view) {
		GebrGeoXmlObject *	parameter;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter,
			PARAMETER_XMLPOINTER, &parameter,
			-1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(parameter));
	}
}

/* Function: parameter_paste
 * Paste parameters on clipboard
 */
void
parameter_paste(void)
{
	GebrGeoXmlSequence *	pasted;
	GtkTreeIter		pasted_iter;

	pasted = (GebrGeoXmlSequence*)gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(debr.program));
	if (pasted == NULL) {
		debr_message(GEBR_LOG_ERROR, _("Could not paste parameter"));
		return;
	}

	do {
		gtk_tree_store_append(debr.ui_parameter.tree_store, &pasted_iter, NULL);
		gtk_tree_store_set(debr.ui_parameter.tree_store, &pasted_iter,
			PARAMETER_XMLPOINTER, pasted,
			-1);
		parameter_load_iter(&pasted_iter, TRUE);
	} while (!gebr_geoxml_sequence_next(&pasted));

	parameter_select_iter(pasted_iter);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_change_type
 * Change type of selected parameter to _type_
 */
void
parameter_change_type(enum GEBR_GEOXML_PARAMETER_TYPE type)
{
	const gchar *	keyword;

	if (parameter_get_selected(NULL, TRUE) == FALSE)
		return;

	/* save keyword */
	keyword = (gebr_geoxml_parameter_get_is_program_parameter(debr.parameter))
		? gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(debr.parameter)) : NULL;

	gebr_geoxml_parameter_set_type(debr.parameter, type);

	/* restore keyword */
	if (gebr_geoxml_parameter_get_is_program_parameter(debr.parameter))
		gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(debr.parameter), keyword);

	parameter_load_selected();
}

/* Function: parameter_properties
 * Show parameter's properties according to its type
 */
void
parameter_properties(void)
{
	parameter_activated();
}

/*
 * Section: Private
 */

/* Function: parameter_dialog_setup_ui
 * Create an dialog to configure the current selected parameter (not a group)
 */
static void
parameter_dialog_setup_ui(void)
{
	struct ui_parameter_dialog *	ui;
	enum GEBR_GEOXML_PARAMETER_TYPE	type;

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

	GebrGeoXmlProgramParameter *	program_parameter;
	struct gebr_gui_parameter_widget *	gebr_gui_parameter_widget;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(debr.ui_parameter.tree_view));
	if (parameter_get_selected(NULL, TRUE) == FALSE)
		return;

	ui = g_malloc(sizeof(struct ui_parameter_dialog));
	ui->parameter = debr.parameter;
	program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter);
	type = gebr_geoxml_parameter_get_type(ui->parameter);

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

	if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
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
	}

	gebr_gui_parameter_widget = gebr_gui_parameter_widget_new(ui->parameter, TRUE, NULL);
	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FILE: {
		GtkWidget *		type_label;
		GtkWidget *		type_combo;
		GtkWidget *		filter_label;
		GtkWidget *		hbox;
		GtkWidget *		filter_name_entry;
		GtkWidget *		filter_pattern_entry;
		gchar *			filter_name;
		gchar *			filter_pattern;

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
			(GCallback)parameter_file_type_changed, gebr_gui_parameter_widget);

		/* file or directory? */
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo),
			gebr_geoxml_program_parameter_get_file_be_directory(program_parameter) == TRUE ? 1 : 0);

		/*
		 * Filter
		 */
		filter_label = gtk_label_new(_("Filter file types:"));
		gtk_widget_show(filter_label);
		gtk_table_attach(GTK_TABLE(table), filter_label, 0, 1, row, row+1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(filter_label), 0, 0.5);
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		filter_name_entry = gebr_gui_gtk_enhanced_entry_new_with_empty_text(_("name"));
		gtk_widget_show(filter_name_entry);
		gtk_box_pack_start(GTK_BOX(hbox), filter_name_entry, FALSE, FALSE, 0);
		g_signal_connect(filter_name_entry, "changed",
			(GCallback)parameter_file_filter_name_changed, gebr_gui_parameter_widget);
		filter_pattern_entry = gebr_gui_gtk_enhanced_entry_new_with_empty_text(_("pattern (eg. *.jpg *.png)"));
		gtk_widget_show(filter_pattern_entry);
		gtk_box_pack_start(GTK_BOX(hbox), filter_pattern_entry, FALSE, FALSE, 0);
		g_signal_connect(filter_pattern_entry, "changed",
			(GCallback)parameter_file_filter_pattern_changed, gebr_gui_parameter_widget);

		gebr_geoxml_program_parameter_get_file_filter(program_parameter, &filter_name, &filter_pattern);
		gebr_gui_gtk_enhanced_entry_set_text(GEBR_GUI_GTK_ENHANCED_ENTRY(filter_name_entry), filter_name);
		gebr_gui_gtk_enhanced_entry_set_text(GEBR_GUI_GTK_ENHANCED_ENTRY(filter_pattern_entry), filter_pattern);

		break;
	} case GEBR_GEOXML_PARAMETER_TYPE_INT: case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE: {
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

		gebr_geoxml_program_parameter_get_number_min_max(program_parameter,
			&min_str, &max_str);

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
		g_signal_connect(min_entry, "activate",
			(GCallback)parameter_number_min_on_activate, gebr_gui_parameter_widget);
		g_signal_connect(min_entry, "focus-out-event",
			(GCallback)parameter_number_min_on_focus_out, gebr_gui_parameter_widget);

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
		g_signal_connect(max_entry, "activate",
			(GCallback)parameter_number_max_on_activate, gebr_gui_parameter_widget);
		g_signal_connect(max_entry, "focus-out-event",
			(GCallback)parameter_number_max_on_focus_out, gebr_gui_parameter_widget);

		gtk_entry_set_text(GTK_ENTRY(min_entry), min_str);
		gtk_entry_set_text(GTK_ENTRY(max_entry), max_str);

		if (type != GEBR_GEOXML_PARAMETER_TYPE_RANGE)
			break;

		gebr_geoxml_program_parameter_get_range_properties(program_parameter,
			&min_str, &max_str, &inc_str, &digits_str);

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
		g_signal_connect(inc_entry, "activate",
			(GCallback)parameter_range_inc_on_activate, gebr_gui_parameter_widget);
		g_signal_connect(inc_entry, "focus-out-event",
			(GCallback)parameter_range_inc_on_focus_out, gebr_gui_parameter_widget);

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
		g_signal_connect(digits_entry, "activate",
			(GCallback)parameter_range_digits_on_activate, gebr_gui_parameter_widget);
		g_signal_connect(digits_entry, "focus-out-event",
			(GCallback)parameter_range_digits_on_focus_out, gebr_gui_parameter_widget);

		gtk_entry_set_text(GTK_ENTRY(inc_entry), inc_str);
		gtk_entry_set_text(GTK_ENTRY(digits_entry), digits_str);

		break;
	} case GEBR_GEOXML_PARAMETER_TYPE_ENUM: {
		GtkWidget *		enum_option_edit;
		GtkWidget *		options_label;
		GtkWidget *		vbox;

		GebrGeoXmlSequence *	option;

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

		gebr_geoxml_program_parameter_get_enum_option(program_parameter, &option, 0);
		enum_option_edit = enum_option_edit_new(GEBR_GEOXML_ENUM_OPTION(option), program_parameter);
		gtk_widget_show(enum_option_edit);
		gtk_table_attach(GTK_TABLE(table), enum_option_edit, 1, 2, row, row+1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0), ++row;
		g_signal_connect(GTK_OBJECT(enum_option_edit), "changed",
			G_CALLBACK(parameter_enum_options_changed), ui);

		break;
	} default:
		break;
	}

	ui->gebr_gui_parameter_widget = gebr_gui_parameter_widget;

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
	default_widget = gebr_gui_parameter_widget->widget;
	gtk_widget_show(default_widget);

	if (type == GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
		GtkWidget *	label;

		label = gtk_label_new(_("enabled by default"));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), default_widget, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), label, TRUE, TRUE, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	} else
		gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), default_widget, TRUE, TRUE, 0);
	gtk_table_attach(GTK_TABLE(table), default_widget_hbox, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gebr_gui_parameter_widget_set_auto_submit_callback(gebr_gui_parameter_widget,
		(changed_callback)parameter_default_widget_changed, NULL);

	/* parameter -> UI */
	gtk_entry_set_text(GTK_ENTRY(label_entry), gebr_geoxml_parameter_get_label(ui->parameter));
	gtk_entry_set_text(GTK_ENTRY(keyword_entry),
		gebr_geoxml_program_parameter_get_keyword(program_parameter));
	if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(required_check_button),
			gebr_geoxml_program_parameter_get_required(program_parameter));
		if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == TRUE)
			gtk_entry_set_text(GTK_ENTRY(ui->separator_entry),
				gebr_geoxml_program_parameter_get_list_separator(program_parameter));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(is_list_check_button),
			gebr_geoxml_program_parameter_get_is_list(program_parameter));
		gtk_widget_set_sensitive(ui->separator_entry, gebr_geoxml_program_parameter_get_is_list(program_parameter));

	}

	gtk_dialog_run(GTK_DIALOG(dialog));

	/* save not automatically synced changes */
	gebr_geoxml_parameter_set_label(ui->parameter, gtk_entry_get_text(GTK_ENTRY(label_entry)));
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
		gtk_entry_get_text(GTK_ENTRY(keyword_entry)));
	if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
		gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(required_check_button)));
		gebr_geoxml_program_parameter_set_be_list(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(is_list_check_button)));
		if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == TRUE)
			gebr_geoxml_program_parameter_set_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
				gtk_entry_get_text(GTK_ENTRY(ui->separator_entry)));
	}

	parameter_load_selected();

	/* frees */
	gtk_widget_destroy(ui->dialog);
	g_free(ui);
}


/*
 * Function: parameter_append_to_ui
 * Create an item for _parameter_ on parameter tree view
 */
static GtkTreeIter
parameter_append_to_ui(GebrGeoXmlParameter * parameter, GtkTreeIter * parent)
{
	GtkTreeIter	iter;

	gtk_tree_store_append(debr.ui_parameter.tree_store, &iter, parent);
	gtk_tree_store_set(debr.ui_parameter.tree_store, &iter,
		PARAMETER_XMLPOINTER, parameter,
		-1);
	parameter_load_iter(&iter, TRUE);

	return iter;
}

/*
 * Function: parameter_load_iter
 * Load _iter_ columns from its parameter
 */
static void
parameter_load_iter(GtkTreeIter * iter, gboolean load_group)
{
	GtkTreeIter		parent;
	GebrGeoXmlParameter *	parameter;
	GString *		keyword_label;
	GString *		parameter_type;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), iter,
		PARAMETER_XMLPOINTER, &parameter,
		-1);

	if (gebr_geoxml_parameter_get_is_program_parameter(GEBR_GEOXML_PARAMETER(parameter)) == TRUE) {
		GString *		keyword;
		GString *               default_value;

		keyword = g_string_new(NULL);
		g_string_assign(keyword, gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter)));

		if (keyword->str[keyword->len-1] ==  ' ') {
			keyword_label = g_string_new_len(keyword->str, keyword->len - 1);
			g_string_append_printf(keyword_label, "%lc", 0x02FD);
		} else
			keyword_label = g_string_new(keyword->str);

		default_value = gebr_geoxml_program_parameter_get_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE);
		if (default_value->len)
			g_string_append_printf(keyword_label,  " [%s]", default_value->str);

		g_string_free(default_value, TRUE);
		g_string_free(keyword, TRUE);
	} else {
		GebrGeoXmlSequence *	instance;

		keyword_label = g_string_new(NULL);
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);

		if (gebr_geoxml_parameter_group_get_is_instanciable(GEBR_GEOXML_PARAMETER_GROUP(parameter))){
			if ( gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter)) == 1)
				g_string_printf(keyword_label, _("with 1 instance"));
			else {
				g_string_printf(keyword_label,
					_("with %lu instances"),
					gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter)));
			}
		} else
			g_string_printf(keyword_label, _("not instanciable"));

		if (gebr_geoxml_parameters_get_exclusive(GEBR_GEOXML_PARAMETERS(instance)) != NULL)
			g_string_append_printf(keyword_label, _(" and exclusive"));
		else
			g_string_append_printf(keyword_label, _(" and not exclusive"));

		if (load_group) {
			GebrGeoXmlSequence *	first_instance;
			GebrGeoXmlSequence *	group_parameter;

			/* remove all childs for reload */
// 			if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(debr.ui_parameter.tree_store),
// 			iter, NULL, 0)) {
// 				GtkTreeIter	child = *iter;
// 				gboolean	has_next;
// 				do {
// 					GtkTreeIter	tmp;
// 
// 					tmp = child;
// 					has_next = gtk_tree_model_iter_next(
// 						GTK_TREE_MODEL(debr.ui_parameter.tree_store), &tmp);
// 					gtk_tree_store_remove(debr.ui_parameter.tree_store, &child);
// 					child = tmp;
// 				} while (has_next);
// 			}

			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &first_instance, 0);
			gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(first_instance), &group_parameter, 0);
			for (; group_parameter != NULL; gebr_geoxml_sequence_next(&group_parameter))
				parameter_append_to_ui(GEBR_GEOXML_PARAMETER(group_parameter), iter);
		}
	}

	parameter_type = g_string_new(combo_type_map_get_title(gebr_geoxml_parameter_get_type(parameter)));
	if (gebr_geoxml_parameter_get_is_program_parameter(parameter) &&
	gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(parameter)))
		g_string_append(parameter_type, "(s)");

	gtk_tree_store_set(debr.ui_parameter.tree_store, iter,
		PARAMETER_KEYWORD, keyword_label->str,
		PARAMETER_LABEL, gebr_geoxml_parameter_get_label(parameter),
		-1);

	if (gebr_geoxml_parameter_get_is_program_parameter(GEBR_GEOXML_PARAMETER(parameter)) == TRUE &&
	gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameter))){
		GString *	markup;

		markup = g_string_new(NULL);
		g_string_append(markup, g_markup_printf_escaped("<b>%s</b>", parameter_type->str));
		gtk_tree_store_set(debr.ui_parameter.tree_store, iter,
			PARAMETER_TYPE, markup->str,
			-1);
		g_string_free(markup, TRUE);
	} else {
		gtk_tree_store_set(debr.ui_parameter.tree_store, iter,
			PARAMETER_TYPE, parameter_type->str,
			-1);
	}

	if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &parent, iter))
		parameter_load_iter(&parent, FALSE);

	g_string_free(parameter_type, TRUE);
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
	gtk_tree_selection_unselect_all(tree_selection);
	gtk_tree_selection_select_iter(tree_selection, &iter);
	gebr_gui_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(debr.ui_parameter.tree_view), &iter);

	parameter_selected();
}

/*
 * Function: parameter_get_selected
 * Return true if there is a parameter selected and write it to _iter_
 */
static gboolean
parameter_get_selected(GtkTreeIter * iter, gboolean show_warning)
{
	if (gebr_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_parameter.tree_view), iter) == FALSE) {
		if (show_warning)
			debr_message(GEBR_LOG_ERROR, _("No parameter is selected"));
		return FALSE;
	}

	return TRUE;
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
		"parameter_type_group"), !gebr_geoxml_parameter_get_is_in_group(debr.parameter));
	g_signal_handlers_block_matched(
		G_OBJECT(gtk_action_group_get_action(debr.action_group, "parameter_type_real")),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
		(GCallback)on_parameter_type_activate, NULL);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(debr.action_group,
		parameter_type_radio_actions_entries[
			combo_type_map_get_index(gebr_geoxml_parameter_get_type(debr.parameter))].name)),
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
	if (gebr_geoxml_parameter_get_is_program_parameter(debr.parameter) == TRUE)
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

	if (gebr_gui_gtk_tree_store_can_move_up(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, "parameter_top")));
	if (gebr_gui_gtk_tree_store_can_move_down(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, "parameter_bottom")));
	if (gebr_gui_gtk_tree_store_can_move_up(debr.ui_parameter.tree_store, &iter) == TRUE ||
	gebr_gui_gtk_tree_store_can_move_down(debr.ui_parameter.tree_store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_new")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_copy")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_paste")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_delete")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "parameter_properties")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_preview")));

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
	GebrGeoXmlParameter *	parameter;
	GebrGeoXmlParameter *	position_parameter;
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
	gebr_geoxml_parameter_get_type(position_parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		gebr_geoxml_sequence_move_into_group(GEBR_GEOXML_SEQUENCE(parameter), GEBR_GEOXML_PARAMETER_GROUP(position_parameter));

		gtk_tree_store_append(debr.ui_parameter.tree_store, &new, position);
		gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &new, iter);
		gtk_tree_store_remove(debr.ui_parameter.tree_store, iter);

		parameter_load_iter(position, FALSE);
	} else {
		if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
			gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(parameter), GEBR_GEOXML_SEQUENCE(position_parameter));
		else
			gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(parameter), GEBR_GEOXML_SEQUENCE(position_parameter));

		if (gebr_gui_gtk_tree_iter_equal_to(&parent, &position_parent)) {
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
			gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &new, iter);
			gtk_tree_store_remove(debr.ui_parameter.tree_store, iter);
		}

		if (gebr_gui_gtk_tree_model_iter_is_valid(&position_parent))
			parameter_load_iter(&position_parent, FALSE);
	}
	if (gebr_gui_gtk_tree_model_iter_is_valid(&parent))
		parameter_load_iter(&parent, FALSE);

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
	GebrGeoXmlParameter *	parameter;
	GebrGeoXmlParameter *	position_parameter;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), iter,
		PARAMETER_XMLPOINTER, &parameter, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), position,
		PARAMETER_XMLPOINTER, &position_parameter, -1);

	if (gebr_geoxml_parameter_get_type(parameter) != GEBR_GEOXML_PARAMETER_TYPE_GROUP)
		return TRUE;
	/* group inside another expanded group */
	if (gebr_geoxml_parameter_get_group(position_parameter) != NULL)
		return FALSE;
	/* group inside another group */
	if ((drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) &&
	gebr_geoxml_parameter_get_type(position_parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP)
		return FALSE;

	return TRUE;
}

static void
parameter_reconfigure_default_widget(struct ui_parameter_dialog * ui)
{
	gebr_gui_parameter_widget_reconfigure(ui->gebr_gui_parameter_widget);
	gtk_box_pack_start(GTK_BOX(ui->default_widget_hbox), ui->gebr_gui_parameter_widget->widget, TRUE, TRUE, 0);
	gtk_widget_show(ui->gebr_gui_parameter_widget->widget);
}

static void
parameter_default_widget_changed(struct gebr_gui_parameter_widget * widget)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_required_toggled(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui)
{
	gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
		gtk_toggle_button_get_active(toggle_button));
	parameter_reconfigure_default_widget(ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct ui_parameter_dialog * ui)
{
	gebr_geoxml_program_parameter_set_be_list(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
		gtk_toggle_button_get_active(toggle_button));
	gtk_widget_set_sensitive(ui->separator_entry, gtk_toggle_button_get_active(toggle_button));

	parameter_reconfigure_default_widget(ui);
	if (gtk_toggle_button_get_active(toggle_button))
		parameter_separator_changed(GTK_ENTRY(ui->separator_entry), ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_separator_changed(GtkEntry * entry, struct ui_parameter_dialog * ui)
{
	gebr_geoxml_program_parameter_set_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER(ui->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));
	gebr_gui_parameter_widget_update_list_separator(ui->gebr_gui_parameter_widget);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_type_changed(GtkComboBox * combo, struct gebr_gui_parameter_widget * widget)
{
	gboolean	is_directory;

	is_directory = gtk_combo_box_get_active(combo) == 0 ? FALSE : TRUE;

	gebr_gui_gtk_file_entry_set_choose_directory(GEBR_GUI_GTK_FILE_ENTRY(widget->value_widget), is_directory);
	gebr_geoxml_program_parameter_set_file_be_directory(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter), is_directory);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_filter_name_changed(GebrGuiGtkEnhancedEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	gebr_geoxml_program_parameter_set_file_filter(widget->program_parameter,
		gebr_gui_gtk_enhanced_entry_get_text(entry), NULL);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_filter_pattern_changed(GebrGuiGtkEnhancedEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	gebr_geoxml_program_parameter_set_file_filter(widget->program_parameter,
		NULL, gebr_gui_gtk_enhanced_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_number_min_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	const gchar *	min_str;
	gdouble		min;

	min_str = gebr_validate_float((gchar*)gtk_entry_get_text(GTK_ENTRY(entry)), NULL, NULL);
	gtk_entry_set_text(entry, min_str);
	min = atof(min_str);

	if (widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
		gdouble		max;
		GtkSpinButton *	spin_button;

		spin_button = GTK_SPIN_BUTTON(widget->value_widget);
		
		gtk_spin_button_get_range(spin_button, NULL, &max);
		gtk_spin_button_set_range(spin_button, min, max);
	} else
		gebr_gui_parameter_widget_validate(widget);

	gebr_geoxml_program_parameter_set_number_min_max(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		min_str, NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_number_min_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget)
{
	parameter_number_min_on_activate(entry, widget);
	return FALSE;
}

static void
parameter_number_max_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	const gchar *	max_str;
	gdouble		max;

	max_str = gebr_validate_float((gchar*)gtk_entry_get_text(GTK_ENTRY(entry)), NULL, NULL);
	gtk_entry_set_text(entry, max_str);
	max = atof(max_str);

	if (widget->parameter_type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
		gdouble		min;
		GtkSpinButton *	spin_button;

		spin_button = GTK_SPIN_BUTTON(widget->value_widget);

		gtk_spin_button_get_range(spin_button, &min, NULL);
		gtk_spin_button_set_range(spin_button, min, max);
	} else
		gebr_gui_parameter_widget_validate(widget);

	gebr_geoxml_program_parameter_set_number_min_max(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		NULL, max_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_number_max_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget)
{
	parameter_number_max_on_activate(entry, widget);
	return FALSE;
}

static void
parameter_range_inc_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		inc;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	gebr_geoxml_program_parameter_get_range_properties(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	inc_str = (gchar*)gebr_validate_float((gchar*)gtk_entry_get_text(GTK_ENTRY(entry)), NULL, NULL);
	gtk_entry_set_text(entry, inc_str);
	inc = atof(inc_str);

	gtk_spin_button_set_increments(spin_button, inc, 0);
	gebr_geoxml_program_parameter_set_range_properties(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_range_inc_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget)
{
	parameter_range_inc_on_activate(entry, widget);
	return FALSE;
}

static void
parameter_range_digits_on_activate(GtkEntry * entry, struct gebr_gui_parameter_widget * widget)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		digits;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(widget->value_widget);
	gebr_geoxml_program_parameter_get_range_properties(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	digits_str = (gchar*)gebr_validate_int((gchar*)gtk_entry_get_text(GTK_ENTRY(entry)), NULL, NULL);
	gtk_entry_set_text(entry, digits_str);
	digits = atof(digits_str);

	gtk_spin_button_set_digits(spin_button, digits);
	gebr_geoxml_program_parameter_set_range_properties(GEBR_GEOXML_PROGRAM_PARAMETER(widget->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_range_digits_on_focus_out(GtkEntry * entry, GdkEvent * event, struct gebr_gui_parameter_widget * widget)
{
	parameter_range_digits_on_activate(entry, widget);
	return FALSE;
}

static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct ui_parameter_dialog * ui)
{
	parameter_reconfigure_default_widget(ui);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
