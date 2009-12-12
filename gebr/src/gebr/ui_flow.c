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
 * File: ui_flow.c
 */

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/gui/gtkfileentry.h>
#include <libgebr/gui/utils.h>

#include "ui_flow.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_server.h"

#define GEBR_FLOW_UI_RESPONSE_EXECUTE 1

/*
 * Declarations
 */

static gboolean
flow_io_select_function(GtkTreeSelection * selection, GtkTreeModel * model, GtkTreePath * path,
gboolean path_currently_selected, struct ui_flow_io * ui_flow_io);

static void
flow_io_populate		(struct ui_flow_io *	ui_flow_io);

static gboolean
flow_io_actions			(gint			response,
				 struct ui_flow_io *	ui_flow_io);
static void
flow_io_run			(GebrGeoXmlFlowServer *	server);

static void
on_renderer_edited		(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io);
static void
on_renderer_editing_started	(GtkCellRenderer *	renderer,
				 GtkCellEditable *	editable,
				 gchar *		path,
				 struct ui_flow_io *	ui_flow_io);
static void
on_renderer_entry_icon_release	(GtkEntry *		widget,
				 GtkEntryIconPosition	position,
				 GdkEvent *		event,
				 struct ui_flow_io *	ui_flow_io);
static void
on_button_add_clicked		(GtkButton *		button,
				 struct ui_flow_io *	ui_flow_io);
static void
on_entry_activate		(GtkEntry *		entry,
				 struct ui_flow_io *	ui_flow_io);
static void
on_entry_icon_release		(GtkEntry *		entry);
static void
on_combo_changed		(GtkComboBox *		combo,
				 struct ui_flow_io *	ui_flow_io);

static GtkMenu *
on_menu_popup			(GtkTreeView *		treeview,
				 struct ui_flow_io *	data);

static void
on_delete_server_io_activate	(GtkWidget *		menu_item,
				 struct ui_flow_io *	ui_flow_io);

static gboolean
on_tree_view_tooltip		(GtkTreeView *		treeview,
				 gint			x,
				 gint			y,
				 gboolean		keyboard_mode,
				 GtkTooltip *		tooltip,
				 struct ui_flow_io *	ui_flow_io);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: flow_io_setup_ui
 * A dialog for user selection of the flow IO files
 *
 * Return:
 * The structure containing relevant data.
 * It will be automatically freed when the
 * dialog closes.
 */
