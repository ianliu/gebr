/*   GeBR - An environment for seismic processing.
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

#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gebr-iexpr.h>
#include <libgebr/gebr-expr.h>
#include <gdk/gdkkeysyms.h>

#include "ui_flow_edition.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_parameters.h"
#include "ui_help.h"
#include "callbacks.h"
#include "ui_document.h"

/*
 * Prototypes
 */

static gboolean flow_edition_may_reorder(GtkTreeView *tree_view,
					 GtkTreeIter *iter,
					 GtkTreeIter * position,
					 GtkTreeViewDropPosition drop_position,
					 struct ui_flow_edition *ui_flow_edition);

static gboolean flow_edition_reorder(GtkTreeView * tree_view,
				     GtkTreeIter * iter,
				     GtkTreeIter * position,
				     GtkTreeViewDropPosition drop_position,
				     struct ui_flow_edition *ui_flow_edition);

static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path);
static void flow_edition_component_editing_canceled(GtkCellRenderer *renderer, gpointer user_data);
static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text);
static void flow_edition_component_selected(void);
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected);

static void flow_edition_menu_add(void);
static void flow_edition_menu_show_help(void);

static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static void flow_edition_on_combobox_changed(GtkComboBox * combobox);

static gboolean on_flow_sequence_query_tooltip(GtkTreeView * treeview,
					       gint x,
					       gint y,
					       gboolean keyboard_tip,
					       GtkTooltip * tooltip,
					       struct ui_flow_edition *ui_flow_edition);

static void on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
						       GtkCellRenderer *cell,
						       GtkTreeModel    *tree_model,
						       GtkTreeIter     *iter,
						       gpointer         data);

static void on_queue_combobox_changed (GtkComboBox *combo, GtkComboBox *server_combo);

/*
 * Public functions
 */

struct ui_flow_edition *flow_edition_setup_ui(void)
{
	struct ui_flow_edition *ui_flow_edition;

	GtkWidget *hpanel;
	GtkWidget *frame;
	GtkWidget *alignment;
	GtkWidget *label;
	GtkWidget *left_vbox;
	GtkWidget *scrolled_window;
	GtkWidget *vbox;
	GtkWidget *combobox;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* alloc */
	ui_flow_edition = g_new(struct ui_flow_edition, 1);

