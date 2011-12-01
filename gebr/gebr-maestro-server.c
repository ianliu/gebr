/*
 * gebr-maestro-server.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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


#include "gebr-maestro-server.h"

#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <stdlib.h>

struct _GebrMaestroServerPriv {
	GebrCommServer *server;
	GtkListStore *store;
	GtkTreeModel *filter;
	GHashTable *jobs;

	GtkListStore *queues_model;
};

enum {
	JOB_DEFINE,
	GROUP_CHANGED,
	LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

enum {
	PROP_0,
	PROP_ADDRESS,
};

static void log_message(GebrCommServer *server, GebrLogMessageType type,
			     const gchar *message, gpointer user_data);

static void state_changed(GebrCommServer *comm_server,
			       gpointer user_data);

static GString *ssh_login(GebrCommServer *server, const gchar *title,
			   const gchar *message, gpointer user_data);

static gboolean ssh_question(GebrCommServer *server, const gchar *title,
			      const gchar *message, gpointer user_data);

static void process_request(GebrCommServer *server, GebrCommHttpMsg *request,
			    gpointer user_data);

static void process_response(GebrCommServer *server, GebrCommHttpMsg *request,
			     GebrCommHttpMsg *response, gpointer user_data);

static void parse_messages(GebrCommServer *comm_server, gpointer user_data);

static void gebr_maestro_server_connectable_init(GebrConnectableIface *iface);

static const struct gebr_comm_server_ops maestro_ops = {
	.log_message      = log_message,
	.state_changed    = state_changed,
	.ssh_login        = ssh_login,
	.ssh_question     = ssh_question,
	.process_request  = process_request,
	.process_response = process_response,
	.parse_messages   = parse_messages
};

G_DEFINE_TYPE_WITH_CODE(GebrMaestroServer, gebr_maestro_server, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_CONNECTABLE,
					      gebr_maestro_server_connectable_init));

void
log_message(GebrCommServer *server,
	    GebrLogMessageType type,
	    const gchar *message,
	    gpointer user_data)
{
	g_debug("[MAESTRO] LOG_MESSAGE: %s", message);
}

void
state_changed(GebrCommServer *comm_server,
	      gpointer user_data)
{
	g_debug("[MAESTRO] STATUS CHANGED");
}

GString *
ssh_login(GebrCommServer *server,
	  const gchar *title, const gchar *message,
	  gpointer user_data)
{
	g_debug("[MAESTRO] ssh login");

	return g_string_new("");
}

gboolean
ssh_question(GebrCommServer *server,
	     const gchar *title,
	     const gchar *message,
	     gpointer user_data)
{
	g_debug("[MAESTRO] ssh question: %s", message);
	return TRUE;
}

void
process_request(GebrCommServer *server,
		GebrCommHttpMsg *request,
		gpointer user_data)
{
	g_debug("[MAESTRO] request");
}

void
process_response(GebrCommServer *server,
		 GebrCommHttpMsg *request,
		 GebrCommHttpMsg *response,
		 gpointer user_data)
{
	g_debug("[MAESTRO] response");
}

static GebrDaemonServer *
get_daemon_from_address(GebrMaestroServer *server,
			const gchar *address,
			GtkTreeIter *_iter)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(server->priv->store);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		GebrDaemonServer *daemon;
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		if (g_strcmp0(address, gebr_daemon_server_get_address(daemon)) == 0) {
			if (_iter)
				*_iter = iter;
			return daemon;
		}
	}
	return NULL;
}

void
update_queues_model(GebrMaestroServer *maestro, GebrJob *job)
{
	GebrCommJobStatus stat = gebr_job_get_status(job);

	gboolean has_job = FALSE;
	GtkTreeIter iter, found;
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->queues_model);
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		GebrJob *j;
		gtk_tree_model_get(model, &iter, 0, &j, -1);
		if (j == job) {
			has_job = TRUE;
			found = iter;
			break;
		}
	}

	switch (stat) {
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_FAILED:
		if (has_job)
			gtk_list_store_remove(maestro->priv->queues_model, &found);
		break;
	case JOB_STATUS_RUNNING:
	case JOB_STATUS_QUEUED:
		gtk_list_store_append(maestro->priv->queues_model, &iter);
		gtk_list_store_set(maestro->priv->queues_model, &iter, 0, job, -1);
		break;
	default:
		break;
	}
}

void
parse_messages(GebrCommServer *comm_server,
	       gpointer user_data)
{
	g_debug("[MAESTRO] parse messages");
	GList *link;
	struct gebr_comm_message *message;

	GebrMaestroServer *maestro = user_data;

	while ((link = g_list_last(comm_server->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;
		if (message->hash == gebr_comm_protocol_defs.ssta_def.code_hash) {
			g_debug("on function state_changed, ssta_def");
			GList *arguments;
			GString *addr, *ssta;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			addr = g_list_nth_data(arguments, 0);
			ssta = g_list_nth_data(arguments, 1);

			g_debug("addr: %s, ssta:%s",addr->str, ssta->str);

			GebrCommServerState state = gebr_comm_server_state_from_string(ssta->str);

			GebrDaemonServer *daemon = gebr_maestro_server_get_daemon(maestro, addr->str);

			if (!daemon) {
				daemon = gebr_daemon_server_new(GEBR_CONNECTABLE(maestro),
								addr->str, state);
				gebr_maestro_server_add_daemon(maestro, daemon);
			} else
				gebr_daemon_server_set_state(daemon, state);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 16)) == NULL)
				goto err;

			GString *id          = g_list_nth_data(arguments, 0);
			GString *nprocs      = g_list_nth_data(arguments, 1);
			GString *server_list = g_list_nth_data(arguments, 2);
			GString *hostname    = g_list_nth_data(arguments, 3);
			GString *title       = g_list_nth_data(arguments, 4);
			GString *parent_id   = g_list_nth_data(arguments, 5);
			GString *nice        = g_list_nth_data(arguments, 6);
			GString *input       = g_list_nth_data(arguments, 7);
			GString *output      = g_list_nth_data(arguments, 8);
			GString *error       = g_list_nth_data(arguments, 9);
			GString *submit_date = g_list_nth_data(arguments, 10);
			GString *group       = g_list_nth_data(arguments, 11);
			GString *speed       = g_list_nth_data(arguments, 12);
			GString *status      = g_list_nth_data(arguments, 13);
			GString *start_date  = g_list_nth_data(arguments, 14);
			GString *finish_date = g_list_nth_data(arguments, 15);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gboolean init = (job == NULL);

			g_debug(">>>>>>>>>>>>>>> On gebr-maestro-server.c, %s, nprocs: %s", __func__, nprocs->str);

			if (!job)
				job = gebr_job_new_with_id(id->str, parent_id->str);

			/* These properties are computed after Maestro sent the
			 * tasks to its Daemons, so they can not be set before
			 * that. The interface should update this properties.
			 */
			gebr_job_set_servers(job, server_list->str);
			gebr_job_set_submit_date(job, submit_date->str);
			gebr_job_set_nprocs(job, nprocs->str);

			if (init) {
				/* Creates a job and populates some of its information.
				 * This condition happens when GeBR connects to Maestro.
				 */
				gebr_job_set_hostname(job, hostname->str);
				gebr_job_set_title(job, title->str);
				gebr_job_set_nice(job, nice->str);
				gebr_job_set_io(job, input->str, output->str, error->str);
				gebr_job_set_server_group(job, group->str);
				gebr_job_set_exec_speed(job, atoi(speed->str));
				gebr_job_set_static_status(job, gebr_comm_job_get_status_from_string(status->str));
				if (start_date->len > 0)
					gebr_job_set_start_date(job, start_date->str);
				if (finish_date->len > 0)
					gebr_job_set_finish_date(job, finish_date->str);

				g_hash_table_insert(maestro->priv->jobs, g_strdup(id->str), job);
			}

			update_queues_model(maestro, job);

			g_signal_emit(maestro, signals[JOB_DEFINE], 0, job);
			g_debug("CREATE JOB %s ON GEBR", title->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.iss_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *issues = g_list_nth_data(arguments, 1);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_set_issues(job, issues->str);

			g_debug("ISSUES from %s: %s", id->str, issues->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.cmd_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *cmd = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_set_cmd_line(job, atoi(frac->str) - 1, cmd->str);
			g_debug("CMDLINE from %s(%s): %s", id->str, frac->str, cmd->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *output = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_append_output(job, atoi(frac->str) - 1, output->str);
			g_debug("OUTPUT from %s: %s", id->str, output->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *status = g_list_nth_data(arguments, 1);
			GString *parameter = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			GebrCommJobStatus status_enum;
			status_enum = gebr_comm_job_get_status_from_string(status->str);
			gebr_job_set_status(job, gebr_comm_job_get_status_from_string(status->str), parameter->str);

			update_queues_model(maestro, job);

			g_debug("STATUS from %s: %s", id->str, status->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.agrp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *tags = g_list_nth_data(arguments, 1);
			gchar **tagsv = g_strsplit(tags->str, ",", -1);

			g_debug("GEBR RECEIVED addr:%s tags:%s", addr->str, tags->str);

			GtkTreeIter iter;
			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, &iter);
			gebr_daemon_server_set_tags(daemon, tagsv);

			GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(maestro->priv->store), &iter);
			gtk_tree_model_row_changed(GTK_TREE_MODEL(maestro->priv->store), path, &iter);
			gtk_tree_path_free(path);

			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

			g_strfreev(tagsv);
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.dgrp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *group = g_list_nth_data(arguments, 0);

			g_debug("TODO: Impelement or remove? %s", group->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	}

	return;

err:
	gebr_comm_message_free(message);
	comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	gebr_comm_server_disconnect(comm_server);
}


static void
gebr_maestro_server_get(GObject    *object,
			guint       prop_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		g_value_set_string(value, maestro->priv->server->address->str);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_maestro_server_set(GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		maestro->priv->server = gebr_comm_server_new(g_value_get_string(value),
							     &maestro_ops);
		maestro->priv->server->user_data = maestro;
		gebr_comm_server_connect(maestro->priv->server, TRUE);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_maestro_server_finalize(GObject *object)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	gtk_list_store_clear(maestro->priv->store);
	g_object_unref(maestro->priv->store);
	g_object_unref(maestro->priv->filter);
	g_hash_table_unref(maestro->priv->jobs);

	G_OBJECT_CLASS(gebr_maestro_server_parent_class)->finalize(object);
}

static void
gebr_maestro_server_class_init(GebrMaestroServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_maestro_server_get;
	object_class->set_property = gebr_maestro_server_set;
	object_class->finalize = gebr_maestro_server_finalize;

	signals[JOB_DEFINE] =
		g_signal_new("job-define",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, job_define),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__OBJECT,
			     G_TYPE_NONE, 1, GEBR_TYPE_JOB);

	signals[GROUP_CHANGED] =
		g_signal_new("group-changed",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, group_changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Server address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private(klass, sizeof(GebrMaestroServerPriv));
}

static void
gebr_maestro_server_init(GebrMaestroServer *maestro)
{
	maestro->priv = G_TYPE_INSTANCE_GET_PRIVATE(maestro,
						    GEBR_TYPE_MAESTRO_SERVER,
						    GebrMaestroServerPriv);

	maestro->priv->store = gtk_list_store_new(1, G_TYPE_POINTER);
	maestro->priv->jobs = g_hash_table_new(g_str_hash, g_str_equal);
	maestro->priv->queues_model = gtk_list_store_new(1, GEBR_TYPE_JOB);
	// Insert queue Immediately
	GtkTreeIter iter;
	gtk_list_store_append(maestro->priv->queues_model, &iter);
	gtk_list_store_set(maestro->priv->queues_model, &iter, 0, NULL, -1);

	GebrDaemonServer *autochoose =
		gebr_daemon_server_new(GEBR_CONNECTABLE(maestro), NULL, SERVER_STATE_CONNECT);
	gebr_maestro_server_add_daemon(maestro, autochoose);
}

/* Implementation of GebrConnectable interface {{{ */
static void
gebr_maestro_server_connect(GebrConnectable *connectable,
			    const gchar *address)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gchar *url = g_strconcat("/server/", address, NULL);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_disconnect(GebrConnectable *connectable,
			       const gchar *address)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gchar *url = g_strconcat("/disconnect/", address, NULL);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_connectable_init(GebrConnectableIface *iface)
{
	iface->connect = gebr_maestro_server_connect;
	iface->disconnect = gebr_maestro_server_disconnect;
}
/* }}} */

GebrMaestroServer *
gebr_maestro_server_new(const gchar *addr)
{
	return g_object_new(GEBR_TYPE_MAESTRO_SERVER,
			    "address", addr,
			    NULL);
}

void
gebr_maestro_server_add_daemon(GebrMaestroServer *maestro,
			       GebrDaemonServer *daemon)
{
	GtkTreeIter iter;
	gtk_list_store_append(maestro->priv->store, &iter);
	gtk_list_store_set(maestro->priv->store, &iter, 0, daemon, -1);
}

GebrDaemonServer *
gebr_maestro_server_get_daemon(GebrMaestroServer *maestro,
			       const gchar *addr)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);
	const gchar *daddr;

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		daddr = gebr_daemon_server_get_address(daemon);

		if (g_strcmp0(daddr, addr) == 0)
			return daemon;
	}

	return NULL;
}

