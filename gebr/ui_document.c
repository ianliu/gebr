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

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-value-sequence-edit.h>
#include <libgebr/gui/gebr-gui-file-entry.h>

#include "ui_parameters.h"
#include "ui_document.h"
#include "ui_flow_edition.h"
#include "gebr.h"
#include "defines.h"
#include "flow.h"
#include "ui_help.h"
#include "document.h"
#include "ui_project_line.h"
#include "ui_paths.h"
#include "line.h"
#include "gebr-expr.h"
#include "gebr-iexpr.h"

enum {
	DICT_EDIT_KEYWORD_IMAGE,
	DICT_EDIT_KEYWORD,
	DICT_EDIT_VALUE_TYPE,
	DICT_EDIT_VALUE,
	DICT_EDIT_VALUE_TYPE_IMAGE,
	DICT_EDIT_VALUE_TYPE_TOOLTIP,
	DICT_EDIT_COMMENT,
	DICT_EDIT_GEBR_GEOXML_POINTER,
	DICT_EDIT_IS_ADD_PARAMETER,
	DICT_EDIT_KEYWORD_EDITABLE,
	DICT_EDIT_VALUE_TYPE_VISIBLE,
	DICT_EDIT_VALUE_VISIBLE,
	DICT_EDIT_EDITABLE,
	DICT_EDIT_SENSITIVE,
	DICT_EDIT_N_COLUMN,
};

struct dict_edit_data {
	GtkWidget *widget;

	GebrGeoXmlDocument *current_document;
	GtkTreeIter current_document_iter;
	GebrGeoXmlDocument *documents[4];
	GtkTreeIter iters[3];

	GtkActionGroup *action_group;
	GtkTreeModel *tree_model;
	GtkTreeModel *type_model;
	GtkCellRenderer *cell_renderer_array[20];

	GtkWidget *warning_image;
	GtkWidget *label;
	GtkWidget *event_box;
	GtkWidget *tree_view;

	/* in edition parameters */
	GtkCellEditable *in_edition; /* NULL if not editing */
	GtkTreeIter editing_iter;
	GtkCellRenderer *editing_cell;
	gboolean edition_valid;
	gboolean guard;
	gboolean is_inserting_new;
};

typedef struct {
	GebrGeoXmlDocument * document;
	GtkWidget * window;
	GtkWidget * author;
	GtkWidget * description;
	GtkWidget * email;
	GtkWidget * title;

	/* line stuff */
	GtkWidget *path_sequence_edit;
	GtkWidget *groups_combo;
	gint previous_active_group;

	GebrPropertiesResponseFunc func;
	gboolean accept_response;
} GebrPropertiesData;

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void on_dict_edit_cursor_changed(GtkTreeView * tree_view, struct dict_edit_data *data);

static void on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data *data);

static void on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data *data);

static void on_dict_edit_renderer_editing_started(GtkCellRenderer * renderer, GtkCellEditable * editable, gchar * path,
						  struct dict_edit_data *data);

static gboolean dict_edit_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
					   GtkTreeIter * iter, GtkTreeViewColumn * column, struct dict_edit_data *data);
static gboolean on_dict_edit_tree_view_button_press_event(GtkWidget * widget, GdkEventButton * event, struct dict_edit_data *data);

static gboolean dict_edit_validate_editing_cell(struct dict_edit_data *data, gboolean start_edition, gboolean cancel_edition);
static void on_dict_edit_value_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
						struct dict_edit_data *data);

static void on_value_type_editing_canceled(GtkCellRenderer *cell, struct dict_edit_data *data);

static void on_dict_edit_editing_cell_canceled(GtkCellRenderer * cell, struct dict_edit_data *data);
static void on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
				     struct dict_edit_data *data);

static void dict_edit_load_iter(struct dict_edit_data *data, GtkTreeIter * iter, GebrGeoXmlParameter * parameter);

static void dict_edit_check_duplicates(struct dict_edit_data *data, GebrGeoXmlDocument * document,
				       const gchar * keyword);

static gboolean dict_edit_check_duplicate_keyword(struct dict_edit_data *data, GebrGeoXmlProgramParameter * parameter,
						  const gchar * keyword, gboolean show_error);

static gboolean dict_edit_get_selected(struct dict_edit_data *data, GtkTreeIter * iter);

static void dict_edit_start_keyword_editing(struct dict_edit_data *data, GtkTreeIter * iter);

static void dict_edit_append_add_parameter(struct dict_edit_data *data, GtkTreeIter * document_iter);

static GtkMenu *on_dict_edit_popup_menu(GtkWidget * widget, struct dict_edit_data *data);

static GtkTreeIter dict_edit_append_iter(struct dict_edit_data *data, GebrGeoXmlObject * object,
					 GtkTreeIter * document_iter);

static const gchar *document_get_name_from_type(GebrGeoXmlDocument * document, gboolean upper);

void on_response_ok(GtkButton * button, GebrPropertiesData * data);

void on_properties_destroy(GtkWindow * window, GebrPropertiesData * data);

static void on_file_entry_activate (GtkEntry *entry, GebrGuiSequenceEdit *sequence_edit);

static void on_groups_combo_box_changed (GtkComboBox *combo, GebrGuiValueSequenceEdit *edit);

static gboolean dict_edit_reorder(GtkTreeView            *tree_view,
				  GtkTreeIter            *iter,
				  GtkTreeIter            *position,
				  GtkTreeViewDropPosition drop_position,
				  struct dict_edit_data  *data);

static gboolean dict_edit_can_reorder(GtkTreeView            *tree_view,
				      GtkTreeIter            *iter,
				      GtkTreeIter            *position,
				      GtkTreeViewDropPosition drop_position,
				      struct dict_edit_data  *data);

static const GtkActionEntry dict_actions_entries[] = {
	{"add", GTK_STOCK_ADD, NULL, NULL, N_("Add new parameter."),
	 G_CALLBACK(on_dict_edit_add_clicked)},
	{"remove", GTK_STOCK_REMOVE, NULL, NULL, N_("Remove current parameters."),
	 G_CALLBACK(on_dict_edit_remove_clicked)},
};

static void gebr_dict_update_wizard(struct dict_edit_data *data);

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

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

