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

#include <gdk/gdkkeysyms.h>

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
	DICT_EDIT_GEBR_GEOXML_POINTER,
	DICT_EDIT_IS_ADD_PARAMETER,
	DICT_EDIT_KEYWORD_EDITABLE,
	DICT_EDIT_EDITABLE,
	DICT_EDIT_N_COLUMN,
};

struct dict_edit_data {
	GtkWidget *widget;

	GebrGeoXmlDocument *current_document;
	GtkTreeIter current_document_iter;
	GebrGeoXmlDocument *documents[4];
	GtkTreeIter iters[3];

	GtkActionGroup *action_group;
	GtkWidget *tree_view;
	GtkTreeModel *tree_model;
	GtkTreeModel *document_model;
	GtkCellRenderer *cell_renderer_array[10];
};

static void on_dict_edit_cursor_changed(GtkTreeView * tree_view, struct dict_edit_data *data);
static void on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data *data);
static void on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data *data);
static void
on_dict_edit_renderer_editing_started(GtkCellRenderer * renderer,
				      GtkCellEditable * editable, gchar * path, struct dict_edit_data *data);
#if GTK_CHECK_VERSION(2,12,0)
static gboolean
dict_edit_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
			   GtkTreeIter * iter, GtkTreeViewColumn * column, struct dict_edit_data *data);
#endif
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
			      struct dict_edit_data *data);
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text, struct dict_edit_data *data);
static void dict_edit_load_iter(struct dict_edit_data *data, GtkTreeIter * iter, GebrGeoXmlParameter * parameter);
static void
dict_edit_check_duplicates(struct dict_edit_data *data, GebrGeoXmlDocument * document, const gchar * keyword);
static gboolean
dict_edit_check_duplicate_keyword(struct dict_edit_data *data, GebrGeoXmlProgramParameter * parameter,
				  const gchar * keyword, gboolean show_error);
static gboolean dict_edit_get_selected(struct dict_edit_data *data, GtkTreeIter * iter);
static void dict_edit_start_keyword_editing(struct dict_edit_data *data, GtkTreeIter * iter);
static void dict_edit_new_parameter_iter(struct dict_edit_data *data, GebrGeoXmlObject * object, GtkTreeIter * iter);
static void dict_edit_append_add_parameter(struct dict_edit_data *data, GtkTreeIter * document_iter);
static GtkMenu *on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data *data);
static enum GEBR_GEOXML_PARAMETER_TYPE dict_edit_type_text_to_gebr_geoxml_type(const gchar * text);
static gboolean dict_edit_check_empty_keyword(const gchar * keyword);
static GtkTreeIter
dict_edit_append_iter(struct dict_edit_data *data, GebrGeoXmlObject * object, GtkTreeIter * document_iter);

static const gchar *document_get_name_from_type(GebrGeoXmlDocument * document, gboolean upper);

static const GtkActionEntry dict_actions_entries[] = {
	{"add", GTK_STOCK_ADD, NULL, NULL, N_("Add new parameter."),
	 G_CALLBACK(on_dict_edit_add_clicked)},
	{"remove", GTK_STOCK_REMOVE, NULL, NULL, N_("Remove current parameters."),
	 G_CALLBACK(on_dict_edit_remove_clicked)},
};

/*
 * Section: Public
 * Public functions.
 */

/* Function: document_get_current
 * Return current selected and active project, line or flow
 */
