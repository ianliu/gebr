/*
 * gebrm-app.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Team
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

#include <config.h>
#include "gebrm-app.h"

#include "gebrm-daemon.h"
#include "gebrm-job.h"
#include "gebrm-client.h"

#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/comm/gebr-comm-protocol-socket.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gebr-version.h>

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GebrCommListenSocket *listener;
	GebrCommSocketAddress address;
	GList *connections;
	GList *daemons;
	gchar *nfsid;
	gchar *home;

	GebrMaestroSettings *settings;

	gboolean connect_all;

	GQueue *job_def_queue;
	GQueue *job_run_queue;
	GQueue *xauth_queue;

	// Server groups: gchar -> GList<GebrDaemon>
	GTree *groups;

	// Job controller
	GHashTable *jobs;
	GHashTable *jobs_counter;
};

typedef struct {
	GebrmApp *app;
	GebrmJob *job;
} AppAndJob;

typedef struct {
	GebrCommRunner *runner;
	GebrmJob *job;
} RunnerAndJob;

typedef struct {
	GebrmDaemon *daemon;
	GebrmClient *client;
	guint port;
	gchar *host;
} XauthQueueData;

typedef struct {
	GebrmApp *app;
	GebrmClient *client;
	GebrmDaemon *daemon;
} GidInfo;

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;

static void gebrm_config_save_server(GebrmDaemon *daemon);

static gboolean gebrm_config_update_tags_on_server(GebrmApp *app,
                                                   const gchar *server,
                                                   const gchar *tags);

static gboolean gebrm_config_insert_tag_on_server(GebrmApp *app,
						  const gchar *server,
						  const gchar *tag,
						  gchar **_tags);

static gboolean gebrm_config_remove_tag_of_server(GebrmApp *app,
                                                  const gchar *server,
                                                  const gchar *tag,
                                                  gchar **_tags);

static gboolean gebrm_config_set_autoconnect(GebrmApp *app,
					     const gchar *server,
					     const gchar *ac);

static GebrmDaemon *gebrm_add_server_to_list(GebrmApp *app,
					     const gchar *addr,
					     const gchar *tags);

static gboolean gebrm_update_tags_on_list_of_servers(GebrmApp *app,
						     const gchar *address,
						     const gchar *tags);

static void gebrm_config_delete_server(const gchar *serv);

static gboolean gebrm_remove_server_from_list(GebrmApp *app, const gchar *address);

gboolean gebrm_config_load_servers(GebrmApp *app, const gchar *path);

static void send_job_def_to_clients(GebrmApp *app, GebrmJob *job);

static void send_messages_of_jobs(const gchar *id, GebrmJob *job, GebrCommProtocolSocket *protocol);

static gboolean gebrm_app_increment_jobs_counter(GebrmApp *app, const gchar *flow_id);

G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

// Refactor this method to GebrmJobController {{{
static GebrmJob *
gebrm_app_job_controller_find(GebrmApp *app, const gchar *id)
{
	return g_hash_table_lookup(app->priv->jobs, id);
}

static void
gebrm_app_job_controller_add(GebrmApp *app, GebrmJob *job)
{
	g_hash_table_insert(app->priv->jobs,
			    g_strdup(gebrm_job_get_id(job)),
			    job);
}

// Configuration Methods {{{

GebrMaestroSettings *
gebrm_app_create_configuration()
{
	GebrMaestroSettings *ms;

	GString *path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/gebrm/maestro.conf", g_get_home_dir());

	ms = gebr_maestro_settings_new(path->str);

	g_string_free(path, TRUE);

	return ms;
}

gchar *
gebrm_app_get_nfsid(GebrMaestroSettings *ms)
{
	GKeyFile *key;

	key = gebr_maestro_settings_get_key_file(ms);
	gchar *nfsid = g_key_file_get_start_group(key);

	return nfsid;
}

// }}}

static void
gebrm_app_send_home_dir(GebrmApp *app, GebrCommProtocolSocket *socket, const gchar *home)
{
	app->priv->home = g_strdup(home);
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.home_def, 1,
					      home);
}

static void
gebrm_app_send_nfsid(GebrmApp *app, GebrCommProtocolSocket *socket, const gchar *nfsid)
{
	const gchar *hosts = gebr_maestro_settings_get_addrs(app->priv->settings, nfsid);
	const gchar *label = gebr_maestro_settings_get_label_for_domain(app->priv->settings, nfsid, FALSE);

	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.nfsid_def, 3,
					      nfsid,
					      hosts,
					      label);

	GebrmClient *client = g_object_get_data(G_OBJECT(socket), "client");
	gebrm_client_set_sent_nfsid(client, TRUE);
}

static gboolean
gebrm_app_send_mpi_flavors(GebrCommProtocolSocket *socket, GebrmDaemon *daemon)
{
	if (!daemon)
		return FALSE;
	const gchar *flavors = gebrm_daemon_get_mpi_flavors(daemon);

	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.mpi_def, 2,
					      gebrm_daemon_get_address(daemon),
					      flavors);
	return TRUE;
}

static void
send_server_status_message(GebrmApp *app,
			   GebrCommProtocolSocket *socket,
			   GebrmDaemon *daemon,
			   const gchar *ac,
			   GebrCommServerState st)
{
	const gchar *state = gebr_comm_server_state_to_string(st);

	gchar *ncores = g_strdup_printf("%d", gebrm_daemon_get_ncores(daemon));
	gchar *clock = g_strdup_printf("%lf", gebrm_daemon_get_clock(daemon));
	const gchar *memory = gebrm_daemon_get_memory(daemon);
	if (!memory)
		memory = "0";

	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.ssta_def, 8,
					      gebrm_daemon_get_hostname(daemon),
					      gebrm_daemon_get_address(daemon),
					      state,
					      ac,
					      ncores,
					      g_strtod(clock, NULL) > 0? clock : "",
					      gebrm_daemon_get_model_name(daemon),
					      g_strtod(memory, NULL) > 0? memory : "");

	gebrm_daemon_send_error_message(daemon, socket);

	g_free(ncores);
	g_free(clock);
}

static void
send_groups_definitions(GebrCommProtocolSocket *client, GebrmDaemon *daemon)
{
	const gchar *server = gebrm_daemon_get_address(daemon);
	const gchar *tags = gebrm_daemon_get_tags(daemon);

	gebr_comm_protocol_socket_oldmsg_send(client, FALSE,
					      gebr_comm_protocol_defs.agrp_def, 2,
					      server, tags);
}

//static GebrmDaemon *
//find_daemon_by_id(GebrmApp *app, const gchar *id)
//{
//	for (GList *i = app->priv->daemons; i; i = i->next) {
//		const gchar *tmp_id = gebrm_daemon_get_id(i->data);
//		if (g_strcmp0(id, tmp_id) == 0)
//			return i->data;
//	}
//	return NULL;
//}

static void
remove_daemon(GebrmApp *app, const gchar *addr)
{
	gebrm_remove_server_from_list(app, addr);
	gebrm_config_delete_server(addr);

	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);

		// Clean tags for this server
		gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
						      gebr_comm_protocol_defs.agrp_def, 2,
						      addr, "");

		gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
						      gebr_comm_protocol_defs.srm_def, 1,
						      addr);
	}
}

static void
gebrm_app_job_controller_on_task_def(GebrmDaemon *daemon,
				     GebrmTask *task,
				     GebrmApp* app)
{
	const gchar *rid = gebrm_task_get_job_id(task);
	GebrmJob *job = gebrm_app_job_controller_find(app, rid);

	if (!job)
		g_return_if_reached();

	gebrm_job_append_task(job, task);
}

static void
gebrm_app_job_controller_on_issued(GebrmJob    *job,
				   const gchar *issues,
				   GebrmApp    *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.iss_def, 2,
						      gebrm_job_get_id(job),
						      issues);
	}
}

static void
gebrm_app_job_controller_on_cmd_line_received(GebrmJob *job,
					      GebrmTask *task,
					      const gchar *cmd,
					      GebrmApp *app)
{
	gchar *frac;
	for (GList *i = app->priv->connections; i; i = i->next) {
		frac = g_strdup_printf("%d", gebrm_task_get_fraction(task));
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.cmd_def, 3,
						      gebrm_job_get_id(job),
						      frac,
						      cmd);
		g_free(frac);
	}
}

static void
gebrm_app_job_controller_on_output(GebrmJob *job,
				   GebrmTask *task,
				   const gchar *output,
				   GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		gchar *frac = g_strdup_printf("%d", gebrm_task_get_fraction(task));
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.out_def, 3,
						      gebrm_job_get_id(job),
						      frac,
						      output);
		g_free(frac);
	}
}

static void
gebrm_app_job_controller_on_status_change(GebrmJob *job,
					  gint old_status,
					  gint new_status,
					  const gchar *parameter,
					  GebrmApp *app)
{
	if (new_status == JOB_STATUS_FAILED)
		gebrm_job_kill_tasks(job);

	if (new_status == JOB_STATUS_FINISHED
	    || new_status == JOB_STATUS_FAILED
	    || new_status == JOB_STATUS_CANCELED) {
		GList *children = g_object_get_data(G_OBJECT(job), "children");
		for (GList *i = children; i; i = i->next) {
			RunnerAndJob *raj = i->data;
			if (!gebr_comm_runner_run_async(raj->runner))
				gebrm_job_kill_immediately(raj->job);
		}
		g_list_foreach(children, (GFunc)g_free, NULL);
		g_list_free(children);
		g_object_set_data(G_OBJECT(job), "children", NULL);
	}

	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.sta_def, 3,
						      gebrm_job_get_id(job),
						      gebr_comm_job_get_string_from_status(new_status),
						      parameter);

		if (old_status == JOB_STATUS_INITIAL && new_status == JOB_STATUS_RUNNING) {
			GebrmJob *j;
			while (!g_queue_is_empty(app->priv->job_def_queue)) {
				j = g_queue_pop_head(app->priv->job_def_queue);
				send_job_def_to_clients(app, j);
			}
		}
	}

}

static void
verify_connect_all(GebrmApp *app)
{
	for (GList *i = app->priv->daemons; i; i = i->next) {
		GebrmDaemon *daemon = i->data;
		if (gebrm_daemon_get_state(daemon) != SERVER_STATE_LOGGED &&
		    !gebrm_daemon_get_canceled(daemon))
			return;
	}
	app->priv->connect_all = FALSE;
	return;
}

static void
gebrm_app_continue_connections_of_daemons(GebrmApp *app,
                                          gboolean from_append_key)
{
	for (GList *i = app->priv->daemons; i; i = i->next) {
		GebrmDaemon *d = i->data;

		if (!from_append_key) {
			GebrCommServer *server = gebrm_daemon_get_server(d);
			if (gebrm_daemon_get_state(d) == SERVER_STATE_LOGGED &&
			    gebr_comm_server_get_use_public_key(server))
				return;
		}

		if (gebrm_daemon_get_state(d) != SERVER_STATE_LOGGED && !g_strcmp0(gebrm_daemon_get_autoconnect(d), "on")
		    && !gebrm_daemon_get_canceled(d)) {
			for (GList *j = app->priv->connections; j; j = j->next) {
				GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(j->data);
				gebrm_daemon_connect(d, socket);
			}
			if (!from_append_key)
				return;
		}
	}
}

static gboolean
time_out_daemon(gpointer user_data)
{
	g_debug("Daemon TIMEOUT");

	GebrmApp *app = user_data;

	gebrm_app_continue_connections_of_daemons(app, FALSE);
	verify_connect_all(app);

	return FALSE;
}

static void
gebrm_app_daemon_on_state_change(GebrmDaemon *daemon,
				 GebrCommServerState state,
				 GebrmApp *app)
{
	if (state == SERVER_STATE_DISCONNECTED) {
		GList *head = g_queue_peek_head_link(app->priv->xauth_queue);
		for (GList *i = head; i; i = i->next) {
			XauthQueueData *xauth = i->data;
			if (daemon == xauth->daemon) {
				g_queue_delete_link(app->priv->xauth_queue, i);
				g_object_unref(xauth->daemon);
				g_object_unref(xauth->client);
				g_free(xauth);
			}
		}

		gboolean error = gebrm_daemon_get_error_type(daemon) != NULL;
		if (error) {
			guint timeout = gebrm_daemon_get_timeout(daemon);
			if (timeout != -1) {
				g_source_remove(timeout);
				gebrm_daemon_set_timeout(daemon, -1);
			}
			gebrm_daemon_set_canceled(daemon, TRUE);
		}

		if (gebrm_daemon_get_canceled(daemon)) {
			if (app->priv->connect_all) {
				gebrm_app_continue_connections_of_daemons(app, FALSE);
				verify_connect_all(app);
			}
		}

		gboolean reconnect = gebrm_daemon_get_reconnect(daemon);
		const gchar *err = gebrm_daemon_get_error_type(daemon);

		if (!err && reconnect) {
			for (GList *i = app->priv->connections; i; i = i->next) {
				GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
				gebrm_daemon_set_reconnect(daemon, FALSE);
				gebrm_daemon_connect(daemon, socket);
			}
		}
	}

	else if (state == SERVER_STATE_RUN) {
		if (app->priv->connect_all) {
			gebrm_daemon_set_canceled(daemon, TRUE);
			guint timeout = g_timeout_add(6000, time_out_daemon, app);
			gebrm_daemon_set_timeout(daemon, timeout);
		}
	}

	else if (state == SERVER_STATE_LOGGED) {
		guint timeout = gebrm_daemon_get_timeout(daemon);
		if (timeout != -1) {
			g_source_remove(timeout);
			gebrm_daemon_set_timeout(daemon, -1);
		}
		gebrm_daemon_set_canceled(daemon, FALSE);
		if (app->priv->connect_all) {
			gebrm_app_continue_connections_of_daemons(app, FALSE);
			verify_connect_all(app);
		}
	}
	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		send_server_status_message(app, socket, daemon, gebrm_daemon_get_autoconnect(daemon), state);
	}
}

static void
gebrm_app_finalize(GObject *object)
{
	GebrmApp *app = GEBRM_APP(object);
	g_hash_table_unref(app->priv->jobs);
	g_hash_table_unref(app->priv->jobs_counter);
	g_list_foreach(app->priv->connections, (GFunc)g_object_unref, NULL);
	g_list_free(app->priv->connections);
	g_list_free(app->priv->daemons);
	g_queue_free(app->priv->job_def_queue);
	g_queue_free(app->priv->job_run_queue);
	g_queue_free(app->priv->xauth_queue);
	G_OBJECT_CLASS(gebrm_app_parent_class)->finalize(object);
}

static void
gebrm_app_class_init(GebrmAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebrm_app_finalize;
	g_type_class_add_private(klass, sizeof(GebrmAppPriv));
}

static gboolean
process_xauth_queue(gpointer data)
{
	GebrmApp *app = data;

	if (g_queue_is_empty(app->priv->xauth_queue))
		return TRUE;

	XauthQueueData *xauth = g_queue_pop_head(app->priv->xauth_queue);
	gebrm_daemon_send_client_info(xauth->daemon,
				      gebrm_client_get_id(xauth->client),
				      gebrm_client_get_magic_cookie(xauth->client),
				      xauth->host,
				      xauth->port);
	g_object_unref(xauth->daemon);
	g_object_unref(xauth->client);
	g_free(xauth->host);
	g_free(xauth);

	return TRUE;
}

static void
queue_client_info(GebrmApp *app, GebrmDaemon *daemon, GebrmClient *client, const gchar *host, guint port)
{
	XauthQueueData *xauth = g_new(XauthQueueData, 1);
	xauth->daemon = g_object_ref(daemon);
	xauth->client = g_object_ref(client);
	xauth->port = port;
	xauth->host = g_strdup(host);
	g_queue_push_tail(app->priv->xauth_queue, xauth);
}

static void
on_x11_port_defined(GebrCommPortProvider *self,
		    guint port,
		    GidInfo *data)
{
	queue_client_info(data->app, data->daemon, data->client,
			  gebr_comm_port_provider_get_display_host(self),
			  port);

	GebrCommPortForward *forward = gebr_comm_port_provider_get_forward(self);
	gebrm_client_add_forward(data->client, forward);

	g_free(data);
}

static void
gebrm_app_init(GebrmApp *app)
{
	app->priv = G_TYPE_INSTANCE_GET_PRIVATE(app,
						GEBRM_TYPE_APP,
						GebrmAppPriv);
	app->priv->main_loop = g_main_loop_new(NULL, FALSE);
	app->priv->connections = NULL;
	app->priv->daemons = NULL;
	app->priv->jobs = g_hash_table_new_full(g_str_hash, g_str_equal,
						g_free, NULL);
	app->priv->jobs_counter = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                                g_free, NULL);
	app->priv->job_def_queue = g_queue_new();
	app->priv->job_run_queue = g_queue_new();
	app->priv->xauth_queue = g_queue_new();

	app->priv->connect_all = FALSE;

	g_timeout_add(1000, process_xauth_queue, app);
}

void
gebrm_app_singleton_set_factory(GebrmAppFactory fac,
				gpointer data)
{
	__factory = fac;
	__data = data;
}

GebrmApp *
gebrm_app_singleton_get(void)
{
	if (__factory)
		return (*__factory)(__data);

	if (!__app)
		__app = gebrm_app_new();

	return __app;
}

static gboolean
has_duplicated_daemons(GebrmApp *app, const gchar *id)
{
	gboolean has_duplicated = FALSE;
	gboolean added_daemon = FALSE;
	for (GList *i = app->priv->daemons; i; i = i->next) {
		const gchar *tmp = gebrm_daemon_get_id(i->data);
		if (!tmp) {
			const gchar *err_type = gebrm_daemon_get_error_type(i->data);
			if (!err_type || g_strcmp0(err_type, "connection-refused") == 0)
				added_daemon = TRUE;
		}
		if (g_strcmp0(tmp, id) == 0)
			has_duplicated = TRUE;
	}

	return added_daemon && has_duplicated;
}

static void
on_daemon_init(GebrmDaemon *daemon,
	       const gchar *error_type,
	       const gchar *error_msg,
	       gboolean has_gebrm,
	       GebrmApp *app)
{
	const gchar *error = NULL;
	const gchar *nfsid = gebrm_daemon_get_nfsid(daemon);
	const gchar *home = gebrm_daemon_get_home_dir(daemon);
	gboolean remove = FALSE;
	gboolean home_defined = FALSE;
	gboolean send_nfs = FALSE;

	if (!app->priv->nfsid) {
		send_nfs = TRUE;
		app->priv->nfsid = g_strdup(nfsid);
	}

	if (g_strcmp0(app->priv->nfsid, nfsid) != 0) {
		error = "error:nfs";
		goto err;
	}

	if (g_strcmp0(error_type, "connection-refused") == 0) {
		if (has_duplicated_daemons(app, error_msg)) {
                        error = "error:id";
                        remove = TRUE;
                } else {
                        error = "error:connection-refused";
                }
                goto err;
	}

	if (g_strcmp0(error_type, "protocol") == 0) {
		error = "error:protocol";
		goto err;
	}

	gebr_maestro_settings_add_node(app->priv->settings,
				       gebrm_daemon_get_address(daemon),
				       gebrm_daemon_get_tags(daemon),
				       gebrm_daemon_get_autoconnect(daemon));
	if (has_gebrm) {
		gchar *addr = g_strdup_printf("%s@%s", g_get_user_name(), gebrm_daemon_get_address(daemon));
		gebr_maestro_settings_append_address(app->priv->settings, nfsid, addr);
		g_free(addr);
	}
err:
	gebrm_daemon_set_error_type(daemon, error);
	gebrm_daemon_set_error_msg(daemon, error_msg);

	if (error) {
		for (GList *i = app->priv->connections; i; i = i->next) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
			gebrm_daemon_send_error_message(daemon, socket);
		}
		gebrm_daemon_disconnect(daemon);

		if (remove)
			remove_daemon(app, gebrm_daemon_get_address(daemon));
	} else {
		GebrCommServerState state = gebrm_daemon_get_state(daemon);

		if (app->priv->home)
			home_defined = TRUE;

		if (send_nfs) {
			const gchar *label = gebr_maestro_settings_get_label_for_domain(app->priv->settings,
			                                                                nfsid, FALSE);

			gchar *addr = g_strdup_printf("%s@%s", g_get_user_name(), g_get_host_name());
			gebr_maestro_settings_set_domain(app->priv->settings, nfsid, label, addr);

			g_free(addr);
		}

		for (GList *i = app->priv->connections; i; i = i->next) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
			if (!home_defined)
				gebrm_app_send_home_dir(app, socket, home);

			if (!gebrm_client_get_sent_nfsid(i->data))
				gebrm_app_send_nfsid(app, socket, nfsid);

			send_server_status_message(app, socket, daemon, gebrm_daemon_get_autoconnect(daemon), state);

			GebrCommServer *serv = gebrm_daemon_get_server(daemon);

			GidInfo *data = g_new(GidInfo, 1);
			data->app = app;
			data->daemon = daemon;
			data->client = i->data;

			GebrCommPortProvider *port_provider =
				gebr_comm_server_create_port_provider(serv, GEBR_COMM_PORT_TYPE_X11);
			g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_x11_port_defined), data);
			gebr_comm_port_provider_set_display(port_provider,
							    gebrm_client_get_display_port(data->client),
							    gebrm_client_get_display_host(data->client));
			gebr_comm_port_provider_start(port_provider);

			gebrm_app_send_mpi_flavors(socket, daemon);
		}
	}
}

static void
on_daemon_ret_path(GebrmDaemon *daemon,
                   const gchar *daemon_addr,
                   const gchar *status_id,
                   GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.ret_def, 2,
						      daemon_addr,
						      status_id);
	}
}

static void
on_daemon_append_key(GebrmDaemon *daemon,
                     GebrmApp *app)
{
	if (app->priv->connect_all)
		gebrm_app_continue_connections_of_daemons(app, TRUE);
}

static GebrmDaemon *
gebrm_get_daemon_for_address(GebrmApp *app,
                             const gchar *address)
{
	GebrmDaemon *daemon;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		daemon = i->data;
		if (g_strcmp0(address, gebrm_daemon_get_address(daemon)) == 0) {
			return daemon;
		}
	}

	return  NULL;
}

static GebrmDaemon *
gebrm_add_server_to_list(GebrmApp *app,
			 const gchar *address,
			 const gchar *tags)
{
	GebrmDaemon *daemon;

	daemon = gebrm_get_daemon_for_address(app, address);
	if (daemon)
		return daemon;

	daemon = gebrm_daemon_new(address);
	g_signal_connect(daemon, "state-change",
			 G_CALLBACK(gebrm_app_daemon_on_state_change), app);
	g_signal_connect(daemon, "task-define",
			 G_CALLBACK(gebrm_app_job_controller_on_task_def), app);
	g_signal_connect(daemon, "daemon-init",
			 G_CALLBACK(on_daemon_init), app);
	g_signal_connect(daemon, "ret-path",
			 G_CALLBACK(on_daemon_ret_path), app);
	g_signal_connect(daemon, "append-key",
	                 G_CALLBACK(on_daemon_append_key), app);

	gchar **tagsv = tags ? g_strsplit(tags, ",", -1) : NULL;
	if (tagsv) {
		for (int i = 0; tagsv[i]; i++) {
			gebrm_daemon_add_tag(daemon, tagsv[i]);
			g_debug("Inserting daemon %s into group %s", gebrm_daemon_get_address(daemon), tagsv[i]);
		}
	}

	app->priv->daemons = g_list_prepend(app->priv->daemons, daemon);

	return daemon;
}

static gboolean 
gebrm_update_tags_on_list_of_servers(GebrmApp *app,
				     const gchar *address,
				     const gchar *tags)
{
	GebrmDaemon *daemon = NULL;

	daemon = gebrm_get_daemon_for_address(app, address);

	if (!daemon)
		g_warn_if_reached();

	gchar **tagsv = tags ? g_strsplit(tags, ",", -1) : NULL;
	gebrm_daemon_update_tags(daemon, tagsv);

	return TRUE;
}

static GList *
get_comm_servers_min_subset(GList *servers,
                            GList *mpi_flavors)
{
	if (!mpi_flavors)
		return g_list_copy(servers);

	GList *min = NULL;
	gboolean has_flavors;
	for (GList *i = servers; i; i = i->next) {
		has_flavors = TRUE;
		for (GList *k = mpi_flavors; k; k = k->next) {
			if (!gebrm_daemon_accepts_mpi(i->data, k->data)) {
				has_flavors = FALSE;
				break;
			}
		}
		if (has_flavors)
			min = g_list_prepend(min, i->data);
	}

	return min;
}

static GList *
get_comm_servers_max_subset(GList *servers,
                            GList *mpi_flavors)
{
	if (!mpi_flavors)
		return g_list_copy(servers);

	GList *max = NULL;
	for (GList *i = servers; i; i = i->next) {
		for (GList *k = mpi_flavors; k; k = k->next) {
			if (gebrm_daemon_accepts_mpi(i->data, k->data)) {
				max = g_list_prepend(max, i->data);
				break;
			}
		}
	}

	return max;
}

static GList *
get_comm_servers_list(GebrmApp *app, const gchar *group, const gchar *group_type)
{
	GList *servers = NULL;
	gboolean is_single = g_strcmp0(group_type, "daemon") == 0;

	if (is_single) {
		for (GList *i = app->priv->daemons; i; i = i->next) {
			if (g_str_equal(gebrm_daemon_get_address(i->data), group)) {
				servers = g_list_prepend(servers, i->data);
				break;
			}
		}
	} else {
		if (g_strcmp0(group,"") == 0) {	//All servers from maestro
			for (GList *i = app->priv->daemons; i; i = i->next) {
				if (gebr_comm_server_is_logged(gebrm_daemon_get_server(i->data)))
					servers = g_list_prepend(servers, i->data);
			}
		} else {		//All servers from a group
			for (GList *i = app->priv->daemons; i; i = i->next) {
				if (!gebrm_daemon_has_group(i->data, group))
					continue;
				if (gebr_comm_server_is_logged(gebrm_daemon_get_server(i->data)))
					servers = g_list_prepend(servers, i->data);
			}
		}
	}

	return servers;
}

static gboolean
gebrm_remove_server_from_list(GebrmApp *app, const gchar *addr)
{
	for (GList *i = app->priv->daemons; i; i = i->next) {
		GebrmDaemon *daemon = i->data;
		if (g_strcmp0(gebrm_daemon_get_address(daemon), addr) == 0) {
			app->priv->daemons = g_list_delete_link(app->priv->daemons, i);
			gebrm_daemon_disconnect(daemon);
			g_object_unref(daemon);
			return TRUE;
		}
	}
	return FALSE;
}

static void
send_job_def_to_clients(GebrmApp *app, GebrmJob *job)
{
	gchar *infile, *outfile, *logfile;
	gebrm_job_get_io(job, &infile, &outfile, &logfile);

	const gchar *start_date = gebrm_job_get_start_date(job);
	const gchar *finish_date = gebrm_job_get_finish_date(job);
	const gchar *snapshot_title = gebrm_job_get_snapshot_title(job);
	const gchar *snapshot_id = gebrm_job_get_snapshot_id(job);
	const gchar *description = gebrm_job_get_description(job);

	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.job_def, 26,
						      gebrm_job_get_id(job),
						      gebrm_job_get_temp_id(job),
						      gebrm_job_get_flow_id(job),
						      gebrm_job_get_nprocs(job),
						      gebrm_job_get_servers_list(job),
						      gebrm_job_get_hostname(job),
						      gebrm_job_get_title(job),
						      gebrm_job_get_job_counter(job),
						      description ? description : "",
						      snapshot_title ? snapshot_title : "",
						      snapshot_id ? snapshot_id : "",
						      gebrm_job_get_queue(job),
						      gebrm_job_get_nice(job),
						      infile,
						      outfile,
						      logfile,
						      gebrm_job_get_submit_date(job),
						      gebrm_job_get_server_group(job),
						      gebrm_job_get_server_group_type(job),
						      gebrm_job_get_exec_speed(job),
						      gebr_comm_job_get_string_from_status(gebrm_job_get_status(job)),
						      start_date? start_date : "",
						      finish_date? finish_date : "",
						      gebrm_job_get_run_type(job),
						      gebrm_job_get_mpi_owner(job),
						      gebrm_job_get_mpi_flavor(job));
	}
}

static void
on_execution_response(GebrCommRunner *runner,
		      gpointer data)
{
	AppAndJob *aap = data;

	gebrm_job_set_mpi_owner(aap->job, gebr_comm_runner_get_mpi_owner(runner));
	gebrm_job_set_mpi_flavor(aap->job, gebr_comm_runner_get_mpi_flavor(runner));
	gebrm_job_set_total_tasks(aap->job, gebr_comm_runner_get_total(runner));
	gebrm_job_set_servers_list(aap->job, gebr_comm_runner_get_servers_list(runner));
	gebrm_job_set_nprocs(aap->job, gebr_comm_runner_get_ncores(runner));

	g_queue_pop_head(aap->app->priv->job_def_queue);
	send_job_def_to_clients(aap->app, aap->job);

	g_queue_pop_head(aap->app->priv->job_run_queue);
	GebrCommRunner *next_run = g_queue_peek_head(aap->app->priv->job_run_queue);
	if (next_run && !gebr_comm_runner_run_async(next_run)) {
		const gchar *id = gebr_comm_runner_get_id(next_run);
		GebrmJob *job = gebrm_app_job_controller_find(aap->app, id);
		gebrm_job_kill_immediately(job);
	}

	gebr_validator_free(gebr_comm_runner_get_validator(runner));
	gebr_comm_runner_free(runner);
	g_free(aap);
}

static void
gebrm_app_handle_run(GebrmApp *app, GebrCommHttpMsg *request, GebrmClient *client, GebrCommUri *uri)
{
	const gchar *gid		= gebr_comm_uri_get_param(uri, "gid");
	const gchar *parent_id		= gebr_comm_uri_get_param(uri, "parent_id");
	const gchar *temp_parent	= gebr_comm_uri_get_param(uri, "temp_parent");
	const gchar *flow_id		= gebr_comm_uri_get_param(uri, "flow_id");
	const gchar *flow_title		= gebr_comm_uri_get_param(uri, "flow_title");
	const gchar *speed		= gebr_comm_uri_get_param(uri, "speed");
	const gchar *nice		= gebr_comm_uri_get_param(uri, "nice");
	const gchar *name		= gebr_comm_uri_get_param(uri, "name");
	const gchar *server_host	= gebr_comm_uri_get_param(uri, "server-hostname");
	const gchar *group_type		= gebr_comm_uri_get_param(uri, "group_type");
	const gchar *host		= gebr_comm_uri_get_param(uri, "host");
	const gchar *temp_id		= gebr_comm_uri_get_param(uri, "temp_id");
	const gchar *paths		= gebr_comm_uri_get_param(uri, "paths");
	const gchar *snapshot_title	= gebr_comm_uri_get_param(uri, "snapshot_title");
	const gchar *snapshot_id	= gebr_comm_uri_get_param(uri, "snapshot_id");

	if (temp_parent)
		parent_id = gebrm_client_get_job_id_from_temp(client,
							      temp_parent);

	GebrCommJsonContent *json = gebr_comm_json_content_new(request->content->str);
	GString *value = gebr_comm_json_content_to_gstring(json);

	GebrGeoXmlProject **pproj = g_new(GebrGeoXmlProject*, 1);
	GebrGeoXmlLine **pline = g_new(GebrGeoXmlLine*, 1);
	GebrGeoXmlFlow **pflow = g_new(GebrGeoXmlFlow*, 1);

	gebr_geoxml_document_load_buffer((GebrGeoXmlDocument **)pflow, value->str);
	*pproj = gebr_geoxml_project_new();
	*pline = gebr_geoxml_line_new();

	gebr_geoxml_document_split_dict(*((GebrGeoXmlDocument **)pflow),
					*((GebrGeoXmlDocument **)pline),
					*((GebrGeoXmlDocument **)pproj),
					NULL);

	GebrValidator *validator = gebr_validator_new((GebrGeoXmlDocument **)pflow,
						      (GebrGeoXmlDocument **)pline,
						      (GebrGeoXmlDocument **)pproj);

	gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(*pflow));
	gchar *description = gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT(*pflow));

	gint job_counter = gebrm_app_increment_jobs_counter(app, flow_id);

	GebrmJobInfo info = { 0, };
	info.title = g_strdup(title);
	info.description = g_strdup(description);
	info.temp_id = g_strdup(temp_id);
	info.flow_id = g_strdup(flow_id);
	info.flow_title = g_strdup(flow_title);
	info.hostname = g_strdup(host);
	info.parent_id = g_strdup(parent_id);
	info.servers = g_strdup("");
	info.nice = g_strdup(nice);
	info.input = gebr_geoxml_flow_io_get_input_real(*pflow);
	info.output = gebr_geoxml_flow_io_get_output_real(*pflow);
	info.error = gebr_geoxml_flow_io_get_error(*pflow);
	info.snapshot_title = g_strdup(snapshot_title);
	info.snapshot_id = g_strdup(snapshot_id);
	info.job_counter = g_strdup_printf("%d", job_counter);

	if (server_host)
		info.group = g_strdup(server_host);
	else
		info.group = g_strdup(name);

	info.group_type = g_strdup(group_type);
	info.speed = g_strdup(speed);
	info.submit_date = g_strdup(gebr_iso_date());

	GebrmJob *job = gebrm_job_new();

	gebrm_client_add_temp_id(client, temp_id, gebrm_job_get_id(job));

	g_signal_connect(job, "status-change",
			 G_CALLBACK(gebrm_app_job_controller_on_status_change), app);
	g_signal_connect(job, "issued",
			 G_CALLBACK(gebrm_app_job_controller_on_issued), app);
	g_signal_connect(job, "cmd-line-received",
			 G_CALLBACK(gebrm_app_job_controller_on_cmd_line_received), app);
	g_signal_connect(job, "output",
			 G_CALLBACK(gebrm_app_job_controller_on_output), app);

	gebrm_job_init_details(job, &info);
	gebrm_app_job_controller_add(app, job);

	GList *mpi_flavors = gebr_geoxml_flow_get_mpi_flavors(*pflow);

	if (mpi_flavors)
		gebrm_job_set_run_type(job, "mpi");
	else
		gebrm_job_set_run_type(job, "normal");

	GList *servers = get_comm_servers_list(app, name, group_type);

	GList *min_subset_servers = get_comm_servers_min_subset(servers, mpi_flavors);
	GList *max_subset_servers = get_comm_servers_max_subset(servers, mpi_flavors);

	if (!min_subset_servers) {
		gebrm_job_set_status(job, JOB_STATUS_FAILED);

		GString *tmp = g_string_new(NULL);
		for (GList *i = mpi_flavors; i; i = i->next) {
			g_string_append(tmp, " and ");
			g_string_append_printf(tmp, "<b>%s</b>", (gchar *)i->data);
		}
		if (tmp->len)
			g_string_erase(tmp, 0, 5);

		gchar *mpi_issue_message;

		if (!*info.group)
			mpi_issue_message = g_strdup_printf("There is no processing node that supports %s in domain <b>%s</b>", tmp->str, info.hostname);
		else if (g_strcmp0(group_type,"daemon"))
			mpi_issue_message = g_strdup_printf("There is no processing node that supports %s in group <b>%s</b>", tmp->str, info.group);
		else
			mpi_issue_message = g_strdup_printf("The processing node <b>%s</b> does not support %s", info.group, tmp->str);

		g_string_free(tmp, TRUE);

		for (GList *i = app->priv->connections; i; i = i->next) {
			GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
			send_messages_of_jobs(gebrm_job_get_id(job), job, socket_client);
			gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
							      gebr_comm_protocol_defs.iss_def, 2,
							      gebrm_job_get_id(job),
							      mpi_issue_message);
			g_free(mpi_issue_message);
		}
	} else {
		GebrCommRunner *runner = gebr_comm_runner_new(GEBR_GEOXML_DOCUMENT(*pflow),
		                                              min_subset_servers, max_subset_servers,
		                                              gebrm_job_get_id(job),
							      gid, parent_id, speed, nice,
							      name, paths, validator);

		AppAndJob *aap = g_new(AppAndJob, 1);
		aap->app = app;
		aap->job = job;
		gebr_comm_runner_set_ran_func(runner, on_execution_response, aap);

		GebrmJob *parent;
		gboolean run_immediately = FALSE;

		parent = gebrm_app_job_controller_find(app, parent_id);
		if (parent) {
			GebrCommJobStatus status = gebrm_job_get_status(parent);
			run_immediately = status != JOB_STATUS_QUEUED && status != JOB_STATUS_RUNNING && status != JOB_STATUS_INITIAL;
		}

		if (!parent || run_immediately) {
			g_queue_push_head(app->priv->job_def_queue, job);
			if (g_queue_is_empty(app->priv->job_run_queue)
			    && !gebr_comm_runner_run_async(runner))
				gebrm_job_kill_immediately(job);
			g_queue_push_tail(app->priv->job_run_queue, runner);
		} else {
			GList *parent_on_queue = g_queue_find(app->priv->job_def_queue, parent);
			if (parent_on_queue)
				g_queue_insert_after(app->priv->job_def_queue, parent_on_queue, job);

			GList *l = g_object_get_data(G_OBJECT(parent), "children");

			RunnerAndJob *raj = g_new(RunnerAndJob, 1);
			raj->runner = runner;
			raj->job = job;
			l = g_list_prepend(l, raj);
			g_object_set_data(G_OBJECT(parent), "children", l);

			if (g_queue_is_empty(app->priv->job_def_queue))
				send_job_def_to_clients(app, job);
		}
	}

	g_list_foreach(mpi_flavors, (GFunc)g_free, NULL);
	g_list_free(mpi_flavors);
	gebrm_job_info_free(&info);
	g_list_free(servers);
	g_free(title);
	g_free(description);
}

static void
connect_all_daemons(GebrmApp *app, GebrCommProtocolSocket *socket, const gchar *addr)
{
	gboolean has_daemons = FALSE;
	gboolean connect_daemon = FALSE;
	app->priv->connect_all = TRUE;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		has_daemons = TRUE;
		GebrmDaemon *daemon = i->data;

		gebrm_daemon_set_canceled(daemon, FALSE);

		if (!connect_daemon && gebrm_daemon_get_state(daemon) == SERVER_STATE_DISCONNECTED &&
		    g_strcmp0(gebrm_daemon_get_autoconnect(daemon), "on") == 0) {
			gebrm_daemon_connect(daemon, socket);
			connect_daemon = TRUE;
		}
	}

	if (addr) {
		GebrmDaemon *d = gebrm_add_server_to_list(app, addr, NULL);
		if (g_strcmp0(gebrm_daemon_get_autoconnect(d), "on") == 0) {
			gebrm_daemon_connect(d, socket);
			gebrm_config_save_server(d);
		}
	}

	if (!connect_daemon)
		app->priv->connect_all = FALSE;
}

static void
on_client_request(GebrCommProtocolSocket *socket,
		  GebrCommHttpMsg *request,
		  GebrmApp *app)
{
	GebrmClient *client = g_object_get_data(G_OBJECT(socket), "client");
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_parse(uri, request->url->str);
	const gchar *prefix = gebr_comm_uri_get_prefix(uri);

	if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		if (g_strcmp0(prefix, "/server") == 0) {
			const gchar *respect_ac = gebr_comm_uri_get_param(uri, "respect-ac");
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			gchar *ac = NULL;

			gebr_maestro_settings_get_node_info(app->priv->settings, addr, NULL, &ac);
			if (!atoi(respect_ac) || (atoi(respect_ac) && g_strcmp0(ac, "on") == 0)) {
				addr = gebr_apply_pattern_on_address(addr);

				GebrmDaemon *d = gebrm_add_server_to_list(app, addr, NULL);

				gebrm_daemon_connect(d, socket);
				gebrm_config_save_server(d);
			}
		}
		else if (g_strcmp0(prefix, "/set-password") == 0) {
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *pass = gebr_comm_uri_get_param(uri, "pass");
			const gchar *has_key = gebr_comm_uri_get_param(uri, "haskey");

			GebrmDaemon *daemon = gebrm_get_daemon_for_address(app, addr);
			if (daemon) {
				gboolean use_key;
				if (!g_strcmp0(has_key, "yes"))
					use_key = TRUE;
				else
					use_key = FALSE;

				GebrCommServer *server = gebrm_daemon_get_server(daemon);
				gebr_comm_server_set_use_public_key(server, use_key);

				if (pass) {
					gebrm_daemon_set_password(daemon, pass);
				} else {
					gebrm_daemon_invalid_password(daemon);
					gebrm_daemon_send_error_message(daemon, socket);
				}
			}

		}
		else if (g_strcmp0(prefix, "/connect-daemons") == 0) {
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			connect_all_daemons(app, socket, addr);
		}
		else if (g_strcmp0(prefix, "/ssh-answer") == 0) {
			GebrmDaemon *daemon = NULL;
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *resp = gebr_comm_uri_get_param(uri, "response");

			daemon = gebrm_get_daemon_for_address(app, addr);
			if (daemon)
				gebrm_daeamon_answer_question(daemon, resp);
		}

		else if (g_strcmp0(prefix, "/disconnect") == 0) {
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *confirm = gebr_comm_uri_get_param(uri, "confirm");
			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrmDaemon *daemon = i->data;
				if (g_strcmp0(gebrm_daemon_get_address(daemon), addr) == 0) {
					if (g_strcmp0(confirm, "remove") == 0 &&
					    gebrm_daemon_get_uncompleted_tasks(daemon) <= 0)
						gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
										      gebr_comm_protocol_defs.cfrm_def, 2,
										      addr, "remove-immediately");
					else if (g_strcmp0(confirm, "yes") == 0
						 || gebrm_daemon_get_uncompleted_tasks(daemon) <= 0) {
						gebrm_daemon_set_disconnecting(daemon, TRUE);
						GList *jobs = gebrm_daemon_get_list_of_jobs(daemon);
						for (GList *i = jobs; i; i = i->next) {
							GebrmJob *job = g_hash_table_lookup(app->priv->jobs, i->data);
							if (job && gebrm_job_can_kill(job))
								gebrm_job_kill_immediately(job);
						}
						gebrm_daemon_set_disconnecting(daemon, FALSE);
						gebrm_daemon_disconnect(daemon);
						gebrm_client_kill_forward_by_address(client, addr);
					}
					else if (g_strcmp0(confirm, "remove") == 0)
						gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
										      gebr_comm_protocol_defs.cfrm_def, 2,
										      addr, "remove");
					else
						gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
										      gebr_comm_protocol_defs.cfrm_def, 2,
										      addr, "disconnect");
					break;
				}
			}
		}
		else if (g_strcmp0(prefix, "/stop") == 0) {
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *confirm = gebr_comm_uri_get_param(uri, "confirm");
			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrmDaemon *d = i->data;
				if (!g_strcmp0(gebrm_daemon_get_address(d), addr)) {
					if (g_strcmp0(confirm, "yes") == 0
					    || gebrm_daemon_get_uncompleted_tasks(d) <= 0) {
						gebrm_daemon_set_disconnecting(d, TRUE);
						GList *jobs = gebrm_daemon_get_list_of_jobs(d);
						for (GList *j = jobs; j; j = j->next) {
							GebrmJob *job = g_hash_table_lookup(app->priv->jobs, j->data);
							if (job && gebrm_job_can_kill(job))
								gebrm_job_kill_immediately(job);
						}
						gebrm_daemon_set_disconnecting(d, FALSE);
						GebrCommServer *server = gebrm_daemon_get_server(d);
						gebr_comm_server_kill(server);
						gebrm_daemon_set_error_type(d, "error:stop");
						gebrm_daemon_set_error_msg(d, NULL);
						send_server_status_message(app, socket, d, gebrm_daemon_get_autoconnect(d), SERVER_STATE_DISCONNECTED);
					}
					else
						gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						                                      gebr_comm_protocol_defs.cfrm_def, 2,
						                                      addr, "stop");
				}
			}
		}
		else if (g_strcmp0(prefix, "/remove") == 0) {
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			remove_daemon(app, addr);
		}
		else if (g_strcmp0(prefix, "/close") == 0) {
			const gchar *id = gebr_comm_uri_get_param(uri, "id");
			GebrmJob *job = g_hash_table_lookup(app->priv->jobs, id);
			if (job) {
				gebrm_job_close(job);
				g_hash_table_remove(app->priv->jobs, id);

				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.jcl_def, 1,
					                                      id);
				}
			}
		}
		else if (g_strcmp0(prefix, "/kill") == 0) {
			const gchar *id = gebr_comm_uri_get_param(uri, "id");
			GebrmJob *job = g_hash_table_lookup(app->priv->jobs, id);
			if (job) {
				if (gebrm_job_get_status(job) == JOB_STATUS_QUEUED) {
					const gchar *parent_id = gebrm_job_get_queue(job);
					GebrmJob *parent = gebrm_app_job_controller_find(app, parent_id);

					GList *child_parent = g_object_get_data(G_OBJECT(parent), "children");
					for (GList *i = child_parent; i; i = i->next) {
						RunnerAndJob *rj = i->data;
						if (job == rj->job) {
							child_parent = g_list_remove(child_parent, rj);
							break;
						}
					}

					GList *child_job = g_object_get_data(G_OBJECT(job), "children");
					for (GList *i = child_job; i; i = i->next) {
						RunnerAndJob *rj = i->data;

						child_job = g_list_remove(child_job, rj);
						gebrm_job_set_queue(rj->job, parent_id);
						child_parent = g_list_prepend(child_parent, rj);
						send_job_def_to_clients(app, rj->job);
					}

					g_object_set_data(G_OBJECT(parent), "children", child_parent);
					g_object_set_data(G_OBJECT(job), "children", child_job);

					gebrm_job_unqueue(job);
				}
				else
					gebrm_job_kill(job);
			}
		}
		else if (g_strcmp0(prefix, "/run") == 0) {

			gebrm_app_handle_run(app, request, client, uri);

		} else if (g_strcmp0(prefix, "/server-tags") == 0) {
			const gchar *server = gebr_comm_uri_get_param(uri, "server");
			const gchar *tags   = gebr_comm_uri_get_param(uri, "tags");

			if (gebrm_config_update_tags_on_server(app, server, tags)) {
				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.agrp_def, 2,
					                                      server, tags);
				}
				gebrm_update_tags_on_list_of_servers(app, server, tags);
			}
		} else if (g_strcmp0(prefix, "/tag-insert") == 0) {
			const gchar *server = gebr_comm_uri_get_param(uri, "server");
			const gchar *tag    = gebr_comm_uri_get_param(uri, "tag");

			gchar *tags;
			if (gebrm_config_insert_tag_on_server(app, server, tag, &tags)) {
				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.agrp_def, 2,
					                                      server, tags);
				}
				gebrm_update_tags_on_list_of_servers(app, server, tags);
				g_free(tags);
			}
		} else if (g_strcmp0(prefix, "/tag-remove") == 0) {
			const gchar *server = gebr_comm_uri_get_param(uri, "server");
			const gchar *tag    = gebr_comm_uri_get_param(uri, "tag");

			gchar *tags;
			if (gebrm_config_remove_tag_of_server(app, server, tag, &tags)) {
				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.agrp_def, 2,
					                                      server, tags);
				}
				gebrm_update_tags_on_list_of_servers(app, server, tags);
				g_free(tags);
			}
		} else if (g_strcmp0(prefix, "/autoconnect") == 0) {
			const gchar *server_addr = gebr_comm_uri_get_param(uri, "server");
			const gchar *is_ac  = gebr_comm_uri_get_param(uri, "ac");

			GebrmDaemon *daemon = NULL;
			daemon = gebrm_get_daemon_for_address(app, server_addr);
			if (daemon)
				gebrm_daemon_set_autoconnect(daemon, is_ac);

			if (gebrm_config_set_autoconnect(app, server_addr, is_ac)) {
				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.ac_def, 2,
					                                      server_addr,
					                                      is_ac);
				}
			}

		} 
	gebr_comm_uri_free(uri);
	}
}

static void
on_client_parse_messages(GebrCommProtocolSocket *socket,
			 GebrmApp *app)
{
	GList *link;
	struct gebr_comm_message *message;

	GebrmClient *client = g_object_get_data(G_OBJECT(socket), "client");

	while ((link = g_list_last(socket->protocol->messages)) != NULL) {
		message = link->data;

		if (message->hash == gebr_comm_protocol_defs.ini_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 7)) == NULL)
				goto err;

			GString *address = g_list_nth_data(arguments, 0);
			GString *version = g_list_nth_data(arguments, 1);
			GString *cookie  = g_list_nth_data(arguments, 2);
			GString *gebr_id = g_list_nth_data(arguments, 3);
			GString *gebr_time_iso = g_list_nth_data(arguments, 4);
			GString *has_maestro = g_list_nth_data(arguments, 5);

			g_debug("Maestro received a X11 cookie: %s", cookie->str);
			g_debug("Maestro received GeBR time: %s", gebr_time_iso->str);

			if (g_strcmp0(version->str, gebr_version()) != 0) {
				g_debug("Gebr's version mismatch! Got: %s Expected: %s",
					version->str, gebr_version());

				gebr_comm_protocol_socket_oldmsg_send(socket, TRUE,
								      gebr_comm_protocol_defs.err_def, 4,
								      g_get_host_name(),
								      "maestro",
								      "error:protocol",
								      gebr_version());
				goto err;
			}

			gint diff_secs = gebr_compute_diff_clock_to_me(gebr_time_iso->str);

			gebrm_client_set_id(client, gebr_id->str);
			gebrm_client_set_magic_cookie(client, cookie->str);

			gchar *nfsid = gebrm_app_get_nfsid(app->priv->settings);
			if (nfsid) {
				if (atoi(has_maestro->str)) {
					gchar *addr = g_strdup_printf("%s@%s", g_get_user_name(), address->str);
					gebr_maestro_settings_append_address(app->priv->settings, nfsid, addr);
					g_free(addr);
				}

//				if (atoi(has_daemon->str))
//					gebr_maestro_settings_add_node(app->priv->settings,
//								       address->str, "", "on");
			}
			g_free(nfsid);

			// Create list for daemons to connect
			gebrm_app_create_possible_daemon_list(app->priv->settings, app);

			gchar *clocks_diff = g_strdup_printf("%d", diff_secs);

			gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 1,
							      clocks_diff);
			g_free(clocks_diff);

			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrCommServerState state = gebrm_daemon_get_state(i->data);
				if (state != SERVER_STATE_LOGGED)
					state = SERVER_STATE_DISCONNECTED;

				send_server_status_message(app, socket, i->data, gebrm_daemon_get_autoconnect(i->data), state);
				send_groups_definitions(socket, i->data);

				if (state == SERVER_STATE_LOGGED) 
					gebrm_app_send_mpi_flavors(socket, i->data);
			}

			if (app->priv->nfsid)
				gebrm_app_send_nfsid(app, socket, app->priv->nfsid);

			if (app->priv->home)
				gebrm_app_send_home_dir(app, socket, app->priv->home);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.path_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *new_path = g_list_nth_data(arguments, 0);
			GString *old_path = g_list_nth_data(arguments, 1);
			GString *option = g_list_nth_data(arguments, 2);


			GebrmDaemon *daemon = NULL;
			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrmDaemon *d = i->data;
				if (gebrm_daemon_get_state(d) == SERVER_STATE_LOGGED) {
					daemon = d;
					break;
				}
			}

			g_debug("Send to daemon: option: %s, new_path: %s, old_path: %s",
			        option->str, new_path->str, old_path->str);

			if (daemon) {
				GebrCommServer *comm_server = gebrm_daemon_get_server(daemon);
				gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, TRUE,
				                                      gebr_comm_protocol_defs.path_def, 3,
				                                      new_path->str,
				                                      old_path->str,
				                                      option->str);
			}
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.nfsid_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *nfsid = g_list_nth_data(arguments, 0);
			GString *label = g_list_nth_data(arguments, 2);

			gebr_maestro_settings_change_label(app->priv->settings, nfsid->str, label->str);
			gebr_maestro_settings_save(app->priv->settings);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.dsp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *display = g_list_nth_data(arguments, 0);
			GString *display_host = g_list_nth_data(arguments, 1);


			guint display_port = atoi(display->str);
			gebrm_client_set_display(client, display_port, display_host->str);

			g_debug("MAESTRO RECEIVED PORT: %d and HOST: %s", display_port, display_host->str);

			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrCommServerState state = gebrm_daemon_get_state(i->data);
				if (state != SERVER_STATE_LOGGED)
					continue;
				GebrCommServer *serv = gebrm_daemon_get_server(i->data);

				GidInfo *data = g_new(GidInfo, 1);
				data->app = app;
				data->daemon = i->data;
				data->client = client;

				GebrCommPortProvider *port_provider =
					gebr_comm_server_create_port_provider(serv, GEBR_COMM_PORT_TYPE_X11);
				g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_x11_port_defined), data);
				gebr_comm_port_provider_set_display(port_provider, display_port, display_host->str);
				gebr_comm_port_provider_start(port_provider);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		socket->protocol->messages = g_list_delete_link(socket->protocol->messages, link);
	}

	return;

err:
	gebr_comm_message_free(message);
	socket->protocol->messages = g_list_delete_link(socket->protocol->messages, link);
	g_object_unref(client);
}

static GKeyFile *
load_servers_keyfile(void)
{
	const gchar *path = gebr_maestro_settings_get_servers_file();
	GKeyFile *keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
		g_key_file_free(keyfile);
		return NULL;
	}

	return keyfile;
}

static GKeyFile *
load_admin_servers_keyfile(void)
{
	const gchar *path = gebrm_app_get_admin_servers_file();
	GKeyFile *keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
		g_key_file_free(keyfile);
		return NULL;
	}

	return keyfile;
}

static gboolean 
save_servers_keyfile(GKeyFile *keyfile)
{
	gchar *content = g_key_file_to_data(keyfile, NULL, NULL);
	gboolean succ = FALSE;

	if (content) {
		const gchar *path = gebr_maestro_settings_get_servers_file();
		succ = g_file_set_contents(path, content, -1, NULL);
		g_free(content);
	}
	return succ;
}

static gboolean
gebrm_config_update_tags_on_server(GebrmApp *app,
                                   const gchar *server,
                                   const gchar *tags)
{
	GKeyFile *keyfile = load_servers_keyfile();

	if (!keyfile)
		return FALSE;

	g_key_file_set_string(keyfile, server, "tags", tags);
	save_servers_keyfile(keyfile);
	g_key_file_free(keyfile);

	return TRUE;
}

static gboolean
gebrm_config_insert_tag_on_server(GebrmApp *app,
				  const gchar *server,
				  const gchar *tag,
				  gchar **_tags)
{
	GKeyFile *keyfile = load_servers_keyfile();

	if (!keyfile)
		return FALSE;

	gboolean has_tag = FALSE;
	gchar *tmp = g_key_file_get_string(keyfile, server, "tags", NULL);
	gchar **tags = g_strsplit(tmp, ",", -1);
	g_free(tmp);

	for (int i = 0; tags[i]; i++)
		if (g_strcmp0(tags[i], tag) == 0)
			has_tag = TRUE;

	if (!has_tag) {
		gchar *tmp = g_strjoinv(",", tags);
		gchar *str;

		if (*tmp)
			str = g_strconcat(tmp, ",", tag, NULL);
		else
			str = g_strdup(tag);


		g_debug("Before %s | After %s", tmp, str);

		g_key_file_set_string(keyfile, server, "tags", str);
		g_free(tmp);

		if (_tags)
			*_tags = str;
		else
			g_free(str);

		save_servers_keyfile(keyfile);
	}

	g_key_file_free(keyfile);
	g_strfreev(tags);

	return !has_tag;
}

static gboolean
gebrm_config_remove_tag_of_server(GebrmApp *app,
                                  const gchar *server,
                                  const gchar *tag,
                                  gchar **_tags)
{
	GKeyFile *keyfile = load_servers_keyfile();

	if (!keyfile)
		return FALSE;

	gboolean has_tag = FALSE;
	gchar *tmp = g_key_file_get_string(keyfile, server, "tags", NULL);
	gchar **tags = g_strsplit(tmp, ",", -1);
	g_free(tmp);

	GString *buf = g_string_new("");
	for (int i = 0; tags[i]; i++) {
		if (g_strcmp0(tags[i], tag) != 0)
			g_string_append_printf(buf, "%s,", tags[i]);
		else
			has_tag = TRUE;
	}

	if (has_tag) {
		if(buf->len)
			g_string_erase(buf, buf->len -1, 1);

		g_key_file_set_string(keyfile, server, "tags", buf->str);
		save_servers_keyfile(keyfile);

		if(_tags)
			*_tags = g_strdup(buf->str);
	}

	g_key_file_free(keyfile);
	g_strfreev(tags);
	g_string_free(buf, TRUE);

	return has_tag;
}

static gboolean
gebrm_config_set_autoconnect(GebrmApp *app,
			     const gchar *server,
			     const gchar *ac)
{
	GKeyFile *keyfile = load_servers_keyfile();

	if (!keyfile)
		return FALSE;

	g_key_file_set_string(keyfile, server, "autoconnect", ac);
	gboolean succ = save_servers_keyfile(keyfile);

	g_key_file_free(keyfile);

	return succ;
}

static void
gebrm_config_save_server(GebrmDaemon *daemon)
{
	GKeyFile *servers = g_key_file_new ();
	const gchar *path = gebr_maestro_settings_get_servers_file();
	gchar *tags = gebrm_daemon_get_tags(daemon);
	const gchar *daemon_addr = gebrm_daemon_get_address(daemon);

	g_key_file_load_from_file(servers, path, G_KEY_FILE_NONE, NULL);
	if (!g_key_file_has_group(servers, daemon_addr)){
	    g_key_file_set_string(servers, gebrm_daemon_get_address(daemon),
				  "tags", tags);
	    g_key_file_set_string(servers, gebrm_daemon_get_address(daemon),
				  "autoconnect", gebrm_daemon_get_autoconnect(daemon));

	    gchar *content = g_key_file_to_data(servers, NULL, NULL);
	    if (content)
	    	g_file_set_contents(path, content, -1, NULL);

	    g_free(content);
	}
	g_free(tags);
	g_key_file_free(servers);
}

static void
gebrm_config_delete_server(const gchar *server)
{
	gchar *final_list_str;
	GKeyFile *servers;

	servers = g_key_file_new ();

	const gchar *path = gebr_maestro_settings_get_servers_file();

	g_key_file_load_from_file (servers, path, G_KEY_FILE_NONE, NULL);

	if (!g_key_file_has_group(servers, server))
		g_debug("File doesn't have:%s", server);

	if (g_key_file_remove_group(servers, server, NULL))
		g_debug("Remove server %s from file", server);

	final_list_str = g_key_file_to_data(servers, NULL, NULL);
	g_file_set_contents(path, final_list_str, -1, NULL);
	g_free(final_list_str);
	g_key_file_free(servers);
}

static gboolean
load_servers_from_key_file(GebrmApp *app,
                           GKeyFile *servers)
{
	gchar **groups = g_key_file_get_groups(servers, NULL);
	if (!groups)
		return FALSE;
	else {
		for (int i = 0; groups[i]; i++) {
			const gchar *tags = g_key_file_get_string(servers, groups[i], "tags", NULL);
			const gchar *ac = g_key_file_get_string(servers, groups[i], "autoconnect", NULL);
			GebrmDaemon *daemon = gebrm_add_server_to_list(app, groups[i], tags);
			gebrm_daemon_set_autoconnect(daemon, ac);
		}
	}
	g_strfreev(groups);

	return TRUE;
}

gboolean
gebrm_config_load_admin_servers(GebrmApp *app)
{
	GKeyFile *adm_servers = load_admin_servers_keyfile();

	if (adm_servers) {
		const gchar *path = gebr_maestro_settings_get_servers_file();
		if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
			load_servers_from_key_file(app, adm_servers);
			save_servers_keyfile(adm_servers);
		}
		g_key_file_free(adm_servers);
	}

	return TRUE;
}

gboolean
gebrm_config_load_servers(GebrmApp *app, const gchar *path)
{
	GKeyFile *servers = load_servers_keyfile();
	gboolean succ = servers != NULL;

	if (succ) {
		load_servers_from_key_file(app, servers);
		g_key_file_free (servers);
	}

	return TRUE;
}

static void
on_client_disconnect(GebrCommProtocolSocket *socket,
		     GebrmApp *app)
{
	g_debug("Client disconnect!");
	GebrmClient *client = g_object_get_data(G_OBJECT(socket), "client");
	app->priv->connections = g_list_remove(app->priv->connections, client);
	gebrm_client_remove_forwards(client);

	GList *head = g_queue_peek_head_link(app->priv->xauth_queue);
	for (GList *i = head; i; i = i->next) {
		XauthQueueData *xauth = i->data;
		if (client == xauth->client) {
			g_queue_delete_link(app->priv->xauth_queue, i);
			g_object_unref(xauth->daemon);
			g_object_unref(xauth->client);
			g_free(xauth);
		}
	}
}

static void
send_messages_of_jobs(const gchar *id,
                      GebrmJob *job,
                      GebrCommProtocolSocket *protocol)
{

	gchar *infile, *outfile, *logfile;

	gebrm_job_get_io(job, &infile, &outfile, &logfile);

	const gchar *start_date = gebrm_job_get_start_date(job);
	const gchar *finish_date = gebrm_job_get_finish_date(job);
	const gchar *description = gebrm_job_get_description(job);

	/* Job def message */
	gebr_comm_protocol_socket_oldmsg_send(protocol, FALSE,
	                                      gebr_comm_protocol_defs.job_def, 26,
	                                      id,
					      gebrm_job_get_temp_id(job),
					      gebrm_job_get_flow_id(job),
					      gebrm_job_get_nprocs(job),
	                                      gebrm_job_get_servers_list(job),
	                                      gebrm_job_get_hostname(job),
	                                      gebrm_job_get_title(job),
	                                      gebrm_job_get_job_counter(job),
					      description ? description : "",
	                                      gebrm_job_get_snapshot_title(job),
	                                      gebrm_job_get_snapshot_id(job),
	                                      gebrm_job_get_queue(job),
	                                      gebrm_job_get_nice(job),
	                                      infile,
	                                      outfile,
	                                      logfile,
	                                      gebrm_job_get_submit_date(job),
	                                      gebrm_job_get_server_group(job),
	                                      gebrm_job_get_server_group_type(job),
	                                      gebrm_job_get_exec_speed(job),
	                                      gebr_comm_job_get_string_from_status(gebrm_job_get_status(job)),
	                                      start_date? start_date : "",
	                                      finish_date? finish_date : "",
	                                      gebrm_job_get_run_type(job),
	                                      gebrm_job_get_mpi_owner(job),
	                                      gebrm_job_get_mpi_flavor(job));

	GList *tasks = gebrm_job_get_list_of_tasks(job);
	
	/* Output message*/
	gchar *frac;
	for(GList *i = tasks; i; i = i->next) {
		frac = g_strdup_printf("%d", gebrm_task_get_fraction(i->data));
		gebr_comm_protocol_socket_oldmsg_send(protocol, FALSE,
						      gebr_comm_protocol_defs.out_def, 3,
						      id,
						      frac,
						      gebrm_task_get_output(i->data));

		g_free(frac);
	}

	/* Command line message */
	for(GList *i = tasks; i; i = i->next) {
		frac = g_strdup_printf("%d", gebrm_task_get_fraction(i->data));
		gebr_comm_protocol_socket_oldmsg_send(protocol, FALSE,
		                                      gebr_comm_protocol_defs.cmd_def, 3,
		                                      id,
		                                      frac,
		                                      gebrm_task_get_cmd_line(i->data));
		g_free(frac);
	}

	/* Issues message */
	const gchar *issues = gebrm_job_get_issues(job);
	if (issues && *issues)
		gebr_comm_protocol_socket_oldmsg_send(protocol, FALSE,
		                                      gebr_comm_protocol_defs.iss_def, 2,
		                                      id,
		                                      issues);

	g_free(infile);
	g_free(outfile);
	g_free(logfile);
}

