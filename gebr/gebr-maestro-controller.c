/*
 * gebr-maestro-controller.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-maestro-controller.h"
#include <config.h>

#include <glib/gi18n.h>
#include "gebr-maestro-server.h"
#include "gebr-marshal.h"
#include <libgebr/gui/gebr-gui-utils.h>

struct _GebrMaestroControllerPriv {
	GList *maestros;
	GtkBuilder *builder;
	GtkListStore *model;
};

enum {
	MAESTRO_CONTROLLER_DAEMON,
	MAESTRO_CONTROLLER_ADDR,
	MAESTRO_CONTROLLER_EDITABLE,
	N_COLUMN
};

enum {
	JOB_DEFINE,
	LAST_SIGNAL
};

G_DEFINE_TYPE(GebrMaestroController, gebr_maestro_controller, G_TYPE_OBJECT);

static guint signals[LAST_SIGNAL] = { 0, };

static GtkTargetEntry entries[] = {
	{"POINTER", GTK_TARGET_SAME_APP, 0},
};
static guint n_entries = G_N_ELEMENTS(entries);

static void connect_to_maestro(GtkEntry *entry, GebrMaestroController *self);

static void on_state_change(GebrMaestroServer *maestro, GebrMaestroController *self);

static void
insert_new_entry(GebrMaestroController *mc)
{
	GtkTreeIter iter;
	gtk_list_store_append(mc->priv->model, &iter);
	gtk_list_store_set(mc->priv->model, &iter,
	                   MAESTRO_CONTROLLER_DAEMON, NULL,
	                   MAESTRO_CONTROLLER_ADDR, _("New"),
	                   MAESTRO_CONTROLLER_EDITABLE, TRUE,
	                   -1);
}

static void
drag_data_received_handl(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
			 GtkSelectionData *selection_data, guint target_type, guint time,
			 gpointer data)
{
	gpointer *daemon;

        gboolean dnd_success = FALSE;
        gboolean delete_selection_data = FALSE;

        if (selection_data && (selection_data->length >= 0)) {
		daemon = (gpointer *)selection_data->data;
		dnd_success = TRUE;
        }

        if (!dnd_success)
		g_warn_if_reached();

	g_debug("Got Address %p: %s", *daemon, gebr_daemon_server_get_address(*daemon));

        gtk_drag_finish(context, dnd_success, delete_selection_data, time);
	g_signal_stop_emission_by_name(widget, "drag-data-received");
}

static gboolean
drag_drop_handl (GtkWidget *widget,
		 GdkDragContext *context,
		 gint x, gint y,
		 guint time,
		 gpointer user_data)
{
        gboolean is_valid_drop_site;
        GdkAtom target_type;

        is_valid_drop_site = TRUE;

        if (context->targets) {
                target_type = GDK_POINTER_TO_ATOM(g_list_nth_data(context->targets, 0));
                gtk_drag_get_data(widget, context, target_type, time);
        } else
                is_valid_drop_site = FALSE;

        return  is_valid_drop_site;
}

static void
drag_data_get_handl(GtkWidget *widget,
		    GdkDragContext *context,
		    GtkSelectionData *selection_data,
		    guint target_type,
		    guint time,
		    gpointer user_data)
{
	GtkTreeView *tv = GTK_TREE_VIEW(widget);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tv);
	GtkTreeModel *model;
	GtkTreeIter iter;

	gpointer daemon;
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(model, &iter, 0, &daemon, -1);

	g_debug("Sending %p: %s", daemon, gebr_daemon_server_get_address(daemon));

	gtk_selection_data_set(selection_data, selection_data->target,
			       sizeof(gpointer) * 8, (guchar*)&daemon, sizeof(gpointer));
}

static void
set_widget_drag_dest(GtkWidget *widget)
{
        g_signal_connect(widget, "drag-data-received",
			 G_CALLBACK(drag_data_received_handl), NULL);
        g_signal_connect(widget, "drag-drop",
			 G_CALLBACK(drag_drop_handl), NULL);

	gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_MOTION
			  | GTK_DEST_DEFAULT_HIGHLIGHT,
			  entries, n_entries, GDK_ACTION_COPY);
}

static void
notebook_group_show_address(GtkTreeViewColumn *tree_column,
			    GtkCellRenderer *cell,
			    GtkTreeModel *tree_model,
			    GtkTreeIter *iter,
			    gpointer data)
{
	GebrDaemonServer *daemon;
	gtk_tree_model_get(tree_model, iter, 0, &daemon, -1);

	if (!daemon)
		return;

	g_object_set(cell, "text", gebr_daemon_server_get_display_address(daemon), NULL);
}

static GtkTreeModel *
copy_model_for_groups(GtkTreeModel *orig_model)
{
	GtkTreeIter iter, new_iter;
	GtkListStore *new_model;
	GebrDaemonServer *daemon;

	new_model = gtk_list_store_new(2,
                                       G_TYPE_OBJECT,
                                       G_TYPE_STRING);

	gebr_gui_gtk_tree_model_foreach(iter, orig_model) {
		gtk_tree_model_get(orig_model, &iter, 0, &daemon, -1);

		gtk_list_store_append(new_model, &new_iter);
		gtk_list_store_set(new_model, &new_iter,
		                   MAESTRO_CONTROLLER_DAEMON, daemon,
		                   MAESTRO_CONTROLLER_ADDR, gebr_daemon_server_get_display_address(daemon),
		                   -1);
	}

	gtk_list_store_append(new_model, &new_iter);
	gtk_list_store_set(new_model, &new_iter,
	                   MAESTRO_CONTROLLER_DAEMON, NULL,
	                   MAESTRO_CONTROLLER_ADDR, _("Drop servers to increment this group!"),
	                   -1);

	return GTK_TREE_MODEL(new_model);
}

static void
on_server_group_changed(GebrMaestroServer *maestro,
			GebrMaestroController *self)
{
	if (!self->priv->builder)
		return;

	GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(self->priv->builder, "notebook_groups"));
	gint n = gtk_notebook_get_n_pages(nb);

	for (int i = 0; i < n-1; i++)
		gtk_notebook_remove_page(nb, 0);

	GList *tags = gebr_maestro_server_get_all_tags(maestro);

	for (GList *i = tags; i; i = i->next) {
		const gchar *tag = i->data;
		GtkWidget *label = gtk_label_new(tag);

		GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, tag);
		GtkTreeModel *new_model = copy_model_for_groups(model);
		g_object_unref(model);

		GtkWidget *view = gtk_tree_view_new_with_model(new_model);

		GtkTreeViewColumn *col = gtk_tree_view_column_new();
		GtkCellRenderer *cell = gtk_cell_renderer_text_new();
		gtk_tree_view_column_set_title(col, _("Address"));
		gtk_tree_view_column_pack_start(col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, cell, notebook_group_show_address, self, NULL);

		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

		gtk_notebook_prepend_page(nb, view, label);

		gtk_widget_show(label);
		gtk_widget_show(view);

		set_widget_drag_dest(view);
	}
}

static void
gebr_maestro_controller_init(GebrMaestroController *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_MAESTRO_CONTROLLER,
						 GebrMaestroControllerPriv);

	self->priv->model = gtk_list_store_new(3,
	                                       G_TYPE_OBJECT,
	                                       G_TYPE_STRING,
	                                       G_TYPE_BOOLEAN);
}

static void
gebr_maestro_controller_class_init(GebrMaestroControllerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[JOB_DEFINE] =
		g_signal_new("job-define",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, job_define),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__OBJECT_OBJECT,
			     G_TYPE_NONE, 2,
			     GEBR_TYPE_MAESTRO_SERVER, GEBR_TYPE_JOB);

	g_type_class_add_private(klass, sizeof(GebrMaestroControllerPriv));
}

GebrMaestroController *
gebr_maestro_controller_new(void)
{
	return g_object_new(GEBR_TYPE_MAESTRO_CONTROLLER, NULL);
}

static void
on_daemons_changed(GebrMaestroServer *maestro,
                   GebrMaestroController *mc)
{
	g_debug("DAEMONS CHANGED");
	GtkTreeIter iter;
	GtkTreeIter new_iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);

	gtk_list_store_clear(mc->priv->model);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		gtk_list_store_append(mc->priv->model, &new_iter);
		gtk_list_store_set(mc->priv->model, &new_iter,
		                   MAESTRO_CONTROLLER_DAEMON, daemon,
		                   MAESTRO_CONTROLLER_ADDR, gebr_daemon_server_get_display_address(daemon),
		                   MAESTRO_CONTROLLER_EDITABLE, FALSE,
		                   -1);
	}
	insert_new_entry(mc);
	g_object_unref(model);
}

static void
daemon_server_address_func(GtkTreeViewColumn *tree_column,
			   GtkCellRenderer *cell,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	const gchar *addr;
	gtk_tree_model_get(model, iter, MAESTRO_CONTROLLER_ADDR, &addr, -1);
	g_object_set(cell, "text", addr, NULL);
}

static void
daemon_server_status_func(GtkTreeViewColumn *tree_column,
			  GtkCellRenderer *cell,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	gboolean editable;
	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_DAEMON, &daemon,
	                   MAESTRO_CONTROLLER_EDITABLE, &editable,
	                   -1);

	if(editable) {
		g_object_set(cell, "stock-id", NULL, NULL);
		return;
	}

	if (!daemon)
		return;

	GebrCommServerState state = gebr_daemon_server_get_state(daemon);
	const gchar *stock_id;

	switch (state) {
	case SERVER_STATE_UNKNOWN:
	case SERVER_STATE_DISCONNECTED:
	case SERVER_STATE_RUN:
	case SERVER_STATE_OPEN_TUNNEL:
		stock_id = GTK_STOCK_DISCONNECT;
		break;
	case SERVER_STATE_CONNECT:
		stock_id = GTK_STOCK_CONNECT;
		break;
	}

	g_object_set(cell, "stock-id", stock_id, NULL);
}

/**
 * server_list_add:
 * @ui_server_list: Pointer to user interface server list
 * @address: The new server address
 *
 * Callback to add a server to the server list
 */
