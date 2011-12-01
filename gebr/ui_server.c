/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-enhanced-entry.h>

#include "ui_server.h"
#include "gebr.h"

#define RESPONSE_CONNECT_ALL	101
#define RESPONSE_DISCONNECT_ALL	102
#define RESPONSE_CLOSE		103

/*
 * Prototypes
 */

static void
server_list_add(struct ui_server_list *ui_server_list,
		const gchar * address);

//static GtkWidget *_ui_server_create_tag_combo_box (struct ui_server_list *server_list);

static void on_tags_editing_started (GtkCellRenderer *cell,
				     GtkEntry *entry,
				     gchar *path,
				     gpointer data);

static gchar *sort_and_remove_doubles (const gchar *tags_str);

//static GList *ui_server_tags_removed (const gchar *last_tags, const gchar *new_tags);

/*
 * Section: Private
 * Private functions.
 */

/* Function: server_common_tooltip_callback
 * Callback for tree view tooltip
 */
#if 0
static gboolean
server_common_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
			       GtkTreeIter * iter, GtkTreeViewColumn * column, struct ui_server_common *ui)
{
	if (gtk_tree_view_get_column(tree_view, SERVER_STATUS_ICON) == column) {
		GebrServer *server;
		gboolean had_error = TRUE;

		gtk_tree_model_get(GTK_TREE_MODEL(ui->sort_store), iter, SERVER_POINTER, &server, -1);
		if (server->last_error->len) //after connected state
			gtk_tooltip_set_text(tooltip, server->last_error->str);
		else if (server->comm->last_error->len) //before connected state
			gtk_tooltip_set_text(tooltip, server->comm->last_error->str);
		else
			had_error = FALSE;

		return had_error;
	}
	if (gtk_tree_view_get_column(tree_view, SERVER_AUTOCONNECT) == column) {
		gboolean autoconnect;

		gtk_tree_model_get(GTK_TREE_MODEL(ui->sort_store), iter, SERVER_AUTOCONNECT, &autoconnect, -1);
		if (autoconnect)
			gtk_tooltip_set_text(tooltip, _("Autoconnect ON"));
		else
			gtk_tooltip_set_text(tooltip, _("Autoconnect OFF"));

		return TRUE;
	}

	return FALSE;
}
#endif

/* Function; server_common_popup_menu
 * Context menu for server tree view
 */
static GtkMenu *server_common_popup_menu(GtkWidget * widget, struct ui_server_list *sl)
{
	GList *rows;
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sl->common.view));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows)
		return NULL;

	menu = gtk_menu_new ();

	gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			       gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_server, "server_connect")));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			       gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_server, "server_disconnect")));

	// Insert the "Remove" action if:
	//  1. There is at least one server not equal to Local Server
//	for (GList *i = rows; i; i = i->next) {
//		GebrDaemonServer *daemon;
//		GtkTreePath *path = i->data;
//
//		if (!gtk_tree_model_get_iter (model, &iter, path))
//			continue;
//
//		gtk_tree_model_get (model, &iter, 0, &daemon, -1);
//		if (g_strcmp0 (gebr_daemon_server_get_address(daemon), "127.0.0.1") != 0) {
//			gtk_menu_shell_append (GTK_MENU_SHELL (menu),
//					       gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_server, "server_remove")));
//			break;
//		}
//	}
//
//	gtk_menu_shell_append (GTK_MENU_SHELL (menu),
//			       gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_server, "server_stop")));

	gtk_widget_show_all (menu);
	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);

	return GTK_MENU (menu);
}

static void on_tags_edited(GtkCellRendererText *cell,
			   gchar *pathstr,
			   gchar *new_text,
			   struct ui_server_list *sl)
{
	GtkTreeIter iter;
	gboolean ret;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = gebr_maestro_server_get_model(sl->maestro, FALSE, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(gebr.ui_server_list->common.dialog), gebr.accel_group_array[ACCEL_SERVER]);

	ret = gtk_tree_model_get_iter_from_string(model, &iter, pathstr);

	if (!ret)
		return;

	gtk_tree_model_get(model, &iter, 0, &daemon,-1);

	gchar *new_tags = sort_and_remove_doubles(new_text);