void
flow_io_setup_ui(gboolean executable)
{
	struct ui_flow_io *	ui_flow_io;

	GtkWidget *		dialog;
	GtkWidget *		treeview;
	GtkListStore *		store;

	GtkWidget *		address;
	GtkWidget *		input;
	GtkWidget *		output;
	GtkWidget *		error;

	GtkWidget *		content;
	GtkWidget *		button_add;
	GtkWidget *		table;
	GtkSizeGroup *		size_group;
	GtkCellRenderer *	renderer;
	GtkTreeViewColumn *	column;
	GtkTreeIter		iter;
	GtkTreeIter		server_iter;
	GebrGeoXmlFlowServer *	flow_server;
	struct server *		server;

	gint			response;

	// Sizes for columns
	const gint size_addr   = 140;
	const gint size_input  = 90;
	const gint size_output = 90;
	const gint size_error  = 90;

	ui_flow_io = g_malloc(sizeof(struct ui_flow_io));
	store = gtk_list_store_new(FLOW_IO_N,
		GDK_TYPE_PIXBUF,	// Icon
		G_TYPE_STRING,		// Address
		G_TYPE_STRING,		// Input
		G_TYPE_STRING,		// Output
		G_TYPE_STRING,		// Error
		G_TYPE_BOOLEAN,	 	// Sensitive
		G_TYPE_POINTER,		// GebrGeoXmlFlowServer
		G_TYPE_POINTER);	// struct server
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_selection_set_select_function(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
		(GtkTreeSelectionFunc)flow_io_select_function,
		ui_flow_io, NULL);
	dialog = gtk_dialog_new_with_buttons(_("Server and I/O setup"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	if (executable)
		gtk_dialog_add_button(GTK_DIALOG(dialog),
			GTK_STOCK_EXECUTE, GEBR_FLOW_UI_RESPONSE_EXECUTE);
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
		GTK_SELECTION_BROWSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 2);
#if GTK_CHECK_VERSION(2,12,0)
	g_object_set(G_OBJECT(treeview), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT(treeview), "query-tooltip",
		G_CALLBACK(on_tree_view_tooltip), ui_flow_io);
#endif
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(treeview),
		(GebrGuiGtkPopupCallback)on_menu_popup, ui_flow_io);

	//---------------------------------------
	// Setting up the GtkTreeView
	//---------------------------------------
	
	// Server column
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer,
		"pixbuf", FLOW_IO_ICON);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer,
		"text", FLOW_IO_SERVER_NAME);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_title(column, _("Server"));
	gtk_tree_view_column_set_fixed_width(column, size_addr);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	// Input column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started",
		G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Input"), renderer,
		"text", FLOW_IO_INPUT, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_input);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", (gpointer)FLOW_IO_INPUT);

	// Output column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started",
		G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Output"), renderer,
		"text", FLOW_IO_OUTPUT, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_output);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", (gpointer)FLOW_IO_OUTPUT);

	// Error column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started",
		G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Error"), renderer,
		"text", FLOW_IO_ERROR, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_error);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", (gpointer)FLOW_IO_ERROR);

	//---------------------------------------
	// Setting up the edition area
	//---------------------------------------

	// |____|^| Address combo box
	address  = gtk_combo_box_new_with_model(
		GTK_TREE_MODEL(gebr.ui_server_list->common.store));
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(address), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(address), renderer,
		"pixbuf", SERVER_STATUS_ICON);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(address), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(address), renderer,
		"text", SERVER_NAME);
	gtk_combo_box_set_active(GTK_COMBO_BOX(address), 0);
	gtk_widget_set_size_request(address, size_addr, -1);
	g_signal_connect(address, "changed",
		G_CALLBACK(on_combo_changed), ui_flow_io);

	// |______| Input, Output and Error entries
	input  = gtk_entry_new();
	output = gtk_entry_new();
	error  = gtk_entry_new();
	gtk_widget_set_size_request(input, size_input, -1);
	gtk_widget_set_size_request(output, size_output, -1);
	gtk_widget_set_size_request(error, size_error, -1);
	g_signal_connect(input, "activate",
		G_CALLBACK(on_entry_activate), ui_flow_io);
	g_signal_connect(output, "activate",
		G_CALLBACK(on_entry_activate), ui_flow_io);
	g_signal_connect(error, "activate",
		G_CALLBACK(on_entry_activate), ui_flow_io);
	g_signal_connect(input, "icon-release",
		G_CALLBACK(on_entry_icon_release), NULL);
	g_signal_connect(output, "icon-release",
		G_CALLBACK(on_entry_icon_release), NULL);
	g_signal_connect(error, "icon-release",
		G_CALLBACK(on_entry_icon_release), NULL);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(input),
		GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIRECTORY);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(output),
		GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIRECTORY);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(error),
		GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIRECTORY);

	// [+] Add button
	button_add = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button_add),
		gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button_add, "clicked",
		G_CALLBACK(on_button_add_clicked), ui_flow_io);

	//---------------------------------------
	// Packing the widgets
	//---------------------------------------

	table = gtk_table_new(2, 5, FALSE);
	gtk_table_attach(GTK_TABLE(table), gtk_label_new("Address"),
		0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), gtk_label_new("Input"),
		1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), gtk_label_new("Output"),
		2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), gtk_label_new("Error"),
		3, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), address,
		0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), input,
		1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), output,
		2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), error,
		3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), button_add,
		4, 5, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
	gtk_size_group_add_widget(size_group, address);
	gtk_size_group_add_widget(size_group, input);
	gtk_size_group_add_widget(size_group, output);
	gtk_size_group_add_widget(size_group, error);

	gtk_box_pack_start(GTK_BOX(content), treeview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(content), table, FALSE, TRUE, 0);

	ui_flow_io->dialog     = dialog;
	ui_flow_io->store      = store;
	ui_flow_io->treeview   = treeview;
	ui_flow_io->address    = address;
	ui_flow_io->input      = input;
	ui_flow_io->output     = output;
	ui_flow_io->error      = error;

	// Fill the GtkListStore with the configurations available
	flow_io_populate(ui_flow_io);
	on_combo_changed(GTK_COMBO_BOX(address), ui_flow_io);

	gtk_widget_show_all(dialog);
	do
		response = gtk_dialog_run(GTK_DIALOG(dialog));
	while (!flow_io_actions(response, ui_flow_io));

	if (flow_io_get_selected(ui_flow_io, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
			FLOW_IO_FLOW_SERVER_POINTER, &flow_server,
			FLOW_IO_SERVER_POINTER, &server, -1);
		/* move selected server/IO to the first server/IO of the server */
		gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(flow_server), NULL);
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
		/* select current server on flow edition */
		server_find(server, &server_iter);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox),
			&server_iter);
	} else
		gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), 0);
	flow_edition_on_server_changed();

	// Clean up
	gtk_widget_destroy(dialog);
	g_free(ui_flow_io);
}

