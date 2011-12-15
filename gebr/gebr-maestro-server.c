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

#include "gebr-marshal.h"
#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <stdlib.h>

struct _GebrMaestroServerPriv {
	GebrCommServer *server;
	GtkListStore *store;
	GtkTreeModel *filter;
	GHashTable *jobs;
	GHashTable *temp_jobs;
	gchar *address;

	GtkListStore *groups_store;
	GtkListStore *queues_model;
};

enum {
	JOB_DEFINE,
	GROUP_CHANGED,
	QUESTION_REQUEST,
	PASSWORD_REQUEST,
	DAEMONS_CHANGED,
	STATE_CHANGE,
	AC_CHANGE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

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

static void
update_groups_store(GebrMaestroServer *maestro)
{
	GtkTreeIter iter; 

	gtk_list_store_clear(maestro->priv->groups_store);

	gtk_list_store_append(maestro->priv->groups_store, &iter);
	gtk_list_store_set(maestro->priv->groups_store, &iter,
	                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_GROUP,
	                   MAESTRO_SERVER_NAME, "",
	                   -1);

	GList *groups = gebr_maestro_server_get_all_tags(maestro);

	for (GList *g = groups; g; g = g->next) {
		const gchar *tag = g->data;

		gtk_list_store_append(maestro->priv->groups_store, &iter);
		gtk_list_store_set(maestro->priv->groups_store, &iter,
		                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_GROUP,
		                   MAESTRO_SERVER_NAME, tag,
		                   -1);
	}

	GtkTreeIter daemons_iter;
	GtkTreeModel *daemons_model = gebr_maestro_server_get_model(maestro, FALSE, NULL);

	gebr_gui_gtk_tree_model_foreach(daemons_iter, daemons_model) {
		GebrDaemonServer *daemon;

		gtk_tree_model_get(daemons_model, &daemons_iter,
		                   0, &daemon, -1);

		gtk_list_store_append(maestro->priv->groups_store, &iter);
		gtk_list_store_set(maestro->priv->groups_store, &iter,
		                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_DAEMON,
		                   MAESTRO_SERVER_NAME, gebr_daemon_server_get_address(daemon),
		                   -1);
	}
	g_object_unref(daemons_model);
}

void
log_message(GebrCommServer *server,
	    GebrLogMessageType type,
	    const gchar *message,
	    gpointer user_data)
{
}

void
state_changed(GebrCommServer *comm_server,
	      gpointer user_data)
{
	GebrMaestroServer *maestro = user_data;

	GebrCommServerState state = gebr_comm_server_get_state(comm_server);

	if (state == SERVER_STATE_CONNECT
	    || state == SERVER_STATE_DISCONNECTED)
		g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

	g_signal_emit(maestro, signals[STATE_CHANGE], 0);
}

GString *
ssh_login(GebrCommServer *server,
	  const gchar *title,
	  const gchar *message,
	  gpointer user_data)
{
	gchar *password;
	GebrMaestroServer *maestro = user_data;

	g_signal_emit(maestro, signals[PASSWORD_REQUEST], 0,
		      gebr_maestro_server_get_display_address(maestro), &password);

	return g_string_new(password);
}

gboolean
ssh_question(GebrCommServer *server,
	     const gchar *title,
	     const gchar *question,
	     gpointer user_data)
{
	gboolean *response;
	GebrMaestroServer *maestro = user_data;

	g_signal_emit(maestro, signals[QUESTION_REQUEST], 0,
		      gebr_maestro_server_get_display_address(maestro),
		      title, question, &response);

	return response;
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
}

static GebrDaemonServer *
get_daemon_from_address(GebrMaestroServer *server,
			const gchar *address,
			GtkTreeIter *_iter)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(server->priv->store);

	if (!address)
		return NULL;

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
		if (!has_job) {
			gtk_list_store_append(maestro->priv->queues_model, &iter);
			gtk_list_store_set(maestro->priv->queues_model, &iter, 0, job, -1);
		}
		break;
	default:
		break;
	}
}