static void
server_list_add(GebrMaestroController *mc,
		const gchar * address)
{
	gchar *url = g_strdup_printf("/server?address=%s;pass=", address);
	GebrCommServer *server = gebr_maestro_server_get_server(mc->priv->maestros->data);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

static void
on_servers_editing_started (GtkCellRenderer *cell,
                            GtkEntry *entry,
                            gchar *path,
                            GebrMaestroController *mc)
{
	gtk_entry_set_text(entry, "");
}

static void
on_servers_edited(GtkCellRendererText *cell,
                  gchar *pathstr,
                  gchar *new_text,
                  GebrMaestroController *mc)
{
	if (!new_text || !*new_text)
		return;

	server_list_add(mc, new_text);
	insert_new_entry(mc);
}

static void 
on_connect_to_maestro_clicked( GtkButton *button,
			       GebrMaestroController *self)
{
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	connect_to_maestro(entry, self);


}

static void
on_dialog_response(GtkDialog *dialog,
		   gint response,
		   GebrMaestroController *self)
{
	g_object_unref(self->priv->builder);
	self->priv->builder = NULL;
}

GtkDialog *
gebr_maestro_controller_create_dialog(GebrMaestroController *self)
{
	self->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(self->priv->builder,
				  GEBR_GLADE_DIR"/gebr-maestro-dialog.glade",
				  NULL);

	/*
	 * Maestro combobox
	 */
	GebrMaestroServer *maestro = self->priv->maestros->data;
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	gtk_entry_set_text(entry, gebr_maestro_server_get_display_address(maestro));
	g_signal_connect(entry, "activate", G_CALLBACK(connect_to_maestro), self);
	on_state_change(maestro, self);

	GtkButton *connect_button = GTK_BUTTON(gtk_builder_get_object(self->priv->builder, "btn_connect"));
	g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_to_maestro_clicked), self);

	/*
	 * Servers treeview
	 */
	GtkTreeModel *model = GTK_TREE_MODEL(self->priv->model);
	GtkTreeView *view = GTK_TREE_VIEW(gtk_builder_get_object(self->priv->builder, "treeview_servers"));
	gtk_tree_view_set_model(view, model);
	insert_new_entry(self);

        gtk_drag_source_set(GTK_WIDGET(view), GDK_BUTTON1_MASK, entries, n_entries, GDK_ACTION_COPY);
        g_signal_connect(view, "drag-data-get", G_CALLBACK(drag_data_get_handl), NULL);
	gtk_drag_source_set_icon_stock(GTK_WIDGET(view), GTK_STOCK_NETWORK);

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

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

	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(col), renderer, "editable", MAESTRO_CONTROLLER_EDITABLE);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_servers_editing_started), self);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_servers_edited), self);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	on_server_group_changed(maestro, self);

	GtkEventBox *event = GTK_EVENT_BOX(gtk_builder_get_object(self->priv->builder, "eventbox_drop"));
	set_widget_drag_dest(GTK_WIDGET(event));

	GtkDialog *dialog = GTK_DIALOG(gtk_builder_get_object(self->priv->builder, "dialog_maestro"));
	g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), self);

	return dialog;
}