	GebrCommHttpMsg *msg;
	gchar *url = g_strdup_printf("/server-tags?server=%s;tags=%s;", gebr_daemon_server_get_address(daemon), new_tags);
	GebrCommServer *server = gebr_maestro_server_get_server(gebr.ui_server_list->maestro);
	msg = gebr_comm_protocol_socket_send_request(server->socket,
	                                             GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
	g_free(new_tags);
	g_object_unref(model);
}

#if 0
static void on_ac_toggled (GtkCellRendererToggle *cell_renderer,
			   gchar *path,
			   struct ui_server_common *ui_server_common)
{
	gboolean ac;
	GebrServer *server;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (ui_server_common->sort_store);

	if (!gtk_tree_model_get_iter_from_string (model, &iter, path))
		return;

	gtk_tree_model_get (model, &iter, SERVER_POINTER, &server, -1);
	ac = gebr_server_get_autoconnect (server);
	gebr_server_set_autoconnect (server, !ac);
}
#endif

static void
daemon_server_address_func(GtkTreeViewColumn *tree_column,
			   GtkCellRenderer *cell,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, iter, 0, &daemon, -1);
	gchar *addr = gebr_daemon_server_get_display_address(daemon);
	g_object_set(cell, "text", addr, NULL);
	g_free(addr);
}

static void
daemon_server_status_func(GtkTreeViewColumn *tree_column,
			  GtkCellRenderer *cell,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, iter, 0, &daemon, -1);
	GebrCommServerState state = gebr_daemon_server_get_state(daemon);
	const gchar *stock_id;

	switch (state) {
	case SERVER_STATE_UNKNOWN:
	case SERVER_STATE_DISCONNECTED:
	case SERVER_STATE_RUN:
	case SERVER_STATE_OPEN_TUNNEL:
	case SERVER_STATE_CONNECTED:
		stock_id = GTK_STOCK_DISCONNECT;
		break;
	case SERVER_STATE_CONNECT:
		stock_id = GTK_STOCK_CONNECT;
		break;
	}

	g_object_set(cell, "stock-id", stock_id, NULL);
}

static void
daemon_server_group_func(GtkTreeViewColumn *tree_column,
                         GtkCellRenderer *cell,
                         GtkTreeModel *model,
                         GtkTreeIter *iter,
                         gpointer data)
{
	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, iter, 0, &daemon, -1);

	if (!daemon)
		return;

	GList *groups = gebr_daemon_server_get_tags(daemon);
	GString *group_list = g_string_new("");

	for (GList *i = groups; i; i = i->next)
		g_string_append_printf(group_list, "%s,", (gchar*)i->data);
	if (group_list->len)
		g_string_erase(group_list, group_list->len-1, 1);

	g_object_set(cell, "text", group_list->str, NULL);
	g_string_free(group_list, TRUE);
}

/* Function: server_common_setup
 * Setup common server dialogs stuff
 */
static void server_common_setup(struct ui_server_common *ui_server_common, struct ui_server_list *sl)
{
	GtkWidget *scrolled_window;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	ui_server_common->add_local_button = NULL;
	gtk_widget_set_size_request(ui_server_common->dialog, 580, 300);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	ui_server_common->widget = scrolled_window;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	view = gtk_tree_view_new();
	ui_server_common->view = view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);

	//gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
	//			    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
						  (GebrGuiGtkPopupCallback) server_common_popup_menu, sl);
	//gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
	//					    (GebrGuiGtkTreeViewTooltipCallback) server_common_tooltip_callback,
	//					    ui_server_common);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Address"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, daemon_server_status_func,
						NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, daemon_server_address_func,
						NULL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Groups"));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, TRUE);
	g_object_set(renderer, "editable", TRUE, NULL);
	gtk_tree_view_column_set_cell_data_func(col, renderer, daemon_server_group_func,
	                                        NULL, NULL);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_tags_editing_started), sl);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_tags_edited), sl);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

}

/*
 * Function: server_common_actions
 * Take the appropriate action when the server dialog emmits
 * a response signal.
 */
