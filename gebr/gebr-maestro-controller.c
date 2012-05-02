/*
 * gebr-maestro-controller.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Core Team (www.gebrproject.com)
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
#include <gebr.h>

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "gebr-maestro-server.h"
#include "gebr-marshal.h"
#include <locale.h>
#include "ui_project_line.h"
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>

struct _GebrMaestroControllerPriv {
	GebrMaestroServer *maestro;
	GtkBuilder *builder;
	GtkWidget *servers_view;

	GtkListStore *model;

	gchar *last_tag;

	GtkWindow *window;
};

enum {
	MAESTRO_CONTROLLER_DAEMON,
	MAESTRO_CONTROLLER_ADDR,
	MAESTRO_CONTROLLER_AUTOCONN,
	MAESTRO_CONTROLLER_EDITABLE,
	MAESTRO_CONTROLLER_MPI,
	N_COLUMN
};

enum {
	JOB_DEFINE,
	MAESTRO_LIST_CHANGED,
	GROUP_CHANGED,
	MAESTRO_STATE_CHANGED,
	DAEMONS_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_WINDOW,
};
G_DEFINE_TYPE(GebrMaestroController, gebr_maestro_controller, G_TYPE_OBJECT);

static guint signals[LAST_SIGNAL] = { 0, };

static GtkTargetEntry entries[] = {
	{"POINTER", GTK_TARGET_SAME_APP, 0},
};
static guint n_entries = G_N_ELEMENTS(entries);

static void connect_to_maestro(GtkEntry *entry, GebrMaestroController *self);

static void on_state_change(GebrMaestroServer *maestro, GebrMaestroController *self);

static void gebr_maestro_controller_maestro_state_changed_real(GebrMaestroController *mc,
							       GebrMaestroServer *maestro);
static void update_maestro_view(GebrMaestroController *mc,
                                GebrMaestroServer *maestro,
                                gboolean emit_daemon_changed);

static void on_maestro_error(GebrMaestroServer *maestro,
			     const gchar *addr,
			     const gchar *error_type,
			     const gchar *error_msg,
			     GebrMaestroController *mc);

static void gebr_maestro_controller_finalize(GObject *object);

static void
gebr_maestro_controller_get(GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GebrMaestroController *mc = GEBR_MAESTRO_CONTROLLER(object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		g_value_set_pointer(value, mc->priv->window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_maestro_controller_set(GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GebrMaestroController *mc = GEBR_MAESTRO_CONTROLLER(object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		mc->priv->window = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
static void
insert_new_entry(GebrMaestroController *mc)
{
	GtkTreeIter iter;
	gtk_list_store_prepend(mc->priv->model, &iter);
	gtk_list_store_set(mc->priv->model, &iter,
	                   MAESTRO_CONTROLLER_DAEMON, NULL,
	                   MAESTRO_CONTROLLER_ADDR, _("New"),
	                   MAESTRO_CONTROLLER_EDITABLE, TRUE,
	                   -1);
}

static void
cancel_group_creation(GtkWidget *widget,
		      GebrMaestroController *mc)
{
	GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(mc->priv->builder, "notebook_groups"));
	GtkWidget *dummy = g_object_get_data(G_OBJECT(widget), "dummy-widget");
	gint page = gtk_notebook_page_num(nb, dummy);

	if (page != -1)
		gtk_notebook_remove_page(nb, page);
}

static void
finish_group_creation(GtkWidget *widget,
		      GebrMaestroController *mc)
{
	GtkEntry *entry = GTK_ENTRY(widget);
	gchar *tag = g_strdup(gtk_entry_get_text(entry));
	GebrDaemonServer *daemon = g_object_get_data(G_OBJECT(widget), "daemon");

	g_strstrip(tag);
	if (!*tag) {
		cancel_group_creation(widget, mc);
		return;
	}
	GList *tags = gebr_maestro_server_get_all_tags(mc->priv->maestro);
	gboolean group_exists = g_list_find_custom(tags, tag, (GCompareFunc)g_strcmp0) ==0 ? FALSE: TRUE;
	gboolean name_valid = gebr_utf8_is_asc_alnum(tag);

	if (group_exists || !name_valid) {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(widget), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
		const gchar *toolt = group_exists ? _("The group name already exists") : _("The group name must be alphanumeric") ;
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget), GTK_ENTRY_ICON_SECONDARY, toolt);
		gtk_widget_grab_focus(widget);
 		g_free(tag);
		return;
	}

	GtkWidget *box = gtk_hbox_new(FALSE, 5);

#if GTK_CHECK_VERSION(2,20,0)
	GtkWidget *spinner = gtk_spinner_new();
	gtk_box_pack_start(GTK_BOX(box), spinner, FALSE, TRUE, 0);
	gtk_spinner_start(GTK_SPINNER(spinner));
#endif

	gtk_box_pack_start(GTK_BOX(box), gtk_label_new(tag), TRUE, TRUE, 0);
	gtk_widget_show_all(box);

	GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(mc->priv->builder, "notebook_groups"));
	GtkWidget *dummy = g_object_get_data(G_OBJECT(widget), "dummy-widget");
	gtk_notebook_set_tab_label(nb, dummy, box);
	gebr_maestro_server_add_tag_to(mc->priv->maestro, daemon, tag);

	if (mc->priv->last_tag)
		g_free(mc->priv->last_tag);
	mc->priv->last_tag = g_strdup(tag);

	g_free(tag);
}

static gboolean
on_group_creation_entry_focus_out(GtkWidget *widget,
				  GdkEventFocus *event,
				  GebrMaestroController *mc)
{
	finish_group_creation(widget, mc);
	return FALSE;
}

static gboolean
on_group_creation_entry_key_press(GtkWidget *widget,
				  GdkEventKey *event,
				  GebrMaestroController *mc)
{
	switch (event->keyval) {
	case GDK_Escape:
		g_signal_handlers_disconnect_by_func(widget, on_group_creation_entry_key_press, mc);
		g_signal_handlers_disconnect_by_func(widget, on_group_creation_entry_focus_out, mc);
		cancel_group_creation(widget, mc);
		return TRUE;
	case GDK_KP_Enter:
	case GDK_Return:
		gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
				     NULL);
		return TRUE;
	default:
		return FALSE;
	}
}

static void
drag_data_received_handl(GtkWidget *widget,
			 GdkDragContext *context,
			 gint x, gint y,
			 GtkSelectionData *selection_data,
			 guint target_type,
			 guint time,
			 GebrMaestroController *mc)
{
	gpointer *daemon;

        gboolean dnd_success = FALSE;
        gboolean delete_selection_data = FALSE;

        if (selection_data && (selection_data->length >= 0)) {
		daemon = (gpointer *)selection_data->data;
		if (*daemon)
			dnd_success = TRUE;
        }

        if (!dnd_success)
		return;

	g_debug("Got Address %p: %s", *daemon, gebr_daemon_server_get_address(*daemon));

	if (GTK_IS_TREE_VIEW(widget)) {
		const gchar *tag = g_object_get_data(G_OBJECT(widget), "tag");
		gebr_maestro_server_add_tag_to(mc->priv->maestro, *daemon, tag);
	} else {
		GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(mc->priv->builder, "notebook_groups"));
		gint n = gtk_notebook_get_n_pages(nb);

		GtkWidget *dummy = gtk_label_new(_("Choose the group name and press ENTER"));
		GtkWidget *entry = gtk_entry_new();
		g_object_set_data(G_OBJECT(entry), "dummy-widget", dummy);
		g_object_set_data(G_OBJECT(entry), "daemon", *daemon);

		GtkBorder border = { 0, };
		//gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
		gtk_entry_set_inner_border(GTK_ENTRY(entry), &border);
		gtk_notebook_insert_page(nb, dummy, entry, n-1);
		gtk_widget_show(dummy);
		gtk_widget_show(entry);
		gtk_notebook_set_current_page(nb, n-1);
		gtk_widget_grab_focus(entry);

		g_signal_connect(entry, "key-press-event",
				 G_CALLBACK(on_group_creation_entry_key_press), mc);
		g_signal_connect(entry, "focus-out-event",
				 G_CALLBACK(on_group_creation_entry_focus_out), mc);
	}

        gtk_drag_finish(context, dnd_success, delete_selection_data, time);

	// This line is needed because GtkTreeView's already implement
	// ::drag-data-received signal. This prevents GtkTreeView from
	// calling it.
	g_signal_stop_emission_by_name(widget, "drag-data-received");
}

static gboolean
drag_drop_handl(GtkWidget *widget,
		GdkDragContext *context,
		gint x, gint y,
		guint time,
		GebrMaestroController *mc)
{
        gboolean is_valid_drop_site;
        GdkAtom target_type;

        if (context->targets) {
                target_type = GDK_POINTER_TO_ATOM(g_list_nth_data(context->targets, 0));
                gtk_drag_get_data(widget, context, target_type, time);
		is_valid_drop_site = TRUE;
        } else
                is_valid_drop_site = FALSE;

        return is_valid_drop_site;
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
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		gtk_tree_model_get_iter(model, &iter, i->data);
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		gtk_selection_data_set(selection_data, selection_data->target,
		                       sizeof(gpointer) * 8, (guchar*)&daemon, sizeof(gpointer));
	}
}

static void
set_widget_drag_dest(GebrMaestroController *mc, GtkWidget *widget)
{
        g_signal_connect(widget, "drag-data-received",
			 G_CALLBACK(drag_data_received_handl), mc);
        g_signal_connect(widget, "drag-drop",
			 G_CALLBACK(drag_drop_handl), mc);

	gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_MOTION
			  | GTK_DEST_DEFAULT_HIGHLIGHT,
			  entries, n_entries, GDK_ACTION_COPY);
}

static void
notebook_group_show_icon(GtkTreeViewColumn *tree_column,
			 GtkCellRenderer *cell,
			 GtkTreeModel *tree_model,
			 GtkTreeIter *iter,
			 gpointer data)
{
	GebrDaemonServer *daemon;

	gtk_tree_model_get(tree_model, iter,
			   MAESTRO_CONTROLLER_DAEMON, &daemon,
			   -1);

	if (!daemon)
		g_object_set(cell, "stock-id", NULL, NULL);
	else if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
		g_object_set(cell, "stock-id", GTK_STOCK_CONNECT, NULL);
	else
		g_object_set(cell, "stock-id", GTK_STOCK_DISCONNECT, NULL);
}

static void
notebook_group_show_address(GtkTreeViewColumn *tree_column,
			    GtkCellRenderer *cell,
			    GtkTreeModel *tree_model,
			    GtkTreeIter *iter,
			    gpointer data)
{
	GebrDaemonServer *daemon;
	gchar *label;

	gtk_tree_model_get(tree_model, iter,
			   MAESTRO_CONTROLLER_DAEMON, &daemon,
			   MAESTRO_CONTROLLER_ADDR, &label,
			   -1);

	if (daemon) {
		const gchar *host = gebr_daemon_server_get_hostname(daemon);
		g_object_set(cell,
			     "text", !*host ? label : host,
			     "sensitive", TRUE,
			     NULL);
		g_free(label);
		g_object_unref(daemon);
	} else if (label) {
		g_object_set(cell,
			     "text", label,
			     "sensitive", FALSE,
			     NULL);
		g_free(label);
	}
}

static GtkTreeModel *
copy_model_for_groups(GtkTreeModel *orig_model)
{
	GtkTreeIter iter, new_iter;
	GtkListStore *new_model;
	GebrDaemonServer *daemon;

	new_model = gtk_list_store_new(2,
                                       GEBR_TYPE_DAEMON_SERVER,
                                       G_TYPE_STRING);

	gebr_gui_gtk_tree_model_foreach(iter, orig_model) {
		gtk_tree_model_get(orig_model, &iter, 0, &daemon, -1);

		gtk_list_store_append(new_model, &new_iter);
		gtk_list_store_set(new_model, &new_iter,
		                   MAESTRO_CONTROLLER_DAEMON, daemon,
		                   MAESTRO_CONTROLLER_ADDR, gebr_daemon_server_get_address(daemon),
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
on_server_group_remove(GtkMenuItem *menuitem,
                       GebrMaestroController *mc)
{
	const gchar *tag = g_object_get_data(G_OBJECT(menuitem), "tag");
	GebrDaemonServer *daemon = g_object_get_data(G_OBJECT(menuitem), "daemon");

	gebr_maestro_server_remove_tag_from(mc->priv->maestro, daemon, tag);
}

static GtkMenu *
server_group_popup_menu(GtkWidget * widget,
                        GebrMaestroController *mc)
{
	GList *rows;
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows || g_list_length(rows) > 1)
		return NULL;

	GtkWidget *parent = gtk_widget_get_parent(widget);
	GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(mc->priv->builder, "notebook_groups"));
	GtkWidget *label = gtk_notebook_get_tab_label(nb, parent);
	const gchar *tag = gtk_label_get_text(GTK_LABEL(label));

	GtkTreeIter iter;
	GebrDaemonServer *daemon;

	gtk_tree_model_get_iter(model, &iter, rows->data);
	gtk_tree_model_get(model, &iter, MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

	if (!daemon)
		return NULL;

	menu = gtk_menu_new ();

	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic(_("_Remove from group"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_server_group_remove), mc);

	gchar *tagdup = g_strdup(tag);
	g_object_set_data(G_OBJECT(item), "tag", tagdup);
	g_object_weak_ref(G_OBJECT(item), (GWeakNotify)g_free, tagdup);

	g_object_set_data(G_OBJECT(item), "daemon", daemon);
	g_object_weak_ref(G_OBJECT(item), (GWeakNotify)g_object_unref, tagdup);

	gtk_widget_show_all (menu);
	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);

	return GTK_MENU (menu);
}

static void
gebr_maestro_controller_group_changed_real(GebrMaestroController *self,
                                           GebrMaestroServer *maestro)
{
	if (!self->priv->builder)
		return;

	GtkNotebook *nb = GTK_NOTEBOOK(gtk_builder_get_object(self->priv->builder, "notebook_groups"));
	gint n = gtk_notebook_get_n_pages(nb);
	gint current = gtk_notebook_get_current_page(nb);

	for (int i = 0; i < n-1; i++)
		gtk_notebook_remove_page(nb, 0);

	GList *tags = gebr_maestro_server_get_all_tags(maestro);

	for (GList *i = tags; i; i = i->next) {
		const gchar *tag = i->data;
		GtkWidget *label = gtk_label_new(tag);
		GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, tag);
		GtkTreeModel *new_model = copy_model_for_groups(model);

		GtkWidget *view = gtk_tree_view_new_with_model(new_model);

		GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		gtk_container_add(GTK_CONTAINER(scrolled_window), view);
		g_object_unref(model);
		g_object_unref(new_model);

		GtkTreeViewColumn *col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(col, _("Address"));

		GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(col, cell, FALSE);
		gtk_tree_view_column_set_cell_data_func(col, cell, notebook_group_show_icon, self, NULL);

		cell = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, cell, notebook_group_show_address, self, NULL);

		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		gtk_notebook_prepend_page(nb, scrolled_window, label);

		gtk_widget_show(label);
		gtk_widget_show_all(scrolled_window);

		gchar *tagdup = g_strdup(tag);
		g_object_set_data(G_OBJECT(view), "tag", tagdup);
		g_object_weak_ref(G_OBJECT(view), (GWeakNotify)g_free, tagdup);
		set_widget_drag_dest(self, view);

		gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
		                                          (GebrGuiGtkPopupCallback) server_group_popup_menu, self);
	}

	for (int i = 0; i < n-1; i++) {
		GtkWidget *child = gtk_notebook_get_nth_page(nb, i);
		if (child) {
			const gchar *tmp = gtk_notebook_get_tab_label_text(nb, child);
			if (tmp && g_strcmp0(tmp, self->priv->last_tag) == 0)
				current = i;
		}
	}
	gtk_notebook_set_current_page(nb, current);
}

static void
on_server_group_changed(GebrMaestroServer *maestro,
			GebrMaestroController *self)
{
	g_signal_emit(self, signals[GROUP_CHANGED], 0, maestro);
}

static void
gebr_maestro_controller_init(GebrMaestroController *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_MAESTRO_CONTROLLER,
						 GebrMaestroControllerPriv);

	self->priv->model = gtk_list_store_new(N_COLUMN,
	                                       G_TYPE_OBJECT,
	                                       G_TYPE_STRING,
	                                       G_TYPE_BOOLEAN,
					       G_TYPE_BOOLEAN,
	                                       G_TYPE_STRING);
	self->priv->maestro = NULL;

}

static void
gebr_maestro_controller_class_init(GebrMaestroControllerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_maestro_controller_get;
	object_class->set_property = gebr_maestro_controller_set;
	object_class->finalize = gebr_maestro_controller_finalize;

	klass->group_changed = gebr_maestro_controller_group_changed_real;
	klass->maestro_state_changed = gebr_maestro_controller_maestro_state_changed_real;

	signals[JOB_DEFINE] =
		g_signal_new("job-define",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroControllerClass, job_define),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__OBJECT_OBJECT,
			     G_TYPE_NONE, 2,
			     GEBR_TYPE_MAESTRO_SERVER, GEBR_TYPE_JOB);

	signals[MAESTRO_LIST_CHANGED] =
		g_signal_new("maestro-list-changed",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroControllerClass, maestro_list_changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	signals[GROUP_CHANGED] =
		g_signal_new("group-changed",
		             G_OBJECT_CLASS_TYPE(object_class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(GebrMaestroControllerClass, group_changed),
		             NULL, NULL,
		             g_cclosure_marshal_VOID__OBJECT,
		             G_TYPE_NONE, 1, GEBR_TYPE_MAESTRO_SERVER);

	signals[MAESTRO_STATE_CHANGED] =
		g_signal_new("maestro-state-changed",
		             G_OBJECT_CLASS_TYPE(object_class),
		             G_SIGNAL_RUN_LAST,
		             G_STRUCT_OFFSET(GebrMaestroControllerClass, maestro_state_changed),
		             NULL, NULL,
		             g_cclosure_marshal_VOID__OBJECT,
		             G_TYPE_NONE, 1, GEBR_TYPE_MAESTRO_SERVER);

	signals[DAEMONS_CHANGED] =
			g_signal_new("daemons-changed",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroControllerClass, daemons_changed),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__VOID,
			             G_TYPE_NONE, 0);

	g_object_class_install_property(object_class,
					PROP_WINDOW,
					g_param_spec_pointer("window",
							     "Window",
							     "Window of SFTP",
							     G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(GebrMaestroControllerPriv));
}

static void
gebr_maestro_controller_finalize(GObject *object)
{
	GebrMaestroController *mc = GEBR_MAESTRO_CONTROLLER(object);

	g_object_unref(mc->priv->maestro);
	g_object_unref(mc->priv->builder);
	g_free(mc->priv->last_tag);
	g_object_unref(mc->priv->window);

	G_OBJECT_CLASS(gebr_maestro_controller_parent_class)->finalize(object);
}

GebrMaestroController *
gebr_maestro_controller_new()
{
	return g_object_new(GEBR_TYPE_MAESTRO_CONTROLLER, NULL);
}

static void
on_server_connect(GtkMenuItem *menuitem,
                  GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model;

	GtkTreeView *view = GTK_TREE_VIEW(mc->priv->servers_view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;

		if (!gtk_tree_model_get_iter(model, &iter, path))
			continue;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &daemon,-1);

		if (!daemon)
			continue;

		if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
			continue;

		gebr_connectable_connect(GEBR_CONNECTABLE(mc->priv->maestro),
					 gebr_daemon_server_get_address(daemon));
	}
}

static void
on_server_disconnect(GtkMenuItem *menuitem,
                     GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model;

	GtkTreeView *view = GTK_TREE_VIEW(mc->priv->servers_view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;

		if (!gtk_tree_model_get_iter(model, &iter, path))
			continue;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

		if (!daemon)
			continue;

		if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_DISCONNECTED)
			continue;

		gebr_connectable_disconnect(GEBR_CONNECTABLE(mc->priv->maestro),
					    gebr_daemon_server_get_address(daemon),
					    "");
	}
}

static void
on_server_remove(GtkMenuItem *menuitem,
                 GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model;

	GtkTreeView *view = GTK_TREE_VIEW(mc->priv->servers_view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;

		if (!gtk_tree_model_get_iter(model, &iter, path))
			continue;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

		if (!daemon)
			continue;

		if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED) {
			gebr_connectable_disconnect(GEBR_CONNECTABLE(mc->priv->maestro),
			                            gebr_daemon_server_get_address(daemon),
			                            "remove");
		} else
			gebr_connectable_remove(GEBR_CONNECTABLE(mc->priv->maestro), gebr_daemon_server_get_address(daemon));
	}
	on_server_group_changed(mc->priv->maestro, mc);
}

static void
on_server_stop(GtkMenuItem *menuitem,
                 GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model;

	GtkTreeView *view = GTK_TREE_VIEW(mc->priv->servers_view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;

		if (!gtk_tree_model_get_iter(model, &iter, path))
			continue;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

		if (!daemon)
			continue;

		gebr_connectable_stop(GEBR_CONNECTABLE(mc->priv->maestro), gebr_daemon_server_get_address(daemon));
	}
	on_server_group_changed(mc->priv->maestro, mc);
}

GtkMenu *
gebr_maestro_controller_server_popup_menu(GtkWidget * widget,
                                          GebrMaestroController *mc)
{
	GList *rows;
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows)
		return NULL;

	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	gtk_tree_model_get_iter(model, &iter, rows->data);
	gtk_tree_model_get(model, &iter, MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

	if(!daemon)
		return NULL;

	menu = gtk_menu_new ();

	GtkWidget *item;

	mc->priv->servers_view = widget;
	item = gtk_menu_item_new_with_mnemonic(_("_Connect"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_server_connect), mc);

	item = gtk_menu_item_new_with_mnemonic(_("_Disconnect"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_server_disconnect), mc);

	item = gtk_menu_item_new_with_mnemonic(_("_Remove"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_server_remove), mc);

	item = gtk_menu_item_new_with_mnemonic(_("Stop Server"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(on_server_stop), mc);

	gtk_widget_show_all (menu);
	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);

	return GTK_MENU (menu);
}

void
gebr_maestro_controller_update_daemon_model(GebrMaestroServer *maestro,
                                            GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GtkTreeIter new_iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);
	const gchar *hostname;

	gtk_list_store_clear(mc->priv->model);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		hostname = gebr_daemon_server_get_hostname(daemon);

		gtk_list_store_append(mc->priv->model, &new_iter);
		gtk_list_store_set(mc->priv->model, &new_iter,
		                   MAESTRO_CONTROLLER_DAEMON, daemon,
		                   MAESTRO_CONTROLLER_ADDR, !g_strcmp0(hostname, "")? gebr_daemon_server_get_address(daemon) : hostname,
		                		   MAESTRO_CONTROLLER_AUTOCONN, gebr_daemon_server_get_ac(daemon),
		                		   MAESTRO_CONTROLLER_EDITABLE, FALSE,
		                		   -1);
	}
	insert_new_entry(mc);
	g_object_unref(model);
}

static void
on_daemons_changed(GebrMaestroServer *maestro,
                   GebrMaestroController *mc)
{
	gebr_maestro_controller_update_daemon_model(maestro, mc);

	g_signal_emit(mc, signals[DAEMONS_CHANGED], 0);
}

void
gebr_maestro_controller_daemon_server_address_func(GtkTreeViewColumn *tree_column,
                                                   GtkCellRenderer *cell,
                                                   GtkTreeModel *model,
                                                   GtkTreeIter *iter,
                                                   gpointer data)
{
	gboolean insert_new = GPOINTER_TO_INT(data);

	const gchar *addr;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_DAEMON, &daemon,
	                   MAESTRO_CONTROLLER_ADDR, &addr,
	                   -1);

	if (!daemon && !insert_new) {
		gchar *text = g_strdup_printf(_("Servers of Maestro %s"), gebr_maestro_server_get_address(gebr.maestro_controller->priv->maestro));
		g_object_set(cell, "text", text, NULL);
		g_object_set(cell, "sensitive", FALSE, NULL);
		g_free(text);
	} else {
		g_object_set(cell, "text", addr, NULL);
		g_object_set(cell, "sensitive", TRUE, NULL);
	}
}

void
gebr_maestro_controller_daemon_server_status_func(GtkTreeViewColumn *tree_column,
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
	const gchar *stock_id = NULL;

	const gchar *error = gebr_daemon_server_get_error(daemon);

	if (!error || !*error)
	{
		switch (state) {
		case SERVER_STATE_UNKNOWN:
		case SERVER_STATE_DISCONNECTED:
		case SERVER_STATE_CONNECT:
		case SERVER_STATE_RUN:
		case SERVER_STATE_OPEN_TUNNEL:
			stock_id = GTK_STOCK_DISCONNECT;
			break;
		case SERVER_STATE_LOGGED:
			stock_id = GTK_STOCK_CONNECT;
			break;
		default:
			g_warn_if_reached();
			break;
		}
	}
	else
		stock_id = GTK_STOCK_DIALOG_WARNING;

	g_object_set(cell, "stock-id", stock_id, NULL);
}

static void
daemon_server_ac_func(GtkTreeViewColumn *tree_column,
                      GtkCellRenderer *cell,
                      GtkTreeModel *model,
                      GtkTreeIter *iter,
                      gpointer data)
{
	gboolean ac, editable;

	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_AUTOCONN, &ac,
	                   MAESTRO_CONTROLLER_EDITABLE, &editable,
	                   -1);

	if (editable) {
		g_object_set(cell, "visible", FALSE, NULL);
		return;
	}

	g_object_set(cell, "activatable", TRUE, NULL);
	g_object_set(cell, "visible", TRUE, NULL);
	g_object_set(cell, "active", ac, NULL);
}

static void
daemon_server_mpi_func(GtkTreeViewColumn *tree_column,
                      GtkCellRenderer *cell,
                      GtkTreeModel *model,
                      GtkTreeIter *iter,
                      gpointer data)
{
	gboolean editable;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_EDITABLE, &editable,
			   MAESTRO_CONTROLLER_DAEMON, &daemon,
	                   -1);

	if (editable) {
		g_object_set(cell, "stock-id", NULL, NULL);
		return;
	}
	const gchar *mpi_flavors = gebr_daemon_server_get_mpi_flavors(daemon);

	if (mpi_flavors && *mpi_flavors)
		g_object_set(cell, "stock-id", "mpi-icon", NULL);
	else
		g_object_set(cell, "stock-id", NULL, NULL);

}

static void
daemon_server_cpu_clock_func(GtkTreeViewColumn *tree_column,
                             GtkCellRenderer *cell,
                             GtkTreeModel *model,
                             GtkTreeIter *iter,
                             gpointer data)
{
	gboolean editable;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_EDITABLE, &editable,
	                   MAESTRO_CONTROLLER_DAEMON, &daemon,
	                   -1);

	if (editable) {
		g_object_set(cell, "text", NULL, NULL);
		return;
	}

	gchar *cpu_clock;
	gdouble clock = atof(gebr_daemon_server_get_cpu_clock(daemon))/1000;

	cpu_clock = g_strdup_printf("%.1lf GHz", clock);

	g_object_set(cell, "text", cpu_clock, NULL);

	g_free(cpu_clock);
}

static void
daemon_server_memory_func(GtkTreeViewColumn *tree_column,
                          GtkCellRenderer *cell,
                          GtkTreeModel *model,
                          GtkTreeIter *iter,
                          gpointer data)
{
	gboolean editable;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter,
	                   MAESTRO_CONTROLLER_EDITABLE, &editable,
	                   MAESTRO_CONTROLLER_DAEMON, &daemon,
	                   -1);

	if (editable) {
		g_object_set(cell, "text", NULL, NULL);
		return;
	}

	gchar *memory;
	gdouble mem = g_strtod(gebr_daemon_server_get_memory(daemon), NULL)/1000/1000;

	memory = g_strdup_printf("%.1lf GB", mem);

	g_object_set(cell, "text", memory, NULL);

	g_free(memory);
}

/**
 * server_list_add:
 * @ui_server_list: Pointer to user interface server list
 * @address: The new server address
 *
 * Callback to add a server to the server list
 */
