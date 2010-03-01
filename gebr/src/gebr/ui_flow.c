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

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/gui/gtkfileentry.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/icons.h>

#include "ui_flow.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_server.h"
#include "ui_moab.h"

#define GEBR_FLOW_UI_RESPONSE_EXECUTE 1

static void flow_io_populate(struct ui_flow_io *ui_flow_io);

static gboolean flow_io_actions(gint response, struct ui_flow_io *ui_flow_io);
static void flow_io_run(GebrGeoXmlFlowServer * server);

static void flow_io_insert(struct ui_flow_io *ui_flow_io, GebrGeoXmlFlowServer * flow_server, GtkTreeIter * iter);

static void
on_renderer_edited(GtkCellRendererText * renderer, gchar * path, gchar * new_text, struct ui_flow_io *ui_flow_io);

static void
on_renderer_combo_edited(GtkCellRendererText * renderer, gchar * path, gchar * new_text, struct ui_flow_io *ui_flow_io);

static void
on_renderer_editing_started(GtkCellRenderer * renderer,
			    GtkCellEditable * editable, gchar * path, struct ui_flow_io *ui_flow_io);

static void
on_renderer_entry_icon_release(GtkEntry * widget,
			       GtkEntryIconPosition position, GdkEvent * event, struct ui_flow_io *ui_flow_io);

static GtkMenu *on_menu_popup(GtkTreeView * treeview, struct ui_flow_io *data);

static void on_delete_server_io_activate(GtkWidget * menu_item, struct ui_flow_io *ui_flow_io);

static void on_tree_view_cursor_changed(GtkTreeView * treeview, struct ui_flow_io *ui_flow_io);

#if GTK_CHECK_VERSION(2,12,0)
static gboolean
on_tree_view_tooltip(GtkTreeView * treeview,
		     gint x, gint y, gboolean keyboard_mode, GtkTooltip * tooltip, struct ui_flow_io *ui_flow_io);
#endif

