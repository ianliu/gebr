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

static void
flow_io_populate		(struct ui_flow_io *	ui_flow_io);

static void
flow_io_actions			(gint			response,
				 struct ui_flow_io *	ui_flow_io);

static void
on_renderer_output_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io);
static void
on_renderer_input_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io);
static void
on_renderer_error_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io);

static void
on_button_add_clicked		(GtkButton *		button,
				 struct ui_flow_io *	ui_flow_io);

static void
on_entry_activate		(GtkEntry *		entry,
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
flow_io_setup_ui()
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
	GtkWidget *		box;
	GtkSizeGroup *		size_group;
	GtkCellRenderer *	renderer;
	GtkTreeViewColumn *	column;
	GtkTreeSelection *	selection;

	gint			response;

	// Sizes for columns
	const gint size_addr   = 150;
	const gint size_input  = 70;
	const gint size_output = 70;
	const gint size_error  = 70;

	ui_flow_io = (struct ui_flow_io*)g_malloc(sizeof(struct ui_flow_io));

	store = gtk_list_store_new(FLOW_IO_N,
		GDK_TYPE_PIXBUF,	// Icon
		G_TYPE_STRING,  	// Address
		G_TYPE_STRING,  	// Input
		G_TYPE_STRING,  	// Output
		G_TYPE_STRING,  	// Error
		G_TYPE_STRING,  	// Date
		G_TYPE_POINTER);	// GeoXmlFlowServer
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	dialog = gtk_dialog_new_with_buttons(_("Server IO configuration"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		GTK_STOCK_EXECUTE, GEBR_FLOW_UI_RESPONSE_EXECUTE,
		NULL);
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);

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
		"text", FLOW_IO_ADDRESS);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_title(column, _("Server"));
	gtk_tree_view_column_set_fixed_width(column, size_addr);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	// Input column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_input_edited), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Input"), renderer,
		"text", FLOW_IO_INPUT, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_input);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	// Output column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_output_edited), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Output"), renderer,
		"text", FLOW_IO_OUTPUT, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_output);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	// Error column
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(on_renderer_error_edited), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Error"), renderer,
		"text", FLOW_IO_ERROR, NULL);
	g_object_set(column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	gtk_tree_view_column_set_fixed_width(column, size_error);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

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
		"text", SERVER_ADDRESS);
	gtk_combo_box_set_active(GTK_COMBO_BOX(address), 0);
	gtk_widget_set_size_request(address, size_addr, -1);

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

	// [+] Add button
	box = gtk_hbox_new(FALSE, 0);
	button_add = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(button_add),
		gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button_add, "clicked",
		G_CALLBACK(on_button_add_clicked), ui_flow_io);

	//---------------------------------------
	// Packing the widgets
	//---------------------------------------

	gtk_box_pack_start(GTK_BOX(box), address, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), output, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), error, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), button_add, FALSE, TRUE, 0);

	size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
	gtk_size_group_add_widget(size_group, address);
	gtk_size_group_add_widget(size_group, input);
	gtk_size_group_add_widget(size_group, output);
	gtk_size_group_add_widget(size_group, error);

	gtk_box_pack_start(GTK_BOX(content), treeview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(content), box, FALSE, TRUE, 0);

	ui_flow_io->dialog   = dialog;
	ui_flow_io->store    = store;
	ui_flow_io->treeview = treeview;
	ui_flow_io->address  = address;
	ui_flow_io->input    = input;
	ui_flow_io->output   = output;
	ui_flow_io->error    = error;

	// Fill the GtkListStore with the configurations available
	flow_io_populate(ui_flow_io);

	gtk_widget_show_all(dialog);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	flow_io_actions(response, ui_flow_io);

	// Clean up
	gtk_list_store_clear(store);
	gtk_widget_destroy(dialog);
	g_free(ui_flow_io);
}