static void server_common_actions(GtkDialog * dialog, gint arg1, struct ui_server_list *sl)
{
	switch (arg1) {
	case RESPONSE_CONNECT_ALL:
	{
		GtkTreeIter iter;
		GtkTreeModel *model = gebr_maestro_server_get_model(sl->maestro, FALSE, NULL);
		gebr_gui_gtk_tree_model_foreach(iter, model) {
			GebrDaemonServer *daemon;
			gtk_tree_model_get(model, &iter, 0, &daemon, -1);
			if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_CONNECT)
				continue;
			gebr_connectable_connect(GEBR_CONNECTABLE(sl->maestro), gebr_daemon_server_get_address(daemon));
		}
		g_object_unref(model);
		break;
	}
	case RESPONSE_DISCONNECT_ALL:
	{
		GtkTreeIter iter;
		GtkTreeModel *model = gebr_maestro_server_get_model(sl->maestro, FALSE, NULL);
		gebr_gui_gtk_tree_model_foreach(iter, model) {
			GebrDaemonServer *daemon;
			gtk_tree_model_get(model, &iter, 0, &daemon, -1);
			if(gebr_daemon_server_get_state(daemon) == SERVER_STATE_DISCONNECTED)
				continue;
			gebr_connectable_disconnect(GEBR_CONNECTABLE(sl->maestro), gebr_daemon_server_get_address(daemon));
		}
		g_object_unref(model);
		break;
	}
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		if (dialog != GTK_DIALOG(gebr.ui_server_list->common.dialog))
			break;
	case RESPONSE_CLOSE:	/* Only for server list */
		gtk_widget_hide(gebr.ui_server_list->common.dialog);
		gtk_window_remove_accel_group(GTK_WINDOW(gebr.ui_server_list->common.dialog), gebr.accel_group_array[ACCEL_SERVER]);
		gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);
		gebr_config_save(FALSE);
		break;
	default:
		break;
	}
}

/**
 * server_list_add:
 * @ui_server_list: Pointer to user interface server list
 * @address: The new server address
 *
 * Callback to add a server to the server list
 */
static void
server_list_add(struct ui_server_list *ui_server_list,
		const gchar * address)
{
	gchar *url = g_strdup_printf("/server?address=%s;pass=", address);
	GebrCommServer *server = gebr_maestro_server_get_server(ui_server_list->maestro);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (gebr.ui_project_line->servers_filter));
}

/* Function: on_add_clicked
 * Call for entry activation
 */
static void on_entry_activate(GtkEntry * entry, struct ui_server_list *ui_server_list)
{
	gchar *data = g_strdup (gtk_entry_get_text(entry));
	g_strstrip (data);

	if (!g_strcmp0 ( _("Type here [user@]serverhostname"), data) || strlen (data) == 0) {
		g_free(data);
		return;
	}

	server_list_add(ui_server_list, gebr_gui_enhanced_entry_get_text(GEBR_GUI_ENHANCED_ENTRY(entry)));
	gtk_entry_set_text(entry, "");
	g_free(data);
}

/* Function: on_add_clicked
 * Call for add button
 */
static void on_add_clicked(GtkButton * button, struct ui_server_list *ui_server_list)
{
	GtkEntry *entry;

	g_object_get(button, "user-data", &entry, NULL);
	gchar *data = g_strdup (gtk_entry_get_text(entry));
	g_strstrip (data);

	if (!g_strcmp0 ( _("Type here [user@]serverhostname"), data) || strlen (data) == 0) {
		g_free(data);
		return;
	}

	on_entry_activate(entry, ui_server_list);
	g_free(data);
}

/*
 * Section: Public
 * Public functions.
 */

static void
on_job_define(GebrMaestroServer *maestro,
	      GebrJob *job,
	      struct ui_server_list *sl)
{
	gebr_job_control_add(gebr.job_control, job);
}


static void
ui_server_update_groups_model(struct ui_server_list *sl)
{
	GtkTreeIter iter;
	gchar *display;
	gchar *title;

	gtk_list_store_clear(sl->group_store);

	void traverse_func(gpointer value, gpointer data)
	{
		display = gebr_maestro_server_get_display_address(sl->maestro);
		// Comment for translators: [group] at [Maestro]
		title = g_strdup_printf(_("%s at %s"), (gchar*)value, display);
		g_free(display);
		gtk_list_store_append(sl->group_store, &iter);
		gtk_list_store_set(sl->group_store, &iter,
				   GEBR_UI_SERVER_GROUP_TITLE, title,
				   GEBR_UI_SERVER_GROUP_MAESTRO, sl->maestro,
				   GEBR_UI_SERVER_GROUP_GROUP, value,
				   -1);
	}

	GList *groups = gebr_maestro_server_get_all_tags(sl->maestro);
	display = gebr_maestro_server_get_display_address(sl->maestro);
	gtk_list_store_append(sl->group_store, &iter);
	gtk_list_store_set(sl->group_store, &iter,
			   GEBR_UI_SERVER_GROUP_TITLE, display,
			   GEBR_UI_SERVER_GROUP_MAESTRO, sl->maestro,
			   GEBR_UI_SERVER_GROUP_GROUP, "",
			   -1);
	g_free(display);
	g_list_foreach(groups, traverse_func, NULL);
}

