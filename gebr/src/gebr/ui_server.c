/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>
#include <gui/gtkenhancedentry.h>

#define RESPONSE_CONNECT_ALL	101
#define RESPONSE_DISCONNECT_ALL	102
#define RESPONSE_CLOSE		103

/*
 * Prototypes
 */

static void
server_list_add(struct ui_server_list * ui_server_list, const gchar * address);

static void
server_select_cursor_changed(GtkTreeView * tree_view, struct ui_server_select * ui_server_select);

static void
server_select_row_changed(GtkTreeModel * tree_model, GtkTreePath * path, GtkTreeIter * iter,
	struct ui_server_select * ui_server_select);

/*
 * Section: Private
 * Private functions.
 */

/* Function: server_common_tooltip_callback
 * Callback for tree view tooltip
 */
#if GTK_CHECK_VERSION(2,12,0)
static gboolean
server_common_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
	GtkTreeIter * iter, GtkTreeViewColumn * column, struct ui_server_common * ui)
{
	if (gtk_tree_view_column_get_sort_column_id(column) == SERVER_STATUS_ICON) {
		struct server *		server;

		gtk_tree_model_get(GTK_TREE_MODEL(ui->store), iter, SERVER_POINTER, &server, -1);
		if (server->last_error->len)
			gtk_tooltip_set_text(tooltip, server->last_error->str);

		return (gboolean)server->last_error->len;
	}

	return FALSE;
}
#endif

/* Function: server_common_connect
 * Callback for popup menu action
 */
static void
server_common_connect(GtkMenuItem * menu_item, struct server * server)
{
	comm_server_connect(server->comm);
}

/* Function: server_common_disconnect
 * Callback for popup menu action
 */
static void
server_common_disconnect(GtkMenuItem * menu_item, struct server * server)
{
	comm_server_disconnect(server->comm);
}

/* Function: server_common_autoconnect
 * Callback for popup menu action
 */
static void
server_common_autoconnect_changed(GtkMenuItem * menu_item, struct server * server)
{
	gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter,
		SERVER_AUTOCONNECT, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)),
		-1);
}

/* Function: server_common_remove
 * Callback for popup menu action
 */
static void
server_common_remove(GtkMenuItem * menu_item, struct server * server)
{
	server_free(server);
}

/* Function; server_common_popup_menu
 * Context menu for server tree view
 */
static GtkMenu *
server_common_popup_menu(GtkWidget * widget, struct ui_server_common * ui_server_common)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		menu;
	GtkWidget *		menu_item;

	struct server *		server;
	gboolean		autoconnect;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_common->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	gtk_tree_model_get(model, &iter,
		SERVER_POINTER, &server,
		SERVER_AUTOCONNECT, &autoconnect,
		-1);
	menu = gtk_menu_new();

	/* connect */
	if (server->comm->state == SERVER_STATE_DISCONNECTED) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)server_common_connect, server);
	}
	/* disconnect */
	if (server->comm->state != SERVER_STATE_DISCONNECTED) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DISCONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)server_common_disconnect, server);
	}
	/* autoconnect */
	menu_item = gtk_check_menu_item_new_with_label(_("Autoconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled",
		(GCallback)server_common_autoconnect_changed, server);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), autoconnect);
	/* remove */
	if (!comm_server_is_local(server->comm)) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)server_common_remove, server);
	}

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/* Function: server_common_setup
 * Setup common server dialogs stuff
 */
static void
server_common_setup(struct ui_server_common * ui_server_common)
{
	GtkWidget *		view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	ui_server_common->add_local_button = NULL;
	gtk_widget_set_size_request(ui_server_common->dialog, 580, 300);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_server_common->store));
	ui_server_common->view = view;
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
		(GtkPopupCallback)server_common_popup_menu, ui_server_common);
#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
		(GtkTreeViewTooltipCallback)server_common_tooltip_callback, ui_server_common);