static void
on_new_connection(GebrCommListenSocket *listener,
		  GebrmApp *app)
{
	GebrCommStreamSocket *stream;

	g_debug("New connection!");

	while ((stream = gebr_comm_listen_socket_get_next_pending_connection(listener))) {
		GebrmClient *client = gebrm_client_new(stream);
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(client);
		g_object_set_data(G_OBJECT(socket), "client", client);
		g_object_unref(stream);

		app->priv->connections = g_list_prepend(app->priv->connections, client);

		if (!app->priv->home && app->priv->daemons) {
			const gchar *home = gebrm_daemon_get_home_dir(app->priv->daemons->data);
			gebrm_app_send_home_dir(app, socket, home);
		}
		// Reload Maestro Settings
		gebr_maestro_settings_update(app->priv->settings);

		g_signal_connect(socket, "disconnected",
				 G_CALLBACK(on_client_disconnect), app);
		g_signal_connect(socket, "process-request",
				 G_CALLBACK(on_client_request), app);
		g_signal_connect(socket, "old-parse-messages",
				 G_CALLBACK(on_client_parse_messages), app);

		g_hash_table_foreach(app->priv->jobs, (GHFunc)send_messages_of_jobs, socket);
	}
}

GebrmApp *
gebrm_app_new(void)
{
	return g_object_new(GEBRM_TYPE_APP, NULL);
}