static void
on_server_group_changed(GebrMaestroServer *maestro,
			struct ui_server_list *sl)
{
	ui_server_update_groups_model(sl);
}

static const gchar *
on_password_request(GebrMaestroServer *maestro,
		    const gchar *address)
{
	gdk_threads_enter();
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Enter password"),
							GTK_WINDOW(gebr.window),
							(GtkDialogFlags)(GTK_DIALOG_MODAL |
									 GTK_DIALOG_DESTROY_WITH_PARENT),
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
							GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gchar *message = g_strdup_printf(_("Server %s is asking for password. Enter it below."), address);
	GtkWidget *label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	gtk_widget_show_all(dialog);
	gboolean confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;
	gchar *password = confirmed ? g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))) : NULL;

	gtk_widget_destroy(dialog);
	gdk_threads_leave();
	return password;
}

void
gebr_ui_server_list_connect(struct ui_server_list *sl,
			    const gchar *addr)
{
	if (sl->maestro)
		g_object_unref(sl->maestro);

	sl->maestro = gebr_maestro_server_new(addr);

	g_signal_connect(sl->maestro, "job-define",
			 G_CALLBACK(on_job_define), sl);
	g_signal_connect(sl->maestro, "group-changed",
			 G_CALLBACK(on_server_group_changed), sl);
	g_signal_connect(sl->maestro, "password-request",
			 G_CALLBACK(on_password_request), sl);

	gebr_maestro_server_connect(sl->maestro);

	GtkTreeModel *model = gebr_maestro_server_get_model(sl->maestro, FALSE, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(sl->common.view), model);
	g_string_assign(gebr.config.maestro_address, addr);
	g_object_unref(model);
}

static void
connect_to_maestro(GtkEntry *entry,
		   struct ui_server_list *sl)
{
	gebr_ui_server_list_connect(sl, gtk_entry_get_text(entry));
}

/*
 * Function: server_list_setup_ui
 * Assembly the servers configurations dialog.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_server_list *server_list_setup_ui(void)
{
	struct ui_server_list *ui_server_list;

	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *entry;
	GtkWidget *frame;
	GtkWidget *align;
	GtkWidget *label;

	ui_server_list = g_new0(struct ui_server_list, 1);

	dialog = gtk_dialog_new_with_buttons(_("Servers configuration"),
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 800, -1);

	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Co_nnect all"), RESPONSE_CONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
			     gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Disconnect all"), RESPONSE_DISCONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
			     gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, RESPONSE_CLOSE);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	ui_server_list->common.dialog = dialog;
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response", G_CALLBACK(server_common_actions), ui_server_list);

	server_common_setup(&ui_server_list->common, ui_server_list);

	ui_server_list->group_store = gtk_list_store_new(GEBR_UI_SERVER_GROUP_N,
							 G_TYPE_STRING,
							 GEBR_TYPE_MAESTRO_SERVER,
							 G_TYPE_STRING);

	GtkWidget *maestro_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(maestro_entry), gebr.config.maestro_address->str);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), maestro_entry, TRUE, TRUE, 0);
	g_signal_connect(maestro_entry, "activate", G_CALLBACK(connect_to_maestro), ui_server_list);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->common.widget, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);

	entry = gebr_gui_enhanced_entry_new_with_empty_text(_("Type here [user@]serverhostname"));
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), ui_server_list);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(on_add_clicked), ui_server_list);
	g_object_set(button, "user-data", entry, NULL);

	ui_server_list->common.combo_store = gtk_list_store_new(4,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN);

	align = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 10, 0);

	frame = gtk_frame_new (_("<b>Filter by group</b>"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (frame), align);
	label = gtk_frame_get_label_widget (GTK_FRAME (frame));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

	return ui_server_list;
}

/* Function: server_list_show
 * Show _ui_server_list_ taking appropriate actions
 */
void server_list_show(struct ui_server_list *ui_server_list)
{
	gtk_widget_show_all(ui_server_list->common.dialog);
}

#if 0
gchar **
ui_server_list_tag (GebrServer *server)
{
	gchar *tags;
	gchar **tag_list;
	GtkTreeModel *model;

	if (server == NULL)
		return NULL;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gtk_tree_model_get(model, &server->iter, SERVER_TAGS, &tags, -1);
	tag_list = g_strsplit(tags, ",", 0);
	g_free (tags);

	return tag_list;
}
#endif