	/* Create flow edit ui_flow_edition->widget */
	ui_flow_edition->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_flow_edition->widget), hpanel);

	/*
	 * Left side: flow components
	 */
	left_vbox = gtk_vbox_new(FALSE, 5);
	gtk_paned_pack1(GTK_PANED(hpanel), left_vbox, TRUE, FALSE);
	gtk_widget_set_size_request(left_vbox, 150, -1);

	ui_flow_edition->server_combobox = combobox = gtk_combo_box_new ();
	ui_flow_edition->queue_combobox = gtk_combo_box_new ();
	g_signal_connect (ui_flow_edition->queue_combobox, "changed",
			  G_CALLBACK (on_queue_combobox_changed), combobox);

	renderer = gtk_cell_renderer_text_new();

	gtk_cell_layout_pack_start (
			GTK_CELL_LAYOUT (ui_flow_edition->queue_combobox),
			renderer, TRUE);

	gtk_cell_layout_add_attribute (
			GTK_CELL_LAYOUT (ui_flow_edition->queue_combobox),
			renderer, "text", 0);

	gtk_widget_show (ui_flow_edition->queue_combobox);

	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "pixbuf", SERVER_STATUS_ICON);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combobox), renderer, on_server_disconnected_set_row_insensitive, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "text", SERVER_NAME);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combobox), renderer, on_server_disconnected_set_row_insensitive, NULL, NULL);

	frame = gtk_frame_new(NULL);
	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	label = gtk_label_new_with_mnemonic(_("Server"));
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_container_add(GTK_CONTAINER(alignment), combobox);
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, FALSE, TRUE, 0);
	g_signal_connect(combobox, "changed", G_CALLBACK(flow_edition_on_combobox_changed), NULL);

	frame = gtk_frame_new(NULL);
	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	label = gtk_label_new_with_mnemonic(_("Queue"));
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	ui_flow_edition->queue_bin = GTK_BIN(alignment);
	gtk_widget_set_sensitive(ui_flow_edition->queue_combobox, TRUE);
	gtk_container_add(GTK_CONTAINER(ui_flow_edition->queue_bin), ui_flow_edition->queue_combobox);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Flow sequence"));
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	ui_flow_edition->fseq_store = gtk_list_store_new(FSEQ_N_COLUMN,
							 G_TYPE_STRING,
							 G_TYPE_STRING,
							 G_TYPE_POINTER,
							 G_TYPE_INT,
							 G_TYPE_BOOLEAN,
							 G_TYPE_BOOLEAN,
							 G_TYPE_STRING,
							 G_TYPE_BOOLEAN);
	ui_flow_edition->fseq_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->fseq_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_edition->fseq_view)),
				    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
						  (GebrGuiGtkPopupCallback) flow_edition_component_popup_menu,
						  ui_flow_edition);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_reorder,
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_may_reorder,
						    ui_flow_edition);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_edition->fseq_view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", FSEQ_ICON_COLUMN);

	g_object_set(G_OBJECT(ui_flow_edition->fseq_view), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT(ui_flow_edition->fseq_view), "query-tooltip", G_CALLBACK(on_flow_sequence_query_tooltip), ui_flow_edition);

	ui_flow_edition->text_renderer = renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", FSEQ_TITLE_COLUMN);
	gtk_tree_view_column_add_attribute(col, renderer, "editable", FSEQ_EDITABLE);
	gtk_tree_view_column_add_attribute(col, renderer, "ellipsize", FSEQ_ELLIPSIZE);
	gtk_tree_view_column_add_attribute(col, renderer, "sensitive", FSEQ_SENSITIVE);

	g_signal_connect(renderer, "edited", G_CALLBACK(flow_edition_component_edited), NULL);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(flow_edition_component_editing_started), NULL);
	g_signal_connect(renderer, "editing-canceled", G_CALLBACK(flow_edition_component_editing_canceled), NULL);

	/* Space key pressed on flow component changes its configured status */
	g_signal_connect(ui_flow_edition->fseq_view, "key-press-event",
			 G_CALLBACK(flow_edition_component_key_pressed), ui_flow_edition);

	/* Double click on flow component open its parameter window */
	g_signal_connect(ui_flow_edition->fseq_view, "row-activated",
			 G_CALLBACK(flow_edition_component_activated), ui_flow_edition);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_edition->fseq_view)), "changed",
			 G_CALLBACK(flow_edition_component_selected), ui_flow_edition);

	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_edition->fseq_view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);

	/*
	 * Right side: Menu list
	 */
	frame = gtk_frame_new(_("Menus"));
	gtk_paned_pack2(GTK_PANED(hpanel), frame, FALSE, FALSE);
	gtk_widget_set_size_request(frame, 150, -1);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);

	ui_flow_edition->menu_store = gtk_tree_store_new(MENU_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_flow_edition->menu_store), MENU_TITLE_COLUMN, GTK_SORT_ASCENDING);
	ui_flow_edition->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_edition->menu_view);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->menu_view),
						  (GebrGuiGtkPopupCallback) flow_edition_menu_popup_menu,
						  ui_flow_edition);
	g_signal_connect(ui_flow_edition->menu_view, "key-press-event",
			 G_CALLBACK(flow_edition_component_key_pressed), ui_flow_edition);
	g_signal_connect(GTK_OBJECT(ui_flow_edition->menu_view), "row-activated",
			 G_CALLBACK(flow_edition_menu_add), ui_flow_edition);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_flow_edition->menu_view), MENU_TITLE_COLUMN);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	gtk_paned_set_position(GTK_PANED(hpanel), 150);

	return ui_flow_edition;
}