void
gebrm_app_create_possible_daemon_list(GebrMaestroSettings *ms,
                                      GebrmApp *app)
{
	/* Add servers from admin file */
	gebrm_config_load_admin_servers(app);

	/* Add servers from user file */
	const gchar *path = gebr_maestro_settings_get_servers_file();
	gebrm_config_load_servers(app, path);

	/* Add localhost */
	gebrm_add_server_to_list(app, g_get_host_name(), "");
}

gboolean
gebrm_app_run(GebrmApp *app, int fd, const gchar *version)
{
	GError *error_lock = NULL;
	GError *error_version = NULL;

	GebrCommSocketAddress address = gebr_comm_socket_address_ipv4("127.0.0.1", 0);
	app->priv->listener = gebr_comm_listen_socket_new();

	g_signal_connect(app->priv->listener, "new-connection",
			 G_CALLBACK(on_new_connection), app);
	
	if (!gebr_comm_listen_socket_listen(app->priv->listener, &address)) {
		g_critical("Failed to start listener");
		return FALSE;
	}

	app->priv->address =
		gebr_comm_socket_get_address(GEBR_COMM_SOCKET(app->priv->listener));
	guint16 port = gebr_comm_socket_address_get_ip_port(&app->priv->address);

	gchar *lock_contents = g_strdup_printf("%s:%d", g_get_host_name(), port);

	g_file_set_contents(gebrm_app_get_lock_file(),
			    lock_contents, -1, &error_lock);

	g_free(lock_contents);

	g_file_set_contents(gebrm_app_get_version_file(),
			    version, -1, &error_version);

	if (error_lock) {
		g_critical("Could not create lock: %s", error_lock->message);
		return FALSE;
	}

	if (error_version) {
		g_critical("Could not create version file: %s", error_version->message);
		return FALSE;
	}

	/* success, send port and address */
	const gchar *username = g_get_user_name();
	gchar *addr;
	if (username && *username)
		addr = g_strdup_printf("%s@%s", username, g_get_host_name());
	else
		addr = g_strdup(g_get_host_name());

	gchar *port_str = g_strdup_printf("%s%u\n%s%s\n",
	                                  GEBR_PORT_PREFIX, port,
	                                  GEBR_ADDR_PREFIX, addr);
	g_free(addr);

	ssize_t s = 0;
	do {
		s = write(fd, port_str + s, strlen(port_str) - s);
		if (s == -1)
			exit(-1);
	} while(s != 0);

	g_free(port_str);

	//Generate gebr.key
	gebr_generate_key();

	// Create configuration for NFS
	app->priv->settings = gebrm_app_create_configuration();

	g_main_loop_run(app->priv->main_loop);

	return TRUE;
}