gboolean
flow_io_get_selected(struct ui_flow_io * ui_flow_io, GtkTreeIter * iter)
{
	return gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(
			GTK_TREE_VIEW(ui_flow_io->treeview)), NULL, iter);
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
	GeoXmlSequence *	path_sequence;

	if(gebr.line == NULL)
		return;

	error = NULL;
	geoxml_line_get_path(gebr.line, &path_sequence, 0);
	if (path_sequence != NULL) {
		gtk_file_chooser_set_current_folder(
			chooser, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(path_sequence)));

		do {
			gtk_file_chooser_add_shortcut_folder(
				chooser, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(path_sequence)),
				&error);
			geoxml_sequence_next(&path_sequence);
		} while (path_sequence != NULL);
	}
}

/**
 * flow_add_program_sequence_to_view:
 * @program:
 * @select_last:
 *
 * Add @program sequence (from it to the end of sequence) to flow sequence view.
 */
void
flow_add_program_sequence_to_view(GeoXmlSequence * program, gboolean select_last)
{
	for (; program != NULL; geoxml_sequence_next(&program)) {
		GtkTreeIter		iter;
		gchar *			menu;
		gulong			prog_index;
		const gchar *		status;

		GdkPixbuf *		pixbuf;

		geoxml_program_get_menu(GEOXML_PROGRAM(program), &menu, &prog_index);
		status = geoxml_program_get_status(GEOXML_PROGRAM(program));

		if (strcmp(status, "unconfigured") == 0)
			pixbuf = gebr.pixmaps.stock_warning;
		else if (strcmp(status, "configured") == 0)
			pixbuf = gebr.pixmaps.stock_apply;
		else if (strcmp(status, "disabled") == 0)
			pixbuf = gebr.pixmaps.stock_cancel;
		else {
			gebr_message(LOG_WARNING, TRUE, TRUE, _("Unknown flow program '%s' status"),
				geoxml_program_get_title(GEOXML_PROGRAM(program)));
			pixbuf = NULL;
		}

		/* Add to the GUI */
		gtk_list_store_insert_before(gebr.ui_flow_edition->fseq_store,
			&iter, &gebr.ui_flow_edition->output_iter);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
			FSEQ_TITLE_COLUMN, geoxml_program_get_title(GEOXML_PROGRAM(program)),
			FSEQ_STATUS_COLUMN, pixbuf,
			FSEQ_GEOXML_POINTER, program,
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

/**
 * flow_io_populate:
 * @ui_flow_io: The structure to be filled with data.
 *
 * Fills the #GtkListStore of @ui_flow_io with data.
 */
static void
flow_io_populate(struct ui_flow_io * ui_flow_io)
{
	GeoXmlSequence *	server_io;
	GtkTreeIter		server_iter;
	GtkTreeIter		last_run_iter;
	GtkTreeIter		iter;
	GdkPixbuf *		icon;

	const gchar *		last_run;

	last_run = "";
	geoxml_flow_get_server(gebr.flow, &server_io, 0);
	while (server_io) {
		const gchar *	address;
		const gchar *	input;
		const gchar *	output;
		const gchar *	error;
		const gchar *	date;

		address = geoxml_flow_server_get_address(
			GEOXML_FLOW_SERVER(server_io));
		input   = geoxml_flow_server_io_get_input(
			GEOXML_FLOW_SERVER(server_io));
		output  = geoxml_flow_server_io_get_output(
			GEOXML_FLOW_SERVER(server_io));
		error   = geoxml_flow_server_io_get_error(
			GEOXML_FLOW_SERVER(server_io));
		date    = geoxml_flow_server_get_date_last_run(
			GEOXML_FLOW_SERVER(server_io));

		if (!server_find_address(address, &server_iter))
			continue;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &server_iter,
			SERVER_STATUS_ICON, &icon,
			-1);

		gtk_list_store_append(ui_flow_io->store, &iter);
		gtk_list_store_set(ui_flow_io->store, &iter,
			FLOW_IO_ICON, icon,
			FLOW_IO_ADDRESS, address,
			FLOW_IO_INPUT, input,
			FLOW_IO_OUTPUT, output,
			FLOW_IO_ERROR, error,
			FLOW_IO_DATE, date,
			FLOW_IO_POINTER, server_io,
			-1);

		if (strcmp());

		geoxml_sequence_next(&server_io);
	}
}

/*
 * Function: flow_io_actions
 * Actions for Flow IO files edition dialog
 */
static void
flow_io_actions(gint response, struct ui_flow_io * ui_flow_io)
{
	GtkTreeIter		iter;
	GtkTreeIter		server_iter;

	gchar *			address;
	GdkPixbuf *		icon;
	struct server *		server;
	GeoXmlFlowServer *	flow_server;
	
	switch (response) {
		case GEBR_FLOW_UI_RESPONSE_EXECUTE:

			if (!flow_io_get_selected(ui_flow_io, &iter))
				goto out;

			gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
				FLOW_IO_ICON, &icon,
				FLOW_IO_ADDRESS, &address,
				FLOW_IO_POINTER, &flow_server,
				-1);

			if (!server_find_address(address, &server_iter)) {
				g_free(address);
				goto out;
			}
			g_free(address);

			if (icon != gebr.pixmaps.stock_connect) {
				GtkWidget * dialog;
				dialog = gtk_message_dialog_new(
					GTK_WINDOW(gebr.window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("This server is disconnected."
					" Do you want to connect?"));
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
				goto out;
			}

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &server_iter,
				SERVER_POINTER, &server,
				-1); 

			geoxml_flow_server_set_date_last_run(flow_server, iso_date());

			flow_run(server);

			break;
	}

out:	document_save(GEOXML_DOCUMENT(gebr.flow));
	flow_edition_set_io();
	flow_browse_info_update();
}