void flow_edition_load_components(void)
{
	GebrGeoXmlSequence *first_program;

	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	if (!flow_browse_get_selected(NULL, FALSE))
		return;

	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter);

	/* now into GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);
	flow_add_program_sequence_to_view(first_program, FALSE, FALSE);

	flow_edition_set_io();
}

gboolean flow_edition_get_selected_component(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No program selected."));
		return FALSE;
	}

	return TRUE;
}

void flow_edition_select_component_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter);
	flow_edition_component_selected();
}

void flow_edition_set_io(void)
{
	GError *err = NULL;
	const gchar *input = gebr_geoxml_flow_io_get_input(gebr.flow);
	const gchar *output = gebr_geoxml_flow_io_get_output(gebr.flow);
	const gchar *error = gebr_geoxml_flow_io_get_error(gebr.flow);

	gchar *title;
	gchar *result;
	gchar *tooltip;
	const gchar *icon;
	gboolean is_append;
	GtkTreeModel *model;
	gboolean sensitivity;

	flow_program_check_sensitiveness();

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));


	/* Set the INPUT properties.
	 * INPUT does not have 'Append/Overwrite' so there is only 2 possible icons:
	 *  - gebr-stdin
	 *  - GTK_STOCK_DIALOG_WARNING
	 */
	gtk_tree_model_get(model, &gebr.ui_flow_edition->input_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*input || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Input file"));
		icon = "gebr-stdin";
		tooltip = NULL;
	} else {
		title = g_strdup(input);
		gebr_validator_evaluate(gebr.validator, input, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		if (err) {
			tooltip = g_strdup(err->message);
			icon = GTK_STOCK_DIALOG_WARNING;
			g_clear_error(&err);
		} else {
			tooltip = g_strdup_printf(_("Input file %s"), result);
			icon = "gebr-stdin";
			g_free(result);
		}
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   -1);
	g_free(title);
	g_free(tooltip);

	/* Set the OUTPUT properties.
	 * OUTPUT have 'Append/Overwrite' so there is 3 possible icons:
	 *  - gebr-stdout
	 *  - gebr-append-stdout
	 *  - GTK_STOCK_DIALOG_WARNING
	 */
	result = NULL;
	is_append = gebr_geoxml_flow_io_get_output_append(gebr.flow);
	gtk_tree_model_get(model, &gebr.ui_flow_edition->output_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*output || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Output file"));
		tooltip = NULL;
	} else {
		title = g_strdup(output);
		gebr_validator_evaluate(gebr.validator, output, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		if (err)
			tooltip = g_strdup(err->message);
		else {
			if (is_append)
				tooltip = g_strdup_printf(_("Append to output file %s"), result);
			else
				tooltip = g_strdup_printf(_("Overwrite output file %s"), result);
			g_free(result);
		}
	}

	if (!err)
		icon = is_append ? "gebr-append-stdout":"gebr-stdout";
	else {
		icon = GTK_STOCK_DIALOG_WARNING;
		g_clear_error(&err);
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   -1);
	g_free(title);
	g_free(tooltip);

	/* Set the ERROR properties. */
	result = NULL;
	is_append = gebr_geoxml_flow_io_get_error_append(gebr.flow);
	gtk_tree_model_get(model, &gebr.ui_flow_edition->error_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*error || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Error file"));
		tooltip = NULL;
	} else {
		title = g_strdup(error);
		gebr_validator_evaluate(gebr.validator, error, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		if (err)
			tooltip = g_strdup(err->message);
		else {
			if (is_append)
				tooltip = g_strdup_printf(_("Append to error file %s"), result);
			else
				tooltip = g_strdup_printf(_("Overwrite error file %s"), result);
			g_free(result);
		}
	}

	if (!err)
		icon = is_append ? "gebr-append-stderr":"gebr-stderr";
	else {
		icon = GTK_STOCK_DIALOG_WARNING;
		g_clear_error(&err);
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   -1);
	g_free(title);
	g_free(tooltip);

	flow_browse_info_update();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
}

void flow_edition_component_activated(void)
{
	GtkTreeIter iter;
	gchar *title;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, FSEQ_TITLE_COLUMN, &title, -1);
	gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Configuring program '%s'."), title);
	parameters_configure_setup_ui();
	g_free(title);
}

static void open_activated(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event)
{
	GebrGeoXmlSequence *line_path;
	gchar *path = g_object_get_data(G_OBJECT(entry), "path");
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, path);

	GtkFileChooserAction action;
	gchar *stock;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter)) {
		action = GTK_FILE_CHOOSER_ACTION_OPEN;
		stock = GTK_STOCK_OPEN;
	} else {
		action = GTK_FILE_CHOOSER_ACTION_SAVE;
		stock = GTK_STOCK_SAVE;
	}

	GtkWidget *dialog = gtk_file_chooser_dialog_new(NULL, GTK_WINDOW(gebr.window), action,
							stock, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gebr_geoxml_line_get_path(gebr.line, &line_path, 0);

	if (line_path) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
						    gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
	}

	while (line_path) {
		const gchar *path_str;
		path_str = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path));
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), path_str, NULL);
		gebr_geoxml_sequence_next(&line_path);
	}

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(entry, filename);
		
		GtkCellRenderer *renderer = g_object_get_data(G_OBJECT(entry), "renderer");
		gtk_cell_renderer_stop_editing(renderer, FALSE);
		flow_edition_component_edited(GTK_CELL_RENDERER_TEXT(renderer), path, filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	g_free(path);
}