void
gebr_maestro_controller_server_list_add(GebrMaestroController *mc,
                                        const gchar * address)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/server");
	gebr_comm_uri_add_param(uri, "address", address);
	gebr_comm_uri_add_param(uri, "pass", "");

	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrCommServer *server = gebr_maestro_server_get_server(mc->priv->maestro);
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
	gboolean is_valid = gebr_verify_address_without_username(new_text);

	if (!is_valid && *new_text) {

		//FIXME: Do we need to use threads_enter here???

		//I don t know why markup does not work here
		gebr_gui_message_dialog(GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					_("Invalid input"),
					_("GêBR supports the formats hostname or ip address.\n\n"
					  "The entry %s will be automatically removed"), new_text);

		return;
	}


	gchar *address = gebr_get_address_without_user(new_text);
	if (!address)
		return;

	gebr_maestro_controller_server_list_add(mc, address);
	insert_new_entry(mc);

	GtkTreeIter iter;
	GtkTreeView *view = GTK_TREE_VIEW(gtk_builder_get_object(mc->priv->builder, "treeview_servers"));

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(mc->priv->model), &iter);
	gebr_gui_gtk_tree_view_scroll_to_iter_cell(view, &iter);

	g_free(address);
}

static void
on_stop_to_maestro_clicked(GtkButton *button,
                           GebrMaestroController *self)
{
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));

	gebr_maestro_controller_stop(self, gtk_entry_get_text(entry));
}