GebrGeoXmlDocument *document_get_current(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
	case NOTEBOOK_PAGE_PROJECT_LINE:
		project_line_get_selected(NULL, ProjectLineSelection);
		return gebr.project_line;
	default:
		if (gebr.flow == NULL)
			return NULL;
		flow_browse_get_selected(NULL, TRUE);
		return GEBR_GEOXML_DOCUMENT(gebr.flow);
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
gboolean document_properties_setup_ui(GebrGeoXmlDocument * document)
{
	GtkWidget *dialog;
	gint ret;
	GString *dialog_title;

	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *help_hbox;
	GtkWidget *title;
	GtkWidget *description;
	GtkWidget *help;
	GtkWidget *author;
	GtkWidget *email;

	if (document == NULL)
		return FALSE;

	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		flow_browse_single_selection();

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Properties for %s '%s'"),
			document_get_name_from_type(document, FALSE), gebr_geoxml_document_get_title(document));
	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
					     GTK_WINDOW(gebr.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 260);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Title */
	label = gtk_label_new(_("Title"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	title = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(title), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(title), gebr_geoxml_document_get_title(document));

	/* Description */
	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	description = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(description), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(description), gebr_geoxml_document_get_description(document));

	/* Report */
	label = gtk_label_new(_("Report"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	help_hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), help_hbox, 1, 2, 2, 3,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	help = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_box_pack_start(GTK_BOX(help_hbox), help, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(help), "clicked", G_CALLBACK(help_edit), document);
	g_object_set(G_OBJECT(help), "relief", GTK_RELIEF_NONE, NULL);

	/* Author */
	label = gtk_label_new(_("Author"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	author = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(author), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(author), gebr_geoxml_document_get_author(document));

	/* User email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	email = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(email), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(email), gebr_geoxml_document_get_email(document));

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK:{
			GtkTreeIter iter;

			const gchar *old_title;
			const gchar *new_title;

			enum GEBR_GEOXML_DOCUMENT_TYPE type;

			old_title = gebr_geoxml_document_get_title(document);
			new_title = gtk_entry_get_text(GTK_ENTRY(title));

			gebr_geoxml_document_set_title(document, new_title);
			gebr_geoxml_document_set_description(document, gtk_entry_get_text(GTK_ENTRY(description)));
			gebr_geoxml_document_set_author(document, gtk_entry_get_text(GTK_ENTRY(author)));
			gebr_geoxml_document_set_email(document, gtk_entry_get_text(GTK_ENTRY(email)));
			document_save(document);

			/* Update title in apropriated store */
			switch ((type = gebr_geoxml_document_get_type(document))) {
			case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
			case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
				project_line_get_selected(&iter, DontWarnUnselection);
				gtk_tree_store_set(gebr.ui_project_line->store, &iter,
						   PL_TITLE, gebr_geoxml_document_get_title(document), -1);
				project_line_info_update();
				break;
			case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
				flow_browse_get_selected(&iter, FALSE);
				gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
						   FB_TITLE, gebr_geoxml_document_get_title(document), -1);
				flow_browse_info_update();
				break;
			default:
				break;
			}

			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated."),
				     document_get_name_from_type(document, FALSE), old_title);
			if (strcmp(old_title, new_title) != 0)
				gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'."),
					     document_get_name_from_type(document, FALSE), old_title, new_title);

			ret = TRUE;
			break;
		}
	default:
		ret = FALSE;
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_string_free(dialog_title, TRUE);

	return ret;
}

/* Function: document_dict_edit_setup_ui
 * Open dialog for parameters's edition
 */
