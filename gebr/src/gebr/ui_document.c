/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_document.c
 */

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/utils.h>

#include "ui_document.h"
#include "gebr.h"
#include "flow.h"
#include "ui_help.h"
#include "document.h"
#include "ui_project_line.h"

/*
 * Prototypes
 */

enum {
	DICT_EDIT_TYPE,
	DICT_EDIT_KEYWORD,
	DICT_EDIT_VALUE,
	DICT_EDIT_COMMENT,
	DICT_EDIT_GEOXML_PARAMETER,
	DICT_EDIT_N_COLUMN,
};

struct dict_edit_data {
	GtkWidget *		widget;

	GeoXmlDocument *	document;
	GtkActionGroup *	action_group;
	GtkWidget *		tree_view;
	GtkTreeModel *		tree_model;
	GtkCellRenderer *	cell_renderer_array[4];
} * dict_edit_data;

static void
on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data * data);
static void
on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data * data);
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data);
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data);
static void
dict_edit_load_iter(struct dict_edit_data * data, GtkTreeIter * iter, GeoXmlParameter * parameter);
static gboolean
dict_edit_get_selected(struct dict_edit_data * data, GtkTreeIter * iter);
static GtkMenu *
on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data * data);

static const GtkActionEntry dict_actions_entries [] = {
	{"add",  GTK_STOCK_ADD, NULL, NULL, N_("Add new parameter"),
		(GCallback)on_dict_edit_add_clicked},
	{"remove", GTK_STOCK_REMOVE, NULL, NULL, N_("Remove current parameters"),
		(GCallback)on_dict_edit_remove_clicked},
};

/*
 * Section: Public
 * Public functions.
 */

/* Function: document_get_current
 * Return current selected and active project, line or flow
 */
GeoXmlDocument *
document_get_current(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_PROJECT_LINE:
		project_line_get_selected(NULL, ProjectLineSelection);
		return gebr.project_line;
	case NOTEBOOK_PAGE_FLOW_BROWSE:
		flow_browse_get_selected(NULL, TRUE);
		return GEOXML_DOCUMENT(gebr.flow);
	default:
		return NULL;
	}
}

