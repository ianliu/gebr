/*
 * gebrm-daemon.c
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

#include "gebrm-daemon.h"
#include "gebrm-marshal.h"
#include <stdlib.h>
#include "gebrm-job.h"
#include "gebrm-task.h"

struct _GebrmDaemonPriv {
	gboolean is_initialized;

	GHashTable *tasks;

	GTree *tags;
	GebrCommServer *server;
	GebrCommProtocolSocket *client;
	gchar *ac;

	gchar *nfsid;
	gchar *id;

	gint uncompleted_tasks;
};

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_SERVER,
	PROP_NFSID,
	PROP_CLOCK,
	PROP_NCORES,
};

enum {
	STATE_CHANGE,
	TASK_DEFINE,
	DAEMON_INIT,
	PORT_DEFINE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(GebrmDaemon, gebrm_daemon, G_TYPE_OBJECT);

static void
gebrm_daemon_set_nfsid(GebrmDaemon *daemon,
		       const gchar *nfsid)
{
	g_return_if_fail(GEBRM_IS_DAEMON(daemon));
	g_return_if_fail(daemon->priv->nfsid == NULL);

	daemon->priv->nfsid = g_strdup(nfsid);
	g_object_notify(G_OBJECT(daemon), "nfsid");
}

static void
gebrm_daemon_set_clock_cpu(GebrmDaemon *daemon,
			   gdouble clock)
{
	daemon->priv->server->clock_cpu = clock;
	g_object_notify(G_OBJECT(daemon), "cpu-clock");
}

static void
gebrm_daemon_set_ncores(GebrmDaemon *daemon,
			gint ncores)
{
	daemon->priv->server->ncores = ncores;
	g_object_notify(G_OBJECT(daemon), "cores");
}

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

	if (server->state == SERVER_STATE_DISCONNECTED) {
		daemon->priv->is_initialized = FALSE;
		daemon->priv->uncompleted_tasks = 0;
	}

	g_signal_emit(daemon, signals[STATE_CHANGE], 0, server->state);
}

static GString *
gebrm_server_op_ssh_login(GebrCommServer *server,
			  const gchar *title,
			  const gchar *message,
			  gpointer user_data)
{
	GebrmDaemon *daemon = user_data;

	if (daemon->priv->client)
		gebr_comm_protocol_socket_oldmsg_send(daemon->priv->client, FALSE,
						      gebr_comm_protocol_defs.pss_def, 1,
						      server->address->str);
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
gebrm_daemon_on_task_status_change(GebrmTask *task,
				   gint old_status,
				   gint new_status,
				   const gchar *parameter,
				   GebrmDaemon *daemon)
{
	switch (new_status) {
	case JOB_STATUS_FAILED:
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
		daemon->priv->uncompleted_tasks--;
		break;

	case JOB_STATUS_INITIAL:
	case JOB_STATUS_QUEUED:
	case JOB_STATUS_RUNNING:
	case JOB_STATUS_ISSUED:
	case JOB_STATUS_REQUEUED:
		break;
	}
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

		if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			if (server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.ini_def.code_hash) {
				GList *arguments;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 9)) == NULL)
					goto err;

				GString *hostname     = g_list_nth_data(arguments, 0);
				gchar  **accounts     = g_strsplit(((GString *)g_list_nth_data(arguments, 2))->str, ",", 0);
				//GString *model_name   = g_list_nth_data (arguments, 3);
				//GString *total_memory = g_list_nth_data (arguments, 4);
				GString *nfsid        = g_list_nth_data (arguments, 5);
				GString *ncores       = g_list_nth_data (arguments, 6);
				GString *clock_cpu    = g_list_nth_data (arguments, 7);
				GString *daemon_id    = g_list_nth_data (arguments, 8);

				daemon->priv->is_initialized = TRUE;

				server->socket->protocol->hostname = g_string_assign(server->socket->protocol->hostname, hostname->str);
				gebrm_daemon_set_ncores(daemon, atoi(ncores->str));
				gebrm_daemon_set_clock_cpu(daemon, atof(clock_cpu->str));
				server->socket->protocol->logged = TRUE;
				gebrm_daemon_set_nfsid(daemon, nfsid->str);
				gebrm_daemon_set_id(daemon, daemon_id->str);

				g_signal_emit(daemon, signals[DAEMON_INIT], 0, NULL, NULL);

				g_strfreev(accounts);
				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
			else if (server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.gid_def.code_hash) {
				GList *arguments;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
					goto err;

				GString *gid  = g_list_nth_data(arguments, 0);
				GString *port = g_list_nth_data(arguments, 1);

				g_debug("Got port %s for gid %s", port->str, gid->str);

				g_signal_emit(daemon, signals[PORT_DEFINE], 0, gid->str, port->str);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
		}
		else if (message->hash == gebr_comm_protocol_defs.err_def.code_hash) {
				GList *arguments;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
					goto err;

				GString *error_type = g_list_nth_data(arguments, 0);
				GString *error_msg  = g_list_nth_data(arguments, 1);

				if (!daemon->priv->is_initialized)
					g_signal_emit(daemon, signals[DAEMON_INIT], 0,
						      error_type->str, error_msg->str);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
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

			daemon->priv->uncompleted_tasks++;
			GebrmTask *task = gebrm_task_new(daemon, id->str, frac->str);
			g_signal_connect(task, "status-change",
					 G_CALLBACK(gebrm_daemon_on_task_status_change), daemon);
			gebrm_task_init_details(task, issues, cmd, moab_jid);

			g_hash_table_insert(daemon->priv->tasks, gebrm_task_build_id(id->str, frac->str), task);
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

		if (gebrm_daemon_get_state(daemon) == SERVER_STATE_DISCONNECTED)
			return;

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
gebrm_daemon_get_property(GObject    *object,
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
	case PROP_NFSID:
		g_value_set_string(value, gebrm_daemon_get_nfsid(daemon));
		break;
	case PROP_CLOCK:
		g_value_set_double(value, gebrm_daemon_get_clock(daemon));
		break;
	case PROP_NCORES:
		g_value_set_int(value, gebrm_daemon_get_ncores(daemon));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
on_password_request(GebrCommServer *server,
		    const gchar *title,
		    const gchar *question,
		    GebrmDaemon *daemon)
{
	if (daemon->priv->client)
		gebr_comm_protocol_socket_oldmsg_send(daemon->priv->client, FALSE,
						      gebr_comm_protocol_defs.pss_def, 1,
						      server->address->str);
}

static void
on_question_request(GebrCommServer *server,
		    const gchar *title,
		    const gchar *question,
		    GebrmDaemon *daemon)
{
	if (daemon->priv->client) {
		gebr_comm_protocol_socket_oldmsg_send(daemon->priv->client, FALSE,
						      gebr_comm_protocol_defs.qst_def, 3,
						      server->address->str, title, question);
	}
}

static void
gebrm_daemon_set_property(GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		daemon->priv->server = gebr_comm_server_new(g_value_get_string(value),
							    NULL, &daemon_ops);
		g_signal_connect(daemon->priv->server, "password-request",
				 G_CALLBACK(on_password_request), daemon);
		g_signal_connect(daemon->priv->server, "question-request",
				 G_CALLBACK(on_question_request), daemon);
		gebr_comm_server_set_interactive(daemon->priv->server, TRUE);
		daemon->priv->server->user_data = daemon;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebrm_daemon_finalize(GObject *object)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	gebr_comm_server_disconnect(daemon->priv->server);
	gebr_comm_server_free(daemon->priv->server);
	g_free(daemon->priv->nfsid);
	g_free(daemon->priv->id);
	g_hash_table_destroy(daemon->priv->tasks);
	if (daemon->priv->client)
		g_object_unref(daemon->priv->client);

	G_OBJECT_CLASS(gebrm_daemon_parent_class)->finalize(object);
}

static void
gebrm_daemon_class_init(GebrmDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebrm_daemon_get_property;
	object_class->set_property = gebrm_daemon_set_property;
	object_class->finalize = gebrm_daemon_finalize;

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

	signals[DAEMON_INIT] =
		g_signal_new("daemon-init",
			     G_OBJECT_CLASS_TYPE (object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmDaemonClass, daemon_init),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

	signals[PORT_DEFINE] =
		g_signal_new("port-define",
			     G_OBJECT_CLASS_TYPE (object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmDaemonClass, port_define),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE,
			     2, G_TYPE_STRING, G_TYPE_STRING);

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

	g_object_class_install_property(object_class,
					PROP_NFSID,
					g_param_spec_string("nfsid",
							    "NFS id",
							    "The daemon's NFS id",
							    NULL,
							    G_PARAM_READABLE));

	g_object_class_install_property(object_class,
					PROP_CLOCK,
					g_param_spec_double("cpu-clock",
							    "CPU Clock",
							    "The daemon's CPU Clock",
							    0, G_MAXDOUBLE,
							    0,
							    G_PARAM_READABLE));

	g_object_class_install_property(object_class,
					PROP_NCORES,
					g_param_spec_int("cores",
							 "n-cores",
							 "Number of daemon's cores",
							 0, G_MAXINT,
							 0,
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
	daemon->priv->ac = g_strdup("on");
	daemon->priv->is_initialized = FALSE;
	daemon->priv->tasks = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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
	g_tree_insert(daemon->priv->tags, g_strdup(tag),
		      GINT_TO_POINTER(1));
}

gboolean
gebrm_daemon_update_tags(GebrmDaemon *daemon, gchar **tagsv)
{
	
	if (!tagsv)
		return FALSE;

	g_tree_unref(daemon->priv->tags);
	daemon->priv->tags = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					     NULL, g_free, NULL);

	for (int i = 0; tagsv[i]; i++)
		gebrm_daemon_add_tag(daemon, tagsv[i]);

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

	if (buf->len > 0)
		g_string_erase(buf, buf->len - 1, 1);

	return g_string_free(buf, FALSE);
}

gboolean
gebrm_daemon_has_group(GebrmDaemon *daemon,
                       const gchar *group)
{
	if (!*group)
		return TRUE;

	gboolean has_group = FALSE;
	gchar *tags = gebrm_daemon_get_tags(daemon);
	gchar **split = g_strsplit(tags, ",", -1);

	for (int i = 0; split[i]; i++)
		if (g_strcmp0(group, split[i]) == 0)
			has_group = TRUE;

	g_free(tags);
	g_free(split);
	return has_group;
}

void
gebrm_daemon_connect(GebrmDaemon *daemon,
		     const gchar *pass,
		     GebrCommProtocolSocket *client)
{
	if (pass && *pass)
		gebr_comm_server_set_password(daemon->priv->server, pass);

	if (daemon->priv->client)
		g_object_unref(daemon->priv->client);

	if (client)
		daemon->priv->client = g_object_ref(client);

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

GebrCommServerState
gebrm_daemon_get_state(GebrmDaemon *daemon)
{
	return daemon->priv->server->state;
}

void
gebrm_daemon_set_autoconnect(GebrmDaemon *daemon, const gchar *ac)
{
	if (daemon->priv->ac)
		g_free(daemon->priv->ac);
	daemon->priv->ac = g_strdup(ac);
}

const gchar *
gebrm_daemon_get_autoconnect(GebrmDaemon *daemon)
{
	return daemon->priv->ac;
}

const gchar *
gebrm_daemon_get_nfsid(GebrmDaemon *daemon)
{
	return daemon->priv->nfsid;
}

gint
gebrm_daemon_get_ncores(GebrmDaemon *daemon)
{
	return daemon->priv->server->ncores;
}

gdouble
gebrm_daemon_get_clock(GebrmDaemon *daemon)
{
	return daemon->priv->server->clock_cpu;
}

void
gebrm_daemon_list_tasks_and_forward_x(GebrmDaemon *daemon)
{
	gebr_comm_protocol_socket_oldmsg_send(daemon->priv->server->socket, FALSE,
					      gebr_comm_protocol_defs.lst_def, 0);
}

void
gebrm_daeamon_answer_question(GebrmDaemon *daemon,
			      const gchar *resp)
{
	gboolean response = g_strcmp0(resp, "true") == 0;
	gebr_comm_server_answer_question(daemon->priv->server, response);
}

void
gebrm_daemon_continue_stuck_connection(GebrmDaemon *daemon,
				       GebrCommProtocolSocket *socket)
{
	g_return_if_fail(daemon->priv->client == NULL);
	g_return_if_fail(gebrm_daemon_get_state(daemon) == SERVER_STATE_RUN);

	daemon->priv->client = g_object_ref(socket);
	gebr_comm_server_emit_interactive_state_signals(daemon->priv->server);
}

void
gebrm_daemon_set_id(GebrmDaemon *daemon,
		    const gchar *id)
{
	g_return_if_fail(daemon->priv->id == NULL);
	daemon->priv->id = g_strdup(id);
}

const gchar *
gebrm_daemon_get_id(GebrmDaemon *daemon)
{
	return daemon->priv->id;
}

const gchar *
gebrm_daemon_get_error(GebrmDaemon *daemon)
{
	return gebr_comm_server_get_last_error(daemon->priv->server);
}

gint
gebrm_daemon_get_uncompleted_tasks(GebrmDaemon *daemon)
{
	return daemon->priv->uncompleted_tasks;
}

GList *
gebrm_daemon_get_list_of_jobs(GebrmDaemon *daemon)
{
	GList *jobs = NULL;

	void get_jobs(gpointer key, gpointer value)
	{
		GebrmTask *task = value;

		jobs = g_list_prepend(jobs, g_strdup(gebrm_task_get_job_id(task)));
	}

	g_hash_table_foreach(daemon->priv->tasks, (GHFunc)get_jobs, NULL);

	return jobs;
}

void
gebrm_daemon_send_client_info(GebrmDaemon *daemon,
			      const gchar *id,
			      const gchar *cookie)
{
	gebr_comm_protocol_socket_oldmsg_send(daemon->priv->server->socket, FALSE,
					      gebr_comm_protocol_defs.gid_def, 2,
					      id, cookie);
}

const gchar *
gebrm_daemon_get_hostname(GebrmDaemon *daemon)
{
	return daemon->priv->server->socket->protocol->hostname->str;
}