void flow_io_setup_ui(gboolean executable)
{
	struct ui_flow_io *ui_flow_io;

	GtkWidget *dialog;
	GtkWidget *treeview;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	GtkTreeIter server_iter;
	GebrGeoXmlFlowServer *flow_server;
	struct server *server;

	gint response;

	ui_flow_io = g_malloc(sizeof(struct ui_flow_io));
	store = gtk_list_store_new(FLOW_IO_N,
				   GDK_TYPE_PIXBUF,	// Icon
				   G_TYPE_STRING,	// Address
				   G_TYPE_STRING,	// Input
				   G_TYPE_STRING,	// Output
				   G_TYPE_STRING,	// Error
				   G_TYPE_BOOLEAN,	// server listed
				   G_TYPE_POINTER,	// GebrGeoXmlFlowServer
				   G_TYPE_POINTER,	// struct server
				   G_TYPE_BOOLEAN,	// Is new row? Server column
				   G_TYPE_BOOLEAN);	// Is new row? Other columns
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	dialog = gtk_dialog_new_with_buttons(_("Server and I/O setup"),
					     GTK_WINDOW(gebr.window),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	ui_flow_io->execute_button = !executable ? NULL :
		gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_EXECUTE, GEBR_FLOW_UI_RESPONSE_EXECUTE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_BROWSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 2);
	g_object_set(G_OBJECT(treeview), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT(treeview), "query-tooltip", G_CALLBACK(on_tree_view_tooltip), ui_flow_io);
	g_signal_connect(G_OBJECT(treeview), "cursor-changed", G_CALLBACK(on_tree_view_cursor_changed), ui_flow_io);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(treeview),
						  (GebrGuiGtkPopupCallback) on_menu_popup, ui_flow_io);

	//---------------------------------------
	// Setting up the GtkTreeView
	//---------------------------------------

	// Server column
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", FLOW_IO_ICON);
	renderer = gtk_cell_renderer_combo_new();
	g_object_set(renderer,
		     "model", gebr.ui_server_list->common.store, "has-entry", FALSE, "text-column", SERVER_NAME, NULL);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_renderer_combo_edited), ui_flow_io);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	g_object_set(column, "resizable", TRUE, NULL);
	gtk_tree_view_column_set_attributes(column, renderer,
					    "text", FLOW_IO_SERVER_NAME, "editable", FLOW_IO_IS_SERVER_ADD, NULL);
	gtk_tree_view_column_set_title(column, _("Server"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(FLOW_IO_IS_SERVER_ADD));

	// Input column
	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(renderer, "edited", G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Input"), renderer, "text", FLOW_IO_INPUT,
							  "editable", FLOW_IO_IS_SERVER_ADD2, NULL);
	g_object_set(column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(FLOW_IO_INPUT));

	// Output column
	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(renderer, "edited", G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Output"), renderer, "text", FLOW_IO_OUTPUT,
							  "editable", FLOW_IO_IS_SERVER_ADD2, NULL);
	g_object_set(column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(FLOW_IO_OUTPUT));

	// Error column
	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(renderer, "edited", G_CALLBACK(on_renderer_edited), ui_flow_io);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_renderer_editing_started), ui_flow_io);
	column = gtk_tree_view_column_new_with_attributes(_("Error"), renderer, "text", FLOW_IO_ERROR,
							  "editable", FLOW_IO_IS_SERVER_ADD2, NULL);
	g_object_set(column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(FLOW_IO_ERROR));

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), treeview, TRUE, TRUE, 0);

	ui_flow_io->dialog = dialog;
	ui_flow_io->store = store;
	ui_flow_io->treeview = treeview;
	flow_io_populate(ui_flow_io);

	gtk_widget_show_all(dialog);
	do
		response = gtk_dialog_run(GTK_DIALOG(dialog));
	while (!flow_io_actions(response, ui_flow_io));

	if (flow_io_get_selected(ui_flow_io, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
				   FLOW_IO_FLOW_SERVER_POINTER, &flow_server, FLOW_IO_SERVER_POINTER, &server, -1);
		/* move selected server/IO to the first server/IO of the server */
		gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(flow_server), NULL);
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
		/* select current server on flow edition */
		server_find(server, &server_iter);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &server_iter);
	} else
		gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), 0);
	flow_edition_on_server_changed();

	// Clean up
	gtk_widget_destroy(dialog);
	g_free(ui_flow_io);
}

gboolean flow_io_get_selected(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter)
{
	return gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_io->treeview)), NULL,
					       iter);
}

void flow_io_select_iter(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_flow_io->treeview), iter);
}

void flow_io_customized_paths_from_line(GtkFileChooser * chooser)
{
	GError *error;
	GebrGeoXmlSequence *path_sequence;

	if (gebr.line == NULL)
		return;

	error = NULL;
	gebr_geoxml_line_get_path(gebr.line, &path_sequence, 0);
	if (path_sequence != NULL) {
		gtk_file_chooser_set_current_folder(chooser,
						    gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE
										   (path_sequence)));

		do {
			gtk_file_chooser_add_shortcut_folder(chooser,
							     gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE
											    (path_sequence)), &error);
			gebr_geoxml_sequence_next(&path_sequence);
		} while (path_sequence != NULL);
	}
}

void flow_io_set_server(GtkTreeIter * server_iter, const gchar * input, const gchar * output, const gchar * error)
{
	struct server *server;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), server_iter, SERVER_POINTER, &server, -1);

	gebr_geoxml_flow_server_set_address(gebr.flow_server, server->comm->address->str);
	gebr_geoxml_flow_server_io_set_input(gebr.flow_server, input);
	gebr_geoxml_flow_server_io_set_output(gebr.flow_server, output);
	gebr_geoxml_flow_server_io_set_error(gebr.flow_server, error);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