void document_properties_setup_ui (GebrGeoXmlDocument * document,
				   GebrPropertiesResponseFunc func,
				   gboolean is_new)
{
	int row = 0;
	GebrPropertiesData * data;
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button_box;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	gchar *window_title;

	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *title;
	GtkWidget *description;
	GtkWidget *author;
	GtkWidget *email;
	GtkWidget *line_path_label;
	GtkWidget *file_entry;
	GtkWidget *path_sequence_edit;
	GtkWidget *groups_combo;
	GebrGeoXmlSequence *path_sequence;

	if (document == NULL)
		return;

	data = g_new(GebrPropertiesData, 1);
	data->func = func;
	data->accept_response = FALSE;
	data->document = document;

	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		flow_browse_single_selection();

	data->window = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(gebr.window));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

	window_title = g_strdup_printf(_("%s '%s' properties"),
				       document_get_name_from_type(document, TRUE),
				       gebr_geoxml_document_get_title(document));
	gtk_window_set_title(GTK_WINDOW(window), window_title);
	g_free(window_title);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, -1);

	table = gtk_table_new(5, 2, FALSE);
	button_box = gtk_hbutton_box_new();

	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	g_signal_connect(window, "destroy", G_CALLBACK(on_properties_destroy), data);
	g_signal_connect(ok_button, "clicked", G_CALLBACK(on_response_ok), data);
	g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), window);

	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(button_box), cancel_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(button_box), ok_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, TRUE, 0);

	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(ok_button);

	/* Title */
	label = gtk_label_new(_("Title"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	data->title = title = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(title), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), title, 1, 2, row, row+1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	row++;

	/* read */
	gtk_entry_set_text(GTK_ENTRY(title), gebr_geoxml_document_get_title(document));

	/* Description */
	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	data->description = description = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(description), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), description, 1, 2, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	row++;

	/* read */
	gtk_entry_set_text(GTK_ENTRY(description), gebr_geoxml_document_get_description(document));

	/* Author */
	label = gtk_label_new(_("Author"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	data->author = author = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(author), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), author, 1, 2, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	row++;

	/* read */
	gtk_entry_set_text(GTK_ENTRY(author), gebr_geoxml_document_get_author(document));

	/* User email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	data->email = email = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(email), TRUE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email, 1, 2, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
	row++;

	/* read */
	gtk_entry_set_text(GTK_ENTRY(email), gebr_geoxml_document_get_email(document));

	data->previous_active_group = 0;
	data->groups_combo = NULL;
	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
	{
		gboolean is_fs, is_fs2;
		GtkTreeIter active, iter;
		const gchar *curr_group;
		GtkTreeModel *model;

		model = GTK_TREE_MODEL (gebr.ui_server_list->common.combo_store);
		gtk_tree_model_get_iter_first (model, &active);
		data->groups_combo = groups_combo = ui_server_create_tag_combo_box ();
		curr_group = gebr_geoxml_line_get_group (GEBR_GEOXML_LINE (document), &is_fs);

		/* If group is empty, select All Servers */
		if (strlen (curr_group) == 0)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (groups_combo), &active);
		else {
			/* Otherwise, skip the first entry and search for the current group */
			gboolean valid;
			iter = active;
			valid = gtk_tree_model_iter_next (model, &iter);
			while (valid) {
				gboolean is_sep;
				gchar *name;
				gtk_tree_model_get (model, &iter,
						    TAG_SEP, &is_sep,
						    TAG_NAME, &name,
						    TAG_FS, &is_fs2,
						    -1);
				if (!is_sep && is_fs == is_fs2 && g_strcmp0 (name, curr_group) == 0) {
					active = iter;
					g_free (name);
					break;
				}
				g_free (name);
				valid = gtk_tree_model_iter_next (model, &iter);
			}
			/* If valid = TRUE, we found the group */
			if (valid || is_new)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (groups_combo), &active);
			else
				gtk_combo_box_set_active (GTK_COMBO_BOX (groups_combo), -1);
		}

		data->previous_active_group = gtk_combo_box_get_active(GTK_COMBO_BOX(groups_combo));

		label = gtk_label_new (_("Server group"));
		gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
		gtk_table_attach (GTK_TABLE (table), groups_combo, 1, 2, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
		row++;
		gtk_widget_show (label);
		gtk_widget_show (groups_combo);

		/* Line Path's*/
		line_path_label = gtk_label_new(_("Path"));
		gtk_widget_show(line_path_label);
		gtk_table_attach(GTK_TABLE(table), line_path_label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 3);
		gtk_misc_set_alignment(GTK_MISC(line_path_label), 0, 0);

		file_entry = gebr_gui_file_entry_new(NULL, NULL);

		gebr_gui_file_entry_set_choose_directory(GEBR_GUI_FILE_ENTRY(file_entry), TRUE);
		gtk_widget_set_size_request(file_entry, 220, 30);

		gebr_geoxml_line_get_path(gebr.line, &path_sequence, 0);
		data->path_sequence_edit = path_sequence_edit = gebr_gui_value_sequence_edit_new(file_entry);
		gebr_gui_value_sequence_edit_load(GEBR_GUI_VALUE_SEQUENCE_EDIT(path_sequence_edit), path_sequence,
						  (ValueSequenceSetFunction) gebr_geoxml_value_sequence_set,
						  (ValueSequenceGetFunction) gebr_geoxml_value_sequence_get, NULL);

		g_signal_connect (groups_combo, "changed",
				  G_CALLBACK (on_groups_combo_box_changed), path_sequence_edit);

		g_signal_connect (GEBR_GUI_FILE_ENTRY (file_entry)->entry, "activate",
				  G_CALLBACK (on_file_entry_activate), path_sequence_edit);
		g_signal_connect (path_sequence_edit, "add-request",
				  G_CALLBACK (path_add), NULL);
		g_signal_connect (path_sequence_edit, "changed",
				  G_CALLBACK (path_save), NULL);
		g_signal_connect (path_sequence_edit, "renamed",
				  G_CALLBACK (path_renamed), NULL);

		gtk_table_attach (GTK_TABLE (table),
				  path_sequence_edit, 1, 2, row, row+1,
				  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
				  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
				  3, 3);
	}

	gtk_widget_show_all(window);
}

static void
validate_param_and_set_icon_tooltip(struct dict_edit_data *data, GtkTreeIter *iter)
{
	const gchar *keyword;
	GError *error = NULL;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameterType type;

	gtk_tree_model_get(data->tree_model, iter, DICT_EDIT_GEBR_GEOXML_POINTER, &param, -1);

	// Avoid rows not having a parameter
	if (!param)
		return;

	keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));
	type = gebr_geoxml_parameter_get_type(param);

	if (G_UNLIKELY(g_str_equal(keyword, "iter")))
		return;

	gchar * tooltip = NULL;
	gebr_validator_evaluate(gebr.validator, param, NULL, type, &tooltip, &error);
	if (error) {
		gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
				   DICT_EDIT_VALUE_TYPE_IMAGE, GTK_STOCK_DIALOG_WARNING,
				   DICT_EDIT_VALUE_TYPE_TOOLTIP, error->message, -1);

		gtk_label_set_text(GTK_LABEL(data->label), error->message);
		g_clear_error(&error);
	} else {
		gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
				   DICT_EDIT_VALUE_TYPE_IMAGE,
				   type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "string-icon" : "integer-icon",
				   DICT_EDIT_VALUE_TYPE_TOOLTIP,
				   tooltip, -1);
	}
	g_free(tooltip);
}

static void
validate_dict_iter(struct dict_edit_data *data, GtkTreeIter *iter)
{
	GtkTreeIter it;
	GtkTreeIter child;
	gboolean valid = gtk_tree_model_get_iter_first(data->tree_model, &it);

	while (valid)
	{
		valid = gtk_tree_model_iter_children(data->tree_model, &child, &it);
		while (valid)
		{
			validate_param_and_set_icon_tooltip(data, &child);
			valid = gtk_tree_model_iter_next(data->tree_model, &child);
		}

		valid = gtk_tree_model_iter_next(data->tree_model, &it);
	}
}

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
	GtkTreeIter iter;

	GString *dialog_title;
	GebrGeoXmlDocument *document;
	GebrGeoXmlSequence *parameter;
	struct dict_edit_data *data;

	document = document_get_current();
	if (document == NULL)
		return;

	GtkWidget *entry_for_binding = gtk_entry_new();
	GtkBindingSet *binding_set = gtk_binding_set_by_class(GTK_ENTRY_GET_CLASS(entry_for_binding));
	gtk_binding_entry_remove(binding_set, GDK_Return, 0);
	gtk_binding_entry_remove(binding_set, GDK_ISO_Enter, 0);
	gtk_binding_entry_remove(binding_set, GDK_KP_Enter, 0);
	gtk_widget_destroy(entry_for_binding);

	data = g_new(struct dict_edit_data, 1);
	data->in_edition = NULL;
	data->editing_cell = NULL;
	data->is_inserting_new = FALSE;
	tree_store = gtk_tree_store_new(DICT_EDIT_N_COLUMN,
					G_TYPE_STRING,	/* keyword stock */
					G_TYPE_STRING,	/* keyword */
					G_TYPE_STRING,	/* value's type */
					G_TYPE_STRING,	/* value */
					G_TYPE_STRING,  /* Type stock */
					G_TYPE_STRING,  /* Tooltip text */
					G_TYPE_STRING,	/* comment */
					G_TYPE_POINTER,	/* GebrGeoXmlParameter */
					G_TYPE_BOOLEAN,	/* is add parameter */
					G_TYPE_BOOLEAN,	/* wheter value type rend. is visible */
					G_TYPE_BOOLEAN,	/* wheter value rend. is visible */
					G_TYPE_BOOLEAN,	/* keyword editable */
					G_TYPE_BOOLEAN, /* editable */
				        G_TYPE_BOOLEAN  /* sensitive */ );
	data->tree_model = GTK_TREE_MODEL(tree_store);

	data->action_group = action_group = gtk_action_group_new("dict_edit");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, dict_actions_entries, G_N_ELEMENTS(dict_actions_entries), data);
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), accel_group);
	gebr_gui_gtk_action_group_set_accel_group(action_group, accel_group);

	dialog_title = g_string_new(NULL);
	g_string_printf(dialog_title, _("Parameter dictionary for %s '%s'"),
			document_get_name_from_type(document, FALSE), gebr_geoxml_document_get_title(document));
	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_widget_set_size_request(dialog, 600, 500);

	GtkWidget *warning_image = gtk_image_new();
	GtkWidget *label = gtk_label_new("");

	data->warning_image = warning_image;
	data->label = label;
	gtk_image_set_from_stock(GTK_IMAGE(warning_image), GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);

	GtkWidget *message_hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(message_hbox), warning_image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(message_hbox), label, FALSE, TRUE, 0);

	data->event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(data->event_box), message_hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), data->event_box, FALSE, TRUE, 0);

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
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(tree_view),
						    (GebrGuiGtkTreeViewTooltipCallback) dict_edit_tooltip_callback,
						    data);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(tree_view),
						    (GebrGuiGtkTreeViewReorderCallback)dict_edit_reorder,
						    (GebrGuiGtkTreeViewReorderCallback)dict_edit_can_reorder,
						    data);
	g_signal_connect(GTK_OBJECT(tree_view), "cursor-changed", G_CALLBACK(on_dict_edit_cursor_changed), data);
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(on_dict_edit_tree_view_button_press_event), data);

	/* keyword */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Keyword"));
	cell_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell_renderer, FALSE);
	data->cell_renderer_array[DICT_EDIT_KEYWORD] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-canceled", G_CALLBACK(on_dict_edit_editing_cell_canceled), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "markup", DICT_EDIT_KEYWORD);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_KEYWORD_EDITABLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "sensitive", DICT_EDIT_SENSITIVE);
	pixbuf_cell_renderer = gtk_cell_renderer_pixbuf_new();
	data->cell_renderer_array[DICT_EDIT_KEYWORD_IMAGE] = pixbuf_cell_renderer;