#if 0

GList *
ui_server_servers_with_tag (const gchar *tag)
{
	GList *list = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gebr_gui_gtk_tree_model_foreach (iter, model) {
		GebrServer *server;
		gchar *tags;
		gchar **tag_list;
		gboolean is_auto_choose;

		gtk_tree_model_get(model, &iter, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		gtk_tree_model_get(model, &iter,
				   SERVER_POINTER, &server,
				   SERVER_TAGS, &tags,
				   -1);

		tag_list = g_strsplit (tags, ",", 0);	
		for (gint i = 0; tag_list[i]; i++) {
			if (g_str_equal (tag_list[i], tag)) {
				list = g_list_prepend (list, server);
				break;
			}
		}
		g_free (tags);
		g_strfreev (tag_list);
	}

	return g_list_reverse (list);
}

gboolean
ui_server_has_tag (GebrServer *server,
		   const gchar *tag)
{
	gchar *tags;
	gchar **tag_list;
	GtkTreeModel *model;
	gboolean retval = FALSE;

	g_return_val_if_fail(tag != NULL, FALSE);
	g_return_val_if_fail(g_strcmp0(tag, "") != 0, FALSE);
	g_return_val_if_fail(server != NULL, FALSE);

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gtk_tree_model_get (model, &server->iter, SERVER_TAGS, &tags, -1);
	if (!tags)
		return FALSE;

	tag_list = g_strsplit (tags, ",", 0);
	for (gint i = 0; !retval && tag_list[i]; i++)
		if (g_str_equal (tag_list[i], tag))
			retval = TRUE;

	g_strfreev (tag_list);
	g_free (tags);

	return retval;
}
#endif

/*
 * Sorts @tags_str and removes duplicate tags.
 * Free the returned value with g_free().
 */
static gchar *sort_and_remove_doubles (const gchar *tags_str)
{
	gchar **tagsv;
	GString *tags;
	GPtrArray *array;

	tagsv = g_strsplit (tags_str, ",", 0);
	array = g_ptr_array_new ();
	tags  = g_string_new ("");

	for (int i = 0; tagsv[i]; i++) {
		gchar *tag = g_strstrip (tagsv[i]);
		if (strlen (tag) > 0)
			g_ptr_array_add (array, tag);
	}

	int sort (gchar **v1, gchar **v2) {
		return g_strcmp0 (*v1, *v2);
	}

	tags = g_string_new ("");

	// Sort the array so we can remove duplicates
	g_ptr_array_sort (array, (GCompareFunc) sort);

	for (int i = 0; i < (gint)array->len - 1; i++) {
		const gchar *tag1 = g_ptr_array_index (array, i);
		const gchar *tag2 = g_ptr_array_index (array, i+1);
		if (!g_str_equal (tag1, tag2))
			g_string_append_printf (tags, "%s,", tag1);
	}

	if (array->len)
		g_string_append (tags, g_ptr_array_index (array, array->len-1));

	g_strfreev (tagsv);
	g_ptr_array_free (array, FALSE);
	return g_string_free (tags, FALSE);
}

#if 0
void
ui_server_set_tags (GebrServer *server,
		    const gchar *str)
{
	g_return_if_fail(server != NULL);
	g_return_if_fail(str != NULL);

	gtk_list_store_set (gebr.ui_server_list->common.store, &server->iter,
			    SERVER_TAGS, str,
			    -1);
	ui_server_update_tags_combobox ();
}

gchar **
ui_server_get_all_tags (void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GString *concat;
	gchar *tags;
	gchar **retval;

	concat = g_string_new ("");
	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gebr_gui_gtk_tree_model_foreach (iter, model) {
		gboolean is_auto_choose;
		gtk_tree_model_get (model, &iter, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		gtk_tree_model_get (model, &iter, SERVER_TAGS, &tags, -1);
		g_string_append_printf (concat, "%s,", tags);
		g_free (tags);
	}

	tags = sort_and_remove_doubles (concat->str);
	retval =  g_strsplit (tags, ",", 0);

	g_free (tags);
	g_string_free (concat, TRUE);

	return retval;
}
#endif

#if 0

gchar **
ui_server_get_all_fsid (void)
{
	gchar *fsid;
	gchar *sorted;
	gchar **retval;
	GString *list;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);
	list = g_string_new ("");

	gebr_gui_gtk_tree_model_foreach (iter, model) {
		gboolean is_auto_choose = FALSE;

		gtk_tree_model_get (model, &iter, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		gtk_tree_model_get (model, &iter, SERVER_FS, &fsid, -1);
		if (fsid)
			g_string_append_printf (list, "%s,", fsid);
		g_free (fsid);
	}

	sorted = sort_and_remove_doubles (list->str);
	retval = g_strsplit (sorted, ",", 0);

	g_free (sorted);
	g_string_free (list, TRUE);

	return retval;
}
#endif

#if 0

/**
 * tag_is_heterogeneous:
 *
 * @tag: The tag to be checked
 *
 * Check if a server tag is heterogeneous. When a tag is
 * heterogeneous it means that this tag is applied to servers
 * in different filesystems.
 */
static gboolean
tag_is_heterogeneous (const gchar *tag)
{
	gchar *fs1, *fs2;
	GList *servers = NULL;
	GList *list = NULL;
	GebrServer *svr1, *srv2;
	GtkTreeModel *model;
	gboolean retval = FALSE;

	g_return_val_if_fail(tag != NULL, FALSE);
	g_return_val_if_fail(g_strcmp0(tag, "") != 0, FALSE);

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);
	list = servers = ui_server_servers_with_tag (tag);
	svr1 = servers->data;

	servers = servers->next;
	if (!servers){
		return retval;
	}
	srv2 = servers->data;

	while (servers) {
		if (!gebr_comm_server_is_logged (svr1->comm)){
			servers = servers->next;
			if (!servers)
				break;

			svr1 = servers->data;
			continue;
		}

		if (!gebr_comm_server_is_logged (srv2->comm)){
			servers = servers->next;
			if (!servers)
				break;

			srv2 = servers->data;
			continue;
		}

		gtk_tree_model_get (model, &svr1->iter, SERVER_FS, &fs1, -1);
		gtk_tree_model_get (model, &srv2->iter, SERVER_FS, &fs2, -1);

		if (g_strcmp0 (fs1, fs2) != 0) {
			retval = TRUE;
			g_free (fs1);
			g_free (fs2);
			break;
		}
		g_free (fs1);
		g_free (fs2);
		servers = servers->next;
		if (!servers)
			break;
		srv2 = servers->data;
	}

	g_list_free (list);

	return retval;
}
#endif

#if 0

void ui_server_update_tags_combobox (void)
{
	int active = 0, len = 0;
	gchar **tags;
	GtkTreeIter iter;
	GtkComboBox *combo;
	GtkTreeModel *model;
	gchar *selected = NULL;

	combo = GTK_COMBO_BOX (gebr.ui_server_list->common.combo);
	model = GTK_TREE_MODEL (gebr.ui_server_list->common.combo_store);

	if (gtk_combo_box_get_active_iter (combo, &iter))
		gtk_tree_model_get (model, &iter, 0, &selected, -1);

	gtk_list_store_clear (gebr.ui_server_list->common.combo_store);

	gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
	gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
			    TAG_NAME, _("All Servers"),
			    TAG_SEP, FALSE,
			    TAG_FS, FALSE,
			    -1);

	gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
	gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
			    TAG_SEP, TRUE,
			    TAG_FS, FALSE,
			    -1);

	tags = ui_server_get_all_tags ();
	for (int i = 0; tags[i]; i++) {
		gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
		gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
				    TAG_NAME, tags[i],
				    TAG_SEP, FALSE,
				    TAG_FS, FALSE,
				    -1);

		if (tag_is_heterogeneous (tags[i]))
			gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
					    TAG_ICON, GTK_STOCK_DIALOG_WARNING,
					    -1);

		if (active == 0 && selected && g_str_equal (tags[i], selected))
			active = i + 2;
		len++;
	}
	g_strfreev (tags);

	gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
	gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
			    TAG_SEP, TRUE,
			    TAG_FS, FALSE,
			    -1);

	tags = ui_server_get_all_fsid ();
	for (int i = 0; tags[i]; i++) {
		gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
		gtk_list_store_set (gebr.ui_server_list->common.combo_store, &iter,
				    TAG_NAME, tags[i],
				    TAG_SEP, FALSE,
				    TAG_FS, TRUE,
				    -1);

		if (active == 0 && selected && g_str_equal (tags[i], selected))
			active = len + i + 3;
	}
	g_strfreev (tags);

	gtk_combo_box_set_active (combo, active);
}
#endif

