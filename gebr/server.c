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

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <libgebr/comm/gebr-comm-streamsocket.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "server.h"
#include "gebr.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_AUTOCONNECT,
	PROP_TAGS,
};

enum {
	RET_INI,
	LAST_SIGNAL
};

static guint signals[ LAST_SIGNAL ] = { 0, };


static void
gebr_server_set_property (GObject         *object,
			 guint            prop_id,
                         const GValue    *value,
                         GParamSpec      *pspec)
{
	GebrServer *self = GEBR_SERVER (object);
	switch (prop_id) {
	case PROP_ADDRESS:
		gtk_list_store_set (gebr.ui_server_list->common.store, &self->iter,
				    SERVER_NAME, server_get_name_from_address (g_value_get_string (value)),
				    -1);
		break;
	case PROP_AUTOCONNECT:
		gebr_server_set_autoconnect (self, g_value_get_boolean (value));
		break;
	case PROP_TAGS:
		gtk_list_store_set (gebr.ui_server_list->common.store, &self->iter,
				    SERVER_TAGS, g_value_get_string (value),
				    -1);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_server_get_property (GObject         *object,
                         guint            prop_id,
                         GValue          *value,
                         GParamSpec      *pspec)
{
	gchar *tags;
	gchar *address;
	GebrServer *self = GEBR_SERVER (object);
	GtkTreeModel *model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	switch (prop_id) {
	case PROP_ADDRESS:
		gtk_tree_model_get (model, &self->iter,
				    SERVER_NAME, &address,
				    -1);
		g_value_take_string (value, address);
		break;
	case PROP_AUTOCONNECT:
		g_value_set_boolean (value, gebr_server_get_autoconnect (self));
		break;
	case PROP_TAGS:
		gtk_tree_model_get (model, &self->iter,
				    SERVER_TAGS, &tags,
				    -1);
		g_value_take_string (value, tags);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

G_DEFINE_TYPE (GebrServer, gebr_server, G_TYPE_OBJECT);

static void gebr_server_class_init (GebrServerClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->set_property = gebr_server_set_property;
	gobject_class->get_property = gebr_server_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_ADDRESS,
					 g_param_spec_string ("address",
							      "Address",
							      "Server address",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class,
					 PROP_AUTOCONNECT,
					 g_param_spec_boolean ("autoconnect",
							       "Autoconnect",
							       "Autoconnect server",
							       TRUE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TAGS,
					 g_param_spec_string ("tags",
							      "Tags",
							      "Server tags",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	signals[ RET_INI ] =
		g_signal_new ("initialized",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GebrServerClass, initialized),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void gebr_server_init (GebrServer *self)
{
	GtkTreeIter iter;

	gtk_list_store_append (gebr.ui_server_list->common.store, &iter);

	self->comm = NULL;
	self->iter = iter;
	self->nfsid = g_string_new ("");
	self->last_error = g_string_new ("");
	self->type = GEBR_COMM_SERVER_TYPE_UNKNOWN;
	self->accounts_model = gtk_list_store_new (1, G_TYPE_STRING);
	self->queues_model = gtk_list_store_new (SERVER_QUEUE_N_COLUMNS,
						 G_TYPE_STRING,
						 G_TYPE_STRING,
						 G_TYPE_POINTER);

	gtk_list_store_set (gebr.ui_server_list->common.store, &iter,
			    SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
			    SERVER_POINTER,     self,
			    -1);
}

/**
 * \internal
 */
static void server_log_message(enum gebr_log_message_type type, const gchar * message)
{
	gebr_message(type, TRUE, TRUE, message);
}

static void server_clear_jobs(GebrServer * server);

/*
 * \internal
 */
static void server_state_changed(struct gebr_comm_server *comm_server, GebrServer * server)
{
	server_list_updated_status(server);
	if (server->comm->state == SERVER_STATE_DISCONNECTED) {
		gtk_list_store_clear(server->accounts_model);
		gtk_list_store_clear(server->queues_model);
		server_clear_jobs(server);
	}
}

/**
 * \internal
 */
static GString *server_ssh_login(const gchar * title, const gchar * message)
{

	GtkWidget *dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(gebr.window),
							(GtkDialogFlags)(GTK_DIALOG_MODAL |
									 GTK_DIALOG_DESTROY_WITH_PARENT),
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
							GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	GtkWidget *label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	gtk_widget_show_all(dialog);
	gboolean confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;
	GString *password = !confirmed ? NULL : g_string_new(gtk_entry_get_text(GTK_ENTRY(entry)));

	gtk_widget_destroy(dialog);

	return password;
}

/**
 * \internal
 */
static gboolean server_ssh_question(const gchar * title, const gchar * message)
{
	return gebr_gui_confirm_action_dialog(title, message);
}

/**
 * \internal
 */
static void server_clear_jobs(GebrServer * server)
{
	gboolean server_free_foreach_job(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		GebrJob *job;
		gchar *server_address;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_SERVER_ADDRESS, &server_address,
				   JC_STRUCT, &job, JC_IS_JOB, &is_job, -1);
		if (strcmp(server_address, server->comm->address->str)) {
			g_free(server_address);
			return FALSE;	
		}
		if (!is_job)
			gtk_tree_store_remove(gebr.ui_job_control->store, iter);
		else if (job != NULL)
			job_delete(job);

		g_free(server_address);
		return FALSE;
	}
	/* delete all jobs at server */
	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)server_free_foreach_job, NULL); 
}

gboolean server_find_address (const gchar *address,
			      GtkTreeIter *iter,
			      const gchar *group)
{
	GtkTreeIter i;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);
	gebr_gui_gtk_tree_model_foreach(i, model) {
		GebrServer *server;

		gtk_tree_model_get(model, &i, SERVER_POINTER, &server, -1);
		/* Tests if there is a server with `address' and `group'.
		 * If `group' is NULL, then search all servers.
		 */
		if (!strcmp (address, server->comm->address->str)
		    && (group == NULL || ui_server_has_tag (server, group))) {
			*iter = i;
			return TRUE;
		}
	}

	return FALSE;
}

const gchar *server_get_name_from_address(const gchar * address)
{
	return !strcmp(address, "127.0.0.1") ? _("Local server") : address;
}

GebrServer *gebr_server_new (const gchar * address, gboolean autoconnect, const gchar* tags)
{
	GebrServer *self;
	GtkTreePath *path;
	GtkTreeModel *model;

	static const struct gebr_comm_server_ops ops = {
		.log_message      = server_log_message,
		.state_changed    = (typeof(ops.state_changed)) server_state_changed,
		.ssh_login        = server_ssh_login,
		.ssh_question     = server_ssh_question,
		.process_request  = (typeof(ops.process_request)) client_process_server_request,
		.process_response = (typeof(ops.process_response)) client_process_server_response,
		.parse_messages   = (typeof(ops.parse_messages)) client_parse_server_messages
	};

	self = g_object_new (GEBR_TYPE_SERVER,
			     "address", address,
			     "autoconnect", autoconnect,
			     "tags", tags,
			     NULL);

	self->comm = gebr_comm_server_new (address, &ops);
	self->comm->user_data = self;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);
	path = gtk_tree_model_get_path (model, &self->iter);
	gtk_tree_model_row_changed (model, path, &self->iter);
	gtk_tree_path_free (path);

	if (autoconnect)
		gebr_comm_server_connect (self->comm);

	return self;
}

void server_free(GebrServer *server)
{
	server_clear_jobs (server);

	gtk_list_store_remove(gebr.ui_server_list->common.store, &server->iter);
	gtk_list_store_clear(server->accounts_model);
	gtk_list_store_clear(server->queues_model);

	gebr_comm_server_free(server->comm);
	g_string_free(server->last_error, TRUE);
	g_string_free(server->nfsid, TRUE);

	g_object_unref (server);
}

const gchar *server_get_name(GebrServer * server)
{
	return server_get_name_from_address(server->comm->address->str);
}

gboolean server_find(GebrServer * server, GtkTreeIter * iter)
{
	GtkTreeIter i;

	gebr_gui_gtk_tree_model_foreach(i, GTK_TREE_MODEL(gebr.ui_server_list->common.store)) {
		GebrServer *i_server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &i,
				   SERVER_POINTER, &i_server, -1);
		if (i_server == server) {
			*iter = i;
			return TRUE;
		}
	}

	return FALSE;
}

gboolean server_queue_find(GebrServer * server, const gchar * name, GtkTreeIter * _iter)
{
	GtkTreeIter iter;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(server->queues_model)) {
		gchar * i_name;

		gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, SERVER_QUEUE_ID, &i_name, -1);
		if (!strcmp(name, i_name)) {
			if (_iter != NULL)
				*_iter = iter;
			g_free(i_name);
			return TRUE;
		}

		g_free(i_name);
	}

	return FALSE;
}