//	gtk_tree_view_column_add_attribute(column, pixbuf_cell_renderer, "stock-id", DICT_EDIT_KEYWORD_IMAGE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	/* value */
	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_VALUE] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-canceled", G_CALLBACK(on_dict_edit_editing_cell_canceled), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Value"));
	gtk_tree_view_column_pack_start(column, cell_renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_VALUE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "visible", DICT_EDIT_VALUE_VISIBLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "sensitive", DICT_EDIT_SENSITIVE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(data->tree_view), DICT_EDIT_VALUE_TYPE_TOOLTIP);
	cell_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_end(column, cell_renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "stock-id", DICT_EDIT_VALUE_TYPE_IMAGE);

	/* value type */
	GtkListStore *type_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	data->type_model = GTK_TREE_MODEL(type_model);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter, 0, _("Number"), 1, GEBR_GEOXML_PARAMETER_TYPE_FLOAT, -1);
	gtk_list_store_append(type_model, &iter);
	gtk_list_store_set(type_model, &iter, 0, _("String"), 1, GEBR_GEOXML_PARAMETER_TYPE_STRING, -1);
	cell_renderer = gtk_cell_renderer_combo_new();
	data->cell_renderer_array[DICT_EDIT_VALUE_TYPE] = cell_renderer;
	gtk_tree_view_column_pack_start(column, cell_renderer, FALSE);
	gtk_tree_view_column_set_min_width(column, 200);
	g_object_set(cell_renderer, "has-entry", FALSE, "editable", TRUE, "model", type_model, "text-column", 0, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_value_type_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-canceled", G_CALLBACK(on_value_type_editing_canceled), data);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_VALUE_TYPE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "visible", DICT_EDIT_VALUE_TYPE_VISIBLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "sensitive", DICT_EDIT_SENSITIVE);
	/* add column */
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	cell_renderer = gtk_cell_renderer_text_new();
	data->cell_renderer_array[DICT_EDIT_COMMENT] = cell_renderer;
	g_object_set(cell_renderer, "editable", TRUE, NULL);
	g_signal_connect(cell_renderer, "edited", G_CALLBACK(on_dict_edit_cell_edited), data);
	g_signal_connect(cell_renderer, "editing-canceled", G_CALLBACK(on_dict_edit_editing_cell_canceled), data);
	g_signal_connect(cell_renderer, "editing-started", G_CALLBACK(on_dict_edit_renderer_editing_started), data);
	column = gtk_tree_view_column_new_with_attributes(_("Comment"), cell_renderer, NULL);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "text", DICT_EDIT_COMMENT);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "editable", DICT_EDIT_EDITABLE);
	gtk_tree_view_column_add_attribute(column, cell_renderer, "sensitive", DICT_EDIT_SENSITIVE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	/*
	 * Add data widgets
	 */
	add_hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), add_hbox, FALSE, TRUE, 0);

	int i = 0;
	data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.project);
	if (gebr_geoxml_document_get_type(document) != GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.line);
	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		data->documents[i++] = GEBR_GEOXML_DOCUMENT(gebr.flow);
	data->documents[i] = NULL;

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GtkTreeIter document_iter;
		GString *document_name;

		document_name = g_string_new(NULL);
		g_string_printf(document_name, "<b>%s</b>", document_get_name_from_type(data->documents[i], TRUE));
		gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &document_iter, NULL);
		data->iters[i] = document_iter;
		gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &document_iter,
				   DICT_EDIT_KEYWORD, document_name->str,
				   DICT_EDIT_GEBR_GEOXML_POINTER, data->documents[i],
				   DICT_EDIT_KEYWORD_EDITABLE, FALSE, DICT_EDIT_EDITABLE, FALSE, DICT_EDIT_SENSITIVE, TRUE, -1);
		g_string_free(document_name, TRUE);

		parameter = gebr_geoxml_document_get_dict_parameter(data->documents[i]);
		for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
			iter = dict_edit_append_iter(data, GEBR_GEOXML_OBJECT(parameter), &document_iter);
			dict_edit_check_duplicates(data, data->documents[i],
						   gebr_geoxml_program_parameter_get_keyword
						   (GEBR_GEOXML_PROGRAM_PARAMETER(parameter)));
			gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE);
			validate_param_and_set_icon_tooltip(data, &iter);
		}
		dict_edit_append_add_parameter(data, &document_iter);

		gebr_gui_gtk_tree_view_expand_to_iter(GTK_TREE_VIEW(tree_view), &document_iter);
		if (document == data->documents[i])
			gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(tree_view), &document_iter);
		if(data->documents[i+1] == NULL) {
			GtkTreePath *tree_path;
			tree_path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view)), &document_iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), tree_path, TRUE);
			gtk_tree_path_free(tree_path);
		}
	}
	gebr_dict_update_wizard(data);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	flow_edition_revalidate_programs();

	for (int i = 0; data->documents[i] != NULL; ++i)
		document_save(data->documents[i], TRUE, TRUE);

	for (int i = 0; data->documents[i] != NULL; ++i) {
		GebrGeoXmlSequence *i_parameter;

		i_parameter =
		    gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_document_get_dict_parameters
							       (data->documents[i]));
		for (; i_parameter != NULL; gebr_geoxml_sequence_next(&i_parameter)) {
			gtk_tree_iter_free(gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(i_parameter)));

			GebrGeoXmlObject * object = GEBR_GEOXML_OBJECT(i_parameter);
			gebr_geoxml_object_set_user_data(object, NULL);
		}

	}
	g_free(data);
	g_string_free(dialog_title, TRUE);
	gtk_widget_destroy(dialog);

	/* restore binding sets */
	entry_for_binding = gtk_entry_new();
	binding_set = gtk_binding_set_by_class(GTK_ENTRY_GET_CLASS(entry_for_binding));
	gtk_binding_entry_add_signal(binding_set, GDK_Return, 0, "activate", 0);
	gtk_binding_entry_add_signal(binding_set, GDK_ISO_Enter, 0, "activate", 0);
	gtk_binding_entry_add_signal(binding_set, GDK_KP_Enter, 0, "activate", 0);
	gtk_widget_destroy(entry_for_binding);

}