void document_dict_edit_setup_ui(void)
{
	GtkWidget *dialog;
	GtkWidget *frame;
	GtkWidget *scrolled_window;
	GtkWidget *tree_view;
	GtkWidget *add_hbox;

	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	GtkCellRenderer *cell_renderer;
	GtkCellRenderer *pixbuf_cell_renderer;
	GtkTreeViewColumn *column;
	GtkTreeStore *tree_store;
	GtkListStore *type_model;
	GtkListStore *document_model;
	GtkTreeIter iter;

	GString *dialog_title;
	GebrGeoXmlDocument *document;
	GebrGeoXmlSequence *parameter;
	struct dict_edit_data *data;

	document = document_get_current();
	if (document == NULL)
		return;

	data = g_malloc(sizeof(struct dict_edit_data));
	tree_store = gtk_tree_store_new(DICT_EDIT_N_COLUMN, G_TYPE_STRING,	/* document */
					G_TYPE_STRING,	/* type */
					G_TYPE_STRING,	/* keyword stock */
					G_TYPE_STRING,	/* keyword */
					G_TYPE_STRING,	/* value */
					G_TYPE_STRING,	/* comment */
					G_TYPE_POINTER,	/* GebrGeoXmlParameter */
					G_TYPE_BOOLEAN,	/* is add parameter */
					G_TYPE_BOOLEAN,	/* keyword editable */
					G_TYPE_BOOLEAN /* editable */ );
	data->tree_model = GTK_TREE_MODEL(tree_store);

	action_group = gtk_action_group_new("dict_edit");
	data->action_group = action_group;
	gtk_action_group_set_translation_domain(action_group, PACKAGE);
	gtk_action_group_add_actions(action_group, dict_actions_entries, G_N_ELEMENTS(dict_actions_entries), data);
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), accel_group);
	gebr_gui_gtk_action_group_set_accel_group(action_group, accel_group);

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Parameter dictionary for %s '%s'"),
			document_get_name_from_type(document, FALSE), gebr_geoxml_document_get_title(document));
	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
					     GTK_WINDOW(gebr.window),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_widget_set_size_request(dialog, 500, 300);

	frame = gtk_frame_new("");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), frame);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_store));
	data->tree_view = tree_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view),
						  (GebrGuiGtkPopupCallback) on_dict_edit_popup_menu, data);
#if GTK_CHECK_VERSION(2,12,0)
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(tree_view),
						    (GebrGuiGtkTreeViewTooltipCallback) dict_edit_tooltip_callback,
						    data);
#endif
	g_signal_connect(GTK_OBJECT(tree_view), "cursor-changed", G_CALLBACK(on_dict_edit_cursor_changed), data);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_DOCUMENT] = cell_renderer;
	column = gtk_tree_view_column_new_with_attributes("", cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "markup", DICT_EDIT_DOCUMENT);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	document_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
	data->document_model = GTK_TREE_MODEL(document_model);

	type_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter, 0, "integer", 1, GEBR_GEOXML_PARAMETER_TYPE_INT, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter, 0, "real", 1, GEBR_GEOXML_PARAMETER_TYPE_FLOAT, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter, 0, "string", 1, GEBR_GEOXML_PARAMETER_TYPE_STRING, -1);
	cell_renderer = gtk_cell_renderer_combo_new();
	data->cell_renderer_array[DICT_EDIT_TYPE] = cell_renderer;
	column = gtk_tree_view_column_new_with_attributes(_("Type"), cell_renderer, NULL);
	gtk_tree_view_column_set_min_width(column, 100);
	g_object_set(cell_renderer, "has-entry", FALSE, "editable", TRUE, "model", type_model, "text-column", 0, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_type_cell_edited), data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "markup", DICT_EDIT_TYPE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_KEYWORD_EDITABLE);
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
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_KEYWORD);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_VALUE] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	column = gtk_tree_view_column_new_with_attributes(_("Value"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_VALUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_COMMENT] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	column = gtk_tree_view_column_new_with_attributes(_("Comment"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_COMMENT);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	/*
	 * Add data widgets
	 */
	add_hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), add_hbox, FALSE, TRUE, 0);

	int i = 0;
	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.flow);
	if (gebr_geoxml_document_get_type(document) != GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.line);
	data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.project);
	data->documents[i] = NULL;

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GtkTreeIter document_iter;
		GString *document_name;

		document_name = g_string_new(NULL);
		g_string_printf(document_name, "<b>%s</b>", document_get_name_from_type(data->documents[i], TRUE));
		gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &document_iter, NULL);
		data->iters[i] = document_iter;
		gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &document_iter,
				   DICT_EDIT_DOCUMENT, document_name->str,
				   DICT_EDIT_GEBR_GEOXML_POINTER, data->documents[i],
				   DICT_EDIT_KEYWORD_EDITABLE, FALSE, DICT_EDIT_EDITABLE, FALSE, -1);
		g_string_free(document_name, TRUE);

		parameter =
		    gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters
							       (data->documents[i]));
		for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
			iter = dict_edit_append_iter(data, GEBR_GEOXML_OBJECT(parameter), &document_iter);
			dict_edit_load_iter(data, &iter, GEBR_GEOXML_PARAMETER(parameter));
			if (i == 0)
				continue;
			dict_edit_check_duplicates(data, data->documents[i],
						   gebr_geoxml_program_parameter_get_keyword
						   (GEBR_GEOXML_PROGRAM_PARAMETER(parameter)));
		}

		dict_edit_append_add_parameter(data, &document_iter);

		gebr_gui_gtk_tree_view_expand_to_iter(GTK_TREE_VIEW(tree_view), &document_iter);
		if (document == data->documents[i])
			gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(tree_view), &document_iter);
	}

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	for (int i = 0; data->documents[i] != NULL; ++i)
		document_save(data->documents[i]);

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GebrGeoXmlSequence *i_parameter;

		i_parameter =
		    gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters
							       (data->documents[i]));
		for (; i_parameter != NULL; gebr_geoxml_sequence_next(&i_parameter))
			gtk_tree_iter_free(gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(i_parameter)));
	}
	g_free(data);
	g_string_free(dialog_title, TRUE);
	gtk_widget_destroy(dialog);
}