/* Function: document_properties_setup_ui
 * Show the _document_ properties in a dialog
 * Create the user interface for editing _document_(flow, line or project) properties,
 * like author, email, report, etc.
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
gboolean
document_properties_setup_ui(GeoXmlDocument * document)
{
	GtkWidget *		dialog;
	gint			ret;
	GString *		dialog_title;

	GtkWidget *		table;
	GtkWidget *		label;
	GtkWidget *		help_show_button;
	GtkWidget *		help_hbox;
	GtkWidget *		title;
	GtkWidget *		description;
	GtkWidget *		help;
	GtkWidget *		author;
	GtkWidget *		email;

	if (document == NULL)
		return FALSE;

	dialog_title = g_string_new(NULL);
	switch (geoxml_document_get_type(document)) {
	case GEOXML_DOCUMENT_TYPE_PROJECT:
		g_string_printf(dialog_title, _("Properties for project '%s'"), geoxml_document_get_title(document));
		break;
	case GEOXML_DOCUMENT_TYPE_LINE:
		g_string_printf(dialog_title, _("Properties for line '%s'"), geoxml_document_get_title(document));
		break;
	case GEOXML_DOCUMENT_TYPE_FLOW:
		flow_browse_single_selection();
		g_string_printf(dialog_title, _("Properties for flow '%s'"), geoxml_document_get_title(document));
		break;
	default:
		g_string_printf(dialog_title, _("Properties for document '%s'"), geoxml_document_get_title(document));
		break;
	}

	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
		GTK_WINDOW(gebr.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 260);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Title */
	label = gtk_label_new(_("Title"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	title = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(title), geoxml_document_get_title(document));

	/* Description */
	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	description = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(description), geoxml_document_get_description(document));

	/* Report */
	label = gtk_label_new(_("Report"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	help_hbox = gtk_hbox_new(FALSE,0);
	gtk_table_attach(GTK_TABLE(table), help_hbox, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	help_show_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_show_button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(help_show_button), "clicked",
		(GCallback)help_show_callback, document);
	g_object_set(G_OBJECT(help_show_button), "relief", GTK_RELIEF_NONE, NULL);

	help = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_box_pack_start(GTK_BOX(help_hbox), help, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(help), "clicked",
		GTK_SIGNAL_FUNC(help_edit), document);
	g_object_set(G_OBJECT(help), "relief", GTK_RELIEF_NONE, NULL);

	/* Author */
	label = gtk_label_new(_("Author"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	author = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(author), geoxml_document_get_author(document));

	/* User email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	email = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(email), geoxml_document_get_email(document));

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter			iter;

		const gchar *			old_title;
		const gchar *			new_title;
		gchar *				doc_type;

		enum GEOXML_DOCUMENT_TYPE	type;

		old_title = geoxml_document_get_title(document);
		new_title = gtk_entry_get_text(GTK_ENTRY(title));

		geoxml_document_set_title(document, new_title);
		geoxml_document_set_description(document, gtk_entry_get_text(GTK_ENTRY(description)));
		geoxml_document_set_author(document, gtk_entry_get_text(GTK_ENTRY(author)));
		geoxml_document_set_email(document, gtk_entry_get_text(GTK_ENTRY(email)));
		document_save(document);

		/* Update title in apropriated store */
		switch ((type = geoxml_document_get_type(document))) {
		case GEOXML_DOCUMENT_TYPE_PROJECT:
		case GEOXML_DOCUMENT_TYPE_LINE:
			project_line_get_selected(&iter, DontWarnUnselection);
			gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				PL_TITLE, geoxml_document_get_title(document),
				-1);
			doc_type = (type == GEOXML_DOCUMENT_TYPE_PROJECT) ? "project" : "line";
			project_line_info_update();
			break;
		case GEOXML_DOCUMENT_TYPE_FLOW:
			flow_browse_get_selected(&iter, FALSE);
			gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
				FB_TITLE, geoxml_document_get_title(document),
				-1);
			doc_type = "flow";
			flow_browse_info_update();
			break;
		default:
			break;
		}

		gebr_message(LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated"), doc_type, old_title);
		if (strcmp(old_title, new_title) != 0)
			gebr_message(LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'"),
				doc_type, old_title, new_title);

		ret = TRUE;
		break;
	} default:
		ret = FALSE;
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_string_free(dialog_title, TRUE);

	return ret;
}

/* Function: document_dict_edit_setup_ui
 * Open dialog for parameters's edition
 */