/**
 * flow_io_get_selected:
 * @ui_flow_io: The structure containing the #GtkTreeView.
 * @iter: The iterator that will point to the selected item.
 *
 * Makes @iter point to the selected item in the #GtkTreeView
 * of @ui_flow_io structure.
 *
 * Returns: %TRUE if there is a selection, %FALSE otherwise.
 * In case of %FALSE, @iter is invalid.
 */
gboolean
flow_io_get_selected(struct ui_flow_io * ui_flow_io, GtkTreeIter * iter)
{
	return gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(
			GTK_TREE_VIEW(ui_flow_io->treeview)), NULL, iter);
}

/**
 * flow_io_select_iter:
 * @ui_flow_io: The structure containing the #GtkTreeView.
 * @iter: A valid iterator the @ui_flow_io 's model to.
 *
 * Sets the selection of the #GtkTreeView of @ui_flow_io to
 * @iter. You must garantee that @iter is valid.
 */
void
flow_io_select_iter(struct ui_flow_io * ui_flow_io, GtkTreeIter * iter)
{
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_io->treeview)), iter);
}

/**
 * customize_paths_from_line:
 * @chooser: A #GtkFileChooser.
 *
 * Set line's path to input/output/error files.
 */
void
flow_io_customized_paths_from_line(GtkFileChooser * chooser)
{
	GError *		error;
	GebrGeoXmlSequence *	path_sequence;

	if(gebr.line == NULL)
		return;

	error = NULL;
	gebr_geoxml_line_get_path(gebr.line, &path_sequence, 0);
	if (path_sequence != NULL) {
		gtk_file_chooser_set_current_folder(
			chooser, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(path_sequence)));

		do {
			gtk_file_chooser_add_shortcut_folder(
				chooser, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(path_sequence)),
				&error);
			gebr_geoxml_sequence_next(&path_sequence);
		} while (path_sequence != NULL);
	}
}

/*
 * flow_io_set_server:
 * Write server to current flow
 */
void
flow_io_set_server(GtkTreeIter * server_iter, const gchar * input, const gchar * output,
const gchar * error)
{
	struct server *	server;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), server_iter,
		SERVER_POINTER, &server,
		-1);

	gebr_geoxml_flow_server_set_address(gebr.flow_server, server->comm->address->str);
	gebr_geoxml_flow_server_io_set_input(gebr.flow_server, input);
	gebr_geoxml_flow_server_io_set_output(gebr.flow_server, output);
	gebr_geoxml_flow_server_io_set_error(gebr.flow_server, error);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