#if 0

static GtkWidget *_ui_server_create_tag_combo_box (struct ui_server_list *server_list)
{
	GtkWidget *widget;
	GtkComboBox *combo;
	GtkCellLayout *layout;
	GtkCellRenderer *renderer;

	if (!server_list)
		server_list = gebr.ui_server_list;

	widget = gtk_combo_box_new_with_model (
			GTK_TREE_MODEL(server_list->common.combo_store));
	combo = GTK_COMBO_BOX (widget);
	layout = GTK_CELL_LAYOUT(combo);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(layout, renderer, TRUE);
	gtk_cell_layout_add_attribute(layout, renderer, "text", TAG_NAME);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(layout, renderer, FALSE);
	gtk_cell_layout_add_attribute(layout, renderer, "stock-id", TAG_ICON);

	return widget;
}

GtkWidget *ui_server_create_tag_combo_box (void)
{
	return _ui_server_create_tag_combo_box (NULL);
}
#endif


static void on_tags_editing_started (GtkCellRenderer *cell,
				     GtkEntry *entry,
				     gchar *path,
				     gpointer data)
{
	g_debug("Editing Started");
        gtk_window_remove_accel_group(GTK_WINDOW(gebr.ui_server_list->common.dialog), gebr.accel_group_array[ACCEL_SERVER]);

	gtk_entry_set_icon_from_stock (entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_HELP);
	gtk_entry_set_icon_activatable (entry, GTK_ENTRY_ICON_SECONDARY, FALSE);
	gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY,
					 _("Enter group names separated by comma and press ENTER to confirm."));
}