static void populate_io_popup(GtkEntry *entry, GtkMenu *menu)
{
	GtkWidget *submenu;
	GtkWidget *item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);

	submenu = gebr_gui_parameter_add_variables_popup(entry,
							 GEBR_GEOXML_DOCUMENT (gebr.flow),
							 GEBR_GEOXML_DOCUMENT (gebr.line),
							 GEBR_GEOXML_DOCUMENT (gebr.project),
							 GEBR_GEOXML_PARAMETER_TYPE_STRING);
	item = gtk_menu_item_new_with_label(_("Insert Variable"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
}


static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path)
{
	GtkTreeIter iter;
	GtkTreeModel *completion_model;
	GtkEntry *entry = GTK_ENTRY(editable);
	const gchar *input;
	const gchar *output;
	const gchar *error;

	gtk_window_remove_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	g_object_set_data(G_OBJECT(entry), "path", g_strdup(path));
	g_object_set_data(G_OBJECT(entry), "renderer", renderer);

	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;

	completion_model = gebr_gui_parameter_get_completion_model(GEBR_GEOXML_DOCUMENT (gebr.flow),
								   GEBR_GEOXML_DOCUMENT (gebr.line),
								   GEBR_GEOXML_DOCUMENT (gebr.project),
								   GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_gui_parameter_set_entry_completion(entry, completion_model, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	input = gebr_geoxml_flow_io_get_input(gebr.flow);
	output = gebr_geoxml_flow_io_get_output(gebr.flow);
	error = gebr_geoxml_flow_io_get_error(gebr.flow);

	if ((gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) && (g_strcmp0(input, "") == 0))  ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) && (g_strcmp0(output, "") == 0)) ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter) && (g_strcmp0(error, "") == 0)))
		gtk_entry_set_text(entry, "");

	g_signal_connect(entry, "icon-release", G_CALLBACK(open_activated), NULL);
	g_signal_connect(entry, "populate-popup", G_CALLBACK(populate_io_popup), NULL);
}

static void flow_edition_component_editing_canceled(GtkCellRenderer *renderer, gpointer user_data)
{
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);
}


static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text)
{
	GtkTreeIter iter;

	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, path);

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter))
		gebr_geoxml_flow_io_set_input(gebr.flow, new_text);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		gebr_geoxml_flow_io_set_output(gebr.flow, new_text);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		gebr_geoxml_flow_io_set_error(gebr.flow, new_text);

	flow_edition_set_io();
}