static gchar *
gebrm_app_build_path(const gchar *last_folder)
{
	const gchar *dirname = gebr_maestro_settings_get_directory();
	gchar *path = g_build_filename(dirname, last_folder, NULL);
	return path;
}

gboolean
gebrm_app_create_folder_for_addr(const gchar *addr)
{
	gchar *path = gebrm_app_build_path(addr);
	gboolean succ = g_mkdir_with_parents(path, 0755) == 0;
	g_free(path);
	return succ;
}

const gchar *
gebrm_app_get_lock_file(void)
{
	static gchar *lock = NULL;

	if (!lock)
		lock = gebrm_app_build_path("singleton_lock");

	return lock;
}

const gchar *
gebrm_app_get_version_file(void)
{
	static gchar *version = NULL;

	if (!version)
		version = gebrm_app_build_path("version");

	return version;
}

const gchar *
gebrm_app_get_log_file_for_address(const gchar *addr)
{
	static gchar *logfile = NULL;

	if (!logfile) {
		gchar *last_folder = g_build_filename(addr, "log", NULL);
		logfile = gebrm_app_build_path(last_folder);
		g_free(last_folder);
	}

	return logfile;
}
const gchar *
gebrm_app_get_version_file_for_addr(const gchar *addr)
{
	static gchar *version = NULL;

	if (!version) {
		gchar *dirname = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", addr, NULL);
		version = g_build_filename(dirname, "version", NULL);
		g_free(dirname);
	}

	return version;
}

const gchar *
gebrm_app_get_admin_servers_file(void)
{
	static gchar *path = NULL;

	if (!path)
		path = g_build_filename("/etc", "gebr", "servers.conf", NULL);

	return path;
}

static gint
gebrm_app_increment_jobs_counter(GebrmApp *app, const gchar *flow_id)
{
	gint *counter = g_hash_table_lookup(app->priv->jobs_counter, flow_id);
	gint *new_counter = g_new0(gint,1);

	*new_counter = counter ? *counter + 1 : 1;
	g_hash_table_replace(app->priv->jobs_counter, g_strdup(flow_id), new_counter);

	return *new_counter;
}