/*
 * Section: Private
 * Private functions.
 */

/* Function: on_dict_edit_cursor_changed
 * 
 */
static void on_dict_edit_cursor_changed(GtkTreeView * tree_view, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GtkTreeIter parent;
//      gboolean        is_add_parameter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	if (!gtk_tree_model_iter_parent(data->tree_model, &parent, &iter))
		parent = iter;
	gtk_tree_model_get(data->tree_model, &parent, DICT_EDIT_GEBR_GEOXML_POINTER, &data->current_document, -1);
	data->current_document_iter = parent;

//      gtk_tree_model_get(data->tree_model, &iter,
//              DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter,
//              -1);
//      if (is_add_parameter) {
//              g_signal_handlers_block_matched(G_OBJECT(data->tree_view),
//                      G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
//                      G_CALLBACK(on_dict_edit_cursor_changed), NULL);
//              gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &iter,
//                      gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 1), TRUE);
//              g_signal_handlers_unblock_matched(G_OBJECT(data->tree_view),
//                      G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
//                      G_CALLBACK(on_dict_edit_cursor_changed), NULL);
//      }
}

/* Function: on_dict_edit_add_clicked
 * Add new parameter
 */
static void on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramParameter *parameter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	parameter =
	    GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_parameters_append_parameter
					  (gebr_geoxml_document_get_dict_parameters(data->current_document),
					   GEBR_GEOXML_PARAMETER_TYPE_INT));

	iter = dict_edit_append_iter(data, GEBR_GEOXML_OBJECT(parameter), &data->current_document_iter);
	/* before special parameter for add */
	gebr_gui_gtk_tree_store_move_up(GTK_TREE_STORE(data->tree_model), &iter);
	dict_edit_load_iter(data, &iter, GEBR_GEOXML_PARAMETER(parameter));
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(data->tree_view), &iter);
	dict_edit_start_keyword_editing(data, &iter);
}

/* Function: on_dict_edit_remove_clicked
 * Remove parameter
 */
static void on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *parameter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);

	gebr_geoxml_sequence_remove(parameter);
	gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &iter);
}