gboolean flow_edition_component_key_pressed(GtkWidget *view, GdkEventKey *key)
{
	GList			* listiter;
	GList			* paths;
	GebrGeoXmlProgram	* program;
	GebrGeoXmlProgramStatus	  status;
	GtkTreeIter		  iter;
	GtkTreeModel		* model;
	GtkTreeSelection	* selection;
	gboolean		  has_disabled = FALSE;

	if (key->keyval != GDK_space)
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_edition->fseq_view));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	/* Removes the input/output/errors special iters from the paths list, and if there is no selection, return.
	 */
	GtkTreePath *in_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->input_iter);
	GtkTreePath *out_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->output_iter);
	GtkTreePath *error_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->error_iter);
	listiter = paths;
	while (listiter) {
		GList * remove_node = NULL;

		if (gtk_tree_path_compare (in_path, listiter->data) == 0 ||
		    gtk_tree_path_compare (out_path, listiter->data) == 0 ||
		    gtk_tree_path_compare (error_path, listiter->data) == 0)
			remove_node = listiter;

		if (remove_node) {
			listiter = listiter->next;
			paths = g_list_remove_link (paths, remove_node);
			gtk_tree_path_free (remove_node->data);
			g_list_free_1 (remove_node);
		} else
			listiter = listiter->next;
	}
	gtk_tree_path_free (in_path);
	gtk_tree_path_free (out_path);
	gtk_tree_path_free (error_path);

	if (!paths)
		return FALSE;

	listiter = paths;

	gtk_tree_model_get_iter (model, &iter, listiter->data);
	gtk_tree_model_get (model, &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &program,
			    -1);

	do {
		gboolean has_error;

		gtk_tree_model_get_iter (model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
				    FSEQ_GEBR_GEOXML_POINTER, &program,
				    -1);

		status = gebr_geoxml_program_get_status (program);
		has_error = gebr_geoxml_program_get_error_id(program, NULL);

		if ((status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) ||
		    (status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED && !has_error))
		{
			has_disabled = TRUE;
			break;
		}

		listiter = listiter->next;
	} while (listiter);

	if (has_disabled) {
		status = GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED;
	} else {
		status = GEBR_GEOXML_PROGRAM_STATUS_DISABLED;
	}

	listiter = paths;
	while (listiter) {
		gtk_tree_model_get_iter(model, &iter, listiter->data);
		flow_edition_change_iter_status(status, &iter);
		listiter = listiter->next;
	}

	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	flow_edition_set_io();

	return TRUE;
}

void flow_edition_change_iter_status(GebrGeoXmlProgramStatus status, GtkTreeIter *iter)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	const gchar *icon;
	GtkTreePath *input_path, *output_path, *error_path;
	GebrGeoXmlProgram *program;
	gboolean has_error;

	model = GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store);
	input_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->input_iter);
	output_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->output_iter);
	error_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->error_iter);

	path = gtk_tree_model_get_path (model, iter);
	if (gtk_tree_path_compare (path, input_path) == 0 ||
	    gtk_tree_path_compare (path, output_path) == 0 ||
	    gtk_tree_path_compare (path, error_path) == 0) {
		goto out;
	}

	gtk_tree_model_get(model, iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);

	if (gebr_geoxml_program_get_status(program) == status)
		return;

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, iter, FSEQ_NEVER_OPENED,
	                   FALSE, -1);

	has_error = gebr_geoxml_program_get_error_id(program, NULL);

	if (has_error && status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
		status = GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

	if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		GebrGeoXmlSequence *parameter;

		if(status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		} else {
			gebr_geoxml_flow_insert_iter_dict(gebr.flow);
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		}
		flow_edition_revalidate_programs();
	}

	gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), status);
	icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, iter, FSEQ_ICON_COLUMN, icon, -1);
out:
	gtk_tree_path_free (path);
	gtk_tree_path_free (input_path);
	gtk_tree_path_free (output_path);
	gtk_tree_path_free (error_path);
}

void flow_edition_status_changed(guint status)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		flow_edition_change_iter_status(status, &iter);
	}

	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

void flow_edition_on_server_changed(void)
{
	flow_edition_on_combobox_changed(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox));
}

/**
 * \internal
 * Return TRUE if there is a selected menu and put it into _iter_
 * If _warn_unselected_ is TRUE then a error message is displayed if the FALSE is returned
 */
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 */
static gboolean
flow_edition_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeModel *model;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramControl con;

	if (gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->error_iter))
		return FALSE;
	if (drop_position != GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->input_iter))
		return FALSE;
	if (drop_position == GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->output_iter))
		return FALSE;
	if (gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->error_iter))
		return FALSE;

	/* Check if the moving iter is a control program */
	model = gtk_tree_view_get_model (tree_view);
	gtk_tree_model_get (model, iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	con = gebr_geoxml_program_get_control (program);
	if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return FALSE;

	/* Check if the target iter is a control program */
	gtk_tree_model_get (model, position, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	con = gebr_geoxml_program_get_control (program);
	if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return FALSE;

	return TRUE;
}

/**
 * \internal
 */
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *position_program;

	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), position,
			   FSEQ_GEBR_GEOXML_POINTER, &position_program, -1);

	if (drop_position != GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_before(program, position_program);
		gtk_list_store_move_before(gebr.ui_flow_edition->fseq_store, iter, position);
	} else {
		gebr_geoxml_sequence_move_after(program, position_program);
		gtk_list_store_move_after(gebr.ui_flow_edition->fseq_store, iter, position);
	}
	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	return FALSE;
}