static void 
on_connect_to_maestro_clicked(GtkButton *button,
			      GebrMaestroController *self)
{
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	connect_to_maestro(entry, self);
}

static gboolean
server_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
                        GtkTreeIter * iter, GtkTreeViewColumn * column, GebrMaestroController *self)
{
	if (gtk_tree_view_get_column(tree_view, 0) == column) {
		gboolean autoconnect;

		gtk_tree_model_get(GTK_TREE_MODEL(self->priv->model), iter, MAESTRO_CONTROLLER_AUTOCONN, &autoconnect, -1);

		if (autoconnect)
			gtk_tooltip_set_text(tooltip, _("Connect server when GêBR starts"));
		else
			gtk_tooltip_set_text(tooltip, _("Do not connect server when GêBR starts"));

		return TRUE;
	}
	else if (gtk_tree_view_get_column(tree_view, 1) == column) {
		GebrDaemonServer *daemon;
		gchar *mpi_flavors;

		gtk_tree_model_get(GTK_TREE_MODEL(self->priv->model), iter,
		                   MAESTRO_CONTROLLER_DAEMON, &daemon,
		                   -1);

		if (!daemon)
			return FALSE;

		const gchar *tmp_flavors = gebr_daemon_server_get_mpi_flavors(daemon);

		if (!tmp_flavors || !*tmp_flavors)
			return FALSE;

		mpi_flavors = g_strdup(tmp_flavors);

		gtk_tooltip_set_text(tooltip, mpi_flavors);
		
		g_free(mpi_flavors);

		return TRUE;
	}
	else if (gtk_tree_view_get_column(tree_view, 2) == column) {
		GebrDaemonServer *daemon;

		gtk_tree_model_get(GTK_TREE_MODEL(self->priv->model), iter, MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

		if (!daemon)
			return FALSE;

		const gchar *error = gebr_daemon_server_get_error(daemon);

		if (!error || !*error)
			return FALSE;

		gtk_tooltip_set_text(tooltip, error);
		return TRUE;
	}
	return FALSE;
}

static void
on_dialog_response(GtkDialog *dialog,
                   gint response,
                   GebrMaestroController *self)
{
	g_object_unref(self->priv->builder);
	self->priv->builder = NULL;
}

static void
on_dialog_destroy(GtkWidget *dialog, GebrMaestroController *mc)
{
	gebr_maestro_server_set_window(mc->priv->maestro, mc->priv->window);
}

static void 
on_ac_toggled (GtkCellRendererToggle *cell_renderer,
	       gchar *path,
	       GebrMaestroController *mc)
{
	gboolean ac;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = GTK_TREE_MODEL(mc->priv->model);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_from_string (model, &iter, path))
		return;

	gtk_tree_model_get(model, &iter, MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

	if (!daemon)
		return;

	ac = gtk_cell_renderer_toggle_get_active(cell_renderer);

	gebr_maestro_server_set_autoconnect(mc->priv->maestro, daemon, !ac);
}


GtkDialog *
gebr_maestro_controller_create_dialog(GebrMaestroController *self)
{
	self->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(self->priv->builder,
				  GEBR_GLADE_DIR"/gebr-maestro-dialog.glade",
				  NULL);

	GtkTreeView *view = GTK_TREE_VIEW(gtk_builder_get_object(self->priv->builder, 
								 "treeview_servers"));

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view), GTK_SELECTION_MULTIPLE);

	/*
	 * Maestro combobox
	 */
	GebrMaestroServer *maestro = self->priv->maestro;
	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(self->priv->builder, "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	gtk_entry_set_text(entry, gebr_maestro_server_get_address(maestro));
	g_signal_connect(entry, "activate", G_CALLBACK(connect_to_maestro), self);

	const gchar *error_type, *error_msg;
	gebr_maestro_server_get_error(maestro, &error_type, &error_msg);
	on_maestro_error(maestro, gebr_maestro_server_get_address(maestro),
			 error_type, error_msg, self);
	update_maestro_view(self, maestro, FALSE);

	GtkButton *connect_button = GTK_BUTTON(gtk_builder_get_object(self->priv->builder, "btn_connect"));
	g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_to_maestro_clicked), self);

	GtkButton *stop_button = GTK_BUTTON(gtk_builder_get_object(self->priv->builder, "btn_stop"));
	g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_to_maestro_clicked), self);

	/*
	 * Servers treeview
	 */
	GtkTreeModel *model = GTK_TREE_MODEL(self->priv->model);
	gtk_tree_view_set_model(view, model);
	GtkCellRenderer *renderer ;

        gtk_drag_source_set(GTK_WIDGET(view), GDK_BUTTON1_MASK, entries, n_entries, GDK_ACTION_COPY);
        g_signal_connect(view, "drag-data-get", G_CALLBACK(drag_data_get_handl), NULL);
	gtk_drag_source_set_icon_stock(GTK_WIDGET(view), GTK_STOCK_NETWORK);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
	                                          (GebrGuiGtkPopupCallback) gebr_maestro_controller_server_popup_menu, self);
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
	                                            (GebrGuiGtkTreeViewTooltipCallback) server_tooltip_callback, self);

	// AC Column
	GtkTreeViewColumn *ac_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(ac_col, _("AC"));

	renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ac_col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(ac_col, renderer, daemon_server_ac_func,
	                                        NULL, NULL);
	g_signal_connect(renderer, "toggled", G_CALLBACK (on_ac_toggled), self);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), ac_col);
	// End of AC Column

	// MPI column
	GtkTreeViewColumn *mpi_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(mpi_col, "MPI");

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mpi_col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(mpi_col, renderer, daemon_server_mpi_func,
	                                        NULL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), mpi_col);
	// End of MPI column

	// Server Column
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Address"));
	gtk_tree_view_column_set_min_width(col, 100);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_status_func,
	                                        NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_address_func,
	                                        GINT_TO_POINTER(TRUE), NULL);

	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(col), renderer, "editable", MAESTRO_CONTROLLER_EDITABLE);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_servers_editing_started), self);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_servers_edited), self);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	// End of Server Column

	// CPU Clock column
	GtkTreeViewColumn *cpu_clock = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(cpu_clock, "CPU Clock");

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cpu_clock), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(cpu_clock, renderer, daemon_server_cpu_clock_func,
	                                        NULL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), cpu_clock);
	// End of CPU Clock column

	// Memory column
	GtkTreeViewColumn *memory_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(memory_col, "Memory");

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(memory_col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(memory_col, renderer, daemon_server_memory_func,
	                                        NULL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), memory_col);
	// End of Memory column

	gebr_maestro_controller_group_changed_real(self, maestro);

	GtkEventBox *event = GTK_EVENT_BOX(gtk_builder_get_object(self->priv->builder, "eventbox_drop"));
	set_widget_drag_dest(self, GTK_WIDGET(event));

	GtkDialog *dialog = GTK_DIALOG(gtk_builder_get_object(self->priv->builder, "dialog_maestro"));

	g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), self);
	g_signal_connect(dialog, "destroy", G_CALLBACK(on_dialog_destroy), self);

	gebr_maestro_server_set_window(maestro, GTK_WINDOW(dialog));

	return dialog;
}