void
parse_messages(GebrCommServer *comm_server,
	       gpointer user_data)
{
	GList *link;
	struct gebr_comm_message *message;

	GebrMaestroServer *maestro = user_data;

	while ((link = g_list_last(comm_server->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;
		if (message->hash == gebr_comm_protocol_defs.ssta_def.code_hash) {
			GList *arguments;
			GString *addr, *ssta, *ac;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			addr = g_list_nth_data(arguments, 0);
			ssta = g_list_nth_data(arguments, 1);
			ac = g_list_nth_data(arguments, 2);

			GtkTreeIter iter;
			GebrCommServerState state = gebr_comm_server_state_from_string(ssta->str);
			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, &iter);

			if (!daemon) {
				daemon = gebr_daemon_server_new(GEBR_CONNECTABLE(maestro),
								addr->str, state);
				gebr_maestro_server_add_daemon(maestro, daemon);
			} else {
				GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(maestro->priv->store), &iter);
				gebr_daemon_server_set_state(daemon, state);
				gtk_tree_model_row_changed(GTK_TREE_MODEL(maestro->priv->store), path, &iter);
				gtk_tree_path_free(path);
			}

			gboolean is_ac = g_strcmp0(ac->str, "on") == 0 ? TRUE : FALSE;

			g_debug("SERVER %s RECEIVED MESSAGE OF AC: %s", addr->str, ac->str);

			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);
			g_signal_emit(maestro, signals[DAEMONS_CHANGED], 0);
			g_signal_emit(maestro, signals[AC_CHANGE], 0, is_ac, daemon);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 18)) == NULL)
				goto err;

			GString *id          = g_list_nth_data(arguments, 0);
			GString *temp_id     = g_list_nth_data(arguments, 1);
			GString *nprocs      = g_list_nth_data(arguments, 2);
			GString *server_list = g_list_nth_data(arguments, 3);
			GString *hostname    = g_list_nth_data(arguments, 4);
			GString *title       = g_list_nth_data(arguments, 5);
			GString *parent_id   = g_list_nth_data(arguments, 6);
			GString *nice        = g_list_nth_data(arguments, 7);
			GString *input       = g_list_nth_data(arguments, 8);
			GString *output      = g_list_nth_data(arguments, 9);
			GString *error       = g_list_nth_data(arguments, 10);
			GString *submit_date = g_list_nth_data(arguments, 11);
			GString *group       = g_list_nth_data(arguments, 12);
			GString *group_type  = g_list_nth_data(arguments, 13);
			GString *speed       = g_list_nth_data(arguments, 14);
			GString *status      = g_list_nth_data(arguments, 15);
			GString *start_date  = g_list_nth_data(arguments, 16);
			GString *finish_date = g_list_nth_data(arguments, 17);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);

			if (!job) {
				job = g_hash_table_lookup(maestro->priv->temp_jobs, temp_id->str);
				if (job) {
					g_hash_table_remove(maestro->priv->temp_jobs, temp_id->str);
					gebr_job_set_runid(job, id->str);
					g_hash_table_insert(maestro->priv->jobs, g_strdup(id->str), job);
				}
			}

			gboolean init = (job == NULL);

			if (!job)
				job = gebr_job_new_with_id(id->str, parent_id->str);

			/* These properties are computed after Maestro sent the
			 * tasks to its Daemons, so they can not be set before
			 * that. The interface should update this properties.
			 */
			gebr_job_set_servers(job, server_list->str);
			gebr_job_set_submit_date(job, submit_date->str);
			gebr_job_set_nprocs(job, nprocs->str);
			gebr_job_set_static_status(job, gebr_comm_job_get_status_from_string(status->str));

			if (init) {
				/* Creates a job and populates some of its information.
				 * This condition happens when GeBR connects to Maestro.
				 */
				gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
				gebr_job_set_hostname(job, hostname->str);
				gebr_job_set_title(job, title->str);
				gebr_job_set_nice(job, nice->str);
				gebr_job_set_io(job, input->str, output->str, error->str);
				gebr_job_set_server_group(job, group->str);
				gebr_job_set_server_group_type(job, group_type->str);
				gebr_job_set_exec_speed(job, atoi(speed->str));
				gebr_job_set_static_status(job, gebr_comm_job_get_status_from_string(status->str));
				if (start_date->len > 0)
					gebr_job_set_start_date(job, start_date->str);
				if (finish_date->len > 0)
					gebr_job_set_finish_date(job, finish_date->str);

				g_hash_table_insert(maestro->priv->jobs, g_strdup(id->str), job);
			}

			update_queues_model(maestro, job);

			g_debug("On Job define FROM MAESTRO SERVER!!!!");

			g_signal_emit(maestro, signals[JOB_DEFINE], 0, job);

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

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.agrp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *tags = g_list_nth_data(arguments, 1);
			gchar **tagsv = g_strsplit(tags->str, ",", -1);

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
		else if (message->hash == gebr_comm_protocol_defs.qst_def.code_hash) {
			GList *arguments, *i;

			if ((i = arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *addr = i->data; i = i->next;
			GString *title = i->data; i = i->next;
			GString *question = i->data; i = i->next;
			gboolean *response;

			g_signal_emit(maestro, signals[QUESTION_REQUEST], 0,
				      addr->str, title->str, question->str, &response);

			const gchar *resp = response ? "true" : "false";

			GebrCommUri *uri = gebr_comm_uri_new();
			gebr_comm_uri_set_prefix(uri, "/ssh-answer");
			gebr_comm_uri_add_param(uri, "address", addr->str);
			gebr_comm_uri_add_param(uri, "response", resp);
			gchar *url = gebr_comm_uri_to_string(uri);
			gebr_comm_uri_free(uri);

			gebr_comm_protocol_socket_send_request(comm_server->socket,
							       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
			g_free(url);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.pss_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *addr = arguments->data;
			gchar *password;
			g_signal_emit(maestro, signals[PASSWORD_REQUEST], 0, addr->str, &password);

			if (password) {
				GebrCommUri *uri = gebr_comm_uri_new();
				gebr_comm_uri_set_prefix(uri, "/server");
				gebr_comm_uri_add_param(uri, "address", addr->str);
				gebr_comm_uri_add_param(uri, "pass", password);
				gchar *url = gebr_comm_uri_to_string(uri);
				gebr_comm_uri_free(uri);

				gebr_comm_protocol_socket_send_request(comm_server->socket,
								       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
				g_free(url);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.ac_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *ac = g_list_nth_data(arguments, 1);
			gboolean is_ac = g_strcmp0(ac->str, "on") == 0 ? TRUE : FALSE;

			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, NULL);

			g_debug("Emit signal to change AC!!!!!");

			g_signal_emit(maestro, signals[AC_CHANGE], 0, is_ac, daemon);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.srm_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);

			g_debug("Removing server %s", addr->str);

			GtkTreeIter iter;
			GebrDaemonServer *daemon;
			GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

			gebr_gui_gtk_tree_model_foreach(iter, model) {
				gtk_tree_model_get(model, &iter, 0, &daemon, -1);
				if (g_strcmp0(addr->str, gebr_daemon_server_get_address(daemon)) == 0)
					gtk_list_store_remove(maestro->priv->store, &iter);
			}

			g_signal_emit(maestro, signals[DAEMONS_CHANGED], 0);
			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

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
		maestro->priv->address = g_value_dup_string(value);
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
	g_hash_table_unref(maestro->priv->jobs);
	g_hash_table_unref(maestro->priv->temp_jobs);

	G_OBJECT_CLASS(gebr_maestro_server_parent_class)->finalize(object);
}

static void
gebr_maestro_server_group_changed_real(GebrMaestroServer *maestro)
{
	update_groups_store(maestro);
}

static void
gebr_maestro_server_state_change_real(GebrMaestroServer *maestro)
{
	update_groups_store(maestro);
}

static void
gebr_maestro_server_class_init(GebrMaestroServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_maestro_server_get;
	object_class->set_property = gebr_maestro_server_set;
	object_class->finalize = gebr_maestro_server_finalize;

	klass->group_changed = gebr_maestro_server_group_changed_real;
	klass->state_change  = gebr_maestro_server_state_change_real;

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

	signals[QUESTION_REQUEST] =
		g_signal_new("question-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, question_request),
			     NULL, NULL,
			     gebr_cclosure_marshal_BOOL__STRING_STRING_STRING,
			     G_TYPE_BOOLEAN, 3,
			     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[PASSWORD_REQUEST] =
		g_signal_new("password-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, password_request),
			     NULL, NULL,
			     gebr_cclosure_marshal_STRING__STRING,
			     G_TYPE_STRING, 1, G_TYPE_STRING);

	signals[DAEMONS_CHANGED] =
			g_signal_new("daemons-changed",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, daemons_changed),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__VOID,
			             G_TYPE_NONE, 0);

	signals[STATE_CHANGE] =
		g_signal_new("state-change",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, state_change),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	signals[AC_CHANGE] =
			g_signal_new("ac-change",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, ac_change),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__INT_OBJECT,
			             G_TYPE_NONE, 2, G_TYPE_INT, GEBR_TYPE_DAEMON_SERVER);

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
	maestro->priv->temp_jobs = g_hash_table_new(g_str_hash, g_str_equal);
	maestro->priv->queues_model = gtk_list_store_new(1, GEBR_TYPE_JOB);

	// Insert queue 'Immediately'
	GtkTreeIter iter;
	gtk_list_store_append(maestro->priv->queues_model, &iter);
	gtk_list_store_set(maestro->priv->queues_model, &iter, 0, NULL, -1);

	maestro->priv->groups_store = gtk_list_store_new(MAESTRO_SERVER_N,
							 G_TYPE_INT,
							 G_TYPE_STRING);

	GebrDaemonServer *autochoose =
		gebr_daemon_server_new(GEBR_CONNECTABLE(maestro), NULL, SERVER_STATE_CONNECT);
	gebr_maestro_server_add_daemon(maestro, autochoose);
}

/* Implementation of GebrConnectable interface {{{ */
static void
gebr_maestro_server_connectable_connect(GebrConnectable *connectable,
					const gchar *address)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/server");
	gebr_comm_uri_add_param(uri, "address", address);
	gebr_comm_uri_add_param(uri, "pass", "");
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_disconnect(GebrConnectable *connectable,
			       const gchar *address)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/disconnect");
	gebr_comm_uri_add_param(uri, "address", address);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_remove(GebrConnectable *connectable,
			       const gchar *address)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/remove");
	gebr_comm_uri_add_param(uri, "address", address);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}
static void
gebr_maestro_server_connectable_init(GebrConnectableIface *iface)
{
	iface->connect = gebr_maestro_server_connectable_connect;
	iface->disconnect = gebr_maestro_server_disconnect;
	iface->remove = gebr_maestro_server_remove;
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

GebrCommServer *
gebr_maestro_server_get_server(GebrMaestroServer *maestro)
{
	return maestro->priv->server;
}

typedef struct {
	gboolean include_auto;
	gchar *group;
} GroupAndAutochoose;

static void
free_group_and_autochoose(gpointer data)
{
	GroupAndAutochoose *gaa = data;
	g_free(gaa->group);
	g_free(gaa);
}

static gboolean
visible_servers_func(GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
	GroupAndAutochoose *gaa = data;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter, 0, &daemon, -1);

	if (!daemon)
		return FALSE;

	if (gebr_daemon_server_is_autochoose(daemon))
		return gaa->include_auto;

	return gebr_daemon_server_has_tag(daemon, gaa->group);
}

GtkTreeModel *
gebr_maestro_server_get_model(GebrMaestroServer *maestro,
			      gboolean include_autochoose,
			      const gchar *group)
{
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

	if (include_autochoose && (!group || !*group))
		return g_object_ref(model);

	GroupAndAutochoose *gaa = g_new(GroupAndAutochoose, 1);
	gaa->include_auto = include_autochoose;
	gaa->group = g_strdup(group);

	GtkTreeModel *filter = gtk_tree_model_filter_new(model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       visible_servers_func, gaa,
					       free_group_and_autochoose);

	return filter;
}

const gchar *
gebr_maestro_server_get_address(GebrMaestroServer *maestro)
{
	return maestro->priv->address;
}

gchar *
gebr_maestro_server_get_display_address(GebrMaestroServer *maestro)
{
	const gchar *addr = maestro->priv->address;

	if (g_strcmp0(addr, "127.0.0.1") == 0)
		return g_strdup(_("Local Maestro"));

	return g_strdup_printf(_("Maestro %s"), addr);
}

GList *
gebr_maestro_server_get_all_tags(GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *daemons = gebr_maestro_server_get_model(maestro, FALSE, NULL);
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
	g_object_unref(daemons);
	g_tree_unref(tree);
	return groups;
}

typedef struct {
	GebrMaestroServerGroupType type;
	gchar *name;
} GroupId;

static gboolean
visible_queue_func(GtkTreeModel *model,
		   GtkTreeIter *iter,
		   gpointer data)
{
	GebrJob *job;
	GroupId *group_id = data;
	gtk_tree_model_get(model, iter, 0, &job, -1);

	if (!job)
		return TRUE;

	const gchar *jgroup = gebr_job_get_server_group(job);
	return g_strcmp0(group_id->name, jgroup) == 0;
}

static void
group_id_free(gpointer data)
{
	GroupId *id = data;
	g_free(id->name);
	g_free(id);
}

GtkTreeModel *
gebr_maestro_server_get_queues_model(GebrMaestroServer *maestro,
				     GebrMaestroServerGroupType type,
				     const gchar *name)
{
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->queues_model);

	if (!name || !*name)
		return g_object_ref(model);

	GroupId *group_id = g_new0(GroupId, 1);
	group_id->type = type;
	group_id->name = g_strdup(name);

	GtkTreeModel *filter = gtk_tree_model_filter_new(model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       visible_queue_func,
					       group_id, group_id_free);
	return filter;
}