static GList *program_list_from_used_variables(const gchar *var_name)
{
	GtkTreeIter iter;
	GList *list = NULL;
	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
	for(; valid; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter)) {
		GebrGeoXmlProgram *program;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		if (!program)
			continue;

		if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
			continue;

		if (gebr_geoxml_program_is_var_used(program, var_name))
			list = g_list_prepend(list, gtk_tree_iter_copy(&iter));
	}
	return list;
}

static void program_list_warn_undefined_variable(GList * program_list, gboolean var_exists)
{
	GebrGeoXmlProgram *program;

	for (GList *i = program_list; i; i = i->next) {
		GtkTreeIter *it = i->data;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), it,
		                   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		if (!var_exists) {
			flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, it);
			gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_UNDEF_VAR);
		} else {
			if (validate_program_iter(it, NULL))
				flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, it);
			else
				flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, it);
		}
	}
}

static void program_list_free(GList * program_list)
{
	g_list_foreach(program_list,(GFunc)gtk_tree_iter_free, NULL);
	g_list_free(program_list);
}

void dict_edit_check_programs_using_variables(const gchar *var_name, gboolean var_exists)
{
	GList *list = program_list_from_used_variables(var_name);
	program_list_warn_undefined_variable(list, var_exists);
	program_list_free(list);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

static gint dict_edit_get_column_index_for_renderer(GtkCellRenderer *renderer, struct dict_edit_data *data)
{
	gint index;
	for (index = DICT_EDIT_KEYWORD; index < DICT_EDIT_COMMENT; index++)
		if (renderer == data->cell_renderer_array[index])
			break;
	return index;
}

static void gebr_dict_update_wizard(struct dict_edit_data *data) {

	g_object_set(G_OBJECT(data->warning_image),
		     "stock", GTK_STOCK_INFO,
		     "icon-size", GTK_ICON_SIZE_MENU, NULL);

	if (!data->in_edition) {
		gtk_label_set_text(GTK_LABEL(data->label), _("Click 'New' to create a variable."));
		return;
	}

	GebrGeoXmlProgramParameter *parameter;
	gtk_tree_model_get(data->tree_model, &data->editing_iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);

	switch (dict_edit_get_column_index_for_renderer(data->editing_cell, data)) {
	case DICT_EDIT_KEYWORD:
		gtk_label_set_text(GTK_LABEL(data->label),
				   _("Please enter an unique variable name."));
		break;
	case DICT_EDIT_VALUE: {
		GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter));
		switch (type) {
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
			gtk_label_set_text(GTK_LABEL(data->label),
					   _("Please enter a number."));
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_STRING:
			gtk_label_set_text(GTK_LABEL(data->label),
					   _("Please enter a text."));
			break;
		default:
			break;
		}
		break;
	} case DICT_EDIT_COMMENT:
		gtk_label_set_text(GTK_LABEL(data->label),
		                   _("Please enter a comment."));
		break;
	}

}

static void on_dict_edit_cursor_changed(GtkTreeView * tree_view, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GtkTreeIter parent;

	if (!dict_edit_get_selected(data, &iter))
		return;

	if (!gtk_tree_model_iter_parent(data->tree_model, &parent, &iter))
		parent = iter;
	gtk_tree_model_get(data->tree_model, &parent, DICT_EDIT_GEBR_GEOXML_POINTER, &data->current_document, -1);
	data->current_document_iter = parent;
}

/*
 * on_dict_edit_add_clicked:
 * Add new parameter
 */
static void on_dict_edit_add_clicked(GtkButton * button, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramParameter *parameter;

	if (!dict_edit_get_selected(data, &iter))
		return;

	parameter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_parameters_append_parameter
						  (gebr_geoxml_document_get_dict_parameters(data->current_document),
						   GEBR_GEOXML_PARAMETER_TYPE_INT));
	gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE);

	iter = dict_edit_append_iter(data, GEBR_GEOXML_OBJECT(parameter), &data->current_document_iter);
	/* before special parameter for add */
	gebr_gui_gtk_tree_store_move_up(GTK_TREE_STORE(data->tree_model), &iter);
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(data->tree_view), &iter);

	dict_edit_start_keyword_editing(data, &iter);
}

/*
 * on_dict_edit_remove_clicked:
 * Remove parameter
 */
static void on_dict_edit_remove_clicked(GtkButton * button, struct dict_edit_data *data)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *parameter;
	gboolean is_editable = FALSE;
	const gchar *var_name;
	GList *affected;
	GError *err = NULL;

	if (!dict_edit_get_selected(data, &iter))
		return;

	gtk_tree_model_get(data->tree_model, &iter,
			   DICT_EDIT_GEBR_GEOXML_POINTER, &parameter,
			   DICT_EDIT_EDITABLE, &is_editable,
			   -1);

	if (is_editable) {
		var_name = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter));
		GList *list = program_list_from_used_variables(var_name);

		if (list) {
			gboolean confirmed = gebr_gui_confirm_action_dialog(_("Do you really want to delete this variable?"),
									    _("One or more programs use this variable. Deleting it will invalidate those programs."));
			if (confirmed) {
				gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), &affected, &err);
				validate_dict_iter(data, &iter);
				gebr_geoxml_sequence_remove(parameter);
				gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &iter);
				program_list_warn_undefined_variable(list, FALSE);
			}
			program_list_free(list);
		}
		else {
			gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), &affected, &err);
			validate_dict_iter(data, &iter);
			gebr_geoxml_sequence_remove(parameter);
			gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &iter);
		}
	}
	gebr_dict_update_wizard(data);
}

static gboolean on_dict_edit_tree_view_button_press_event(GtkWidget * widget, GdkEventButton * event, struct dict_edit_data *data)
{
	if (data->in_edition && !dict_edit_validate_editing_cell(data, FALSE, FALSE)) {
		GtkTreeViewColumn *column = gebr_gui_gtk_tree_view_get_column_from_renderer(GTK_TREE_VIEW(data->tree_view), data->editing_cell);
		 return column == gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 0);
	}
	return FALSE;
}

static gboolean on_renderer_entry_key_press_event(GtkWidget * widget, GdkEventKey * event, struct dict_edit_data *data)
{
	switch (event->keyval) {
	case GDK_Down: 
	case GDK_Tab:
	case GDK_ISO_Enter:
	case GDK_KP_Enter:
	case GDK_Return: {

		GtkTreeViewColumn *column = gebr_gui_gtk_tree_view_get_column_from_renderer(GTK_TREE_VIEW(data->tree_view), data->editing_cell);
		GtkTreeViewColumn *next_column = gebr_gui_gtk_tree_view_get_next_column(GTK_TREE_VIEW(data->tree_view), column);

		if (data->in_edition)
			dict_edit_validate_editing_cell(data, FALSE, FALSE);

		if (!data->edition_valid)
			if (column == gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 0) ||
			    event->keyval == GDK_Return ||
			    event->keyval == GDK_ISO_Enter ||
			    event->keyval == GDK_KP_Enter)
				return TRUE;

		gtk_cell_editable_editing_done(data->in_edition);

		if (next_column != NULL)
			gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &data->editing_iter, next_column, TRUE);
		else {
			gboolean is_add_parameter;

			gtk_tree_model_iter_next(data->tree_model, &data->editing_iter);
			gtk_tree_model_get(data->tree_model, &data->editing_iter,
					   DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);

			if (is_add_parameter)
				gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &data->editing_iter,
								  gtk_tree_view_get_column(GTK_TREE_VIEW
											   (data->tree_view),
											   0), TRUE);
			else
				dict_edit_start_keyword_editing(data, &data->editing_iter);
		}
		return TRUE;
	}
	case GDK_Up:
		return TRUE;
	default:
		return FALSE;
	}
}