static void
connect_to_maestro(GtkEntry *entry,
		   GebrMaestroController *self)
{
	const gchar *entry_text = gtk_entry_get_text(entry);
	const gchar *address;
	if (g_strcmp0(entry_text, "127.0.0.1")==0 || g_strcmp0(entry_text, "localhost")==0)
		address = g_get_host_name();
	else
		address = entry_text;
	gebr_maestro_controller_connect(self, address);
}

static void
on_job_define(GebrMaestroServer *maestro,
	      GebrJob *job,
	      GebrMaestroController *self)
{
	g_signal_emit(self, signals[JOB_DEFINE], 0, maestro, job);
}

static gboolean
on_question_request(GebrMaestroServer *maestro,
		    const gchar *address,
		    const gchar *title,
		    const gchar *question)
{
	gdk_threads_enter();
	GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
							       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							       GTK_MESSAGE_QUESTION,
							       GTK_BUTTONS_YES_NO,
							       _("<span size='large'>%s</span>\n%s"),
							       title, question);

	gchar *win_title = g_strdup_printf(_("SSH question from %s"), address);
	gtk_window_set_title(GTK_WINDOW(dialog), win_title);

	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	g_debug("USER ANSWERED %d RESPONSE!!!!!!", response);
	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	return response == GTK_RESPONSE_YES;
}