void server_queue_find_at_job_control(GebrServer * server, const gchar * name, GtkTreeIter * _iter)
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(gebr.ui_job_control->store)) {
		gchar *i_name;
		gchar *i_address;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				   JC_SERVER_ADDRESS, &i_address,
				   JC_QUEUE_NAME, &i_name,
				   JC_IS_JOB, &is_job,
				   -1);
		if (!is_job && !strcmp(server->comm->address->str, i_address) && (!strcmp(name, i_name) || 
		    (name[0] == 'j' && i_name[0] == 'j') /* immediately */)) {
			if (_iter != NULL)
				*_iter = iter;
			found = TRUE;
		}
		
		g_free(i_address);
		g_free(i_name);
	}
	if (found)
		return;

	gtk_tree_store_append(gebr.ui_job_control->store, &iter, NULL);
	if (_iter != NULL)
		*_iter = iter;

	GString *title = g_string_new(NULL);
	gboolean immediately = name[0] == 'j';
	g_string_printf(title, _("%s at %s"),
			(server->type == GEBR_COMM_SERVER_TYPE_MOAB) ? name : immediately ? _("Immediately") : name + 1,
			server_get_name_from_address(server->comm->address->str));
	gtk_tree_store_set(gebr.ui_job_control->store, &iter,
			   JC_SERVER_ADDRESS, server->comm->address->str,
			   JC_QUEUE_NAME, name,
			   JC_TITLE, title->str,
			   JC_STRUCT, NULL,
			   JC_IS_JOB, FALSE,
			   -1);
	g_string_free(title, TRUE);
}

void gebr_server_emit_initialized (GebrServer *self)
{
	g_signal_emit (self, signals[ RET_INI ], 0);
}

gboolean gebr_server_get_autoconnect (GebrServer *self)
{
	gboolean setting;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);
	gtk_tree_model_get (model, &self->iter,
			    SERVER_AUTOCONNECT, &setting,
			    -1);
	return setting;
}

void gebr_server_set_autoconnect (GebrServer *self, gboolean setting)
{
	gtk_list_store_set (gebr.ui_server_list->common.store, &self->iter,
			    SERVER_AUTOCONNECT, setting,
			    -1);
	if (setting && self->comm)
		gebr_comm_server_connect (self->comm);
}