static void
connect_to_maestro(GtkEntry *entry,
		   GebrMaestroController *self)
{
	g_debug(".............Maestro Entry: %s",gtk_entry_get_text(entry));
	gebr_maestro_controller_connect(self, gtk_entry_get_text(entry));
}

static void
on_job_define(GebrMaestroServer *maestro,
	      GebrJob *job,
	      GebrMaestroController *self)
{
	g_signal_emit(self, signals[JOB_DEFINE], 0, maestro, job);
}

static const gchar *
on_password_request(GebrMaestroServer *maestro,
		    const gchar *address)
{
	gdk_threads_enter();
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Enter password"),
							NULL,
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
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

static void 
on_state_change(GebrMaestroServer *maestro,
		GebrMaestroController *self)
{
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, 
									     "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));

	GebrCommServer *server = gebr_maestro_server_get_server(maestro);
	GebrCommServerState state = gebr_comm_server_get_state(server);
	if (state == SERVER_STATE_DISCONNECTED)
		gtk_entry_set_icon_from_stock(entry,
					      GTK_ENTRY_ICON_SECONDARY,
					      GTK_STOCK_DISCONNECT);
	else if (state == SERVER_STATE_CONNECT)
		gtk_entry_set_icon_from_stock(entry,
					      GTK_ENTRY_ICON_SECONDARY,
					      GTK_STOCK_CONNECT);
}

void
gebr_maestro_controller_connect(GebrMaestroController *self,
				const gchar *address)
{
	GebrMaestroServer *maestro = gebr_maestro_server_new(address);
	self->priv->maestros = g_list_prepend(self->priv->maestros, maestro);

	g_signal_connect(maestro, "job-define",
			 G_CALLBACK(on_job_define), self);
	g_signal_connect(maestro, "group-changed",
			 G_CALLBACK(on_server_group_changed), self);
	g_signal_connect(maestro, "password-request",
			 G_CALLBACK(on_password_request), self);
	g_signal_connect(maestro, "daemons-changed",
				 G_CALLBACK(on_daemons_changed), self);
	g_signal_connect(maestro, "state-change",
			 G_CALLBACK(on_state_change), self);
	gebr_maestro_server_connect(maestro);

	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);
	GtkTreeView  *view = GTK_TREE_VIEW(gtk_builder_get_object(self->priv->builder, 
								 "treeview_servers"));
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model);
	g_object_unref(model);
}
