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
	DICT_EDIT_DOCUMENT,
	DICT_EDIT_TYPE,
	DICT_EDIT_KEYWORD_IMAGE,
	DICT_EDIT_KEYWORD,
	DICT_EDIT_VALUE,
	DICT_EDIT_COMMENT,
	DICT_EDIT_GEOXML_POINTER,
	DICT_EDIT_EDITABLE,
	DICT_EDIT_N_COLUMN,
};

struct dict_edit_data {
	GtkWidget *		widget;

	GeoXmlDocument *	documents[4];
	GtkTreeIter		iters[3];

	GtkWidget *		document_combo;
	GtkWidget *		type_combo;
	GtkWidget *		keyword_entry;
	GtkWidget *		value_entry;
	GtkWidget *		comment_entry;

	GtkActionGroup *	action_group;
	GtkWidget *		tree_view;
	GtkTreeModel *		tree_model;
	GtkTreeModel *		document_model;
	GtkCellRenderer *	cell_renderer_array[6];
};

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
static void
dict_edit_check_duplicates(struct dict_edit_data * data, GeoXmlDocument * document, const gchar * keyword);
static gboolean
dict_edit_check_duplicate_keyword(struct dict_edit_data * data, GeoXmlProgramParameter * parameter,
const gchar * keyword, gboolean show_error);
static gboolean
dict_edit_get_selected(struct dict_edit_data * data, GtkTreeIter * iter);
static GtkMenu *
on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data * data);
static enum GEOXML_PARAMETERTYPE
dict_edit_type_text_to_geoxml_type(const gchar * text);
static gboolean
dict_edit_check_empty_keyword(const gchar * keyword);
static GtkTreeIter
dict_edit_append_iter(struct dict_edit_data * data, GeoXmlObject * object, GtkTreeIter * document_iter);