static void
on_renderer_input_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter		iter;
	GeoXmlFlowServer *	server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_POINTER, &server,
		-1);
	gtk_list_store_set(ui_flow_io->store, &iter,
		FLOW_IO_INPUT, new_text,
		-1);
	geoxml_flow_server_io_set_input(server, new_text);
}

static void
on_renderer_output_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter		iter;
	GeoXmlFlowServer *	server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_POINTER, &server,
		-1);
	gtk_list_store_set(ui_flow_io->store, &iter,
		FLOW_IO_OUTPUT, new_text,
		-1);
	geoxml_flow_server_io_set_output(server, new_text);
}

static void
on_renderer_error_edited	(GtkCellRendererText *	renderer,
				 gchar *		path,
				 gchar *		new_text,
				 struct ui_flow_io *	ui_flow_io)
{
	GtkTreeIter		iter;
	GeoXmlFlowServer *	server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
		FLOW_IO_POINTER, &server,
		-1);
	gtk_list_store_set(ui_flow_io->store, &iter,
		FLOW_IO_ERROR, new_text,
		-1);
	geoxml_flow_server_io_set_error(server, new_text);
}

static void
on_button_add_clicked		(GtkButton *		button,
				 struct ui_flow_io *	ui_flow_io)
{
	GeoXmlFlowServer *	server;
	GdkPixbuf *		icon;
	GtkTreeIter		iter;

	gchar *			address;
	const gchar *		input;
	const gchar *		output;
	const gchar *		error;

	// Fetch data
	gtk_combo_box_get_active_iter(
		GTK_COMBO_BOX(ui_flow_io->address), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
		SERVER_STATUS_ICON, &icon,
		SERVER_ADDRESS, &address,
		-1);
	input  = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->input));
	output = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->output));
	error  = gtk_entry_get_text(GTK_ENTRY(ui_flow_io->error));

	// Append data to XML
	server = geoxml_flow_append_server(gebr.flow);
	geoxml_flow_server_set_address(server, address);
	geoxml_flow_server_io_set_input(server, input);
	geoxml_flow_server_io_set_output(server, output);
	geoxml_flow_server_io_set_error(server, error);

	// Append data to Dialog
	gtk_list_store_append(ui_flow_io->store, &iter);
	gtk_list_store_set(ui_flow_io->store, &iter,
		FLOW_IO_ICON, icon,
		FLOW_IO_ADDRESS, address,
		FLOW_IO_INPUT, input,
		FLOW_IO_OUTPUT, output,
		FLOW_IO_ERROR, error,
		-1);
}

static void
on_entry_activate		(GtkEntry *		entry,
				 struct ui_flow_io *	ui_flow_io)
{
	on_button_add_clicked(NULL, ui_flow_io);
}
