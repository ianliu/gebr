/*
 * gebrm-daemon.c
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

#include "gebrm-daemon.h"
#include "gebrm-marshal.h"
#include <stdlib.h>
#include "gebrm-job.h"
#include "gebrm-task.h"

struct _GebrmDaemonPriv {
	GTree *tags;
	GebrCommServer *server;
};

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_SERVER,
};

enum {
	STATE_CHANGE,
	TASK_DEFINE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(GebrmDaemon, gebrm_daemon, G_TYPE_OBJECT);

/* Operations for GebrCommServer {{{ */
static void
gebrm_server_op_log_message(GebrCommServer *server,
			    GebrLogMessageType type,
			    const gchar *message,
			    gpointer user_data)
{
	g_debug("[DAEMON] %s: (type: %d) %s", __func__, type, message);
}

static void
gebrm_server_op_state_changed(GebrCommServer *server,
			      gpointer user_data)
{
	GebrmDaemon *daemon = user_data;
	g_signal_emit(daemon, signals[STATE_CHANGE], 0, server->state);
}

static GString *
gebrm_server_op_ssh_login(GebrCommServer *server,
			  const gchar *title,
			  const gchar *message,
			  gpointer user_data)
{
	g_debug("[DAEMON] %s: Login %s %s", __func__, title, message);
	return NULL;
}

static gboolean
gebrm_server_op_ssh_question(GebrCommServer *server,
			     const gchar *title,
			     const gchar *message,
			     gpointer user_data)
{
	g_debug("[DAEMON] %s: Question %s %s", __func__, title, message);
	return TRUE;
}

static void
gebrm_server_op_process_request(GebrCommServer *server,
				GebrCommHttpMsg *request,
				gpointer user_data)
{
	g_debug("[DAEMON] %s", __func__);
}

static void
gebrm_server_op_process_response(GebrCommServer *server,
				 GebrCommHttpMsg *request,
				 GebrCommHttpMsg *response,
				 gpointer user_data)
{
	g_debug("[DAEMON] %s", __func__);
}

static void
gebrm_server_op_parse_messages(GebrCommServer *server,
			       gpointer user_data)
{
	GList *link;
	struct gebr_comm_message *message;
	GebrmDaemon *daemon = user_data;

	while ((link = g_list_last(server->socket->protocol->messages)) != NULL) {
		message = link->data;

		if (message->hash == gebr_comm_protocol_defs.err_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *last_error = g_list_nth_data(arguments, 0);
			g_critical("Server '%s' reported the error '%s'.",
				   server->address->str, last_error->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			if (server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.ini_def.code_hash) {
				GList *arguments;
				GString *hostname;
				GString *display_port;
				gchar ** accounts;
				GString *model_name;
				GString *total_memory;
				GString *nfsid;
				GString *ncores;
				GString *clock_cpu;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 9)) == NULL)
					goto err;

				hostname = g_list_nth_data(arguments, 0);
				display_port = g_list_nth_data(arguments, 1);
				accounts = g_strsplit(((GString *)g_list_nth_data(arguments, 3))->str, ",", 0);
				model_name = g_list_nth_data (arguments, 4);
				total_memory = g_list_nth_data (arguments, 5);
				nfsid = g_list_nth_data (arguments, 6);
				ncores = g_list_nth_data (arguments, 7);
				clock_cpu = g_list_nth_data (arguments, 8);

				g_strfreev(accounts);

				server->ncores = atoi(ncores->str);
				server->clock_cpu = atof(clock_cpu->str);

				/* say we are logged */
				server->socket->protocol->logged = TRUE;
				g_string_assign(server->socket->protocol->hostname, hostname->str);

				if (!gebr_comm_server_is_local(server)) {
					if (display_port->len) {
						if (!strcmp(display_port->str, "0"))
							g_critical("Server '%s' could not add X11 authorization to redirect graphical output.",
								   server->address->str);
						else
							gebr_comm_server_forward_x11(server, atoi(display_port->str));
					} else 
						g_critical("Server '%s' could not redirect graphical output.",
							   server->address->str);
				}

				gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
								      gebr_comm_protocol_defs.lst_def, 0);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
		}
		else if (message->hash == gebr_comm_protocol_defs.tsk_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 5)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *issues = g_list_nth_data(arguments, 2);
			GString *cmd = g_list_nth_data(arguments, 3);
			GString *moab_jid = g_list_nth_data(arguments, 4);

			GebrmTask *task = gebrm_task_new(daemon, id->str, frac->str);
			gebrm_task_init_details(task, issues, cmd, moab_jid);
			g_signal_emit(daemon, signals[TASK_DEFINE], 0, task);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *output, *rid, *frac;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;

			output = g_list_nth_data(arguments, 1);
			rid = g_list_nth_data(arguments, 2);
			frac = g_list_nth_data(arguments, 3);

			GebrmTask *task = gebrm_task_find(rid->str, frac->str);
			gebrm_task_emit_output_signal(task, output->str);

			g_debug("[%s] OUT_DEF! %s", server->address->str, output->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;
			GString *status, *parameter, *rid, *frac;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 5)) == NULL)
				goto err;

			status = g_list_nth_data(arguments, 1);
			parameter = g_list_nth_data(arguments, 2);
			rid = g_list_nth_data(arguments, 3);
			frac = g_list_nth_data(arguments, 4);

			GebrmTask *task = gebrm_task_find(rid->str, frac->str);
			gebrm_task_emit_status_changed_signal(task, gebrm_task_translate_status(status),
							      parameter->str);

			g_debug("[%s] STA_DEF! %s", server->address->str, status->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		server->socket->protocol->messages = g_list_delete_link(server->socket->protocol->messages, link);
	}

	return;