static void on_dict_edit_renderer_editing_started(GtkCellRenderer * renderer,
						  GtkCellEditable * editable,
						  gchar * path,
						  struct dict_edit_data *data)
{
	GtkTreeIter iter;
	dict_edit_get_selected(data, &iter);
	gboolean is_add_parameter;
	gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);

	GtkWidget *widget = GTK_WIDGET(editable);
	if (is_add_parameter) {
		/* add another 'New' */
		gtk_entry_set_text(GTK_ENTRY(widget), "");
		dict_edit_append_add_parameter(data, &data->current_document_iter);

		GebrGeoXmlProgramParameter *parameter =
		       	GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_parameters_append_parameter
						      (gebr_geoxml_document_get_dict_parameters(data->current_document),
						       GEBR_GEOXML_PARAMETER_TYPE_INT));
		gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE);
		dict_edit_load_iter(data, &iter, GEBR_GEOXML_PARAMETER(parameter));
		data->is_inserting_new = TRUE;
	} else if (renderer == data->cell_renderer_array[DICT_EDIT_VALUE]) {
		GebrGeoXmlProgramParameter *parameter;
		GtkTreeModel *completion_model;
		gchar *var;
		const gchar *keyword;
		GtkTreeIter it;
		GtkEntry *entry = GTK_ENTRY(editable);

		gtk_tree_model_get(data->tree_model, &iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);
		GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter));
		keyword = gebr_geoxml_program_parameter_get_keyword(parameter);

		GebrGeoXmlFlow *flow;
		flow = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook)) == NOTEBOOK_PAGE_FLOW_EDITION ? gebr.flow : NULL;
		completion_model = gebr_gui_parameter_get_completion_model(GEBR_GEOXML_DOCUMENT (flow),
									   GEBR_GEOXML_DOCUMENT (gebr.line),
									   GEBR_GEOXML_DOCUMENT (gebr.project),
									   type);
		gebr_gui_gtk_tree_model_foreach(it, completion_model) {
			gtk_tree_model_get(completion_model, &it, 0, &var, -1);
			if(!g_strcmp0(keyword, var))
				gtk_list_store_remove(GTK_LIST_STORE(completion_model), &it);
			g_free(var);
		}
		gebr_gui_parameter_set_entry_completion(entry, completion_model, type);
	}

	gtk_widget_set_events(GTK_WIDGET(widget), GDK_KEY_PRESS_MASK);
	g_signal_connect(widget, "key-press-event", G_CALLBACK(on_renderer_entry_key_press_event), data);
	g_object_set(widget, "user-data", renderer, NULL);
	data->in_edition = editable;
	data->editing_cell = renderer;
	data->guard = FALSE;
	data->editing_iter = iter;
	gebr_dict_update_wizard(data);
}

/*
 * dict_edit_tooltip_callback:
 * Set tooltip for New iters
 */
static gboolean dict_edit_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip, GtkTreeIter * iter,
					   GtkTreeViewColumn * column, struct dict_edit_data *data)
{
	gboolean is_add_parameter;

	gtk_tree_model_get(data->tree_model, iter, DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter, -1);
	if (is_add_parameter) {
		gtk_tooltip_set_text(tooltip, _("Click to select type for a new parameter"));
		return TRUE;
	}

	return FALSE;
}

/*
 * on_dict_edit_type_cell_edited:
 * Edit type of parameter
 */
static void on_dict_edit_value_type_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
						struct dict_edit_data *data)
{
	GebrGeoXmlParameterType type;
	GError * error = NULL;
	if(!new_text)
		return;
	if (!strcmp(new_text, _("String")))
		type = GEBR_GEOXML_PARAMETER_TYPE_STRING;
	else if (!strcmp(new_text, _("Number")))
		type = GEBR_GEOXML_PARAMETER_TYPE_FLOAT;
	else
		return;

	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(data->tree_model, &iter, path_string);

	GebrGeoXmlProgramParameter *parameter;
	gboolean is_add_parameter;
	gtk_tree_model_get(data->tree_model, &iter, 
			   DICT_EDIT_IS_ADD_PARAMETER, &is_add_parameter,
			   DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);
	gebr_geoxml_parameter_set_type(GEBR_GEOXML_PARAMETER(parameter), type);
	gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);

	gchar * tooltip = NULL;
	const gchar * expr = NULL;
	expr = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE);

	gebr_validator_evaluate(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), expr, type, &tooltip, &error);

	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
	                   DICT_EDIT_VALUE_TYPE_IMAGE, type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "string-icon" : "integer-icon",
			   DICT_EDIT_VALUE_TYPE_TOOLTIP, tooltip,
			   DICT_EDIT_VALUE_TYPE_VISIBLE, FALSE,
			   DICT_EDIT_VALUE_VISIBLE, TRUE, -1);
	gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &iter,
					  gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 1), TRUE);
}

static void
on_value_type_editing_canceled(GtkCellRenderer *cell, struct dict_edit_data *data)
{
	GebrGeoXmlProgramParameter *parameter;
	gtk_tree_model_get(data->tree_model, &data->editing_iter,
			   DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);
	gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &data->editing_iter);
	gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(parameter));
}

static gboolean
dict_edit_is_name_valid(const gchar *name, struct dict_edit_data *data)
{
	if (g_strcmp0(name,"iter") == 0) {
		gtk_label_set_text(GTK_LABEL(data->label), 
				   _("Can't use reserved keyword iter."));
		return FALSE;
	}
	if (gebr_expr_is_reserved_word (name)) {
		gtk_label_set_text(GTK_LABEL(data->label), 
				   _("This keyword is reserved."));
		return FALSE;
	}
	if (!gebr_expr_is_name_valid (name)) {
		gtk_label_set_text(GTK_LABEL(data->label), 
				   _("Valid keywords are lower case characters "
				     "from 'a' to 'z', numbers or underscore."));
		return FALSE;
	}

	return TRUE;
}

typedef struct {
	guint *id;
	guint frame;
	guint total;
	GdkColor overlay;
	GdkColor original;
	GtkWidget *widget;
} AnimationData;

static gboolean animate_widget_highlight(gpointer data)
{
	AnimationData *anim = data;
	GdkColor color;

	gdouble r = anim->original.red;
	gdouble g = anim->original.green;
	gdouble b = anim->original.blue;
	gdouble f = anim->frame;
	gdouble t = anim->total;

	color.red   = (guint16)(f/t * r + (t-f)/t * anim->overlay.red);
	color.green = (guint16)(f/t * g + (t-f)/t * anim->overlay.green);
	color.blue  = (guint16)(f/t * b + (t-f)/t * anim->overlay.blue);

	gtk_widget_modify_bg(anim->widget, GTK_STATE_NORMAL, &color);

	if (anim->frame == anim->total) {
		*anim->id = 0;
		return FALSE;
	}

	anim->frame++;
	return TRUE;
}

static void highlight_widget(GtkWidget *widget)
{
	static guint animation_id = 0;
	AnimationData *anim;
	GtkStyle *style;

	/* Do not run multiple animations */
	if (animation_id)
		return;

	anim = g_new(AnimationData, 1);
	style = gtk_widget_get_style(widget);
	anim->id = &animation_id;
	anim->frame = 0;
	anim->total = 1000 / 20;
	anim->widget = widget;
	gdk_color_parse("yellow", &anim->overlay);
	anim->original = style->bg[GTK_STATE_NORMAL];

	animation_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 20,
					  animate_widget_highlight,
					  anim, g_free);
}