static const gchar *
document_get_name_from_type(GeoXmlDocument * document, gboolean upper);

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
	if (geoxml_document_get_type(document) == GEOXML_DOCUMENT_TYPE_FLOW)
		flow_browse_single_selection();

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Properties for %s '%s'"),
		document_get_name_from_type(document, FALSE), geoxml_document_get_title(document));
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
			project_line_info_update();
			break;
		case GEOXML_DOCUMENT_TYPE_FLOW:
			flow_browse_get_selected(&iter, FALSE);
			gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
				FB_TITLE, geoxml_document_get_title(document),
				-1);
			flow_browse_info_update();
			break;
		default:
			break;
		}

		gebr_message(LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated"),
			document_get_name_from_type(document, FALSE), old_title);
		if (strcmp(old_title, new_title) != 0)
			gebr_message(LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'"),
				document_get_name_from_type(document, FALSE), old_title, new_title);

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
document_dict_edit_setup_ui(void)
{
	GtkWidget *		dialog;
	GtkWidget *		frame;
	GtkWidget *		scrolled_window;
	GtkWidget *		tree_view;
	GtkWidget *		add_hbox;
	GtkWidget *		widget;

	GtkActionGroup *	action_group;
	GtkAccelGroup *		accel_group;
	GtkCellRenderer *	cell_renderer;
	GtkCellRenderer *	pixbuf_cell_renderer;
	GtkTreeViewColumn *	column;
	GtkTreeStore *		tree_store;
	GtkListStore *		type_model;
	GtkListStore *		document_model;
	GtkTreeIter		iter;

	GString *		dialog_title;
	GeoXmlDocument *	document;
	GeoXmlSequence *	parameter;
	struct dict_edit_data *	data;

	document = document_get_current();
	data = g_malloc(sizeof(struct dict_edit_data));
	tree_store = gtk_tree_store_new(DICT_EDIT_N_COLUMN,
		G_TYPE_STRING,  /* document */
		G_TYPE_STRING,  /* type */
		G_TYPE_STRING,  /* keyword stock */
		G_TYPE_STRING,  /* keyword */
		G_TYPE_STRING,  /* value */
		G_TYPE_STRING,  /* comment */
		G_TYPE_POINTER, /* GeoXmlParameter */
		G_TYPE_BOOLEAN  /* editable */);
	data->tree_model = GTK_TREE_MODEL(tree_store);

	action_group = gtk_action_group_new("dict_edit");
	data->action_group = action_group;
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, dict_actions_entries,
		G_N_ELEMENTS(dict_actions_entries), data);
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), accel_group);
	libgebr_gui_gtk_action_group_set_accel_group(action_group, accel_group);

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Parameters' dictionary for %s '%s'"),
		document_get_name_from_type(document, FALSE), geoxml_document_get_title(document));
	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, -1, 400);

	frame = gtk_frame_new("");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), frame);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_store));
	gtk_widget_set_size_request(tree_view, 350, 140);
	data->tree_view = tree_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	libgebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view),
		(GtkPopupCallback)on_dict_edit_popup_menu, data);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_DOCUMENT] = cell_renderer;
	column = gtk_tree_view_column_new_with_attributes("", cell_renderer, NULL);
	g_object_set(G_OBJECT(column), "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, 93);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_DOCUMENT);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	document_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
	data->document_model = GTK_TREE_MODEL(document_model);

	type_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "integer", 1, GEOXML_PARAMETERTYPE_INT, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "real", 1, GEOXML_PARAMETERTYPE_FLOAT, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter,
		0, "string", 1, GEOXML_PARAMETERTYPE_STRING, -1);
	cell_renderer = gtk_cell_renderer_combo_new();
	data->cell_renderer_array[DICT_EDIT_TYPE] = cell_renderer;
	column = gtk_tree_view_column_new_with_attributes(_("Type"), cell_renderer, NULL);
	g_object_set(G_OBJECT(column), "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, 93);
	g_object_set(cell_renderer, "has-entry", FALSE, "editable", TRUE,
		"model", type_model, "text-column", 0, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_type_cell_edited, data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_TYPE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Keyword"));
	pixbuf_cell_renderer = gtk_cell_renderer_pixbuf_new();
	data->cell_renderer_array[DICT_EDIT_KEYWORD_IMAGE] = pixbuf_cell_renderer;
	gtk_tree_view_column_pack_start(column, pixbuf_cell_renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, pixbuf_cell_renderer, "stock-id", DICT_EDIT_KEYWORD_IMAGE);
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell_renderer, FALSE);
	data->cell_renderer_array[DICT_EDIT_KEYWORD] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	g_object_set(G_OBJECT(column), "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, 103);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_KEYWORD);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_VALUE] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Value"), cell_renderer, NULL);
	g_object_set(G_OBJECT(column), "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, 103);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_VALUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_COMMENT] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Comment"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_COMMENT);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	/*
	 * Add data widgets
	 */ 
	add_hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), add_hbox, FALSE, TRUE, 0);

	data->document_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(document_model));
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(data->document_combo), cell_renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(data->document_combo), cell_renderer, "text", NULL);
	gtk_widget_set_size_request(data->document_combo, 90, -1);
	gtk_box_pack_start(GTK_BOX(add_hbox), data->document_combo, FALSE, FALSE, 0);

	data->type_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(type_model));
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(data->type_combo), cell_renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(data->type_combo), cell_renderer, "text", NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->type_combo), 0);
	gtk_widget_set_size_request(data->type_combo, 90, -1);
	gtk_box_pack_start(GTK_BOX(add_hbox), data->type_combo, FALSE, FALSE, 0);

	data->keyword_entry = gtk_entry_new();
	g_signal_connect(data->keyword_entry, "activate",
		G_CALLBACK(on_dict_edit_add_clicked), data);
	gtk_widget_set_size_request(data->keyword_entry, 100, -1);
	gtk_box_pack_start(GTK_BOX(add_hbox), data->keyword_entry, FALSE, FALSE, 0);
	data->value_entry = gtk_entry_new();
	gtk_widget_set_size_request(data->value_entry, 100, -1);
	g_signal_connect(data->value_entry, "activate",
		G_CALLBACK(on_dict_edit_add_clicked), data);
	gtk_box_pack_start(GTK_BOX(add_hbox), data->value_entry, FALSE, FALSE, 0);
	data->comment_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(add_hbox), data->comment_entry, TRUE, TRUE, 0);
	g_signal_connect(data->comment_entry, "activate",
		G_CALLBACK(on_dict_edit_add_clicked), data);
	widget = gtk_action_create_tool_item(gtk_action_group_get_action(action_group, "add"));
	gtk_box_pack_start(GTK_BOX(add_hbox), widget, FALSE, FALSE, 0);

	data->documents[0] = GEOXML_DOCUMENT(gebr.project);
	data->documents[1] = GEOXML_DOCUMENT(gebr.line);
	data->documents[2] = geoxml_document_get_type(document) == GEOXML_DOCUMENT_TYPE_LINE
		? NULL : GEOXML_DOCUMENT(gebr.flow);
	data->documents[3] = NULL;

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GtkTreeIter	document_iter;
		const gchar *	document_name;

		document_name = document_get_name_from_type(data->documents[i], TRUE);

		gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &document_iter, NULL);
		data->iters[i] = document_iter;
		gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &document_iter,
			DICT_EDIT_DOCUMENT, document_name,
			DICT_EDIT_GEOXML_POINTER, parameter,
			DICT_EDIT_EDITABLE, FALSE,
			-1);

		/* document combo box */
		gtk_list_store_append(document_model, &iter);
		gtk_list_store_set(document_model, &iter,
			0, document_name, 1, data->documents[i], 2, &data->iters[i], -1);

		parameter = geoxml_parameters_get_first_parameter(
			geoxml_document_get_dict_parameters(data->documents[i]));
		for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
			iter = dict_edit_append_iter(data, GEOXML_OBJECT(parameter), &document_iter);
			dict_edit_load_iter(data, &iter, GEOXML_PARAMETER(parameter));
			if (i == 0)
				continue;
			dict_edit_check_duplicates(data, data->documents[i],
				geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter)));
		}
		if (document == data->documents[i]) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(data->document_combo), i);
			libgebr_gui_gtk_tree_view_expand(GTK_TREE_VIEW(tree_view), &document_iter);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(
				GTK_TREE_VIEW(tree_view)), &document_iter);
		}
	}

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	for (int i = 0; data->documents[i] != NULL; ++i)
		document_save(data->documents[i]);

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GeoXmlSequence *	i_parameter;

		i_parameter = geoxml_parameters_get_first_parameter(
			geoxml_document_get_dict_parameters(data->documents[i]));
		for (; i_parameter != NULL; geoxml_sequence_next(&i_parameter))
			gtk_tree_iter_free(geoxml_object_get_user_data(GEOXML_OBJECT(i_parameter)));
	}
	g_free(data);
	g_string_free(dialog_title, TRUE);
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
	GtkTreeIter			iter;
	GtkTreeIter			document_iter;
	GeoXmlProgramParameter *	parameter;
	const gchar *			keyword;
	GeoXmlDocument *		document;

	document = data->documents[gtk_combo_box_get_active(GTK_COMBO_BOX(data->document_combo))];
	document_iter = data->iters[gtk_combo_box_get_active(GTK_COMBO_BOX(data->document_combo))];
	parameter = GEOXML_PROGRAM_PARAMETER(geoxml_parameters_append_parameter(
		geoxml_document_get_dict_parameters(document),
		dict_edit_type_text_to_geoxml_type(
			gtk_combo_box_get_active_text(GTK_COMBO_BOX(data->type_combo)))));

	keyword = gtk_entry_get_text(GTK_ENTRY(data->keyword_entry));
	if (strlen(keyword)) {
		if (dict_edit_check_duplicate_keyword(data, parameter, keyword, TRUE)) {
			geoxml_sequence_remove(GEOXML_SEQUENCE(parameter));
			return;
		}
		geoxml_program_parameter_set_keyword(parameter, 
			gtk_entry_get_text(GTK_ENTRY(data->keyword_entry)));
	} else {
		GString *	keyword;
		gint		new_parameter_count;

		new_parameter_count = 0;
		keyword = g_string_new(NULL);

		do
			g_string_printf(keyword, "par%d", ++new_parameter_count);
		while (dict_edit_check_duplicate_keyword(data, parameter, keyword->str, FALSE));
		geoxml_program_parameter_set_keyword(parameter, keyword->str);

		g_string_free(keyword, TRUE);
	}

	geoxml_program_parameter_set_first_value(parameter,
		FALSE, gtk_entry_get_text(GTK_ENTRY(data->value_entry)));
	geoxml_parameter_set_label(GEOXML_PARAMETER(parameter),
		gtk_entry_get_text(GTK_ENTRY(data->comment_entry)));

	gtk_entry_set_text(GTK_ENTRY(data->keyword_entry), "");
	gtk_entry_set_text(GTK_ENTRY(data->value_entry), "");
	gtk_entry_set_text(GTK_ENTRY(data->comment_entry), "");
	gtk_widget_grab_focus(data->keyword_entry);

	iter = dict_edit_append_iter(data, GEOXML_OBJECT(parameter), &document_iter);
	dict_edit_load_iter(data, &iter, GEOXML_PARAMETER(parameter));
	libgebr_gui_gtk_tree_view_expand(GTK_TREE_VIEW(data->tree_view), &document_iter);
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view)), &iter);
	libgebr_gui_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(data->tree_view), &iter);
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
		DICT_EDIT_GEOXML_POINTER, &parameter, -1);

	geoxml_sequence_remove(parameter);
	gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &iter);
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
		DICT_EDIT_GEOXML_POINTER, &parameter, -1);

	keyword = geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(parameter));
	value = geoxml_program_parameter_get_first_value(GEOXML_PROGRAM_PARAMETER(parameter), FALSE);

	geoxml_parameter_set_type(parameter, dict_edit_type_text_to_geoxml_type(new_text));

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
		DICT_EDIT_GEOXML_POINTER, &parameter, -1);
	switch (index) {
	case DICT_EDIT_KEYWORD:
		if (!dict_edit_check_empty_keyword(new_text) ||
		dict_edit_check_duplicate_keyword(data, parameter, new_text, TRUE))
			return;

		geoxml_program_parameter_set_keyword(parameter, new_text);
		break;
	case DICT_EDIT_VALUE:
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
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter, index, new_text, -1);
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

	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
		DICT_EDIT_TYPE, type_string,
		DICT_EDIT_KEYWORD_IMAGE, "",
		DICT_EDIT_KEYWORD, geoxml_program_parameter_get_keyword(
			GEOXML_PROGRAM_PARAMETER(parameter)),
		DICT_EDIT_VALUE, geoxml_program_parameter_get_first_value(
			GEOXML_PROGRAM_PARAMETER(parameter), FALSE),
		DICT_EDIT_COMMENT, geoxml_parameter_get_label(GEOXML_PARAMETER(parameter)),
		DICT_EDIT_GEOXML_POINTER, parameter,
		DICT_EDIT_EDITABLE, TRUE,
		-1);
}