static gboolean on_renderer_entry_key_press_event(GtkWidget * widget, GdkEventKey * event, struct dict_edit_data *data)
{
	switch (event->keyval) {
	case GDK_Tab:
	case GDK_Return:{
			GtkCellRenderer *renderer;
			GtkTreeViewColumn *column;
			GtkTreeIter iter;

			g_signal_handlers_disconnect_matched(G_OBJECT(widget),
							     G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
							     G_CALLBACK(on_renderer_entry_key_press_event), data);

			dict_edit_get_selected(data, &iter);

			g_object_get(widget, "user-data", &renderer, NULL);
			gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(widget));

			column = gebr_gui_gtk_tree_view_get_next_column(GTK_TREE_VIEW(data->tree_view),
									gebr_gui_gtk_tree_view_get_column_from_renderer
									(GTK_TREE_VIEW(data->tree_view), renderer));
			if (column != NULL)
				gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &iter, column, TRUE);
			else {
				gboolean is_add_parameter;

				gtk_tree_model_iter_next(data->tree_model, &iter);
				gtk_tree_model_get(data->tree_model, &iter,
						   DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);

				if (is_add_parameter)
					gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &iter,
									  gtk_tree_view_get_column(GTK_TREE_VIEW
												   (data->tree_view),
												   1), TRUE);
				else
					dict_edit_start_keyword_editing(data, &iter);
			}
			return TRUE;
		}
	default:
		return FALSE;
	}
}

/* Function: on_dict_edit_renderer_editing_started
 * 
 */
static void
on_dict_edit_renderer_editing_started(GtkCellRenderer * renderer,
				      GtkCellEditable * editable, gchar * path, struct dict_edit_data *data)
{
	GtkWidget *widget;

	widget = GTK_WIDGET(editable);
	gtk_widget_set_events(GTK_WIDGET(widget), GDK_KEY_PRESS_MASK);
	g_signal_connect(widget, "key-press-event", G_CALLBACK(on_renderer_entry_key_press_event), data);
	g_object_set(widget, "user-data", renderer, NULL);
}

/* Function: dict_edit_tooltip_callback
 * Set tooltip for New iters
 */
#if GTK_CHECK_VERSION(2,12,0)
static gboolean
dict_edit_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
			   GtkTreeIter * iter, GtkTreeViewColumn * column, struct dict_edit_data *data)
{
	gboolean is_add_parameter;

	gtk_tree_model_get(data->tree_model, iter, DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);
	if (is_add_parameter) {
		gtk_tooltip_set_text(tooltip, _("Click to select type for a new parameter"));
		return TRUE;
	}

	return FALSE;
}
#endif

/* Function: on_dict_edit_type_cell_edited
 * Edit type of parameter
 */
static void
on_dict_edit_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
			      struct dict_edit_data *data)
{
	GtkTreeIter iter;
	gboolean is_add_parameter;
	GebrGeoXmlParameter *parameter;
	const gchar *keyword, *value;

	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);
	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);

	if (is_add_parameter) {
		parameter =
		    gebr_geoxml_parameters_append_parameter(gebr_geoxml_document_get_dict_parameters
							    (data->current_document),
							    dict_edit_type_text_to_gebr_geoxml_type(new_text));
		dict_edit_new_parameter_iter(data, GEBR_GEOXML_OBJECT(parameter), &iter);
		dict_edit_load_iter(data, &iter, parameter);
		dict_edit_append_add_parameter(data, &data->current_document_iter);
		dict_edit_start_keyword_editing(data, &iter);
		return;
	}

	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);

	keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter));
	value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE);
	gebr_geoxml_parameter_set_type(parameter, dict_edit_type_text_to_gebr_geoxml_type(new_text));
	/* restore */
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), keyword);
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, value);

	dict_edit_load_iter(data, &iter, parameter);
}

/* Function: on_dict_edit_cell_edited
 * Edit keyword, value and comment of parameter
 */