static void
on_ssh_info_button_clicked (GtkButton *button, gpointer pointer)
{
	gchar *loc;
	const gchar *path;

	loc = setlocale(LC_MESSAGES, NULL);
	if (g_str_has_prefix (loc, "pt"))
		path = "file://" GEBR_USERDOC_DIR "/pt_BR/html/index.html#ssh_authentication";
	else
		path = "file://" GEBR_USERDOC_DIR "/en/html/index.html#ssh_authentication";

	if (!gtk_show_uri(NULL, path, GDK_CURRENT_TIME, NULL)) {
		gtk_show_uri(NULL, "http://www.gebrproject.com", GDK_CURRENT_TIME, NULL);
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not load help. "
				"Certify it was installed correctly."));
	}
}

static PasswordKeys *
on_password_request(GebrMaestroServer *maestro,
		    const gchar *address,
		    gboolean acceps_key)
{
	gdk_threads_enter();
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Enter password"),
							NULL,
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
							GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gchar *title = g_strdup_printf(_("Trying to connect on %s"), address);
	gtk_window_set_title(GTK_WINDOW(dialog), title);

	gchar *message = g_markup_printf_escaped(_("<b>%s</b> is asking for your login password.\nEnter it below."), address);
	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 5);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 5);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	GtkWidget *checkbox = gtk_check_button_new_with_label(_("Use encryption key to automatically authenticate the next session."));
	GtkWidget *down_vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(down_vbox), checkbox, TRUE, TRUE, 5);

	GtkWidget *ssh_info_button = gtk_button_new_with_label(_("More info"));
	GtkWidget *hbox_info = gtk_hbox_new(FALSE, 5);
	gtk_button_set_alignment(GTK_BUTTON(ssh_info_button), 1.0, 0.5);
	gtk_button_set_relief(GTK_BUTTON(ssh_info_button), GTK_RELIEF_HALF);
	g_signal_connect(ssh_info_button, "clicked", G_CALLBACK(on_ssh_info_button_clicked), NULL);
	gtk_box_pack_end(GTK_BOX(hbox_info), ssh_info_button, FALSE, FALSE, 1);
	gtk_box_pack_end(GTK_BOX(down_vbox), hbox_info, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), down_vbox, TRUE, TRUE, 5);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), FALSE);

	if (acceps_key)
		gtk_widget_set_sensitive(checkbox, TRUE);
	else
		gtk_widget_set_sensitive(checkbox, FALSE);

	gtk_widget_show_all(dialog);
	gboolean confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;

	gchar *password;
	gboolean use_key;
	if (confirmed) {
		 password = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		 use_key = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox));
	} else {
		password = NULL;
		use_key = FALSE;
	}

	PasswordKeys *pk = g_new(PasswordKeys, 1);
	pk->password = g_strdup(password);
	pk->use_public_key = use_key;

	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	g_free(message);
	g_free(title);
	g_free(password);

	return pk;
}

