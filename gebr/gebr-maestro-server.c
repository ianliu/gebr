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
#include "gebr-job.h"
#include "gebr.h"

#include <stdlib.h>
#include <libgebr/gui/gui.h>

struct _GebrMaestroServerPriv {
	GebrCommServer *server;
	GtkListStore *store;
	GtkTreeModel *filter;
};

static void      log_message(GebrCommServer *server,
			     GebrLogMessageType type,
			     const gchar *message,
			     gpointer user_data);

static void      state_changed(GebrCommServer *comm_server,
			       gpointer user_data);

static GString  *ssh_login(GebrCommServer *server,
			   const gchar *title,
			   const gchar *message,
			   gpointer user_data);

static gboolean  ssh_question(GebrCommServer *server,
			      const gchar *title,
			      const gchar *message,
			      gpointer user_data);

static void      process_request(GebrCommServer *server,
				 GebrCommHttpMsg *request,
				 gpointer user_data);

static void      process_response(GebrCommServer *server,
				  GebrCommHttpMsg *request,
				  GebrCommHttpMsg *response,
				  gpointer user_data);

static void      parse_messages(GebrCommServer *comm_server,
				gpointer user_data);

static void gebr_maestro_server_connectable_init(GebrConnectableIface *iface);

enum {
	PROP_0,
	PROP_ADDRESS,
};

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

			GString *hostname, *title, *queue, *rid, *server_list,
				*n_procs, *niceness, *input_file, *output_file,
				*log_file, *last_run_date, *server_group_name,
				*exec_speed;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 13)) == NULL)
				goto err;

			rid = g_list_nth_data(arguments, 0);
			n_procs = g_list_nth_data(arguments, 1);
			server_list = g_list_nth_data(arguments, 2);
			hostname = g_list_nth_data(arguments, 3);
			title = g_list_nth_data(arguments, 4);
			queue = g_list_nth_data(arguments, 5);
			niceness = g_list_nth_data(arguments, 6);
			input_file = g_list_nth_data(arguments, 7);
			output_file = g_list_nth_data(arguments, 8);
			log_file = g_list_nth_data(arguments, 9);
			last_run_date = g_list_nth_data(arguments, 10);
			server_group_name = g_list_nth_data(arguments, 11);
			exec_speed = g_list_nth_data(arguments, 12);

			GebrJob *job;

			job = gebr_job_control_find(gebr.job_control, rid->str);

			if (!job) {
				job = gebr_job_new_with_id(rid->str, queue->str);
				gebr_job_set_servers(job, server_list->str);
				gebr_job_set_title(job, title->str);
				gebr_job_set_hostname(job, hostname->str);
				gebr_job_set_server_group(job, server_group_name->str);
				gebr_job_set_exec_speed(job, atoi(exec_speed->str));
				gebr_job_set_io(job, input_file->str, output_file->str, log_file->str);

				gebr_job_set_model(job, gebr_job_control_get_model(gebr.job_control));
				gebr_job_control_add(gebr.job_control, job);
			}

			g_debug("CREATE JOB %s ON GEBR", title->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.iss_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *rid = g_list_nth_data(arguments, 0);
			GString *issues = g_list_nth_data(arguments, 1);
			g_debug("ISSUES from %s: %s", rid->str, issues->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.cmd_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *rid = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *cmd = g_list_nth_data(arguments, 2);

			g_debug("CMDLINE from %s(%s): %s", rid->str, frac->str, cmd->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *output, *rid;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			rid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);
			g_debug("OUTPUT from %s: %s", rid->str, output->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;
			GString *status, *parameter, *rid;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			rid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			parameter = g_list_nth_data(arguments, 2);

			GebrCommJobStatus status_enum;
			status_enum = gebr_comm_job_get_status_from_string(status->str);

			g_debug("STATUS from %s: %s", rid->str, status->str);

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

	G_OBJECT_CLASS(gebr_maestro_server_parent_class)->finalize(object);
}

static void
gebr_maestro_server_class_init(GebrMaestroServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_maestro_server_get;
	object_class->set_property = gebr_maestro_server_set;
	object_class->finalize = gebr_maestro_server_finalize;

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