err:	gebr_comm_message_free(message);
	server->socket->protocol->messages = g_list_delete_link(server->socket->protocol->messages, link);

	if (gebr_comm_server_is_local(server))
		g_critical("Error communicating with the local server. Please reconnect.");
	else
		g_critical("Error communicating with the server '%s'. Please reconnect.", server->address->str);
	gebr_comm_server_disconnect(server);

}

static struct gebr_comm_server_ops daemon_ops = {
	.log_message      = gebrm_server_op_log_message,
	.state_changed    = gebrm_server_op_state_changed,
	.ssh_login        = gebrm_server_op_ssh_login,
	.ssh_question     = gebrm_server_op_ssh_question,
	.process_request  = gebrm_server_op_process_request,
	.process_response = gebrm_server_op_process_response,
	.parse_messages   = gebrm_server_op_parse_messages
};
/* }}} Operations for GebrCommServer */

static void
gebrm_daemon_get(GObject    *object,
		 guint       prop_id,
		 GValue     *value,
		 GParamSpec *pspec)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		g_value_set_string(value, gebrm_daemon_get_address(daemon));
		break;
	case PROP_SERVER:
		g_value_set_pointer(value, daemon->priv->server);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebrm_daemon_set(GObject      *object,
		 guint         prop_id,
		 const GValue *value,
		 GParamSpec   *pspec)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		daemon->priv->server = gebr_comm_server_new(g_value_get_string(value),
							    &daemon_ops);
		daemon->priv->server->user_data = daemon;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebrm_daemon_class_init(GebrmDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebrm_daemon_get;
	object_class->set_property = gebrm_daemon_set;

	signals[STATE_CHANGE] =
		g_signal_new("state-change",
			     G_OBJECT_CLASS_TYPE (object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmDaemonClass, state_change),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__INT,
			     G_TYPE_NONE,
			     1, G_TYPE_INT);

	signals[TASK_DEFINE] =
		g_signal_new("task-define",
			     G_OBJECT_CLASS_TYPE (object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmDaemonClass, task_define),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__OBJECT,
			     G_TYPE_NONE,
			     1, GEBRM_TYPE_TASK);

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Server address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(object_class,
					PROP_SERVER,
					g_param_spec_pointer("server",
							     "Server",
							     "Server",
							     G_PARAM_READABLE));

	g_type_class_add_private(klass, sizeof(GebrmDaemonPriv));
}

static void
gebrm_daemon_init(GebrmDaemon *daemon)
{
	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon,
						   GEBRM_TYPE_DAEMON,
						   GebrmDaemonPriv);
	daemon->priv->tags = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					     NULL, g_free, NULL);
}

GebrmDaemon *
gebrm_daemon_new(const gchar *address)
{
	return g_object_new(GEBRM_TYPE_DAEMON,
			    "address", address,
			    NULL);
}

const gchar *
gebrm_daemon_get_address(GebrmDaemon *daemon)
{
	return daemon->priv->server->address->str;
}

void
gebrm_daemon_add_tag(GebrmDaemon *daemon,
		     const gchar *tag)
{
	g_debug("Adding %s", tag);
	g_tree_insert(daemon->priv->tags, g_strdup(tag),
		      GINT_TO_POINTER(1));
	
}

gboolean
gebrm_daemon_update_tags(GebrmDaemon *daemon,
		     gchar **tagsv){
	
	if (!tagsv)
		return FALSE;

	g_tree_unref(daemon->priv->tags);
	daemon->priv->tags = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					     NULL, g_free, NULL);

		g_debug("On %s, Updating daemon %s", __func__, gebrm_daemon_get_address(daemon));
	for (int i = 0; tagsv[i]; i++){
		gebrm_daemon_add_tag(daemon, tagsv[i]);
	}

	return TRUE;
}
gboolean
gebrm_daemon_has_tag(GebrmDaemon *daemon,
		     const gchar *tag)
{
	return g_tree_lookup(daemon->priv->tags, tag) != NULL;
}

gchar *
gebrm_daemon_get_tags(GebrmDaemon *daemon)
{
	gboolean traverse(gpointer key,
			  gpointer value,
			  gpointer data)
	{
		GString *b = data;
		gchar *tag = key;
		g_string_append_printf(b, "%s,", tag);
		return FALSE;
	}

	GString *buf = g_string_new("");
	g_tree_foreach(daemon->priv->tags, traverse, buf);
	g_string_erase(buf, buf->len - 1, 1);
	return g_string_free(buf, FALSE);
}

void
gebrm_daemon_connect(GebrmDaemon *daemon)
{
	gebr_comm_server_connect(daemon->priv->server, FALSE);
}

void
gebrm_daemon_disconnect(GebrmDaemon *daemon)
{
	gebr_comm_server_disconnect(daemon->priv->server);
}

GebrCommServer *
gebrm_daemon_get_server(GebrmDaemon *daemon)
{
	return daemon->priv->server;
}