static void gebr_dict_alert(struct dict_edit_data *data, gchar *icon, gchar *tooltip)
{
	if (data->guard)
		return;
	data->guard = TRUE;

	GtkTreeIter iter = data->editing_iter;
	GtkTreeViewColumn *column = gebr_gui_gtk_tree_view_get_column_from_renderer(GTK_TREE_VIEW(data->tree_view), data->editing_cell);

	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &data->editing_iter,
		           dict_edit_get_column_index_for_renderer(data->editing_cell, data),
		           gtk_entry_get_text(GTK_ENTRY(data->in_edition)), -1);

	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &data->editing_iter,
	                   DICT_EDIT_VALUE_TYPE_IMAGE, icon,
	                   DICT_EDIT_VALUE_TYPE_TOOLTIP, tooltip, -1);

	gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), &iter, column, TRUE);
	data->guard = FALSE;
}

static gboolean dict_edit_validate_editing_cell(struct dict_edit_data *data, gboolean start_edition, gboolean cancel_edition)
{
	// Does not validate if not editing or is called under gebr_dict_alert set icon guard
	if (!data->in_edition || data->guard)
		goto out;

	data->edition_valid = TRUE;
	const gchar *new_text = (gchar*)gtk_entry_get_text(GTK_ENTRY(data->in_edition));

	GebrGeoXmlProgramParameter *parameter;
	gtk_tree_model_get(data->tree_model, &data->editing_iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);

	switch (dict_edit_get_column_index_for_renderer(data->editing_cell, data)) {
	case DICT_EDIT_KEYWORD: {
		const gchar *keyword = gebr_geoxml_program_parameter_get_keyword(parameter);
		GList *affected;
		GError *err = NULL;
		if (cancel_edition) {
			if(keyword && !strlen(keyword)) {
				gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), &data->editing_iter);
				gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(parameter));
			}
			goto out;
		} else if (!strlen(new_text) || 
			   !dict_edit_is_name_valid(new_text, data) ||
			   dict_edit_check_duplicate_keyword(data, parameter, new_text, TRUE)) {
			data->edition_valid = FALSE;
			goto out;
		} else {
			if (!strlen(keyword))
				// The parameter is insert on validator, just when choose a type, @see on_dict_edit_value_type_cell_edited
				gebr_geoxml_program_parameter_set_keyword(parameter, new_text);
			else if(g_strcmp0(keyword, new_text) != 0)
				gebr_validator_rename(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), new_text, &affected, &err);

			if (err)
				g_clear_error(&err);
		}
		break;
	} case DICT_EDIT_VALUE: {
		GError *error = NULL;

		if (!cancel_edition)
			gebr_validator_change_value(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), new_text, NULL, &error);

		data->edition_valid = (error == NULL);

		if(cancel_edition)
			goto out;

		if (error) {
			gebr_dict_alert(data, GTK_STOCK_DIALOG_WARNING, error->message);
			gtk_label_set_text(GTK_LABEL(data->label), error->message);
			g_clear_error(&error);
		} else {
			GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter));
			gebr_dict_alert(data, type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "string-icon" : "integer-icon",
					type == GEBR_GEOXML_PARAMETER_TYPE_STRING ? "Text value" : "Number value");
		}
		break;
	} case DICT_EDIT_COMMENT:
		gebr_geoxml_parameter_set_label(GEBR_GEOXML_PARAMETER(parameter), new_text);
		break;
	default:
		data->edition_valid = FALSE;
		return data->edition_valid;
	}

out:
	if (!data->edition_valid) {
		g_object_set(G_OBJECT(data->warning_image),
			     "stock", start_edition ? GTK_STOCK_INFO : GTK_STOCK_DIALOG_WARNING,
			     "icon-size", GTK_ICON_SIZE_MENU, NULL);

		if (!cancel_edition && !start_edition)
			highlight_widget(GTK_WIDGET(data->event_box));
	} else {
		gebr_dict_update_wizard(data);
	}

	return data->edition_valid;
}

static void on_dict_edit_editing_cell_canceled(GtkCellRenderer * cell, struct dict_edit_data *data)
{
	GebrGeoXmlProgramParameter *parameter;
	gtk_tree_model_get(data->tree_model, &data->editing_iter, DICT_EDIT_GEBR_GEOXML_POINTER, &parameter, -1);

	data->is_inserting_new = FALSE;

	/* may delete item if empty */
	dict_edit_validate_editing_cell(data, FALSE, TRUE);

	validate_dict_iter(data, &data->editing_iter);
	//dict_edit_check_programs_using_variables(keyword, data->edition_valid);

	data->in_edition = NULL;
	data->editing_cell = NULL;
	gebr_dict_update_wizard(data);
}

/*
 * on_dict_edit_cell_edited:
 * Edit keyword, value and comment of parameter
 */
static void on_dict_edit_cell_edited(GtkCellRenderer * cell, gchar * path_string, gchar * new_text,
				     struct dict_edit_data *data)
{
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &data->editing_iter,
	                   dict_edit_get_column_index_for_renderer(cell, data), new_text,
	                   -1);
	data->in_edition = NULL;
	data->editing_cell = NULL;

	if (!data->is_inserting_new)
		validate_dict_iter(data, &data->editing_iter);
	else
		data->is_inserting_new = FALSE;

	//dict_edit_check_programs_using_variables(new_text, TRUE);
}

/*
 * dict_edit_load_iter:
 * Load @parameter into @iter
 */
static void dict_edit_load_iter(struct dict_edit_data *data, GtkTreeIter * iter, GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter));
	/* MIGRATION: for the dict number type represent a int, but is, * in fact, a float parameter */
	if (type == GEBR_GEOXML_PARAMETER_TYPE_INT)
		gebr_geoxml_parameter_set_type(GEBR_GEOXML_PARAMETER(parameter), GEBR_GEOXML_PARAMETER_TYPE_FLOAT);

	const gchar *keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter));
	gboolean is_loop_iter = strcmp(keyword, "iter") ? FALSE : TRUE;
	GString *value = g_string_new(NULL);
	
	if (is_loop_iter)
	{
		GebrGeoXmlProgram * prog = gebr_geoxml_flow_get_control_program(gebr.flow);
		gchar * step;
		gchar * ini;
		guint counter = gebr_geoxml_program_control_get_n(GEBR_GEOXML_PROGRAM(prog), &step, &ini);
		g_string_printf(value, "%s:%s:%d (%d)", ini, step, atoi(ini)+atoi(step)*(counter-1), counter);
	} 
	else
	{
		g_string_assign(value,
				gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
									      FALSE));
	}

	gchar *keyword_escaped = g_markup_escape_text(keyword, -1);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), iter,
			   DICT_EDIT_KEYWORD_IMAGE, "",
			   DICT_EDIT_KEYWORD, keyword_escaped,
			   DICT_EDIT_VALUE, value->str,
			   DICT_EDIT_COMMENT, gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)),
			   DICT_EDIT_GEBR_GEOXML_POINTER, parameter,
			   DICT_EDIT_IS_ADD_PARAMETER, FALSE,
			   DICT_EDIT_VALUE_TYPE_VISIBLE, type == GEBR_GEOXML_PARAMETER_TYPE_INT ? TRUE : FALSE,
			   DICT_EDIT_VALUE_VISIBLE, type == GEBR_GEOXML_PARAMETER_TYPE_INT ? FALSE : TRUE,
			   DICT_EDIT_VALUE_TYPE_IMAGE, "",
			   DICT_EDIT_VALUE_TYPE_TOOLTIP, "",
			   DICT_EDIT_KEYWORD_EDITABLE, !is_loop_iter,
			   DICT_EDIT_EDITABLE, !is_loop_iter,
			   DICT_EDIT_SENSITIVE, !is_loop_iter, -1);
	g_free(keyword_escaped);

	/* set a reference to the iter for each geoxml pointer */
	GebrGeoXmlObject * object = GEBR_GEOXML_OBJECT(parameter);
	GtkTreeIter *old_iter = gebr_geoxml_object_get_user_data(object);
	if (old_iter != NULL) 
		gtk_tree_iter_free(old_iter);
	gebr_geoxml_object_set_user_data(object, gtk_tree_iter_copy(iter));

	g_string_free(value, TRUE);
}