GebrCommServer *
gebr_maestro_server_get_server(GebrMaestroServer *maestro)
{
	return maestro->priv->server;
}

static gboolean
tree_model_filter_visible_func(GtkTreeModel *model,
			       GtkTreeIter *iter,
			       gpointer data)
{
	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, iter, 0, &daemon, -1);

	return daemon && !gebr_daemon_server_is_autochoose(daemon);
}

GtkTreeModel *
gebr_maestro_server_get_model(GebrMaestroServer *maestro,
			      gboolean include_autochoose)
{
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

	if (include_autochoose)
		return model;

	if (!maestro->priv->filter) {
		maestro->priv->filter = gtk_tree_model_filter_new(model, NULL);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(maestro->priv->filter),
						       tree_model_filter_visible_func, NULL, NULL);
	}

	return maestro->priv->filter;
}

const gchar *
gebr_maestro_server_get_address(GebrMaestroServer *maestro)
{
	return maestro->priv->server->address->str;
}

gchar *
gebr_maestro_server_get_display_address(GebrMaestroServer *maestro)
{
	const gchar *addr = maestro->priv->server->address->str;

	if (g_strcmp0(addr, "127.0.0.1") == 0)
		return g_strdup(_("Local Maestro"));

	return g_strdup_printf(_("Maestro %s"), addr);
}

GList *
gebr_maestro_server_get_all_tags(GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *daemons = gebr_maestro_server_get_model(maestro, FALSE);
	GTree *tree = g_tree_new((GCompareFunc)g_strcmp0);
	GList *groups = NULL;

	gebr_gui_gtk_tree_model_foreach(iter, daemons) {
		GebrDaemonServer *daemon;
		gtk_tree_model_get(daemons, &iter, 0, &daemon, -1);

		GList *tags = gebr_daemon_server_get_tags(daemon);
		for (GList *i = tags; i; i = i->next)
			g_tree_insert(tree, i->data, GUINT_TO_POINTER(TRUE));
	}

	gboolean foreach_func(gpointer key, gpointer value, gpointer data) {
		groups = g_list_prepend(groups, g_strdup((gchar*)(key)));
		return FALSE;
	}

	g_tree_foreach(tree, foreach_func, NULL);
	g_tree_unref(tree);
	return groups;
}

GtkTreeModel *
gebr_maestro_server_get_queues_model(GebrMaestroServer *maestro)
{
	return GTK_TREE_MODEL(maestro->priv->queues_model);
}
