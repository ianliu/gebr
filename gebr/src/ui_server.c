/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team(http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_server.c
 * Server related UI.
 *
 * Assembly servers list configuration dialog and
 * server select dialog for flow execution (TODO).
 */

#include <string.h>

#include "ui_server.h"
#include "gebr.h"
#include "support.h"

#define GEBR_SERVER_CLOSE	101
#define GEBR_SERVER_REMOVE	102
#define GEBR_SERVER_REFRESH	103

/*
 * Prototypes
 */

static void
server_list_add(GtkEntry * entry, struct ui_server_list * ui_server_list);

static void
server_select_cursor_changed(GtkTreeView * tree_view, struct ui_server_select * ui_server_select);

static void
server_select_row_changed(GtkTreeModel * tree_model, GtkTreePath * path, GtkTreeIter * iter,
			  struct ui_server_select * ui_server_select);

/*
 * Section: Private
 * Private functions.
 */

static GtkWidget *
server_create_view(GtkListStore * store)
{
	GtkWidget *		view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", SERVER_STATUS_ICON);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Address"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_ADDRESS);

	return view;
}

/*
 * Function: server_dialog_actions
 * Take the appropriate action when the server dialog emmits
 * a response signal.
 */
static void
server_actions(GtkDialog * dialog, gint arg1)
{
	switch (arg1) {
	case GEBR_SERVER_REFRESH: {
		GtkTreeIter	iter;
		gboolean	valid;

		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
		while (valid) {
			struct server *	server;

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
					SERVER_POINTER, &server,
					-1);

			if (server_is_logged(server) == FALSE)
				server_connect(server);

			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
		}

		break;
	}
	case GEBR_SERVER_CLOSE: /* For server list */
		gtk_widget_hide(gebr.ui_server_list->dialog);
		gebr_config_save();
		break;
	case GEBR_SERVER_REMOVE: { /* For server list */
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreeIter		iter;

		struct server *		server;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_server_list->view));
		if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
			return;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_POINTER, &server,
				-1);
		server_free(server);

		gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_server_list->store), &iter);
		break;
	} default:
		break;
	}
}

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: server_list_setup_ui
 * Assembly the servers configurations dialog.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_server_list *
server_list_setup_ui(void)
{
	struct ui_server_list *		ui_server_list;

	GtkWidget *			dialog;
	GtkWidget *			label;
	GtkWidget *			entry;

	/* alloc */
	ui_server_list = g_malloc(sizeof(struct ui_server_list));

	ui_server_list->store = gtk_list_store_new(SERVER_N_COLUMN,
					GDK_TYPE_PIXBUF,
					G_TYPE_STRING,
					G_TYPE_POINTER);

	dialog = gtk_dialog_new_with_buttons(_("Servers configuration"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_REMOVE, GEBR_SERVER_REMOVE,
						GTK_STOCK_CLOSE, GEBR_SERVER_CLOSE,
						GTK_STOCK_REFRESH, GEBR_SERVER_REFRESH,
						NULL);
	ui_server_list->dialog = dialog;
	gtk_widget_set_size_request(dialog, 380, 300);
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response",
		G_CALLBACK(server_actions), NULL);

	label = gtk_label_new(_("Remote server hostname:"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(entry), "activate",
			GTK_SIGNAL_FUNC(server_list_add), ui_server_list);

	ui_server_list->view = server_create_view(ui_server_list->store);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->view, TRUE, TRUE, 0);

	return ui_server_list;
}

/*
 * Function: server_list_updated_status
 * Update status of _server_ in store
 *
 */
void
server_list_updated_status(struct server * server)
{
	GdkPixbuf *	status_icon;
// 	GtkTreePath *	path;

	status_icon = server->protocol->logged == TRUE ?
		gebr.pixmaps.stock_connect : gebr.pixmaps.stock_disconnect,
	gtk_list_store_set(gebr.ui_server_list->store, &server->iter,
		SERVER_STATUS_ICON, status_icon,
		-1);

// 	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_server_list->store), &server->iter);
	g_signal_emit_by_name(gebr.ui_server_list->store, "row-changed", NULL, &server->iter);
// 	gtk_tree_path_free(path);
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: server_list_add
 * Callback to add a server to the server list
 */
static void
server_list_add(GtkEntry * entry, struct ui_server_list * ui_server_list)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* check if it is already in list */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter,
				SERVER_POINTER, &server,
				-1);

		if (!g_ascii_strcasecmp(server->address->str, gtk_entry_get_text(entry))) {
			GtkTreeSelection *	selection;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_server_list->view));
			gtk_tree_selection_select_iter(selection, &iter);

			return;
		}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->store), &iter);
	}

	/* add to store (creating a new server structure) and ... */
	server_new(gtk_entry_get_text(entry));

	/* reset entry. */
	gtk_entry_set_text(entry, "");
}