void flow_io_simple_setup_ui(gboolean focus_output)
{
	struct ui_flow_simple *simple;
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *label;
	gchar *tmp;

	GtkTreeIter server_iter;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &server_iter);

	simple = (struct ui_flow_simple *)g_malloc(sizeof(struct ui_flow_simple));
	simple->focus_output = focus_output;

	dialog = gtk_dialog_new_with_buttons(_("Flow input/output"),
					     GTK_WINDOW(gebr.window),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	simple->dialog = dialog;

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Input */
	label = gtk_label_new(_("Input file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->input = gebr_gui_gtk_file_entry_new((GebrGuiGtkFileEntryCustomize) flow_io_customized_paths_from_line,
						    NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->input, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	if ((tmp = (gchar *) gebr_geoxml_flow_server_io_get_input(gebr.flow_server)) != NULL)
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(simple->input), tmp);
	gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(simple->input), FALSE);

	/* Output */
	label = gtk_label_new(_("Output file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->output = gebr_gui_gtk_file_entry_new((GebrGuiGtkFileEntryCustomize) flow_io_customized_paths_from_line,
						     NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->output, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	if ((tmp = (gchar *) gebr_geoxml_flow_server_io_get_output(gebr.flow_server)) != NULL)
		gebr_gui_gtk_file_entry_set_path(GEBR_GUI_GTK_FILE_ENTRY(simple->output), tmp);
	gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GEBR_GUI_GTK_FILE_ENTRY(simple->output), FALSE);

	/* Error */
	label = gtk_label_new(_("Error file"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	simple->error = gebr_gui_gtk_file_entry_new((GebrGuiGtkFileEntryCustomize) flow_io_customized_paths_from_line,
						    NULL);
	gtk_widget_set_size_request(simple->input, 140, 30);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), simple->error, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	if ((tmp = (gchar *) gebr_geoxml_flow_server_io_get_error(gebr.flow_server)) != NULL)
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

void flow_fast_run()
{
	flow_io_run(gebr.flow_server);
}

void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program, gboolean select_last)
{
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		GtkTreeIter iter;
		const gchar *icon;

		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));

		gtk_list_store_insert_before(gebr.ui_flow_edition->fseq_store,
					     &iter, &gebr.ui_flow_edition->output_iter);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
				   FSEQ_TITLE_COLUMN, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)),
				   FSEQ_ICON_COLUMN, icon,
				   FSEQ_GEBR_GEOXML_POINTER, program, -1);

		if (select_last)
			flow_edition_select_component_iter(&iter);
	}
}

/**
 * \internal
 * Inserts a new entry in Server/IO model, respecting the special new row position.
 * @param ui_flow_io The structure containing relevant data.
 * @param flow_server The server/IO configuration to be added.
 * @param iter An emtpy iterator that will point to the newly inserted row.
 */
static void flow_io_insert(struct ui_flow_io *ui_flow_io, GebrGeoXmlFlowServer * flow_server, GtkTreeIter * iter)
{
	gchar *name;
	const gchar *address;
	const gchar *input;
	const gchar *output;
	const gchar *error;

	GdkPixbuf *icon;
	struct server *server;
	gboolean server_found;
	GtkTreeIter server_iter;
	GtkTreeIter iter_;

	address = gebr_geoxml_flow_server_get_address(GEBR_GEOXML_FLOW_SERVER(flow_server));
	input = gebr_geoxml_flow_server_io_get_input(GEBR_GEOXML_FLOW_SERVER(flow_server));
	output = gebr_geoxml_flow_server_io_get_output(GEBR_GEOXML_FLOW_SERVER(flow_server));
	error = gebr_geoxml_flow_server_io_get_error(GEBR_GEOXML_FLOW_SERVER(flow_server));

	server_found = server_find_address(address, &server_iter);
	if (server_found)
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &server_iter,
				   SERVER_STATUS_ICON, &icon, SERVER_NAME, &name, SERVER_POINTER, &server, -1);
	else {
		icon = gebr.pixmaps.stock_warning;
		name = g_strdup(address);
		server = NULL;
	}
	gtk_list_store_insert_before(ui_flow_io->store, &iter_, &ui_flow_io->row_new_server);
	gtk_list_store_set(ui_flow_io->store, &iter_,
			   FLOW_IO_ICON, icon,
			   FLOW_IO_SERVER_NAME, name,
			   FLOW_IO_INPUT, input,
			   FLOW_IO_OUTPUT, output,
			   FLOW_IO_ERROR, error,
			   FLOW_IO_SERVER_LISTED, server_found,
			   FLOW_IO_FLOW_SERVER_POINTER, flow_server,
			   FLOW_IO_SERVER_POINTER, server,
			   FLOW_IO_IS_SERVER_ADD, FALSE,
			   FLOW_IO_IS_SERVER_ADD2, TRUE,
			   -1);

	if (iter)
		*iter = iter_;
	g_free(name);
}