void
flow_io_simple_setup_ui(gboolean focus_output)
{
	struct ui_flow_simple *	simple;
	GtkWidget *		dialog;
	GtkWidget *		table;
	GtkWidget *		label;
	gchar *			tmp;

	GtkTreeIter		server_iter;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(
		gebr.ui_flow_edition->server_combobox), &server_iter);

	simple = (struct ui_flow_simple*)g_malloc(sizeof(struct ui_flow_simple));
	simple->focus_output = focus_output;

	dialog = gtk_dialog_new_with_buttons(_("Flow input/output"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);
	simple->dialog = dialog;

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Input */
	label = gtk_label_new(_("Input file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->input = gebr_gui_gtk_file_entry_new(
		(GebrGuiGtkFileEntryCustomize)flow_io_customized_paths_from_line, NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->input, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
GTK_FILL, 3, 3);
	if ((tmp = (gchar*)gebr_geoxml_flow_server_io_get_input(gebr.flow_server)) != NULL)
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(simple->input), tmp);
	gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(simple->input),
 FALSE);

	/* Output */
	label = gtk_label_new(_("Output file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->output = gebr_gui_gtk_file_entry_new(
		(GebrGuiGtkFileEntryCustomize)flow_io_customized_paths_from_line, NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->output, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	if ((tmp = (gchar*)gebr_geoxml_flow_server_io_get_output(gebr.flow_server)) != NULL)
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(simple->output), tmp);
	gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(simple->output), FALSE);

	/* Error */
	label = gtk_label_new(_("Error file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->error = gebr_gui_gtk_file_entry_new(
		(GebrGuiGtkFileEntryCustomize)flow_io_customized_paths_from_line, NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->error, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	if ((tmp = (gchar*)gebr_geoxml_flow_server_io_get_error(gebr.flow_server)) != NULL)
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(simple->error), tmp);
	gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(simple->error), FALSE);

	gtk_widget_show_all(dialog);
	if (focus_output)
		gtk_widget_grab_focus(simple->output);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto out;

	flow_io_set_server(&server_iter,
		gebr_gui_gtk_file_entry_get_path(GEBR_GUI_GTK_FILE_ENTRY(simple->input)),
		gebr_gui_gtk_file_entry_get_path(GEBR_GUI_GTK_FILE_ENTRY(simple->output)),
		gebr_gui_gtk_file_entry_get_path(GEBR_GUI_GTK_FILE_ENTRY(simple->error)));
	flow_edition_set_io();

out:	gtk_widget_destroy(dialog);
}

/**
 * flow_fast_run:
 */
void
flow_fast_run()
{
	flow_io_run(gebr.flow_server);
}

/**
 * flow_add_program_sequence_to_view:
 * @program:
 * @select_last:
 *
 * Add @program sequence (from it to the end of sequence) to flow sequence view.
 */
void
flow_add_program_sequence_to_view(GebrGeoXmlSequence * program, gboolean select_last)
{
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		GtkTreeIter		iter;
		gchar *			menu;
		gulong			prog_index;
		const gchar *		status;

		GdkPixbuf *		pixbuf;

		gebr_geoxml_program_get_menu(GEBR_GEOXML_PROGRAM(program), &menu, &prog_index);
		status = gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program));

		if (strcmp(status, "unconfigured") == 0)
			pixbuf = gebr.pixmaps.stock_warning;
		else if (strcmp(status, "configured") == 0)
			pixbuf = gebr.pixmaps.stock_apply;
		else if (strcmp(status, "disabled") == 0)
			pixbuf = gebr.pixmaps.stock_cancel;
		else {
			gebr_message(GEBR_LOG_WARNING, TRUE, TRUE, _("Unknown flow program '%s' status"),
				gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			pixbuf = NULL;
		}

		/* Add to the GUI */
		gtk_list_store_insert_before(gebr.ui_flow_edition->fseq_store,
			&iter, &gebr.ui_flow_edition->output_iter);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
			FSEQ_TITLE_COLUMN, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)),
			FSEQ_STATUS_COLUMN, pixbuf,
			FSEQ_GEBR_GEOXML_POINTER, program,
			FSEQ_MENU_FILENAME_COLUMN, menu,
			FSEQ_MENU_INDEX, prog_index,
			-1);

		if (select_last)
			flow_edition_select_component_iter(&iter);
	}
}

/*
 * Section: Private
 * Private functions.
 */

/* Function: flow_io_select_function
 * Avoid selection of server not in server list
 */
gboolean
flow_io_select_function(GtkTreeSelection * selection, GtkTreeModel * model, GtkTreePath * path,
gboolean path_currently_selected, struct ui_flow_io * ui_flow_io)
{
	GtkTreeIter	iter;
	gboolean	sensitive;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_SENSITIVE, &sensitive,
		-1);

	return sensitive;
}

/**
 * flow_io_populate:
 * @ui_flow_io: The structure to be filled with data.
 *
 * Fills the #GtkListStore of @ui_flow_io with data.
 */
