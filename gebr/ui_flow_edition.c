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
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-icons.h>
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

/*
 * Prototypes
 */

static gboolean
flow_edition_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition);
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition);

static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path);
static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text);
static void flow_edition_component_selected(void);
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected);

static void flow_edition_menu_add(void);
static void flow_edition_menu_show_help(void);

static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static void flow_edition_on_combobox_changed(GtkComboBox * combobox);

static gboolean
on_has_required_parameter_unfilled_tooltip(GtkTreeView * treeview,
		     gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltip, struct ui_flow_edition *ui_flow_edition);
static void
on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
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
	gtk_paned_pack1(GTK_PANED(hpanel), left_vbox, FALSE, FALSE);

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
	g_signal_connect(G_OBJECT(ui_flow_edition->fseq_view), "query-tooltip", G_CALLBACK(on_has_required_parameter_unfilled_tooltip), ui_flow_edition);

	ui_flow_edition->text_renderer = renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", FSEQ_TITLE_COLUMN);
	gtk_tree_view_column_add_attribute(col, renderer, "editable", FSEQ_EDITABLE);
	gtk_tree_view_column_add_attribute(col, renderer, "ellipsize", FSEQ_ELLIPSIZE);
	gtk_tree_view_column_add_attribute(col, renderer, "sensitive", FSEQ_SENSITIVE);

	g_signal_connect(renderer, "edited", G_CALLBACK(flow_edition_component_edited), NULL);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(flow_edition_component_editing_started), NULL);

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
	gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, TRUE);

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
	g_signal_connect(GTK_OBJECT(ui_flow_edition->menu_view), "row-activated",
			 G_CALLBACK(flow_edition_menu_add), ui_flow_edition);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_flow_edition->menu_view), MENU_TITLE_COLUMN);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Flow"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

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
	flow_edition_set_io();

	/* now into GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);
	flow_add_program_sequence_to_view(first_program, FALSE);
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

	gchar *input_markup  = g_markup_printf_escaped("<i>%s</i>", _("Input file"));
	gchar *output_markup = g_markup_printf_escaped("<i>%s</i>", _("Output file"));
	gchar *error_markup  = g_markup_printf_escaped("<i>%s</i>", _("Log file")); 

	const gchar *input  = (g_strcmp0(gebr_geoxml_flow_io_get_input(gebr.flow),"")   ? gebr_geoxml_flow_io_get_input(gebr.flow)  : input_markup);
	const gchar *output = (g_strcmp0(gebr_geoxml_flow_io_get_output(gebr.flow),"")  ? gebr_geoxml_flow_io_get_output(gebr.flow) : output_markup);
	const gchar *error  = (g_strcmp0(gebr_geoxml_flow_io_get_error(gebr.flow),"")   ? gebr_geoxml_flow_io_get_error(gebr.flow)  : error_markup);

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
			   FSEQ_ICON_COLUMN, "gebr-stdin", FSEQ_TITLE_COLUMN, input, 
			   FSEQ_EDITABLE, TRUE, FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START, -1);

	if (gebr_geoxml_flow_io_get_output_append (gebr.flow))
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stdout", FSEQ_TITLE_COLUMN, output, 
				   FSEQ_EDITABLE, TRUE, FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START, -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-stdout", FSEQ_TITLE_COLUMN, output, 
				   FSEQ_EDITABLE, TRUE, FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START, -1);
	}

	if (gebr_geoxml_flow_io_get_error_append (gebr.flow))
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stderr", FSEQ_TITLE_COLUMN, error,
				   FSEQ_EDITABLE, TRUE, FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START, -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-stderr", FSEQ_TITLE_COLUMN, error,
				   FSEQ_EDITABLE, TRUE, FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START, -1);
	}

	flow_program_check_sensitiveness();
	flow_browse_info_update();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	g_free(input_markup);
	g_free(output_markup);
	g_free(error_markup);
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

static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path)
{
	GtkTreeIter iter;
	GtkEntry *entry = GTK_ENTRY(editable);
	const gchar *input;
	const gchar *output;
	const gchar *error;

	gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	g_object_set_data(G_OBJECT(entry), "path", g_strdup(path));
	g_object_set_data(G_OBJECT(entry), "renderer", renderer);

	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;

	input = gebr_geoxml_flow_io_get_input(gebr.flow);
	output = gebr_geoxml_flow_io_get_output(gebr.flow);
	error = gebr_geoxml_flow_io_get_error(gebr.flow);

	if ((gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) && (g_strcmp0(input, "") == 0))  ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) && (g_strcmp0(output, "") == 0)) ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter) && (g_strcmp0(error, "") == 0)))
		gtk_entry_set_text(entry, "");

	g_signal_connect(entry, "icon-release", G_CALLBACK(open_activated), NULL);

	flow_edition_set_io();
}

static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text)
{
	GtkTreeIter iter;

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
	const gchar		* icon;
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
		gtk_tree_model_get_iter (model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
				    FSEQ_GEBR_GEOXML_POINTER, &program,
				    -1);

		status = gebr_geoxml_program_get_status (program);

		if ((status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) ||
		    ((status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED) &&
		     (!validate_program_iter(&iter)))) {
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
		gtk_tree_model_get_iter (model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
				    FSEQ_GEBR_GEOXML_POINTER, &program,
				    -1);

		if (validate_program_iter(&iter)
		    && status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			gebr_geoxml_program_set_status (program, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
		} else {
			gebr_geoxml_program_set_status (program, status);
		}

		icon = gebr_gui_get_program_icon (program);
		gtk_list_store_set (gebr.ui_flow_edition->fseq_store, &iter,
				    FSEQ_ICON_COLUMN, icon,
				    -1);

		listiter = listiter->next;
	}

	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	return TRUE;
}

void flow_edition_status_changed(guint status)
{
	guint old_status;
	GtkTreeIter iter;
	const gchar *icon;
	GtkTreeModel *model;
	GtkTreePath *input_path, *output_path, *error_path;
	GtkTreePath *path;
	GebrGeoXmlProgram *program;

	model = GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store);
	input_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->input_iter);
	output_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->output_iter);
	error_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->error_iter);

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_compare (path, input_path) == 0 ||
		    gtk_tree_path_compare (path, output_path) == 0 ||
		    gtk_tree_path_compare (path, error_path) == 0) {
			gtk_tree_path_free (path);
			continue;
		}

		gtk_tree_path_free (path);
		gtk_tree_model_get(model, &iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);

		old_status = gebr_geoxml_program_get_status (program);

		if (validate_program_iter (&iter) &&
		    status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			status = GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

		gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), status);
		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter, FSEQ_ICON_COLUMN, icon, -1);
	}

	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	gtk_tree_path_free (input_path);
	gtk_tree_path_free (output_path);
	gtk_tree_path_free (error_path);
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

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	gtk_action_set_sensitive(action, !validate_selected_program());

	action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_help");
	gtk_action_set_sensitive(action, strlen(gebr_geoxml_program_get_help(gebr.program)) != 0);
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
		parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE);

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
	if (xml != NULL && strlen(gebr_geoxml_document_get_help(xml)) <= 1)
		gtk_widget_set_sensitive(menu_item, FALSE);
	if (xml)
		document_free(xml);
	g_free(menu_filename);

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
on_has_required_parameter_unfilled_tooltip(GtkTreeView * treeview,
		     gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltip, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GebrGeoXmlProgram *program;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip, &model, NULL, &iter))
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			   FSEQ_GEBR_GEOXML_POINTER, &program, -1);

	gchar * message;
	if gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) {
		if (g_strcmp0(gebr_geoxml_flow_io_get_input(gebr.flow), "") == 0)
			message = g_strdup(_("Choose input file"));
		else
			message = g_strdup_printf(_("Input file '%s'"), gebr_geoxml_flow_io_get_input(gebr.flow));

		gtk_tooltip_set_text(tooltip, message);
		g_free(message);
	}
	else if gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) {
		if (g_strcmp0(gebr_geoxml_flow_io_get_output(gebr.flow),"") == 0)
			message = g_strdup(_("Choose output file"));
		else
			message = g_strdup_printf(_("Output file '%s'"), gebr_geoxml_flow_io_get_output(gebr.flow));

		gtk_tooltip_set_text(tooltip, message);
		g_free(message);
	}
	else if gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter){
		if (g_strcmp0(gebr_geoxml_flow_io_get_error(gebr.flow),"") == 0)
			message = g_strdup(_("Choose log file"));
		else
			message = g_strdup_printf(_("Log file '%s'"), gebr_geoxml_flow_io_get_error(gebr.flow));

		gtk_tooltip_set_text(tooltip, message);
		g_free(message);
	}
	else if (gebr_geoxml_program_get_status(program) != GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED)
		return FALSE;
	else
		if (validate_program_iter(&iter))
			gtk_tooltip_set_text(tooltip, _("Required parameter unfilled"));
		else
			gtk_tooltip_set_text(tooltip, _("Verify parameter configuration"));

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
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
		g_return_if_reached ();

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