void
document_dict_edit_setup_ui(GeoXmlDocument * document)
{
	GtkWidget *		dialog;
	GtkWidget *		toolbar;
	GtkWidget *		scrolled_window;
	GtkWidget *		tree_view;
	GtkActionGroup *	action_group;
	GtkAccelGroup *		accel_group;
	GtkCellRenderer *	cell_renderer;
	GtkTreeViewColumn *	column;
	GtkListStore *		list_store;
	GtkListStore *		type_model;
	GtkTreeIter		iter;

	GeoXmlSequence *	parameter;
	struct dict_edit_data *	data;

	if (document == NULL)
		return;

	data = g_malloc(sizeof(struct dict_edit_data));
	data->document = document;
	list_store = gtk_list_store_new(DICT_EDIT_N_COLUMN,
		G_TYPE_STRING, /* type */
		G_TYPE_STRING, /* keyword */
		G_TYPE_STRING, /* value */
		G_TYPE_STRING, /* comment */
		G_TYPE_POINTER /* GeoXmlParameter */);
	data->tree_model = GTK_TREE_MODEL(list_store);

	action_group = gtk_action_group_new("dict_edit");
	data->action_group = action_group;
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, dict_actions_entries, G_N_ELEMENTS(dict_actions_entries), data);
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), accel_group);
	libgebr_gui_gtk_action_group_set_accel_group(action_group, accel_group);

	dialog = gtk_dialog_new_with_buttons(_("Edit parameters' dictionary"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);

	toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), toolbar);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(action_group, "add"))), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(gtk_action_create_tool_item(
		gtk_action_group_get_action(action_group, "remove"))), -1);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
	gtk_widget_set_size_request(tree_view, 350, 140);
	data->tree_view = tree_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	libgebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view),
		(GtkPopupCallback)on_dict_edit_popup_menu, data);
	libgebr_gui_gtk_tree_view_set_geoxml_sequence_moveable(GTK_TREE_VIEW(tree_view),
		DICT_EDIT_GEOXML_PARAMETER, NULL, NULL);

	type_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "string", 1, GEOXML_PARAMETERTYPE_STRING, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "integer", 1, GEOXML_PARAMETERTYPE_INT);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "real", 1, GEOXML_PARAMETERTYPE_INT);
	cell_renderer = gtk_cell_renderer_combo_new();
	data->cell_renderer_array[DICT_EDIT_TYPE] = cell_renderer;
	column = gtk_tree_view_column_new_with_attributes(_("Type"), cell_renderer, NULL);
	g_object_set(cell_renderer, "has-entry", FALSE, "editable", TRUE,
		"model", type_model, "text-column", 0, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_type_cell_edited, data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_TYPE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_KEYWORD] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Keyword"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_KEYWORD);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_VALUE] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Value"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_VALUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_COMMENT] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Comment"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_COMMENT);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	parameter = geoxml_parameters_get_first_parameter(geoxml_document_get_dict_parameters(data->document));
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		gtk_list_store_append(GTK_LIST_STORE(data->tree_model), &iter);
		dict_edit_load_iter(data, &iter, GEOXML_PARAMETER(parameter));
	}

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	document_save(data->document);

	g_free(data);
	gtk_widget_destroy(dialog);
}

/*
 * Section: Private
 * Private functions.
 */

/* Function: on_dict_edit_add_clicked
 * Add new parameter
 */
static void
on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data * data)
{
	GtkTreeIter		iter;
	GeoXmlParameter *	parameter;

	parameter = geoxml_parameters_append_parameter(
		geoxml_document_get_dict_parameters(data->document),
		GEOXML_PARAMETERTYPE_STRING);

	gtk_list_store_append(GTK_LIST_STORE(data->tree_model), &iter);
	dict_edit_load_iter(data, &iter, parameter);
}

/* Function: on_dict_edit_remove_clicked
 * Remove parameter
 */
static void
on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data * data)
{
	GtkTreeIter		iter;
	GeoXmlSequence *	parameter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	gtk_tree_model_get(data->tree_model, &iter,
		DICT_EDIT_GEOXML_PARAMETER, &parameter, -1);

	geoxml_sequence_remove(parameter);
	gtk_list_store_remove(GTK_LIST_STORE(data->tree_model), &iter);
}

/* Function: on_dict_edit_type_cell_edited
 * Edit type of parameter
 */
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data)
{
	GtkTreeIter		iter;
	GeoXmlParameter *	parameter;
	const gchar *		keyword, * value;

	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);
	gtk_tree_model_get(data->tree_model, &iter,
		DICT_EDIT_GEOXML_PARAMETER, &parameter, -1);

	keyword = geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter));
	value = geoxml_program_parameter_get_first_value(GEOXML_PROGRAM_PARAMETER(parameter), FALSE);

	if (!strcmp(new_text, "string"))
		geoxml_parameter_set_type(parameter, GEOXML_PARAMETERTYPE_STRING);
	else if (!strcmp(new_text, "integer"))
		geoxml_parameter_set_type(parameter, GEOXML_PARAMETERTYPE_INT);
	else if (!strcmp(new_text, "real"))
		geoxml_parameter_set_type(parameter, GEOXML_PARAMETERTYPE_FLOAT);
	else
		return;

	/* restore */
	geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(parameter), keyword);
	geoxml_program_parameter_set_first_value(GEOXML_PROGRAM_PARAMETER(parameter), FALSE, value);

	dict_edit_load_iter(data, &iter, parameter);
}