/**
 * \internal
 * Fills the #GtkListStore of @ui_flow_io with data.
 * @param ui_flow_io The structure to be filled with data.
 */
static void flow_io_populate(struct ui_flow_io *ui_flow_io)
{
	GebrGeoXmlSequence *flow_server;
	GtkTreeIter iter;

	gtk_list_store_append(ui_flow_io->store, &ui_flow_io->row_new_server);
	gtk_list_store_set(ui_flow_io->store, &ui_flow_io->row_new_server,
			   FLOW_IO_SERVER_NAME, _("New"),
			   FLOW_IO_IS_SERVER_ADD, TRUE,
			   FLOW_IO_IS_SERVER_ADD2, FALSE,
			   -1);

	gebr_geoxml_flow_get_server(gebr.flow, &flow_server, 0);
	for (; flow_server != NULL; gebr_geoxml_sequence_next(&flow_server))
		flow_io_insert(ui_flow_io, GEBR_GEOXML_FLOW_SERVER(flow_server), NULL);

	/* select first, which is the last edited */
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(ui_flow_io->store)) {
		gboolean sensitive;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter, FLOW_IO_SERVER_LISTED, &sensitive, -1);

		if (sensitive) {
			flow_io_select_iter(ui_flow_io, &iter);
			break;
		}
	}
}

/**
 * \internal
 * Actions for Flow IO files edition dialog.
 *
 * @param response The response id sent by the dialog.
 * @param ui_flow_io The structure representing the servers io dialog.
 * @return #TRUE if the dialog should be destroyed, #FALSE otherwise.
 */
static gboolean flow_io_actions(gint response, struct ui_flow_io *ui_flow_io)
{
	GtkTreeIter iter;
	GdkPixbuf *icon;
	GebrGeoXmlFlowServer *flow_server;

	if (response == GEBR_FLOW_UI_RESPONSE_EXECUTE) {
		if (!flow_io_get_selected(ui_flow_io, &iter))
			return TRUE;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter,
				   FLOW_IO_ICON, &icon, FLOW_IO_FLOW_SERVER_POINTER, &flow_server, -1);

		if (icon != gebr.pixmaps.stock_connect) {
			GtkWidget *dialog;
			dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR,
							GTK_BUTTONS_CLOSE,
							_("This server is disconnected. "
							  "To use this server, you must " "connect it first."));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return FALSE;
		}

		flow_io_run(flow_server);
	}

	return TRUE;
}

/**
 * \internal
 *
 * Check for current server and if its connected, for the queue selected.
 */