void
gebr_maestro_server_connect(GebrMaestroServer *maestro)
{
	maestro->priv->server = gebr_comm_server_new(maestro->priv->address,
						     &maestro_ops);
	maestro->priv->server->user_data = maestro;
	gebr_comm_server_connect(maestro->priv->server, TRUE);
}

void
gebr_maestro_server_add_temporary_job(GebrMaestroServer *maestro, 
				      GebrJob *job)
{
	g_hash_table_insert(maestro->priv->temp_jobs,
			    g_strdup(gebr_job_get_id(job)), job);
}

void
gebr_maestro_server_add_tag_to(GebrMaestroServer *maestro,
			       GebrDaemonServer *daemon,
			       const gchar *tag)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/tag-insert");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "tag", tag);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

void
gebr_maestro_server_remove_tag_from(GebrMaestroServer *maestro,
                                    GebrDaemonServer *daemon,
                                    const gchar *tag)
{
	/*
	gchar *url_aux = g_strdup_printf("/tag-remove?server=%s;tag=%s",
				     gebr_daemon_server_get_address(daemon), tag);

	gchar *url = g_uri_escape_string(url_aux, NULL, FALSE);
	*/
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/tag-remove");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "tag", tag);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

void
gebr_maestro_server_set_autoconnect(GebrMaestroServer *maestro,
                                    GebrDaemonServer *daemon,
                                    gboolean ac)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/autoconnect");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "ac", ac? "on" : "off");
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

GtkTreeModel *
gebr_maestro_server_get_groups_model(GebrMaestroServer *maestro)
{
	return GTK_TREE_MODEL(maestro->priv->groups_store);
}

/*
 * Keep the order of this list equal the enums at GebrMaestroServerGroupType!
 */
static const gchar *groups_str_types[] = {
	"group",
	"daemon",
	NULL
};

GebrMaestroServerGroupType
gebr_maestro_server_group_str_to_enum(const gchar *str)
{
	for (int i = 0; groups_str_types[i]; i++)
		if (g_strcmp0(groups_str_types[i], str) == 0)
			return (GebrMaestroServerGroupType) i;

	g_return_val_if_reached(MAESTRO_SERVER_TYPE_GROUP);
}

const gchar *
gebr_maestro_server_group_enum_to_str(GebrMaestroServerGroupType type)
{
	return groups_str_types[type];
}

GebrDaemonServer *
gebr_maestro_server_get_daemon(GebrMaestroServer *server,
			       const gchar *address)
{
	return get_daemon_from_address(server, address, NULL);
}

const gchar *
gebr_maestro_server_get_error(GebrMaestroServer *maestro)
{
	return gebr_comm_server_get_last_error(maestro->priv->server);
}