#endif

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_STATUS_ICON);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", SERVER_STATUS_ICON);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Address"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_ADDRESS);

	renderer = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new_with_attributes(_("Autoconnect"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_AUTOCONNECT);
	gtk_tree_view_column_set_sort_indicator(col, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "active", SERVER_AUTOCONNECT);
}

/*
 * Function: server_common_actions
 * Take the appropriate action when the server dialog emmits
 * a response signal.
 */
static void
server_common_actions(GtkDialog * dialog, gint arg1, struct ui_server_common * ui_server_common)
{
	switch (arg1) {
	case RESPONSE_CONNECT_ALL: {
		GtkTreeIter	iter;
		gboolean	valid;

		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ui_server_common->store), &iter);
		while (valid) {
			struct server *	server;

			gtk_tree_model_get(GTK_TREE_MODEL(ui_server_common->store), &iter,
				SERVER_POINTER, &server,
				-1);

			if (comm_server_is_logged(server->comm) == FALSE)
				comm_server_connect(server->comm);

			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ui_server_common->store), &iter);
		}

		break;
	} case RESPONSE_DISCONNECT_ALL: {
		GtkTreeIter	iter;
		gboolean	valid;

		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ui_server_common->store), &iter);
		while (valid) {
			struct server *	server;

			gtk_tree_model_get(GTK_TREE_MODEL(ui_server_common->store), &iter,
				SERVER_POINTER, &server,
				-1);

			if (comm_server_is_logged(server->comm) == TRUE)
				comm_server_disconnect(server->comm);

			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ui_server_common->store), &iter);
		}

		break;
	} case GTK_RESPONSE_DELETE_EVENT: case GTK_RESPONSE_CANCEL:
		if (dialog != GTK_DIALOG(gebr.ui_server_list->common.dialog))
			break;
	case RESPONSE_CLOSE: /* Only for server list */
		gtk_widget_hide(gebr.ui_server_list->common.dialog);
		gebr_config_save(FALSE);
		break;
	default:
		break;
	}
}

/*
 * Function: server_list_add
 * Callback to add a server to the server list
 */
static void
server_list_add(struct ui_server_list * ui_server_list, const gchar * address)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* check if it is already in list */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ui_server_list->common.store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_server_list->common.store), &iter,
			SERVER_POINTER, &server,
			-1);

		if (!strcmp(server->comm->address->str, address)) {
			GtkTreeSelection *	selection;

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_list->common.view));
			gtk_tree_selection_select_iter(selection, &iter);

			return;
		}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ui_server_list->common.store), &iter);
	}

	/* finally */
	server_new(address, TRUE);
}

/* Function: on_add_clicked
 * Call for entry activation
 */
static void
on_entry_activate(GtkEntry * entry, struct ui_server_list * ui_server_list)
{
	server_list_add(ui_server_list, gtk_enhanced_entry_get_text(GTK_ENHANCED_ENTRY(entry)));
	gtk_entry_set_text(entry, "");
}

/* Function: on_add_clicked
 * Call for add button
 */
static void
on_add_clicked(GtkButton * button, struct ui_server_list * ui_server_list)
{
	GtkEntry *	entry;

	g_object_get(button, "user-data", &entry, NULL);
	on_entry_activate(entry, ui_server_list);
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

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_select->common.view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(ui_server_select->common.store), &iter,
		SERVER_POINTER, &server,
		-1);

	if (server->comm->protocol->logged == FALSE)
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
	server_select_cursor_changed(GTK_TREE_VIEW(ui_server_select->common.view), ui_server_select);
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
	GtkWidget *			button;
	GtkWidget *			hbox;
	GtkWidget *			entry;

	ui_server_list = g_malloc(sizeof(struct ui_server_list));
	ui_server_list->common.store = gtk_list_store_new(SERVER_N_COLUMN,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER,
		G_TYPE_BOOLEAN);

	dialog = gtk_dialog_new_with_buttons(_("Servers configuration"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, RESPONSE_CLOSE,
		NULL);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Connect all"), RESPONSE_CONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
		gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Disconnect all"), RESPONSE_DISCONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
		gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(dialog, "delete-event",
		G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	ui_server_list->common.dialog = dialog;
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response",
		G_CALLBACK(server_common_actions), &ui_server_list->common);

	server_common_setup(&ui_server_list->common);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->common.view, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);

	entry = gtk_enhanced_entry_new_with_empty_text(_("Hostname"));
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(entry), "activate",
		GTK_SIGNAL_FUNC(on_entry_activate), ui_server_list);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(on_add_clicked), ui_server_list);


	return ui_server_list;
}