/*
 * dict_edit_check_duplicates:
 * Check for duplicates keywords for parameters of higher hierarchical documents levels
 */
static void dict_edit_check_duplicates(struct dict_edit_data *data, GebrGeoXmlDocument * document, const gchar * keyword)
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

/*
 * dict_edit_check_duplicate_keyword:
 * Check if @keyword intended to be used by @parameter is already being used
 */
static gboolean dict_edit_check_duplicate_keyword(struct dict_edit_data *data, GebrGeoXmlProgramParameter * parameter,
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
			gtk_label_set_text(GTK_LABEL(data->label), 
					   _("Other parameter already uses this keyword. Please choose another."));
		return TRUE;
	}

	return FALSE;
}

/*
 * dict_edit_get_selected:
 * Get selected iterator into @iter
 */
static gboolean dict_edit_get_selected(struct dict_edit_data *data, GtkTreeIter * iter)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view));
	return gtk_tree_selection_get_selected(selection, &model, iter);
}

/*
 * dict_edit_start_keyword_editing:
 * Set keyword column in @iter
 */
static void dict_edit_start_keyword_editing(struct dict_edit_data *data, GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_set_cursor(GTK_TREE_VIEW(data->tree_view), iter,
					  gtk_tree_view_get_column(GTK_TREE_VIEW(data->tree_view), 0), TRUE);
}

/*
 * dict_edit_append_add_parameter:
 * Add special parameter for easy new parameter creation
 */
static void dict_edit_append_add_parameter(struct dict_edit_data *data, GtkTreeIter * document_iter)
{
	GtkTreeIter iter;

	gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &iter, document_iter);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
			   DICT_EDIT_KEYWORD, _("<i>New</i>"),
			   DICT_EDIT_IS_ADD_PARAMETER, TRUE,
			   DICT_EDIT_VALUE_VISIBLE, FALSE,
			   DICT_EDIT_VALUE_TYPE_VISIBLE, TRUE,
			   DICT_EDIT_KEYWORD_EDITABLE, TRUE,
			   DICT_EDIT_EDITABLE, FALSE,
			   DICT_EDIT_SENSITIVE, TRUE, -1);
}

/*
 * on_dict_edit_popup_menu:
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

/*
 * dict_edit_append_iter:
 * Append @iter
 */
static GtkTreeIter dict_edit_append_iter(struct dict_edit_data *data, GebrGeoXmlObject * object,
					 GtkTreeIter * document_iter)
{
	GtkTreeIter iter;
	gtk_tree_store_append(GTK_TREE_STORE(data->tree_model), &iter, document_iter);

	/* display image spacing */
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter,
			   DICT_EDIT_KEYWORD_IMAGE, GTK_STOCK_DIALOG_WARNING, -1);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &iter, DICT_EDIT_KEYWORD_IMAGE, NULL, -1);

	/* MIGRATION: for the dict number type represent a int, but is, * in fact, a float parameter */
	if (g_strcmp0("iter", gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(object))) == 0)
	{
		GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(object));
		if (type == GEBR_GEOXML_PARAMETER_TYPE_INT)
			gebr_geoxml_parameter_set_type(GEBR_GEOXML_PARAMETER(object), GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
	}
	dict_edit_load_iter(data, &iter, GEBR_GEOXML_PARAMETER(object));

	return iter;
}

/*
 * document_get_name_from_type:
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
	default:
		return upper ? _("Unknown") : _("unknown");
	}
}

void on_response_ok(GtkButton * button, GebrPropertiesData * data)
{
	GtkTreeIter iter;
	const gchar *old_title;
	const gchar *new_title;
	GebrGeoXmlDocumentType type;

	data->accept_response = TRUE;
	old_title = gebr_geoxml_document_get_title(data->document);
	new_title = gtk_entry_get_text(GTK_ENTRY(data->title));

	gebr_geoxml_document_set_title(data->document, new_title);
	gebr_geoxml_document_set_description(data->document, gtk_entry_get_text(GTK_ENTRY(data->description)));
	gebr_geoxml_document_set_author(data->document, gtk_entry_get_text(GTK_ENTRY(data->author)));
	gebr_geoxml_document_set_email(data->document, gtk_entry_get_text(GTK_ENTRY(data->email)));
	document_save(data->document, TRUE, TRUE);

	/* Update title in apropriated store */
	switch ((type = gebr_geoxml_document_get_type(data->document))) {
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
		GebrGeoXmlLine *line = GEBR_GEOXML_LINE(data->document);

		gint current_active_group = gtk_combo_box_get_active(GTK_COMBO_BOX(data->groups_combo));
		gboolean emptyness = gebr_geoxml_line_get_paths_number(line) == 0 && gebr_geoxml_line_get_flows_number(line) == 0;
		if (!emptyness && current_active_group != data->previous_active_group) {
			gboolean clean = gebr_gui_message_dialog (GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
								  _("Clean line and flow(s) path(s)?"),
								  _("Do you wish to clean all paths from"
								    " this line and its respective flows?"));
			if (clean) {
				GebrGeoXmlDocument *flow;
				GebrGeoXmlSequence *sequence;

				gebr_geoxml_line_get_flow (gebr.line, &sequence, 0);
				while (sequence) {
					const gchar *fname;
					fname = gebr_geoxml_line_get_flow_source (GEBR_GEOXML_LINE_FLOW (sequence));
					if (document_load (&flow, fname, TRUE) == GEBR_GEOXML_RETV_SUCCESS)
						flow_set_paths_to_empty (GEBR_GEOXML_FLOW (flow));
					gebr_geoxml_sequence_next (&sequence);
				}

				line_set_paths_to_empty (gebr.line);
				gebr_gui_value_sequence_edit_clear (GEBR_GUI_VALUE_SEQUENCE_EDIT(data->path_sequence_edit));
				flow_browse_reload_selected();
			}
		}
	} case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT: {
		project_line_get_selected(&iter, DontWarnUnselection);
		gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				   PL_TITLE, gebr_geoxml_document_get_title(data->document), -1);
		document_save(data->document, TRUE, TRUE);
		project_line_info_update();
		break;
	} case GEBR_GEOXML_DOCUMENT_TYPE_FLOW: {
		flow_browse_get_selected(&iter, FALSE);
		gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
				   FB_TITLE, gebr_geoxml_document_get_title(data->document), -1);
		flow_browse_info_update();
		break;
	} default:
		break;
	}

	gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated."),
		     document_get_name_from_type(data->document, FALSE), old_title);
	if (strcmp(old_title, new_title) != 0)
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'."),
			     document_get_name_from_type(data->document, FALSE), old_title, new_title);

	gtk_widget_destroy(data->window);
}

void on_properties_destroy(GtkWindow * window, GebrPropertiesData * data)
{
	if (data->func)
		data->func(data->accept_response);
	g_free(data);
}

static void on_file_entry_activate (GtkEntry *entry, GebrGuiSequenceEdit *sequence_edit)
{
	g_signal_emit_by_name (sequence_edit, "add-request");
}

static void on_groups_combo_box_changed (GtkComboBox *combo, GebrGuiValueSequenceEdit *edit)
{
	gint *index;
	gchar *group;
	gboolean is_fs;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;

	if (!gebr.line)
		return;

	if (!gtk_combo_box_get_active_iter (combo, &iter))
		return;

	model = gtk_combo_box_get_model (combo);
	path = gtk_tree_model_get_path (model, &iter);
	index = gtk_tree_path_get_indices (path);

	gtk_tree_model_get (model, &iter,
			    TAG_NAME, &group,
			    TAG_FS, &is_fs,
			    -1);

	/* Empty string means all servers */
	if (!group || index[0] == 0)
		gebr_geoxml_line_set_group (gebr.line, "", FALSE);
	else
		gebr_geoxml_line_set_group (gebr.line, group, is_fs);

	gtk_tree_model_filter_refilter (
			GTK_TREE_MODEL_FILTER (gebr.ui_project_line->servers_filter));

	g_free (group);
	gtk_tree_path_free (path);
}