static void
flow_io_populate(struct ui_flow_io * ui_flow_io)
{
	GebrGeoXmlSequence *	server_io;
	GtkTreeIter		iter;
	GdkPixbuf *		icon;

	gebr_geoxml_flow_get_server(gebr.flow, &server_io, 0);
	for (; server_io != NULL; gebr_geoxml_sequence_next(&server_io)) {
		const gchar *	address;
		gchar *		name;
		const gchar *	input;
		const gchar *	output;
		const gchar *	error;
		const gchar *	date;
		gboolean	server_found;
		GtkTreeIter	server_iter;
		struct server *	server;

		address = gebr_geoxml_flow_server_get_address(GEBR_GEOXML_FLOW_SERVER(server_io));
		input = gebr_geoxml_flow_server_io_get_input(GEBR_GEOXML_FLOW_SERVER(server_io));
		output = gebr_geoxml_flow_server_io_get_output(GEBR_GEOXML_FLOW_SERVER(server_io));
		error = gebr_geoxml_flow_server_io_get_error(GEBR_GEOXML_FLOW_SERVER(server_io));
		date = gebr_geoxml_flow_server_get_date_last_run(GEBR_GEOXML_FLOW_SERVER(server_io));

		server_found = server_find_address(address, &server_iter);
		if (server_found)
			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &server_iter,
				SERVER_STATUS_ICON, &icon,
				SERVER_NAME, &name,
				SERVER_POINTER, &server,
				-1);
		else {
			icon = gebr.pixmaps.stock_warning;
			name = g_strdup(address);
		}

		gtk_list_store_append(ui_flow_io->store, &iter);
		gtk_list_store_set(ui_flow_io->store, &iter,
			FLOW_IO_ICON, icon,
			FLOW_IO_SERVER_NAME, name,
			FLOW_IO_INPUT, input,
			FLOW_IO_OUTPUT, output,
			FLOW_IO_ERROR, error,
			FLOW_IO_SENSITIVE, server_found,
			FLOW_IO_FLOW_SERVER_POINTER, server_io,
			FLOW_IO_SERVER_POINTER, server,
			-1);

		g_free(name);
	}

	/* select first, which is the last edited */
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(ui_flow_io->store)) {
		gboolean sensitive;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
			FLOW_IO_SENSITIVE, &sensitive,
			-1);

		if (sensitive) {
			flow_io_select_iter(ui_flow_io, &iter);
			break;
		}
	}
}

/**
 * flow_io_actions:
 * @response: The response id sent by the dialog.
 * @ui_flow_io: The structure representing the servers io dialog.
 *
 * Actions for Flow IO files edition dialog.
 *
 * Returns: %TRUE if the dialog should be destroyed, %FALSE otherwise.
 */
static gboolean
flow_io_actions(gint response, struct ui_flow_io * ui_flow_io)
{
	GtkTreeIter		iter;
	GdkPixbuf *		icon;
	GebrGeoXmlFlowServer *	flow_server;

	if (response == GEBR_FLOW_UI_RESPONSE_EXECUTE) {
		if (!flow_io_get_selected(ui_flow_io, &iter))
			return TRUE;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
			FLOW_IO_ICON, &icon,
			FLOW_IO_FLOW_SERVER_POINTER, &flow_server,
			-1);

		if (icon != gebr.pixmaps.stock_connect) {
			GtkWidget * dialog;
			dialog = gtk_message_dialog_new(
				GTK_WINDOW(gebr.window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("This server is disconnected. "
				  "To use this server, you must "
				  "connect it first."));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return FALSE;
		}

		flow_io_run(flow_server);
	}

	return TRUE;
}

/*
 * flow_io_run:
 * @server: a #GebrGeoXmlFlowServer
 *
 * Copies the IO informations to the flow and run it.
 */
static void
flow_io_run(GebrGeoXmlFlowServer * flow_server)
{
	GtkTreeIter	iter;
	const gchar *	address;
	struct server *	server;

	address = gebr_geoxml_flow_server_get_address(flow_server);
	if (!server_find_address(address, &iter))
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, "Server should be present on list!");

	gebr_geoxml_flow_server_set_date_last_run(flow_server, gebr_iso_date());
	gebr_geoxml_flow_io_set_from_server(gebr.flow, flow_server);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store),
		&iter, SERVER_POINTER, &server, -1);
	flow_run(server);
}