/* Function: on_dict_edit_cell_edited
 * Edit keyword, value and comment of parameter
 */
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data)
{
	GtkTreeIter			iter;
	gint				index;
	GeoXmlProgramParameter *	parameter;

	for (index = DICT_EDIT_KEYWORD; index < DICT_EDIT_COMMENT; index++)
		if (cell == data->cell_renderer_array[index])
			break;

	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);
	gtk_tree_model_get(data->tree_model, &iter,
		DICT_EDIT_GEOXML_PARAMETER, &parameter, -1);
	switch (index) {
	case DICT_EDIT_KEYWORD: {
		GtkTreeIter			i_iter;
		GeoXmlProgramParameter *	i_parameter;

		/* check if there is a keyword with the same name */
		libgebr_gui_gtk_tree_model_foreach(i_iter, data->tree_model) {
			gtk_tree_model_get(data->tree_model, &i_iter,
				DICT_EDIT_GEOXML_PARAMETER, &i_parameter, -1);
			if (i_parameter == parameter)
				continue;
			if (!strcmp(geoxml_program_parameter_get_keyword(i_parameter), new_text)) {
				libgebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					_("Duplicate keyword"),
					_("Another parameter already uses this keyword, please choose other"));
				return;
			}
		}

		geoxml_program_parameter_set_keyword(parameter, new_text);
		break;
	} case DICT_EDIT_VALUE:
		switch (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter))) {
		case GEOXML_PARAMETERTYPE_INT:
			new_text = (gchar*)libgebr_validate_int(new_text);
			break;
		case GEOXML_PARAMETERTYPE_FLOAT:
			new_text = (gchar*)libgebr_validate_float(new_text);
			break;
		default:
			break;
		}
		geoxml_program_parameter_set_first_value(parameter, FALSE, new_text);
		break;
	case DICT_EDIT_COMMENT:
		geoxml_parameter_set_label(GEOXML_PARAMETER(parameter), new_text);
		break;
	default:
		return;
	}
	gtk_list_store_set(GTK_LIST_STORE(data->tree_model), &iter, index, new_text, -1);
}

/* Function: dict_edit_load_iter
 * Load _parameter_ into _iter_
 */
static void
dict_edit_load_iter(struct dict_edit_data * data, GtkTreeIter * iter, GeoXmlParameter * parameter)
{
	const gchar *	type_string;

	switch (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter))) {
	case GEOXML_PARAMETERTYPE_STRING:
		type_string = "string";
		break;
	case GEOXML_PARAMETERTYPE_INT:
		type_string = "integer";
		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		type_string = "real";
		break;
	default:
		return;
	}

	gtk_list_store_set(GTK_LIST_STORE(data->tree_model), iter,
		DICT_EDIT_TYPE, type_string,
		DICT_EDIT_KEYWORD, geoxml_program_parameter_get_keyword(
			GEOXML_PROGRAM_PARAMETER(parameter)),
		DICT_EDIT_VALUE, geoxml_program_parameter_get_first_value(
			GEOXML_PROGRAM_PARAMETER(parameter), FALSE),
		DICT_EDIT_COMMENT, geoxml_parameter_get_label(GEOXML_PARAMETER(parameter)),
		DICT_EDIT_GEOXML_PARAMETER, parameter,
		-1);
}

/* Function: dict_edit_get_selected
 * Get selected iterator into _iter_
 */
static gboolean
dict_edit_get_selected(struct dict_edit_data * data, GtkTreeIter * iter)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view));
	return gtk_tree_selection_get_selected(selection, &model, iter);
}

/* Function: on_dict_edit_popup_menu
 * Popup menu for parameter removal, etc
 */
static GtkMenu *
on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data * data)
{
	GtkWidget *		menu;

	menu = gtk_menu_new();

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(data->action_group, "add")));

	if (!dict_edit_get_selected(data, NULL))
		goto out;

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(data->action_group, "remove")));

out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}