static void flow_io_run(GebrGeoXmlFlowServer * flow_server)
{
	GtkTreeIter iter;
	const gchar *address;
	struct server *server;
	struct gebr_comm_server_run * config;

	if (!flow_browse_get_selected(NULL, FALSE)) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No flow selected."));
		return;
	}

	/* initialization */
	config = g_new(struct gebr_comm_server_run, 1);
	config->account = config->class = NULL;

	/* find iter */
	address = gebr_geoxml_flow_server_get_address(flow_server);
	if (!server_find_address(address, &iter)) {
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, "Server should be present on list!");
		goto err;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter, SERVER_POINTER, &server, -1);
	
	/* check connection */
	if (!gebr_comm_server_is_logged(server->comm)) {
		if (gebr_comm_server_is_local(server->comm))
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to the local server."), server->comm->address->str);
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to server '%s'."), server->comm->address->str);
		goto err;
	}

	/* get queue information */
	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (!moab_setup_ui(&config->account, server))
			goto err;
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &iter))
			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, 1, &config->class, -1);
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("No available queue for server '%s'."), server->comm->address->str);
	} else {
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox)) == 0)
			config->class = g_strdup("");
		else { 
			gchar *tmp;
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &iter);
			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, 1, &tmp, -1);
			if (tmp[0] == 'j') {
				GtkDialog *dialog;
				GtkWidget *widget;
				gchar *queue_name;

				dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(_("Name queue"),
								     GTK_WINDOW(gebr.window),
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								     GTK_STOCK_OK, GTK_RESPONSE_OK,
								     NULL));
				widget = gtk_label_new(_("Give a name to the queue:"));
				gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), widget, TRUE, TRUE, 0);
				widget = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), widget, TRUE, TRUE, 0);
				
				gtk_widget_show_all(GTK_WIDGET(dialog));
				gboolean is_valid = FALSE;
				do {
					if (gtk_dialog_run(dialog) != GTK_RESPONSE_OK) {
						gtk_widget_destroy(GTK_WIDGET(dialog));
						g_free(tmp);
						goto err;
					}

					queue_name = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget));	
					if (!strlen(queue_name))
						gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
									_("Empty name"), _("Please type a queue name."));
					else {
						gchar *prefixed_queue_name = g_strdup_printf("q%s", queue_name);

						if (server_queue_find(server, prefixed_queue_name, NULL))
							gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
										_("Duplicate name"), _("This queue name is already in use. Please give another one."));
						else
							is_valid = TRUE;

						g_free(prefixed_queue_name);
					}
				} while (!is_valid);
				config->class = g_strdup_printf("q%s", queue_name);

				gtk_list_store_set(server->queues_model, &iter, 1, config->class, -1);
				/* send server request to rename this queue */
				gebr_comm_protocol_send_data(server->comm->protocol, server->comm->stream_socket,
							     gebr_comm_protocol_defs.rnq_def, 2, tmp, config->class);

				gtk_widget_destroy(GTK_WIDGET(dialog));
				g_free(tmp);
			} else
				config->class = tmp;
		}
	}

	gebr_geoxml_flow_io_set_from_server(gebr.flow, flow_server);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	flow_run(server, config);

	/* frees */
err:	g_free(config->account);
	g_free(config->class);
	g_free(config);
}

/**
 * \internal
 * Updates model information based on \p renderer's "column" data and \p new_text.
 * @param renderer Must have a "column" data (set with g_object_set_data) representing the edited column.
 * @param path A string path pointing to the edited row.
 * @param new_text The updated data.
 * @param ui_flow_io Structure containing relevant data.
 */
static void
on_renderer_edited(GtkCellRendererText * renderer, gchar * path, gchar * new_text, struct ui_flow_io *ui_flow_io)
{
	gchar *row_new_path;
	gint column;
	GtkTreeIter iter;
	GebrGeoXmlFlowServer *flow_server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	row_new_path =
	    gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(ui_flow_io->store), &ui_flow_io->row_new_server);
	if (path != NULL && !strcmp(path, row_new_path)) {
		g_free(row_new_path);
		return;
	}
	g_free(row_new_path);

	column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "column"));

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter, FLOW_IO_FLOW_SERVER_POINTER, &flow_server, -1);
	gtk_list_store_set(ui_flow_io->store, &iter, column, new_text, -1);
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