/* Function: dict_edit_check_duplicates
 * Check for duplicates keywords for parameters of higher hierarchical documents levels
 */
static void
dict_edit_check_duplicates(struct dict_edit_data * data, GeoXmlDocument * document, const gchar * keyword)
{
	GeoXmlSequence *	i_parameter;

	for (int i = 0; data->documents[i] != document; ++i) {
		i_parameter = geoxml_parameters_get_first_parameter(
			geoxml_document_get_dict_parameters(data->documents[i]));
		for (; i_parameter != NULL; geoxml_sequence_next(&i_parameter)) {
			GtkTreeIter *	iter;
			const gchar*	pixbuf;

			pixbuf = (strcmp(geoxml_program_parameter_get_keyword(
			GEOXML_PROGRAM_PARAMETER(i_parameter)), keyword))
				? NULL : GTK_STOCK_DIALOG_WARNING;
			iter = (GtkTreeIter*)geoxml_object_get_user_data(GEOXML_OBJECT(i_parameter));
			gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
				DICT_EDIT_KEYWORD_IMAGE, pixbuf, -1);
		}
	}
}

/* Function: dict_edit_check_duplicate_keyword
 * Check if _keyword_ intended to be used by _parameter_ is already being used
 */
static gboolean
dict_edit_check_duplicate_keyword(struct dict_edit_data * data, GeoXmlProgramParameter * parameter,
const gchar * keyword, gboolean show_error)
{
	GeoXmlDocument *	document;
	GeoXmlSequence *	i_parameter;

	document = geoxml_object_get_owner_document(GEOXML_OBJECT(parameter));
	i_parameter = geoxml_parameters_get_first_parameter(
		geoxml_document_get_dict_parameters(document));
	for (; i_parameter != NULL; geoxml_sequence_next(&i_parameter )) {
		dict_edit_check_duplicates(data, document, keyword);

		if (i_parameter == GEOXML_SEQUENCE(parameter))
			continue;
		if (strcmp(geoxml_program_parameter_get_keyword(
		GEOXML_PROGRAM_PARAMETER(i_parameter)), keyword))
			continue;
		if (show_error)
			libgebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("Duplicate keyword"),
				_("Another parameter already uses this keyword, please choose other."));
		return TRUE;
	}

	return FALSE;
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