/**
 * \internal
 * When a flow component (a program in the flow) is selected this funtions get the state of the program and set it on
 * Flow Component Menu.
 */
static void flow_edition_component_selected(void)
{
	gboolean has_error;
	GtkTreeIter iter;
	GtkAction * action;

	gebr.program = NULL;
	if (!flow_edition_get_selected_component(&iter, FALSE))
		return;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			   FSEQ_GEBR_GEOXML_POINTER, &gebr.program, -1);

	/* If the program has an error, disable the 'Configured' status */
	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	has_error = gebr_geoxml_program_get_error_id(gebr.program, NULL);
	gtk_action_set_sensitive(action, !has_error);

	action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_help");
	gchar *tmp_help_p = gebr_geoxml_program_get_help(gebr.program);
	gtk_action_set_sensitive(action, strlen(tmp_help_p) != 0);
	g_free(tmp_help_p);
}

/*
 * flow_edition_menu_add:
 * Add selected menu to flow sequence.
 */
static void flow_edition_menu_add(void)
{
	GtkTreeIter iter;
	gchar *name;
	gchar *filename;
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *menu_programs;
	gint menu_programs_index;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!flow_edition_get_selected_menu(&iter, FALSE)) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter);
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path, FALSE);
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_TITLE_COLUMN, &name, MENU_FILEPATH_COLUMN, &filename, -1);
	menu = menu_load_path(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	GebrGeoXmlProgramControl c1, c2;
	GebrGeoXmlProgram *first_prog;

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &first_prog, -1);

	gebr_geoxml_flow_get_program(menu, &program, 0);

	c1 = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));
	c2 = gebr_geoxml_program_get_control (first_prog);

	if (c1 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY && c2 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
		document_free(GEBR_GEOXML_DOC(menu));
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, _("This flow already contains a loop"));
		goto out;
	}

	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		gebr_geoxml_parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	if (c1 == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		GebrGeoXmlParameter *dict_iter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
		gebr_validator_insert(gebr.validator, dict_iter, NULL, NULL);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE, TRUE);

	document_free(GEBR_GEOXML_DOC(menu));
 out:	g_free(name);
	g_free(filename);
}

/**
 * \internal
 * Show's menus help
 */
static void flow_edition_menu_show_help(void)
{
	GtkTreeIter iter;
	gchar *menu_filename;
	GebrGeoXmlFlow *menu;

	if (!flow_edition_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);

	menu = menu_load_path(menu_filename);
	if (menu == NULL)
		goto out;
	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);

out:	g_free(menu_filename);
}

static void on_output_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_output_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stdout", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-stdout", -1);
	}
	flow_edition_set_io();
}

static void on_error_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_error_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stderr", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-stderr", -1);
	}
	flow_edition_set_io();
}

/**
 * \internal
 * Show popup menu for flow component.
 */