/**
 * \internal
 * Called upon click on the special row to create a new Server/IO configuration.
 */
static void
on_renderer_combo_edited(GtkCellRendererText * renderer, gchar * path, gchar * new_text, struct ui_flow_io *ui_flow_io)
{
	gchar *name;
	gchar *address;
	gboolean has_server;
	GtkTreeIter iter;
	struct server *server;
	GebrGeoXmlFlowServer *flow_server;

	has_server = FALSE;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(gebr.ui_server_list->common.store)) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
				   SERVER_POINTER, &server, SERVER_NAME, &name, -1);
		if (!strcmp(name, new_text)) {
			has_server = TRUE;
			break;
		}
	}

	address = has_server ? server->comm->address->str : new_text;

	flow_server = gebr_geoxml_flow_append_server(gebr.flow);
	gebr_geoxml_flow_server_set_address(flow_server, address);
	flow_io_insert(ui_flow_io, flow_server, &iter);

	// Focus cell
	GtkTreePath *focus_path;
	GtkTreeViewColumn *focus_column;
	GtkCellRenderer *focus_cell;
	GList *cell_list;

	focus_path = gtk_tree_model_get_path(GTK_TREE_MODEL(ui_flow_io->store), &iter);
	focus_column = gtk_tree_view_get_column(GTK_TREE_VIEW(ui_flow_io->treeview), 1);
	cell_list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(focus_column));
	focus_cell = cell_list->data;
	g_list_free(cell_list);
	gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(ui_flow_io->treeview),
					 focus_path, focus_column, focus_cell, TRUE);
}

/**
 * \internal
 * Adds a clickable icon in the cell, when user starts editing it, to fire a #GtkDirectoryChooser.
 */
static void
on_renderer_editing_started(GtkCellRenderer * renderer,
			    GtkCellEditable * editable, gchar * path, struct ui_flow_io *ui_flow_io)
{
	GtkTreeIter iter;
	gint column;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "column"));

	if (column == FLOW_IO_IS_SERVER_ADD) {
		GtkCellLayout *layout;
		GtkCellRenderer *cell;
		layout = GTK_CELL_LAYOUT(editable);
		cell = gtk_cell_renderer_pixbuf_new();
		gtk_cell_layout_pack_start(layout, cell, FALSE);
		gtk_cell_layout_add_attribute(layout, cell, "pixbuf", SERVER_STATUS_ICON);
		gtk_cell_layout_reorder(layout, cell, 0);
		return;
	}

	g_object_set_data(G_OBJECT(editable), "column", GINT_TO_POINTER(column));
	g_object_set_data(G_OBJECT(editable), "renderer", (gpointer) renderer);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(editable), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIRECTORY);
	g_signal_connect(editable, "icon-release", G_CALLBACK(on_renderer_entry_icon_release), ui_flow_io);
}

/**
 * \internal
 * Fires a #GtkDirectoryChooser upon a click on icon.
 */
static void
on_renderer_entry_icon_release(GtkEntry * widget,
			       GtkEntryIconPosition position, GdkEvent * event, struct ui_flow_io *ui_flow_io)
{
	GtkWidget *dialog;
	gint response;
	gchar *filename;
	gint column;
	GebrGeoXmlSequence *path;

	if (!flow_io_get_selected(ui_flow_io, NULL))
		return;

	column = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "column"));
	dialog = gtk_file_chooser_dialog_new(_("Choose a file"),
					     GTK_WINDOW(gebr.window),
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN, GTK_RESPONSE_APPLY, NULL);

	gebr_geoxml_line_get_path(gebr.line, &path, 0);

	if (path) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
						    gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(path)));
	}

	while (path) {
		const gchar *path_str;
		path_str = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(path));
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), path_str, NULL);
		gebr_geoxml_sequence_next(&path);
	}

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response != GTK_RESPONSE_APPLY)
		goto out;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	on_renderer_edited(GTK_CELL_RENDERER_TEXT(g_object_get_data(G_OBJECT(widget), "renderer")),
			   NULL, filename, ui_flow_io);
	g_free(filename);

 out:	gtk_widget_destroy(dialog);
}