static void
on_maestro_confirm(GebrMaestroServer *maestro,
                   const gchar *addr,
                   const gchar *type,
                   GebrMaestroController *mc)
{
	const gchar *msg = NULL;

	if (g_strcmp0(type, "remove-immediately") == 0) {
		gebr_connectable_disconnect(GEBR_CONNECTABLE(mc->priv->maestro),
		                            addr, "yes");
		gebr_connectable_remove(GEBR_CONNECTABLE(mc->priv->maestro), addr);

		return;
	}

	if (g_strcmp0(type, "disconnect") == 0)
		msg = N_("<span size='large' weight='bold'>The daemon %s is executing jobs.\n"
			 "Do you really want to disconnect it and cancel these jobs?</span>");
	else if (g_strcmp0(type, "remove") == 0)
		msg = N_("<span size='large' weight='bold'>The daemon %s is executing jobs.\n"
			 "Do you really want to remove it and cancel these jobs?</span>");

	GtkWidget *dialog  = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
	                                                        _(msg), addr);

	gdk_threads_enter();
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_YES) {
		gebr_connectable_disconnect(GEBR_CONNECTABLE(mc->priv->maestro),
		                            addr, "yes");

		if (g_strcmp0(type, "remove") == 0)
			gebr_connectable_remove(GEBR_CONNECTABLE(mc->priv->maestro), addr);
	}
	gtk_widget_destroy(dialog);
	gdk_threads_leave();
}