#if 0

static GList *ui_server_tags_removed (const gchar *last_tags, const gchar *new_tags)
{
	GList *list = NULL;
	gchar **tag_list_last;
	gchar **tag_list_new;
	gboolean removed;

	tag_list_last = g_strsplit (last_tags, ",", 0);	
	tag_list_new = g_strsplit (new_tags, ",", 0);	

	for (gint i = 0; tag_list_last[i]; i++){
		removed = TRUE;

		for (gint j = 0; tag_list_new[j]; j++)
			if (g_strcmp0(tag_list_last[i], tag_list_new[j]) == 0)
				removed = FALSE;

		if (removed)
			list = g_list_append (list, tag_list_last[i]);

	}

	g_strfreev (tag_list_new);

	return list;
}
#endif

#if 0

gboolean ui_server_ask_for_tags_remove_permission (void){
	GtkTreeIter iter;
	gboolean result = FALSE;
	gchar *tags;
	GString *removed_tags = g_string_new(NULL);
	gchar **removed_tagsv;
	GString *all_tags = g_string_new(NULL);
	gchar **all_tagsv;
	GList *all = NULL;


	/* Build the list with all tags not sorted nor exclusive tags*/
	gebr_gui_gtk_tree_model_foreach (iter, GTK_TREE_MODEL (gebr.ui_server_list->common.store)) {
		gboolean is_auto_choose = FALSE;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
		   				   SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_server_list->common.store), &iter,
				    SERVER_TAGS, &tags,
				    -1);
		g_string_append_printf (all_tags, "%s,", tags);
		g_free (tags);
	}

	all_tagsv = g_strsplit (all_tags->str, ",", 0);

	for (gint i=0; all_tagsv[i]; i++){
		all = g_list_append(all, all_tagsv[i]);
	}

	/* Build the list with all tags REMOVED not sorted nor exclusive tags*/
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_server_list->common.view) {

		GebrServer *server;
		gboolean is_auto_choose = FALSE;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.sort_store), &iter,
		   				   SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		gtk_tree_model_get (gebr.ui_server_list->common.sort_store, &iter,
				    SERVER_POINTER, &server,
				    SERVER_TAGS, &tags,
				    -1);
		if (server && g_strcmp0 (server->comm->address->str, "127.0.0.1") == 0){
			continue;
		}

		g_string_append_printf (removed_tags, "%s,", tags);
		g_free (tags);

	}

	removed_tagsv = g_strsplit (removed_tags->str, ",", 0);
	for (int i = 0; removed_tagsv[i]; i++) {
		GList *remove = g_list_find_custom (all, removed_tagsv[i], (GCompareFunc)g_strcmp0);
		all = g_list_delete_link (all, remove);
	}

	GString *new_tags = g_string_new(NULL);
	while (all) {
		g_string_append_printf (new_tags, "%s,", (gchar *) all->data);
		all = all->next;
	}

	gchar *new_tags_sorted_no_doubles = sort_and_remove_doubles (new_tags->str);
	gchar *all_tags_sorted_no_doubles = sort_and_remove_doubles (all_tags->str);

	if (g_strcmp0(all_tags_sorted_no_doubles, new_tags_sorted_no_doubles) != 0){

		/* Make the difference to discover the list of removed tags*/
		GList *removed = ui_server_tags_removed(all_tags_sorted_no_doubles, new_tags_sorted_no_doubles);
		GtkWidget *dialog;
		GtkWidget *text_view;
		GtkTextBuffer *text_buffer;
		GtkWidget *label;
		GtkWidget *message_label;
		GtkWidget *table = gtk_table_new(3, 2, FALSE);
		GtkWidget * image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
		guint row = 0;
		char *markup = g_markup_printf_escaped("<span font_weight='bold' size='large'>%s</span>", _("Confirm group deletion"));
		GString *message = g_string_new(_("The following documents are about to be deleted.\nThis operation can't be undone! Are you sure?\n"));
		gboolean empty_tag = FALSE;
		GtkWidget *scrolled_window;
		gboolean ret;

		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(scrolled_window, 400, 200);

		text_buffer = gtk_text_buffer_new(NULL);
		text_view = gtk_text_view_new_with_buffer(text_buffer);
		gtk_text_view_set_editable(GTK_TEXT_VIEW (text_view), FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (text_view), FALSE);

		while (removed){
			GString * buf = g_string_new((char *)removed->data);
			g_string_append(buf, "\n");
			empty_tag = TRUE;
			gtk_text_buffer_insert_at_cursor(text_buffer, buf->str, buf->len);
			g_string_free(buf,TRUE);
			removed = removed->next;
		}

		if (empty_tag){
			label = gtk_label_new(NULL);
			message_label = gtk_label_new(message->str);
			dialog = gtk_dialog_new_with_buttons(_("Confirm group deletion"), NULL,
							     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							     GTK_STOCK_YES,
							     GTK_RESPONSE_YES ,
							     GTK_STOCK_NO,
							     GTK_RESPONSE_NO,
							     NULL);
			gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
			gtk_label_set_markup (GTK_LABEL (label), markup);
			g_free (markup);



			gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);
			gtk_table_attach(GTK_TABLE(table), image, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
					 (GtkAttachOptions)GTK_FILL, 3, 3);
			gtk_table_attach(GTK_TABLE(table), label, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
					 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

			gtk_table_attach(GTK_TABLE(table), message_label, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
					 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

			gtk_table_attach(GTK_TABLE(table), scrolled_window, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
					 (GtkAttachOptions)GTK_EXPAND
					 , 3, 3), row++;

			gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
			ret = gtk_dialog_run(GTK_DIALOG(dialog));
			result = (ret == GTK_RESPONSE_YES) ? TRUE : FALSE;

			gtk_widget_destroy(dialog);
		}
		g_string_free(message, TRUE);
		g_list_free(removed);
	}
	else{
		result = TRUE;
	}
	g_strfreev(removed_tagsv);
	g_strfreev(all_tagsv);
	g_list_free(all);
	g_free(new_tags_sorted_no_doubles);
	g_free(all_tags_sorted_no_doubles);
	g_string_free(new_tags, TRUE);
	g_string_free(all_tags, TRUE);
	g_string_free(removed_tags, TRUE);
	return result;
}
#endif

#if 0

gchar *
gebr_get_groups_of_server(gchar *server_address)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar * tags;
	gchar *name;

	model = GTK_TREE_MODEL(gebr.ui_server_list->common.store);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
			   SERVER_NAME, &name,
			   SERVER_TAGS, &tags,
			   -1);
		if (!g_strcmp0(server_address, name))
			return tags;
	}
	return NULL;
}
#endif

GtkListStore *
gebr_ui_server_list_get_autochoose_store(struct ui_server_list *sl)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sl->common.store), &iter))
		g_return_val_if_reached(NULL);

	GebrServerAutochoose *autochoose;
	gtk_tree_model_get(GTK_TREE_MODEL(sl->common.store), &iter, SERVER_POINTER, &autochoose, -1);
	return autochoose->queues;
}

GtkTreeModel *
gebr_ui_server_list_get_groups_model(struct ui_server_list *sl)
{
	return GTK_TREE_MODEL(sl->group_store);
}