/*
 * Function: server_select_setup_ui
 * Select a server to run a flow.
 * The server is build using the store at gebr.ui_server_list.store
 *
 */
struct server *
server_select_setup_ui(void)
{
	struct ui_server_select *	ui_server_select;

	GtkTreeSelection *		selection;
	GtkTreeIter			first_iter;

	gboolean			has_first;
	struct server *			server;

	/* initializations */
	ui_server_select = g_malloc(sizeof(struct ui_server_select));
	server = NULL;

	has_first = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->store), &first_iter);
	if (!has_first) {
		gebr_message(ERROR, TRUE, FALSE,
			_("There is no servers available. Please add at least one at Configure/Server"));
		goto out;
	}
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_server_list->store), NULL) == 1) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->store), &first_iter,
			SERVER_POINTER, &server,
			-1);
		goto out;
	}

	ui_server_select->dialog = gtk_dialog_new_with_buttons(_("Select server"),
							GTK_WINDOW(gebr.window),
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							NULL);
	gtk_widget_set_size_request(ui_server_select->dialog, 380, 300);
	ui_server_select->ok_button = gtk_dialog_add_button(GTK_DIALOG(ui_server_select->dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_buttons(GTK_DIALOG(ui_server_select->dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_REFRESH, GEBR_SERVER_REFRESH,
		NULL);
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(ui_server_select->dialog, "response",
		G_CALLBACK(server_actions), NULL);

	ui_server_select->store = gebr.ui_server_list->store;
	g_signal_connect(G_OBJECT(ui_server_select->store), "row-changed",
		GTK_SIGNAL_FUNC(server_select_row_changed), ui_server_select);

	ui_server_select->view = server_create_view(ui_server_select->store);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_server_select->dialog)->vbox), ui_server_select->view, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_server_select->view), "cursor-changed",
		GTK_SIGNAL_FUNC(server_select_cursor_changed), ui_server_select);

	/* select the first server */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_select->view));
	gtk_tree_selection_select_iter(selection, &first_iter);
	g_signal_emit_by_name(ui_server_select->view, "cursor-changed");

	gtk_widget_show_all(ui_server_select->dialog);
	while (1) {
		switch (gtk_dialog_run(GTK_DIALOG(ui_server_select->dialog))) {
		case GTK_RESPONSE_OK:
			server = ui_server_select->selected;
			break;
		case GTK_RESPONSE_CANCEL:
			server = NULL;
			break;
		default:
			continue;
		}
		break;
	}

	/* frees */
	gtk_widget_destroy(ui_server_select->dialog);
	g_free(ui_server_select);

out:	return server;
}

/*
 * Function: server_select_cursor_changed
 * Enable OK button if we are logged into server.
 *
 */
static void
server_select_cursor_changed(GtkTreeView * tree_view, struct ui_server_select * ui_server_select)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	struct server *		server;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_select->view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(ui_server_select->store), &iter,
			SERVER_POINTER, &server,
			-1);

	if (server->protocol->logged == FALSE)
		g_object_set(ui_server_select->ok_button, "sensitive", FALSE, NULL);
	else {
		g_object_set(ui_server_select->ok_button, "sensitive", TRUE, NULL);
		ui_server_select->selected = server;
	}
}

static void
server_select_row_changed(GtkTreeModel * tree_model, GtkTreePath * path, GtkTreeIter * iter,
			  struct ui_server_select * ui_server_select)
{
	/* A server may change its status and we should
	   enable/disable OK */
	server_select_cursor_changed(GTK_TREE_VIEW(ui_server_select->view), ui_server_select);
}