static gboolean dict_edit_reorder(GtkTreeView            *tree_view,
				  GtkTreeIter            *iter,
				  GtkTreeIter            *position,
				  GtkTreeViewDropPosition drop_position,
				  struct dict_edit_data  *data)
{
	gchar *keyword;
	gboolean is_add;
	GtkTreeIter parent;
	GtkTreeIter newiter;
	GtkTreePath *parent_path;
	GtkTreePath *path_a;
	GtkTreePath *path_b;
	GebrGeoXmlDocument *document;
	GebrGeoXmlSequence *source;
	GebrGeoXmlSequence *pivot = NULL;
	GebrGeoXmlParameterType type;
	const gchar *varname;
	const gchar *value;
	const gchar *comment;
	GList *affected;

	gtk_tree_model_get(data->tree_model, position,
			   DICT_EDIT_KEYWORD, &keyword,
			   DICT_EDIT_IS_ADD_PARAMETER, &is_add,
			   -1);

	/* Target is the scope label (Flow, Line or Project).
	 * Insert `iter' before the last row (the 'New' row) of that label.
	 * The dict_edit_can_reorder() function guarantee that drop_position
	 * is not DROP_BEFORE nor DROP_AFTER.
	 */
	if (gtk_tree_store_iter_depth(GTK_TREE_STORE(data->tree_model), position) == 0)
	{
		GtkTreeIter it;
		gint n = gtk_tree_model_iter_n_children(data->tree_model, position);
		parent = *position;

		// `n' must be at least 1 because of the 'New' row!
		if (n == 0)
			g_return_val_if_reached(FALSE);

		// Get the previous GeoXmlSequence so we can move_after
		if (n >= 2) {
			gtk_tree_model_iter_nth_child(data->tree_model, &it, position, n-2);
			gtk_tree_model_get(data->tree_model, &it,
					   DICT_EDIT_GEBR_GEOXML_POINTER, &pivot, -1);
		}
		gtk_tree_store_insert(GTK_TREE_STORE(data->tree_model), &newiter, position, n-1);
	}
	/* Otherwise, the target is a variable. It may be the `iter' variable or
	 * the special row 'New'. If target is `iter' variable, insert the new row *after*
	 * it, regardless of the drop position. In the case target is 'New', insert *before*.
	 */
	else
	{
		gboolean is_iter = g_strcmp0(keyword, "iter") == 0;
		gboolean is_after = (drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER
				     || drop_position == GTK_TREE_VIEW_DROP_AFTER);

		gtk_tree_model_iter_parent(data->tree_model, &parent, position);

		if (is_iter || (!is_add && is_after)) {
			gtk_tree_store_insert_after(GTK_TREE_STORE(data->tree_model),
						    &newiter, &parent, position);
			gtk_tree_model_get(data->tree_model, position,
					   DICT_EDIT_GEBR_GEOXML_POINTER, &pivot, -1);
		} else {
			GtkTreeIter it;
			GtkTreePath *path;
			path = gtk_tree_model_get_path(data->tree_model, position);
			gtk_tree_store_insert_before(GTK_TREE_STORE(data->tree_model),
						     &newiter, &parent, position);
			if (gtk_tree_path_prev(path)
			    && gtk_tree_model_get_iter(data->tree_model, &it, path))
			{
				gtk_tree_model_get(data->tree_model, &it,
						   DICT_EDIT_GEBR_GEOXML_POINTER, &pivot, -1);
			}
			gtk_tree_path_free(path);
		}
	}

	parent_path = gtk_tree_model_get_path(data->tree_model, &parent);

	switch (gtk_tree_path_get_indices(parent_path)[0]) {
	case 0: document = GEBR_GEOXML_DOCUMENT(gebr.project); break;
	case 1: document = GEBR_GEOXML_DOCUMENT(gebr.line); break;
	case 2: document = GEBR_GEOXML_DOCUMENT(gebr.flow); break;
	default: g_return_val_if_reached(FALSE);
	}

	gtk_tree_path_free(parent_path);
	gtk_tree_model_get(data->tree_model, iter, DICT_EDIT_GEBR_GEOXML_POINTER, &source, -1);
	type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(source));
	varname = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(source));
	value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(source), FALSE);
	comment = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(source));
	gtk_tree_iter_free((GtkTreeIter*)gebr_geoxml_object_get_user_data(GEBR_GEOXML_OBJECT(source)));
	gebr_geoxml_sequence_remove(source);

	// 'document' should refer to pivot's document, so we can use move_after function!
	source = GEBR_GEOXML_SEQUENCE(gebr_geoxml_document_set_dict_keyword(document, type, varname, value));
	gebr_geoxml_parameter_set_label(GEBR_GEOXML_PARAMETER(source), comment);
	gebr_geoxml_sequence_move_after(source, pivot);
	gebr_gui_gtk_tree_model_iter_copy_values(data->tree_model, &newiter, iter);
	gtk_tree_store_set(GTK_TREE_STORE(data->tree_model), &newiter,
			   DICT_EDIT_GEBR_GEOXML_POINTER, source, -1);
	gebr_geoxml_object_set_user_data(GEBR_GEOXML_OBJECT(source), gtk_tree_iter_copy(&newiter));

	path_a = gtk_tree_model_get_path(data->tree_model, iter);
	path_b = gtk_tree_model_get_path(data->tree_model, &newiter);
	if (gtk_tree_path_compare(path_a, path_b) == 1) {
		gtk_tree_path_free(path_a);
		path_a = path_b;
	} else
		gtk_tree_path_free(path_b);

	gtk_tree_store_remove(GTK_TREE_STORE(data->tree_model), iter);

	if (gtk_tree_model_get_iter(data->tree_model, &newiter, path_a))
		validate_dict_iter(data, &newiter);

	gebr_validator_move(gebr.validator, GEBR_GEOXML_PARAMETER(source), GEBR_GEOXML_PARAMETER(pivot), &affected);
	gtk_tree_path_free(path_a);
	g_free(keyword);
	return TRUE;
}

static gboolean dict_edit_can_reorder(GtkTreeView            *tree_view,
				      GtkTreeIter            *iter,
				      GtkTreeIter            *position,
				      GtkTreeViewDropPosition drop_position,
				      struct dict_edit_data  *data)
{
	gchar *keyword;
	gboolean is_add;

	if (gebr_gui_gtk_tree_model_iter_equal_to(data->tree_model, iter, position))
		return FALSE;

	/* Do not allow moving 'Project', 'Line' and 'Flow' labels */
	if (gtk_tree_store_iter_depth(GTK_TREE_STORE(data->tree_model), iter) == 0)
		return FALSE;

	/* Do not allow moving 'after' nor 'before' a label */
	if (gtk_tree_store_iter_depth(GTK_TREE_STORE(data->tree_model), position) == 0
	    && (drop_position == GTK_TREE_VIEW_DROP_BEFORE
		|| drop_position == GTK_TREE_VIEW_DROP_AFTER))
		return FALSE;

	gtk_tree_model_get(data->tree_model, iter,
			   DICT_EDIT_KEYWORD, &keyword, -1);

	/* Do not allow moving 'iter' variable */
	if (g_strcmp0(keyword, "iter") == 0) {
		g_free(keyword);
		return FALSE;
	}

	g_free(keyword);
	gtk_tree_model_get(data->tree_model, position,
			   DICT_EDIT_KEYWORD, &keyword, -1);

	/* Do not allow moving before 'iter' variable */
	if (g_strcmp0(keyword, "iter") == 0
	    && (drop_position == GTK_TREE_VIEW_DROP_BEFORE
		|| drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE))
	{
		g_free(keyword);
		return FALSE;
	}

	g_free(keyword);
	gtk_tree_model_get(data->tree_model, position,
			   DICT_EDIT_IS_ADD_PARAMETER, &is_add, -1);

	/* Do not allow moving after 'New' row */
	if (is_add && drop_position != GTK_TREE_VIEW_DROP_BEFORE)
		return FALSE;

	return TRUE;
}