static void
on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	gint index;
	GebrGeoXmlProgramParameter *parameter;

	for (index = DICT_EDIT_KEYWORD; index < DICT_EDIT_COMMENT; index++)
		if (cell == data->cell_renderer_array[index])
			break;

	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);
	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);
	switch (index) {
	case DICT_EDIT_KEYWORD:
		if (!dict_edit_check_empty_keyword(new_text) ||
		    dict_edit_check_duplicate_keyword(data, parameter, new_text, TRUE)) {
			dict_edit_start_keyword_editing(data, &iter);
			return;
		}

		gebr_geoxml_program_parameter_set_keyword(parameter, new_text);
		break;
	case DICT_EDIT_VALUE:
		switch (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter))) {
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
			new_text = (gchar *) gebr_validate_int(new_text, NULL, NULL);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
			new_text = (gchar *) gebr_validate_float(new_text, NULL, NULL);
			break;
		default:
			break;
		}
		gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, new_text);
		break;
	case DICT_EDIT_COMMENT:
		gebr_geoxml_parameter_set_label(GEBR_GEOXML_PARAMETER(parameter), new_text);
		break;
	default:
		return;
	}
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter, index, new_text, -1);
}

/* Function: dict_edit_load_iter
 * Load _parameter_ into _iter_
 */
static void dict_edit_load_iter(struct dict_edit_data *data, GtkTreeIter * iter, GebrGeoXmlParameter * parameter)
{
	const gchar *type_string;

	switch (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter))) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		type_string = "string";
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		type_string = "integer";
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		type_string = "real";
		break;
	default:
		return;
	}

	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
			   DICT_EDIT_TYPE, type_string,
			   DICT_EDIT_KEYWORD_IMAGE, "",
			   DICT_EDIT_KEYWORD,
			   gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter)),
			   DICT_EDIT_VALUE,
			   gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
									 FALSE), DICT_EDIT_COMMENT,
			   gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)),
			   DICT_EDIT_GEBR_GEOXML_POINTER, parameter, DICT_EDIT_IS_ADD_PARAMETER, FALSE,
			   DICT_EDIT_KEYWORD_EDITABLE, TRUE, DICT_EDIT_EDITABLE, TRUE, -1);
}

/* Function: dict_edit_check_duplicates
 * Check for duplicates keywords for parameters of higher hierarchical documents levels
 */
static void
dict_edit_check_duplicates(struct dict_edit_data *data, GebrGeoXmlDocument * document, const gchar * keyword)
{
	GebrGeoXmlSequence *i_parameter;

	for (int i = 0; data->documents[i] != document; ++i) {
		i_parameter =
		    gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters
							       (data->documents[i]));
		for (; i_parameter != NULL; gebr_geoxml_sequence_next(&i_parameter)) {
			GtkTreeIter *iter;
			const gchar *pixbuf;

			pixbuf =
			    (strcmp
			     (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(i_parameter)),
			      keyword))
			    ? NULL : GTK_STOCK_DIALOG_WARNING;
			iter = (GtkTreeIter *) gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(i_parameter));
			gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter, DICT_EDIT_KEYWORD_IMAGE, pixbuf, -1);
		}
	}
}

/* Function: dict_edit_check_duplicate_keyword
 * Check if _keyword_ intended to be used by _parameter_ is already being used
 */
static gboolean
dict_edit_check_duplicate_keyword(struct dict_edit_data *data, GebrGeoXmlProgramParameter * parameter,
				  const gchar * keyword, gboolean show_error)
{
	GebrGeoXmlDocument *document;
	GebrGeoXmlSequence *i_parameter;

	document = gebr_geoxml_object_get_owner_document(GEBR_GEOXML_OBJECT(parameter));
	i_parameter = gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters(document));
	for (; i_parameter != NULL; gebr_geoxml_sequence_next(&i_parameter)) {
		dict_edit_check_duplicates(data, document, keyword);

		if (i_parameter == GEBR_GEOXML_SEQUENCE(parameter))
			continue;
		if (strcmp
		    (gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(i_parameter)), keyword))
			continue;
		if (show_error)
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Duplicate keyword"),
						_("Other parameter already uses this keyword. Please choose another."));
		return TRUE;
	}

	return FALSE;
}