/* Function: dict_edit_type_text_to_geoxml_type
 * Convert type combo box text to a GEOXML_PARAMETERTYPE
 */
static enum GEOXML_PARAMETERTYPE
dict_edit_type_text_to_geoxml_type(const gchar * text)
{
	if (!strcmp(text, "string"))
		return GEOXML_PARAMETERTYPE_STRING;
	else if (!strcmp(text, "integer"))
		return GEOXML_PARAMETERTYPE_INT;
	else if (!strcmp(text, "real"))
		return GEOXML_PARAMETERTYPE_FLOAT;
	else
		return GEOXML_PARAMETERTYPE_UNKNOWN;
}

static gboolean
dict_edit_check_empty_keyword(const gchar * keyword)
{
	if (!strlen(keyword)) {
		libgebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("Empty keyword"),
			_("Please select a keyword."));
		return FALSE;
	}
	return TRUE;
}

/* Function: dict_edit_append_iter
 * Append iter and references it on the libgebr_geoxml's object
 */
static GtkTreeIter
dict_edit_append_iter(struct dict_edit_data * data, GeoXmlObject * object, GtkTreeIter * document_iter)
{
	GtkTreeIter	iter;

	gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &iter, document_iter);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
		DICT_EDIT_KEYWORD_IMAGE, GTK_STOCK_DIALOG_WARNING, -1);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
		DICT_EDIT_KEYWORD_IMAGE, NULL, -1);
	geoxml_object_set_user_data(object, gtk_tree_iter_copy(&iter));

	return iter;
}

/* Function: document_get_name_from_type
 * Return the document name given its type
 */
static const gchar *
document_get_name_from_type(GeoXmlDocument * document, gboolean upper)
{
	switch (geoxml_document_get_type(document)) {
	case GEOXML_DOCUMENT_TYPE_PROJECT:
		return upper ? _("Project") : _("project");
	case GEOXML_DOCUMENT_TYPE_LINE:
		return upper ? _("Line") : _("line");
	case GEOXML_DOCUMENT_TYPE_FLOW:
		return upper ? _("Flow") : _("flow");
	}
	return "";
}