static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget * menu;
	GtkWidget * menu_item;
	GtkAction * action;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramControl control;
	gboolean is_ordinary;
	gboolean can_move_up;
	gboolean can_move_down;

	if (!flow_edition_get_selected_component(&iter, FALSE))
		return NULL;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter))
	       return NULL;

	menu = gtk_menu_new();

	gboolean is_file = FALSE;
	gboolean is_append;
	const gchar *label;
	GCallback append_cb;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		is_file = TRUE;
		label = _("Overwrite existing output file");
		is_append = gebr_geoxml_flow_io_get_output_append (gebr.flow);
		append_cb = G_CALLBACK (on_output_append_toggled);
	} else
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter)) {
		is_file = TRUE;
		label = _("Overwrite existing error file");
		is_append = gebr_geoxml_flow_io_get_error_append (gebr.flow);
		append_cb = G_CALLBACK (on_error_append_toggled);
	}

	if (is_file) {
		menu_item = gtk_check_menu_item_new_with_label (label);
		g_signal_connect (menu_item, "toggled", append_cb, NULL);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), !is_append);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show(menu_item);
		return GTK_MENU (menu);
	}

	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &program,
			    -1);
	control = gebr_geoxml_program_get_control (program);

	is_ordinary = control == GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;
	can_move_up = gebr_gui_gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter);
	can_move_down = gebr_gui_gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter);

	/* Move top */
	if (is_ordinary && can_move_up) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_top), NULL);
	}
	/* Move bottom */
	if (is_ordinary && can_move_down) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_bottom), NULL);
	}
	/* separator */
	if (is_ordinary && (can_move_up || can_move_down))
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());


	/* status */
	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_disabled");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_unconfigured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	/* separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_help")));

 	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Show popup menu for menu list.
 */
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!flow_edition_get_selected_menu(&iter, FALSE)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_flow_edition->menu_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_flow_edition->menu_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_flow_edition->menu_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_flow_edition->menu_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_show_help), NULL);
	gchar *menu_filename;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
	GebrGeoXmlDocument *xml = GEBR_GEOXML_DOCUMENT(menu_load_path(menu_filename));
	gchar *tmp_help = gebr_geoxml_document_get_help(xml);
	if (xml != NULL && strlen(tmp_help) <= 1)
		gtk_widget_set_sensitive(menu_item, FALSE);
	if (xml)
		document_free(xml);
	g_free(menu_filename);
	g_free(tmp_help);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 */
static void flow_edition_on_combobox_changed(GtkComboBox * combobox)
{
	gint lstq;
	GHashTable *last_queue_hash;
	GebrServer *server;
	GtkTreeIter iter;
	GtkTreeIter flow_iter;

	if (!flow_browse_get_selected(&flow_iter, TRUE))
		return;

	if (!gtk_combo_box_get_active_iter (combobox, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->servers_sort), &iter,
			   SERVER_POINTER, &server,
			   -1);
	gtk_combo_box_set_model (GTK_COMBO_BOX (gebr.ui_flow_edition->queue_combobox),
				 GTK_TREE_MODEL (server->queues_model));

	const gchar *addr = server->comm->address->str;

	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_browse->store), &flow_iter,
			    FB_LAST_QUEUES, &last_queue_hash,
			    -1);

	if (!last_queue_hash) {
		last_queue_hash = g_hash_table_new_full (g_str_hash,
							 g_str_equal,
							 (GDestroyNotify) g_free,
							 NULL);
		g_hash_table_insert (last_queue_hash,
				     g_strdup (addr),
				     GINT_TO_POINTER (0));

		gtk_list_store_set (gebr.ui_flow_browse->store, &flow_iter,
				    FB_LAST_QUEUES, last_queue_hash,
				    -1);
		g_object_weak_ref (G_OBJECT (gebr.ui_flow_browse->store),
				   (GWeakNotify) g_hash_table_destroy, last_queue_hash);
	}

	lstq = GPOINTER_TO_INT (g_hash_table_lookup (last_queue_hash, addr));
	gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), lstq);

	gebr_geoxml_flow_server_set_address (gebr.flow, addr);
	flow_edition_set_io();
	flow_browse_info_update();
}

/**
 * \internal
 * Shows tooltips for each line in component flow tree view.
 */