/**
 * \internal
 * Deletes an entry from the model.
 */
static void on_delete_server_io_activate(GtkWidget * menu_item, struct ui_flow_io *ui_flow_io)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *server;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter, FLOW_IO_FLOW_SERVER_POINTER, &server, -1);

	gebr_geoxml_sequence_remove(server);
	if (gtk_list_store_remove(ui_flow_io->store, &iter))
		gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_flow_io->treeview), &iter);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

/**
 * \internal
 * Pops a context menu to perform actions on it, such as delete.
 */
static GtkMenu *on_menu_popup(GtkTreeView * treeview, struct ui_flow_io *ui_flow_io)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkTreeIter iter;

	if (!flow_io_get_selected(ui_flow_io, &iter)
	    || gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(ui_flow_io->store), &ui_flow_io->row_new_server, &iter))
		return NULL;

	menu = gtk_menu_new();
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(on_delete_server_io_activate), ui_flow_io);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Updates state of the execute button based on connectivity of selected server.
 */
static void on_tree_view_cursor_changed(GtkTreeView * treeview, struct ui_flow_io *ui_flow_io)
{
	GtkTreeIter iter;
	gboolean server_listed;

	if (!flow_io_get_selected(ui_flow_io, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_io->store), &iter, FLOW_IO_SERVER_LISTED, &server_listed, -1);
	if (ui_flow_io->execute_button != NULL)
		gtk_widget_set_sensitive(ui_flow_io->execute_button, server_listed);
}

/**
 * \internal
 * Shows tooltips for each cell in Server/IO tree view.
 */
static gboolean
on_tree_view_tooltip(GtkTreeView * treeview,
		     gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltip, struct ui_flow_io *ui_flow_io)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	gchar *text;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip, &model, NULL, &iter))
		return FALSE;

	if (keyboard_tip)
		gtk_tree_view_convert_widget_to_bin_window_coords(treeview, x, y, &x, &y);

	if (!gtk_tree_view_get_path_at_pos(treeview, x, y, NULL, &column, NULL, NULL)) {
		return FALSE;
	}

	if (gebr_gui_gtk_tree_model_iter_equal_to(model, &ui_flow_io->row_new_server, &iter)) {
		gtk_tooltip_set_text(tooltip, _("Creates a new Input/Output configuration."));
		return TRUE;
	}

	if (gtk_tree_view_get_column(treeview, 0) == column) {
		GdkPixbuf *icon;
		gtk_tree_model_get(model, &iter, FLOW_IO_ICON, &icon, -1);
		if (icon == gebr.pixmaps.stock_warning)
			gtk_tooltip_set_text(tooltip, _("This server no longer exists in the servers list dialog."));
		else if (icon == gebr.pixmaps.stock_disconnect)
			gtk_tooltip_set_text(tooltip, _("This server is disconnected."));
		else
			return FALSE;
	} else if (gtk_tree_view_get_column(treeview, 1) == column) {
		gtk_tree_model_get(model, &iter, FLOW_IO_INPUT, &text, -1);
		if (text && !strlen(text)) {
			g_free(text);
			return FALSE;
		}
		gtk_tooltip_set_text(tooltip, text);
		g_free(text);
	} else if (gtk_tree_view_get_column(treeview, 2) == column) {
		gtk_tree_model_get(model, &iter, FLOW_IO_OUTPUT, &text, -1);
		if (text && !strlen(text)) {
			g_free(text);
			return FALSE;
		}
		gtk_tooltip_set_text(tooltip, text);
		g_free(text);
	} else if (gtk_tree_view_get_column(treeview, 3) == column) {
		gtk_tree_model_get(model, &iter, FLOW_IO_ERROR, &text, -1);
		if (text && !strlen(text)) {
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