/* Function: server_list_show
 * Show _ui_server_list_ taking appropriate actions
 */
void
server_list_show(struct ui_server_list * ui_server_list)
{
	gtk_widget_show_all(ui_server_list->common.dialog);
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

	status_icon = (server->last_error->len)
		? gebr.pixmaps.stock_warning
		: (server->comm->protocol->logged == TRUE)
			? gebr.pixmaps.stock_connect : gebr.pixmaps.stock_disconnect;

	gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter,
		SERVER_STATUS_ICON, status_icon,
		-1);

	/* update view */
	g_signal_emit_by_name(gebr.ui_server_list->common.store, "row-changed", NULL, &server->iter);
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
	GtkTreeIter			iter;
	gboolean			valid;
	gulong				handler_id;

	GtkTreeIter			first_connected_iter;
	guint				connected;
	struct server *			server;

	/* initializations */
	server = NULL;
	connected = 0;
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
				SERVER_POINTER, &server,
				-1);
		if (comm_server_is_logged(server->comm) == TRUE)
			if (connected++ == 0)
				first_connected_iter = iter;

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	}
	if (connected == 0) {
		gebr_message(LOG_ERROR, TRUE, FALSE,
			_("There are no running servers available. Please configure them in Configure/Server"));
		goto out;
	}
	if (connected == 1) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &first_connected_iter,
			SERVER_POINTER, &server,
			-1);
		goto out;
	}

	ui_server_select = g_malloc(sizeof(struct ui_server_select));
	ui_server_select->common.dialog = gtk_dialog_new_with_buttons(_("Select server"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		NULL);
	ui_server_select->ok_button = gtk_dialog_add_button(GTK_DIALOG(ui_server_select->common.dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_buttons(GTK_DIALOG(ui_server_select->common.dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(ui_server_select->common.dialog, "response",
		G_CALLBACK(server_common_actions), &ui_server_select->common);

	ui_server_select->common.store = gebr.ui_server_list->common.store;
	handler_id = g_signal_connect(G_OBJECT(ui_server_select->common.store), "row-changed",
		GTK_SIGNAL_FUNC(server_select_row_changed), ui_server_select);

	server_common_setup(&ui_server_select->common);
	gtk_tree_view_column_set_visible(
		gtk_tree_view_get_column(GTK_TREE_VIEW(ui_server_select->common.view), SERVER_AUTOCONNECT), FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_server_select->common.dialog)->vbox), ui_server_select->common.view, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_server_select->common.view), "cursor-changed",
		GTK_SIGNAL_FUNC(server_select_cursor_changed), ui_server_select);

	/* select the first running server */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_select->common.view));
	gtk_tree_selection_select_iter(selection, &first_connected_iter);
	g_signal_emit_by_name(ui_server_select->common.view, "cursor-changed");

	gtk_widget_show_all(ui_server_select->common.dialog);
	while (1) {
		switch (gtk_dialog_run(GTK_DIALOG(ui_server_select->common.dialog))) {
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
	g_signal_handler_disconnect(G_OBJECT(ui_server_select->common.store), handler_id);
	gtk_widget_destroy(ui_server_select->common.dialog);
	g_free(ui_server_select);

out:	return server;
}