static gboolean
on_flow_sequence_query_tooltip(GtkTreeView * treeview,
			       gint x,
			       gint y,
			       gboolean keyboard_tip,
			       GtkTooltip * tooltip,
			       struct ui_flow_edition *ui_flow_edition)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GebrGeoXmlProgram *program;
	gchar *tooltip_text;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip, &model, NULL, &iter))
		return FALSE;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter)
	    || gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)
	    || gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
	{
		gtk_tree_model_get(model, &iter, FSEQ_TOOLTIP, &tooltip_text, -1);

		if (!tooltip_text)
			return FALSE;

		gtk_tooltip_set_text(tooltip, tooltip_text);
		g_free(tooltip_text);

		goto set_tooltip;
	}

	gtk_tree_model_get(model, &iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);

	if (!program || gebr_geoxml_program_get_status(program) != GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED)
		return FALSE;

	GebrIExprError errorid;
	if (!gebr_geoxml_program_get_error_id(program, &errorid))
		return FALSE;

	gchar *error_message = _("Unknown error");
	switch (errorid) {
	case GEBR_IEXPR_ERROR_SYNTAX:
	case GEBR_IEXPR_ERROR_RUNTIME:
	case GEBR_IEXPR_ERROR_INVAL_TYPE:
	case GEBR_IEXPR_ERROR_TYPE_MISMATCH:
		error_message = _("This program has an invalid expression");
		break;
	case GEBR_IEXPR_ERROR_EMPTY_EXPR:
		error_message = _("A required parameter is unfilled");
		break;
	case GEBR_IEXPR_ERROR_UNDEF_VAR:
	case GEBR_IEXPR_ERROR_UNDEF_REFERENCE:	
		error_message = _("An undefined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_INVAL_VAR:
	case GEBR_IEXPR_ERROR_BAD_REFERENCE:
	case GEBR_IEXPR_ERROR_CYCLE:
		error_message = _("A not well defined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_PATH:
		error_message = _("This program has cleaned their paths");
		break;
	case GEBR_IEXPR_ERROR_BAD_MOVE:
	case GEBR_IEXPR_ERROR_INITIALIZE:
		break;
	}

	gtk_tooltip_set_text(tooltip, error_message);

set_tooltip:
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_set_tooltip_row(treeview, tooltip, path);
	gtk_tree_path_free(path);

	return TRUE;
}

/**
 * \internal
 * If the server was not connected, the combo box item stay insensitive
 */
static void
on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
					   GtkCellRenderer *cell,
					   GtkTreeModel    *tree_model,
					   GtkTreeIter     *iter,
					   gpointer         data)
{

	GebrServer *server;

	gtk_tree_model_get (tree_model, iter, SERVER_POINTER, &server, -1);

	if (server && server->comm)
		g_object_set (cell, "sensitive",
			      gebr_comm_server_is_logged (server->comm), NULL);
}

static void on_queue_combobox_changed (GtkComboBox *combo, GtkComboBox *server_combo)
{
	gint index;
	GebrServer *server;
	GtkTreeIter iter;
	GtkTreeIter server_iter;
	GtkTreeModel *server_model;
	GHashTable *last_queue_hash;

	if (!flow_browse_get_selected (&iter, FALSE))
		return;

	if (!gtk_combo_box_get_active_iter (server_combo, &server_iter))
		return;

	server_model = gtk_combo_box_get_model (server_combo);
	gtk_tree_model_get (server_model, &server_iter,
			    SERVER_POINTER, &server,
			    -1);

	index = gtk_combo_box_get_active (combo);
	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_browse->store), &iter,
			    FB_LAST_QUEUES, &last_queue_hash,
			    -1);

	if (!last_queue_hash)
		return;

	if (index < 0)
		index = 0;

	g_hash_table_insert (last_queue_hash,
			     g_strdup (server->comm->address->str),
			     GINT_TO_POINTER (index));
}

gboolean
flow_edition_find_flow_server (GebrGeoXmlFlow *flow,
			       GtkTreeModel   *model,
			       GtkTreeIter    *iter)
{
  const gchar *addr;
  gboolean     valid;
  GebrServer  *server;

  addr = gebr_geoxml_flow_server_get_address (flow);
  valid = gtk_tree_model_get_iter_first (model, iter);
  while (valid)
    {
      gtk_tree_model_get (model, iter, SERVER_POINTER, &server, -1);
      if (g_strcmp0 (server->comm->address->str, addr) == 0)
        return TRUE;
      valid = gtk_tree_model_iter_next (model, iter);
    }

  gtk_tree_model_get_iter_first (model, iter);
  return FALSE;
}

void
flow_edition_revalidate_programs(void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GebrGeoXmlProgram *program;

	model = GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);

		if (!program)
			continue;

		if (gebr_geoxml_program_get_status(program) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
			continue;

		if(gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			if (validate_program_iter(&iter, NULL))
				gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
			else
				gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
			const gchar *icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter, FSEQ_ICON_COLUMN, icon, -1);
			continue;
		}

		if (validate_program_iter(&iter, NULL))
			flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, &iter);
		else
			flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, &iter);
	}
}