static void
on_maestro_error(GebrMaestroServer *maestro,
		const gchar *addr,
		const gchar *error_type,
		const gchar *error_msg,
		GebrMaestroController *mc)
{
	if (!mc->priv->builder)
		return;

	gchar *message = gebr_maestro_server_translate_error(error_type, error_msg);

	GtkComboBoxEntry *combo = GTK_COMBO_BOX_ENTRY(gtk_builder_get_object(mc->priv->builder, 
									     "combo_maestro"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	GebrCommServer *server = gebr_maestro_server_get_server(maestro);
	GebrCommServerState state = gebr_comm_server_get_state(server);

	if (state == SERVER_STATE_DISCONNECTED) {
		if (!message) {
			gtk_entry_set_icon_from_stock(entry,
			                              GTK_ENTRY_ICON_SECONDARY,
			                              GTK_STOCK_DISCONNECT);
			gtk_entry_set_icon_tooltip_text(entry,
			                                GTK_ENTRY_ICON_SECONDARY,
			                                _("Disconnected"));
		} else {
			gtk_entry_set_icon_from_stock(entry,
						      GTK_ENTRY_ICON_SECONDARY,
						      GTK_STOCK_DIALOG_WARNING);
			gtk_entry_set_icon_tooltip_text(entry,
							GTK_ENTRY_ICON_SECONDARY,
							message);
		}
	} else if (state == SERVER_STATE_LOGGED) {
		gtk_entry_set_icon_from_stock(entry,
					      GTK_ENTRY_ICON_SECONDARY,
					      GTK_STOCK_CONNECT);
		gtk_entry_set_icon_tooltip_text(entry,
						GTK_ENTRY_ICON_SECONDARY,
						_("Connected"));
	}

	if (message)
		g_free(message);
}

static void
on_daemon_error(GebrMaestroServer *maestro,
		const gchar *addr,
		const gchar *error_type,
		const gchar *error_msg,
		GebrMaestroController *mc)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(mc->priv->model);
	const gchar *message = NULL;
	gchar *second = NULL;
	gboolean show_dialog = FALSE;

	if (!*error_type) {
		message = NULL;
	} else if (g_strcmp0(error_type, "error:nfs") == 0) {
		message = _("This server has a different NFS");
	} else if (g_strcmp0(error_type, "error:id") == 0) {
		message = _("Server already added");
		second = g_strdup_printf(_("The server %s was already added in"
					   " this maestro. It will be"
					   " automatically removed."), addr);
		show_dialog = TRUE;
	} else if (g_strcmp0(error_type, "error:protocol") == 0) {
		message = _("This server is using a different protocol version");
	} else if (g_strcmp0(error_type, "error:connection-refused") == 0) {
		message = _("This server is already registered at another maestro");
	} else if (g_strcmp0(error_type, "error:ssh") == 0) {
		message = error_msg;
	} else if (g_strcmp0(error_type, "error:xauth") == 0) {
		message = _("This server cannot connect to display. Try reconnect it");
	}

	if (show_dialog) {
		gdk_threads_enter();
		gebr_gui_message_dialog(GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					message, second);
		gdk_threads_leave();
	} else {
		GebrDaemonServer *daemon;
		gebr_gui_gtk_tree_model_foreach(iter, model) {
			gtk_tree_model_get(model, &iter,
					   MAESTRO_CONTROLLER_DAEMON, &daemon, -1);

			if (!daemon)
				continue;

			if (g_strcmp0(addr, gebr_daemon_server_get_address(daemon)) == 0) {
				gebr_daemon_server_set_error(daemon, message);

				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				gtk_tree_model_row_changed(model, path, &iter);

				break;
			}
		}
	}

	if (second)
		g_free(second);
}


static void
on_ac_change(GebrMaestroServer *maestro,
             gboolean ac,
             GebrDaemonServer *daemon,
             GebrMaestroController *self)
{
	gboolean has_daemon = FALSE;
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(self->priv->model);

	const gchar *addr = gebr_daemon_server_get_address(daemon);

	GebrDaemonServer *d;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &d, -1);
		if (!d)
			continue;
		if (g_strcmp0(addr, gebr_daemon_server_get_address(d)) == 0) {
			has_daemon = TRUE;
			break;
		}
	}
	if (has_daemon) {
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, MAESTRO_CONTROLLER_AUTOCONN, ac, -1);
		gebr_daemon_server_set_ac(daemon, ac);
	}
}