/* Function: dict_edit_get_selected
 * Get selected iterator into _iter_
 */
static gboolean dict_edit_get_selected(struct dict_edit_data *data, GtkTreeIter * iter)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view));
	return gtk_tree_selection_get_selected(selection, &model, iter);
}

/* Function: dict_edit_start_keyword_editing
 * Set keyword column in _iter_
 */
static void dict_edit_start_keyword_editing(struct dict_edit_data *data, GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), iter,
					  gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 2), TRUE);
}

/* Function: dict_edit_append_add_parameter
 * Add special parameter for easy new parameter creation
 */
static void dict_edit_append_add_parameter(struct dict_edit_data *data, GtkTreeIter * document_iter)
{
	GtkTreeIter iter;

	gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &iter, document_iter);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
			   DICT_EDIT_TYPE, _("<i>New</i>"),
			   DICT_EDIT_IS_ADD_PARAMETER, TRUE,
			   DICT_EDIT_KEYWORD_EDITABLE, TRUE, DICT_EDIT_EDITABLE, FALSE, -1);
}

/* Function: on_dict_edit_popup_menu
 * Popup menu for parameter removal, etc
 */
static GtkMenu *on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data *data)
{
	GtkWidget *menu;
	GtkTreeIter iter;
	gboolean is_add_parameter;

	if (!dict_edit_get_selected(data, &iter))
		return NULL;
	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);
	if (is_add_parameter)
		return NULL;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(data->action_group, "add")));

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &data->current_document_iter))
		goto out;

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(data->action_group, "remove")));

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/* Function: dict_edit_type_text_to_gebr_geoxml_type
 * Convert type combo box text to a GEBR_GEOXML_PARAMETER_TYPE
 */
static enum GEBR_GEOXML_PARAMETER_TYPE dict_edit_type_text_to_gebr_geoxml_type(const gchar * text)
{
	if (!strcmp(text, "string"))
		return GEBR_GEOXML_PARAMETER_TYPE_STRING;
	else if (!strcmp(text, "integer"))
		return GEBR_GEOXML_PARAMETER_TYPE_INT;
	else if (!strcmp(text, "real"))
		return GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
	else
		return GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
}

static gboolean dict_edit_check_empty_keyword(const gchar * keyword)
{
	if (!strlen(keyword)) {
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					_("Empty keyword"), _("Please select a keyword."));
		return FALSE;
	}
	return TRUE;
}

/* Function: dict_edit_new_parameter_iter
 * New parameter
 */
static void dict_edit_new_parameter_iter(struct dict_edit_data *data, GebrGeoXmlObject * object, GtkTreeIter * iter)
{
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
			   DICT_EDIT_KEYWORD_IMAGE, GTK_STOCK_DIALOG_WARNING, -1);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter, DICT_EDIT_KEYWORD_IMAGE, NULL, -1);
	gebr_geoxml_object_set_user_data(object, gtk_tree_iter_copy(iter));
}

/* Function: dict_edit_append_iter
 * Append iter
 */
static GtkTreeIter
dict_edit_append_iter(struct dict_edit_data *data, GebrGeoXmlObject * object, GtkTreeIter * document_iter)
{
	GtkTreeIter iter;

	gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &iter, document_iter);
	dict_edit_new_parameter_iter(data, object, &iter);

	return iter;
}

/* Function: document_get_name_from_type
 * Return the document name given its type
 */
static const gchar *document_get_name_from_type(GebrGeoXmlDocument * document, gboolean upper)
{
	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		return upper ? _("Project") : _("project");
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		return upper ? _("Line") : _("line");
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		return upper ? _("Flow") : _("flow");
	}
	return "";
}
