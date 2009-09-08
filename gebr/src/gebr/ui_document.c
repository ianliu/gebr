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
	DICT_EDIT_N_COLUMN,
};

struct dict_edit_data {
	GeoXmlDocument *	document;
	GtkTreeView *		tree_view;
	GtkTreeModel *		tree_model;
	GtkCellRenderer *	cell_renderer_array[4];
};

static void
on_dict_edit_clicked(GtkButton * button, GeoXmlDocument * document);
static void
on_dict_edit_add_clicked(GtkButton * button, GtkListStore * list_store);
static void
on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data * data);
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
GtkTreeModel * tree_model);
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data);
static gboolean
dict_edit_get_selected(struct dict_edit_data * data, GtkTreeIter * iter);
static GtkMenu *
on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data * data);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: document_properties_setup_ui
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
	GtkWidget *	dialog;
	gint		ret;
	GString *	dialog_title;
	GtkWidget *	table;
	GtkWidget *	label;
	GtkWidget *	button;
	GtkWidget *	help_show_button;
	GtkWidget *	help_hbox;
	GtkWidget *	title;
	GtkWidget *	description;
	GtkWidget *	help;
	GtkWidget *	author;
	GtkWidget *	email;

	if (document == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Nothing selected"));
		return FALSE;
	}

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
	gtk_widget_set_size_request(dialog, 390, 260);

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

	/* User email */
	label = gtk_label_new(_("Parameters' dictionary"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	g_signal_connect(GTK_OBJECT(button), "clicked",
		(GCallback)on_dict_edit_clicked, document);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), button, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

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

/*
 * Section: Private
 * Private functions.
 */

/* Function: on_dict_edit_clicked
 * Open dialog for parameters's edition
 */
static void
on_dict_edit_clicked(GtkButton * _button, GeoXmlDocument * document)
{
	GtkWidget *		dialog;
	GtkWidget *		hbox;
	GtkWidget *		button;
	
	GtkWidget *		scrolled_window;
	GtkWidget *		tree_view;
	GtkCellRenderer *	cell_renderer;
	GtkTreeViewColumn *	column;
	GtkListStore *		list_store;
	GtkListStore *		type_model;
	GtkTreeIter		iter;

	struct dict_edit_data *	data;

	data = g_malloc(sizeof(struct dict_edit_data));
	data->document = document;
	dialog = gtk_dialog_new_with_buttons(_("Edit parameters' dictionary"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 500, 340);

	list_store = gtk_list_store_new(4,
		G_TYPE_STRING, /* type */
		G_TYPE_STRING, /* keyword */
		G_TYPE_STRING, /* value */
		G_TYPE_STRING, /* comment */
		G_TYPE_POINTER /* GeoXmlParameter */);
	data->tree_model = GTK_TREE_MODEL(list_store);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	g_signal_connect(GTK_OBJECT(button), "clicked",
		(GCallback)on_dict_edit_add_clicked, data);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), scrolled_window);
	gtk_widget_set_size_request(scrolled_window, 300, -1);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
	data->tree_view = tree_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	libgebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_viewview),
		(GtkPopupCallback)on_dict_edit_popup_menu, data);

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
	column = gtk_tree_view_column_new_with_attributes(_("Type"), cell_renderer, NULL);
	g_object_set(cell_renderer, "has-entry", FALSE, "editable", TRUE, 
		"model", type_model, "text-column", 0, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_type_cell_edited, type_model);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[1] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Keyword"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[2] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Value"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[3] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited",
		(GCallback)on_dict_edit_cell_edited, data);
	column = gtk_tree_view_column_new_with_attributes(_("Comment"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", 3);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	gtk_widget_show_all(GTK_WIDGET(dialog));
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_CLOSE:
		
		break;
	default:
		break;
	}

	g_free(data);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

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

	gtk_list_store_append(GTK_LIST_STORE(data->list_store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, 0, "string", -1);
}

/* Function: on_dict_edit_remove_clicked
 * Remove parameter
 */
static void
on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data * data)
{
	GtkTreeIter	iter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	gtk_tree_model_get(data->)

	gtk_list_store_remove(GTK_LIST_STORE(data->tree_model), &iter);
}

/* Function: on_dict_edit_type_cell_edited
 * Edit type of parameter
 */
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
GtkTreeModel * tree_model)
{
	GtkTreeIter	iter;

	gtk_tree_model_get_iter_from_string(tree_model, &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE(tree_model), 0, new_text, -1);
}

/* Function: on_dict_edit_cell_edited
 * Edit keyword, value and comment of parameter
 */
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
struct dict_edit_data * data)
{
	GtkTreeIter	iter;
	gint		index;

	for (index = 1; index < 3; index++)
		if (cell == data->cell_renderer_array[index])
			break;

	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE(data->tree_model), &iter, index, new_text, -1);
}

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
	GtkWidget *		menu_item;

	menu = gtk_menu_new();

	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate",
		(GCallback)on_dict_edit_add_clicked, data);

	if (dict_edit_get_selected(data, NULL))
		goto out;

	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate",
		(GCallback)on_dict_edit_remove_clicked, data);

out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}