static void
on_mpi_change(GebrMaestroServer *maestro,
	      GebrDaemonServer *daemon,
	      const gchar *mpi_flavor,
	      GebrMaestroController *self)
{
	gboolean has_daemon = FALSE;
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(self->priv->model);

	const gchar *addr = gebr_daemon_server_get_address(daemon);

	GebrDaemonServer *d;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
		                   MAESTRO_CONTROLLER_DAEMON, &d, -1);
		if (!d)
			continue;
		if (g_strcmp0(addr, gebr_daemon_server_get_address(d)) == 0) {
			has_daemon = TRUE;
			break;
		}
	}
	if (has_daemon)
		gebr_daemon_server_set_mpi_flavors(daemon, mpi_flavor);
}

static void
update_maestro_view(GebrMaestroController *mc,
                    GebrMaestroServer *maestro,
                    gboolean emit_daemon_changed)
{
	if (!mc->priv->builder)
		return;

	GebrCommServer *server = gebr_maestro_server_get_server(maestro);
	GebrCommServerState state = gebr_comm_server_get_state(server);
	GtkTreeView *view = GTK_TREE_VIEW(gtk_builder_get_object(mc->priv->builder, "treeview_servers"));

	if (state == SERVER_STATE_DISCONNECTED)
		gtk_list_store_clear(mc->priv->model);
	else if (state == SERVER_STATE_LOGGED) {
		if (emit_daemon_changed)
			on_daemons_changed(maestro, mc);
		else
			gebr_maestro_controller_update_daemon_model(maestro, mc);
	}

	gtk_tree_view_set_model(view, GTK_TREE_MODEL(mc->priv->model));
}

static void
gebr_maestro_controller_maestro_state_changed_real(GebrMaestroController *mc,
						   GebrMaestroServer *maestro)
{
	update_maestro_view(mc, maestro, TRUE);
	project_line_info_update();
}

static void 
on_state_change(GebrMaestroServer *maestro,
		GebrMaestroController *self)
{
	g_signal_emit(self, signals[MAESTRO_STATE_CHANGED], 0, maestro);
}

void
gebr_maestro_controller_stop(GebrMaestroController *self,
                             const gchar *address)
{
	GebrMaestroServer *maestro = self->priv->maestro;
	const gchar *maestro_addr = gebr_maestro_server_get_address(maestro);

	if (g_strcmp0(maestro_addr, address) != 0)
		return;

	GebrCommServer *server = gebr_maestro_server_get_server(maestro);
	gebr_comm_server_kill(server);

	gebr_maestro_server_set_error(maestro, "error:stop", NULL);
	on_maestro_error(maestro, maestro_addr, "error:stop", NULL, self);
}

void
gebr_maestro_controller_connect(GebrMaestroController *self,
				const gchar *address)
{
	GebrMaestroServer *maestro;

	if (self->priv->maestro) {
		GebrCommServerState state = gebr_maestro_server_get_state(self->priv->maestro);
		if (g_strcmp0(address, gebr_maestro_server_get_address(self->priv->maestro)) == 0 &&
		    (state == SERVER_STATE_CONNECT || state == SERVER_STATE_LOGGED))
			return;

		gebr_maestro_server_disconnect(self->priv->maestro, FALSE);
		g_object_unref(self->priv->maestro);
	}

	maestro = gebr_maestro_server_new(address);
	self->priv->maestro = maestro;

	g_signal_connect(maestro, "job-define",
			 G_CALLBACK(on_job_define), self);
	g_signal_connect(maestro, "group-changed",
			 G_CALLBACK(on_server_group_changed), self);
	g_signal_connect(maestro, "question-request",
			 G_CALLBACK(on_question_request), self);
	g_signal_connect(maestro, "password-request",
			 G_CALLBACK(on_password_request), self);
	g_signal_connect(maestro, "daemons-changed",
			 G_CALLBACK(on_daemons_changed), self);
	g_signal_connect(maestro, "state-change",
			 G_CALLBACK(on_state_change), self);
	g_signal_connect(maestro, "ac-change",
			 G_CALLBACK(on_ac_change), self);
	g_signal_connect(maestro, "mpi-changed",
			 G_CALLBACK(on_mpi_change), self);
	g_signal_connect(maestro, "daemon-error",
			 G_CALLBACK(on_daemon_error), self);
	g_signal_connect(maestro, "maestro-error",
			 G_CALLBACK(on_maestro_error), self);
	g_signal_connect(maestro, "confirm",
	                 G_CALLBACK(on_maestro_confirm), self);

	g_signal_emit(self, signals[MAESTRO_LIST_CHANGED], 0);

	gebr_maestro_server_connect(self->priv->maestro);
//	gebr_config_save(FALSE);
}

GebrMaestroServer *
gebr_maestro_controller_get_maestro(GebrMaestroController *self)
{
	return self->priv->maestro;
}

GebrMaestroServer *
gebr_maestro_controller_get_maestro_for_address(GebrMaestroController *mc,
						const gchar *address)
{
	if (!mc->priv->maestro)
		return NULL;
	const gchar *addr = gebr_maestro_server_get_address(mc->priv->maestro);

	if (g_strcmp0(addr, address) == 0)
		return mc->priv->maestro;

	return NULL;
}

GebrMaestroServer *
gebr_maestro_controller_get_maestro_for_line(GebrMaestroController *mc,
					     GebrGeoXmlLine *line)
{
	if (!line)
		return NULL;

	gchar *address = gebr_geoxml_line_get_maestro(line);
	GebrMaestroServer *maestro =
		gebr_maestro_controller_get_maestro_for_address(mc, address);
	g_free(address);
	return maestro;
}

void
gebr_maestro_controller_set_window(GebrMaestroController *mc,
				   GtkWindow *window)
{
	g_object_set(mc, "window", window, NULL);
}

GtkWindow *
gebr_maestro_controller_get_window(GebrMaestroController *mc)
{
	return mc->priv->window;
}
GtkTreeModel *
gebr_maestro_controller_get_servers_model(GebrMaestroController *mc)
{
	return GTK_TREE_MODEL(mc->priv->model);
}