static void
on_renderer_edited		(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io)
{
	gint			column;
	GtkTreeIter		iter;
	GebrGeoXmlFlowServer *	flow_server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	column = (gint)g_object_get_data(G_OBJECT(renderer), "column");

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_FLOW_SERVER_POINTER, &flow_server, -1);
	gtk_list_store_set(ui_flow_io->store, &iter,
		column, new_text,
		-1);
	switch (column) {
		case FLOW_IO_INPUT:
			gebr_geoxml_flow_server_io_set_input(flow_server, new_text);
			break;
		case FLOW_IO_OUTPUT:
			gebr_geoxml_flow_server_io_set_output(flow_server, new_text);
			break;
		case FLOW_IO_ERROR:
			gebr_geoxml_flow_server_io_set_error(flow_server, new_text);
			break;
	}

	gtk_list_store_move_after(ui_flow_io->store, &iter, NULL);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(flow_server), NULL);
	
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

static void
on_renderer_editing_started	(GtkCellRenderer *	renderer,
				 GtkCellEditable *	editable,
				 gchar *		path,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter	iter;
	gpointer	data;

	if (!GTK_IS_ENTRY(editable)
			|| !flow_io_get_selected(ui_flow_io, &iter))
		return;

	data = g_object_get_data(G_OBJECT(renderer), "column");
	g_object_set_data(G_OBJECT(editable), "column", data);
	g_object_set_data(G_OBJECT(editable), "renderer", (gpointer)renderer);

	gtk_entry_set_icon_from_stock(
		GTK_ENTRY(editable),
		GTK_ENTRY_ICON_SECONDARY,
		GTK_STOCK_DIRECTORY);
	g_signal_connect(editable, "icon-release",
		G_CALLBACK(on_renderer_entry_icon_release), ui_flow_io);
}

static void
on_renderer_entry_icon_release	(GtkEntry *		widget,
				 GtkEntryIconPosition	position,
				 GdkEvent *		event,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkWidget *	dialog;
	gint		response;
	gchar *		filename;
	gint		column;

	if (!flow_io_get_selected(ui_flow_io, NULL))
		return;

	column = (gint)g_object_get_data(G_OBJECT(widget), "column");
	dialog = gtk_file_chooser_dialog_new(_("Choose a file"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_APPLY,
		NULL);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response != GTK_RESPONSE_APPLY)
		goto out;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	on_renderer_edited(
		GTK_CELL_RENDERER_TEXT(
			g_object_get_data(G_OBJECT(widget), "renderer")),
		NULL, filename, ui_flow_io);
	g_free(filename);

out:	gtk_widget_destroy(dialog);
}

static void
on_button_add_clicked		(GtkButton *		button,
				 struct ui_flow_io *	ui_flow_io)
{
	GdkPixbuf *		icon;
	GtkTreeIter		iter;

	gchar *			name;
	const gchar *		input;
	const gchar *		output;
	const gchar *		error;
	GebrGeoXmlFlowServer *	flow_server;
	struct server *		server;

	// Fetch data
	gtk_combo_box_get_active_iter(
		GTK_COMBO_BOX(ui_flow_io->address), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
		SERVER_STATUS_ICON, &icon,
		SERVER_NAME, &name,
		SERVER_POINTER, &server,
		-1);
	input  = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->input));
	output = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->output));
	error  = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->error));
	flow_server = gebr_geoxml_flow_append_server(gebr.flow);
	gebr_geoxml_flow_server_set_address(flow_server, server->comm->address->str);
	gebr_geoxml_flow_server_io_set_input(flow_server, input);
	gebr_geoxml_flow_server_io_set_output(flow_server, output);
	gebr_geoxml_flow_server_io_set_error(flow_server, error);

	// Append data to Dialog
	gtk_list_store_append(ui_flow_io->store, &iter);
	gtk_list_store_set(ui_flow_io->store, &iter,
		FLOW_IO_ICON, icon,
		FLOW_IO_SERVER_NAME, name,
		FLOW_IO_INPUT, input,
		FLOW_IO_OUTPUT, output,
		FLOW_IO_ERROR, error,
		FLOW_IO_SENSITIVE, TRUE,
		FLOW_IO_FLOW_SERVER_POINTER, flow_server,
		FLOW_IO_SERVER_POINTER, server,
		-1);

	gtk_list_store_move_after(ui_flow_io->store, &iter, NULL);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(flow_server), NULL);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	g_free(name);
}

static void
on_entry_activate		(GtkEntry *		entry,
				 struct ui_flow_io *	ui_flow_io)
{
	on_button_add_clicked(NULL, ui_flow_io);
}

static void
on_entry_icon_release		(GtkEntry *		entry)
{
	GtkWidget *	dialog;
	gint		response;
	gchar *		filename;

	dialog = gtk_file_chooser_dialog_new(_("Choose a file"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_APPLY,
		NULL);
	flow_io_customized_paths_from_line(GTK_FILE_CHOOSER(dialog));

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response != GTK_RESPONSE_APPLY)
		goto out;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_entry_set_text(entry, filename);
	g_free(filename);

out:	gtk_widget_destroy(dialog);
}

static void
on_combo_changed		(GtkComboBox *		combo,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter	iter;
	struct server *	server;
	gboolean	activatable;

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store),
		&iter, SERVER_POINTER, &server, -1);
	activatable = gebr_comm_server_is_local(server->comm);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(ui_flow_io->input), GTK_ENTRY_ICON_SECONDARY,
		activatable? GTK_STOCK_DIRECTORY:NULL);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(ui_flow_io->output), GTK_ENTRY_ICON_SECONDARY,
		activatable? GTK_STOCK_DIRECTORY:NULL);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(ui_flow_io->error), GTK_ENTRY_ICON_SECONDARY,
		activatable? GTK_STOCK_DIRECTORY:NULL);
}

static void
on_delete_server_io_activate	(GtkWidget *		menu_item,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter		iter;
	GebrGeoXmlSequence *	server;
	
	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_FLOW_SERVER_POINTER, &server, -1);

	gebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(ui_flow_io->treeview));

	gebr_geoxml_sequence_remove(server);
	gtk_list_store_remove(ui_flow_io->store, &iter);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

static GtkMenu *
on_menu_popup			(GtkTreeView *		treeview,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkWidget *		menu;
	GtkWidget *		menu_item;

	if (!flow_io_get_selected(ui_flow_io, NULL))
		return NULL;

	menu = gtk_menu_new();
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
		G_CALLBACK(on_delete_server_io_activate), ui_flow_io);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

#if GTK_CHECK_VERSION(2,12,0)
static gboolean
on_tree_view_tooltip		(GtkTreeView *		treeview,
				 gint			x,
				 gint			y,
				 gboolean		keyboard_tip,
				 GtkTooltip *		tooltip,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeModel *		model;
	GtkTreeIter		iter;
	GtkTreeViewColumn *	column;
	gchar *			text;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip,
			&model, NULL, &iter))
		return FALSE;

	if (keyboard_tip)
		gtk_tree_view_convert_widget_to_bin_window_coords(treeview,
				x, y, &x, &y);

	if (!gtk_tree_view_get_path_at_pos(treeview, x, y, NULL,
			&column, NULL, NULL)) {
		return FALSE;
	}

	if (gtk_tree_view_get_column(treeview, 0) == column) {
		GdkPixbuf * icon;
		gtk_tree_model_get(model, &iter,
			FLOW_IO_ICON, &icon, -1);
		if (icon == gebr.pixmaps.stock_warning)
			gtk_tooltip_set_text(tooltip,
				_("This server no longer exists in the servers list dialog."));
		else if (icon == gebr.pixmaps.stock_disconnect)
			gtk_tooltip_set_text(tooltip,
				_("This server is disconnected."));
		else
			return FALSE;
	} else
	if (gtk_tree_view_get_column(treeview, 1) == column) {
		gtk_tree_model_get(model, &iter,
			FLOW_IO_INPUT, &text, -1);
		if (!strlen(text)) {
			g_free(text);
			return FALSE;
		}
		gtk_tooltip_set_text(tooltip, text);
		g_free(text);
	} else
	if (gtk_tree_view_get_column(treeview, 2) == column) {
		gtk_tree_model_get(model, &iter,
			FLOW_IO_OUTPUT, &text, -1);
		if (!strlen(text)) {
			g_free(text);
			return FALSE;
		}
		gtk_tooltip_set_text(tooltip, text);
		g_free(text);
	} else
	if (gtk_tree_view_get_column(treeview, 3) == column) {
		gtk_tree_model_get(model, &iter,
			FLOW_IO_ERROR, &text, -1);
		if (!strlen(text)) {
			g_free(text);
			return FALSE;
		}
		gtk_tooltip_set_text(tooltip, text);
		g_free(text);
	} else {
		return FALSE;
	}

	return TRUE;
}
#endif

