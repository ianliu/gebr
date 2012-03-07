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

	GQueue *job_def_queue;
	GQueue *job_run_queue;
	GQueue *xauth_queue;

	// Server groups: gchar -> GList<GebrDaemon>
	GTree *groups;

	// Job controller
	GHashTable *jobs;
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
} XauthQueueData;

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
					     const gchar *pass,
					     const gchar *tags);

static gboolean gebrm_update_tags_on_list_of_servers(GebrmApp *app,
						     const gchar *address,
						     const gchar *tags);

static void gebrm_config_delete_server(const gchar *serv);

static gboolean gebrm_remove_server_from_list(GebrmApp *app, const gchar *address);

gboolean gebrm_config_load_servers(GebrmApp *app, const gchar *path);

static void send_job_def_to_clients(GebrmApp *app, GebrmJob *job);

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
// }}}

static void
gebrm_app_send_home_dir(GebrmApp *app, GebrCommProtocolSocket *socket, const gchar *home)
{
	app->priv->home = g_strdup(home);
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.home_def, 1,
					      home);
}


static gboolean
gebrm_app_send_mpi_flavors(GebrCommProtocolSocket *socket, GebrmDaemon *daemon)
{
	if (!daemon)
		return FALSE;
	gchar *flavors = gebrm_daemon_get_mpi_flavors(daemon);

	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.mpi_def, 2,
					      gebrm_daemon_get_address(daemon),
					      flavors);
	g_debug("on maestro, function %s,  daemon %s, SENDING MPI FLAVOR: %s", __func__, gebrm_daemon_get_address(daemon), flavors)  ;
	return TRUE;
}

static void
send_server_status_message(GebrmApp *app,
			   GebrCommProtocolSocket *socket,
			   GebrmDaemon *daemon,
			   const gchar *ac)
{
	const gchar *state = gebr_comm_server_state_to_string(gebrm_daemon_get_state(daemon));
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.ssta_def, 4,
					      gebrm_daemon_get_hostname(daemon),
					      gebrm_daemon_get_address(daemon),
					      state,
					      ac);

	gebrm_daemon_send_error_message(daemon, socket);
}

static void
send_groups_definitions(GebrCommProtocolSocket *client, GebrmDaemon *daemon)
{
	const gchar *server = gebrm_daemon_get_address(daemon);
	const gchar *tags = gebrm_daemon_get_tags(daemon);

	g_debug("Sending groups %s (%s) to gebr", tags, server);
	gebr_comm_protocol_socket_oldmsg_send(client, FALSE,
					      gebr_comm_protocol_defs.agrp_def, 2,
					      server, tags);

}

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
	}
	g_debug("On %s, daemon:%s", __func__, gebrm_daemon_get_address(daemon));

	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		send_server_status_message(app, socket, daemon, gebrm_daemon_get_autoconnect(daemon));
	}
}

static void
gebrm_app_finalize(GObject *object)
{
	GebrmApp *app = GEBRM_APP(object);
	g_hash_table_unref(app->priv->jobs);
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
				      gebrm_client_get_magic_cookie(xauth->client));
	g_object_unref(xauth->daemon);
	g_object_unref(xauth->client);
	g_free(xauth);

	return TRUE;
}

static void
queue_client_info(GebrmApp *app, GebrmDaemon *daemon, GebrmClient *client)
{
	XauthQueueData *xauth = g_new(XauthQueueData, 1);
	xauth->daemon = g_object_ref(daemon);
	xauth->client = g_object_ref(client);
	g_queue_push_tail(app->priv->xauth_queue, xauth);
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
	app->priv->job_def_queue = g_queue_new();
	app->priv->job_run_queue = g_queue_new();
	app->priv->xauth_queue = g_queue_new();

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
	for (GList *i = app->priv->daemons; i; i = i->next) {
		const gchar *tmp = gebrm_daemon_get_id(i->data);
		if (g_strcmp0(tmp, id) == 0)
			return TRUE;
	}

	return FALSE;
}

static void
on_daemon_init(GebrmDaemon *daemon,
	       const gchar *error_type,
	       const gchar *error_msg,
	       GebrmApp *app)
{
	const gchar *error = NULL;
	const gchar *nfsid = gebrm_daemon_get_nfsid(daemon);
	const gchar *home = gebrm_daemon_get_home_dir(daemon);
	gboolean remove = FALSE;
	gboolean home_defined = FALSE;

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

	if (!app->priv->nfsid) {
		app->priv->nfsid = g_strdup(nfsid);
	} else if (g_strcmp0(app->priv->nfsid, nfsid) != 0) {
		error = "error:nfs";
		goto err;
	}

err:
	gebrm_daemon_set_error_type(daemon, error);
	gebrm_daemon_set_error_msg(daemon, error_msg);

	if (error) {
		for (GList *i = app->priv->connections; i; i = i->next) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
			gebrm_daemon_send_error_message(daemon, socket);
		}

		if (remove)
			remove_daemon(app, gebrm_daemon_get_address(daemon));
	} else {
		if (app->priv->home)
			home_defined = TRUE;

		for (GList *i = app->priv->connections; i; i = i->next) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
			if (!home_defined)
				gebrm_app_send_home_dir(app, socket, home);
			send_server_status_message(app, socket, daemon, gebrm_daemon_get_autoconnect(daemon));

			queue_client_info(app, daemon, i->data);
			gebrm_app_send_mpi_flavors(socket, daemon);
		}
	}
}

static void
on_daemon_port_define(GebrmDaemon *daemon,
		      const gchar *gid,
		      const gchar *port,
		      GebrmApp *app)
{
	GebrmClient *client = NULL;
	for (GList *i = app->priv->connections; i; i = i->next) {
		const gchar *id = gebrm_client_get_id(i->data);
		if (g_strcmp0(id, gid) == 0) {
			client = i->data;
			break;
		}
	}

	if (client) {
		if (g_strcmp0(port, "0") == 0) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(client);
			gebr_log(GEBR_LOG_ERROR, "Received port zero from %s for gid %s. Sending error to client.",
				 gebrm_daemon_get_address(daemon), gid);
			gebrm_daemon_send_error_message(daemon, socket);
		} else {
			GebrCommServer *server = gebrm_daemon_get_server(daemon);
			gebrm_client_add_forward(client, server, atoi(port));
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
static GebrmDaemon *
gebrm_add_server_to_list(GebrmApp *app,
			 const gchar *address,
			 const gchar *pass,
			 const gchar *tags)
{
	GebrmDaemon *daemon;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		daemon = i->data;
		if (g_strcmp0(address, gebrm_daemon_get_address(daemon)) == 0) {
			return daemon;
		}
	}

	daemon = gebrm_daemon_new(address);
	g_signal_connect(daemon, "state-change",
			 G_CALLBACK(gebrm_app_daemon_on_state_change), app);
	g_signal_connect(daemon, "task-define",
			 G_CALLBACK(gebrm_app_job_controller_on_task_def), app);
	g_signal_connect(daemon, "daemon-init",
			 G_CALLBACK(on_daemon_init), app);
	g_signal_connect(daemon, "port-define",
			 G_CALLBACK(on_daemon_port_define), app);
	g_signal_connect(daemon, "ret-path",
			 G_CALLBACK(on_daemon_ret_path), app);

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
	GebrmDaemon *daemon;
	gboolean exist = FALSE;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		daemon = i->data;
		if (g_strcmp0(address, gebrm_daemon_get_address(daemon)) == 0) {
			exist = TRUE;
			break;
		}
	}

	if (!exist)
		g_warn_if_reached();

	gchar **tagsv = tags ? g_strsplit(tags, ",", -1) : NULL;
	gebrm_daemon_update_tags(daemon, tagsv);

	return TRUE;
}

static GList *
get_comm_servers_list(GebrmApp *app, const gchar *group, const gchar *group_type, const gchar *mpi)
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

	if (*mpi) {
		for (GList *i = servers; i; i = i->next) {
			if (!gebrm_daemon_accepts_mpi(i->data, mpi))
				servers = g_list_delete_link(servers, i);
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

	for (GList *i = app->priv->connections; i; i = i->next) {
		GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(i->data);
		g_debug("-----------------------%s",gebrm_job_get_nprocs(job));
		gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
						      gebr_comm_protocol_defs.job_def, 18,
						      gebrm_job_get_id(job),
						      gebrm_job_get_temp_id(job),
						      gebrm_job_get_nprocs(job),
						      gebrm_job_get_servers_list(job),
						      gebrm_job_get_hostname(job),
						      gebrm_job_get_title(job),
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
						      finish_date? finish_date : "");
	}
}

static void
on_execution_response(GebrCommRunner *runner,
		      gpointer data)
{
	AppAndJob *aap = data;

	gebrm_job_set_total_tasks(aap->job, gebr_comm_runner_get_total(runner));
	gebrm_job_set_servers_list(aap->job, gebr_comm_runner_get_servers_list(runner));
	g_debug("on %s, ncores:%s", __func__, gebr_comm_runner_get_ncores(runner));
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
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *pass = gebr_comm_uri_get_param(uri, "pass");

			GebrmDaemon *d = gebrm_add_server_to_list(app, addr, pass, NULL);

			gebrm_daemon_connect(d, pass, socket);
			gebrm_config_save_server(d);
		}
		if (g_strcmp0(prefix, "/ssh-answer") == 0) {
			GebrmDaemon *daemon = NULL;
			const gchar *addr = gebr_comm_uri_get_param(uri, "address");
			const gchar *resp = gebr_comm_uri_get_param(uri, "response");

			for (GList *i = app->priv->daemons; !daemon && i; i = i->next)
				if (g_strcmp0(addr, gebrm_daemon_get_address(i->data)) == 0)
					daemon = i->data;
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
			GebrCommJsonContent *json;

			const gchar *gid         = gebr_comm_uri_get_param(uri, "gid");
			const gchar *parent_id   = gebr_comm_uri_get_param(uri, "parent_id");
			const gchar *temp_parent = gebr_comm_uri_get_param(uri, "temp_parent");
			const gchar *speed       = gebr_comm_uri_get_param(uri, "speed");
			const gchar *nice        = gebr_comm_uri_get_param(uri, "nice");
			const gchar *name        = gebr_comm_uri_get_param(uri, "name");
			const gchar *server_host = gebr_comm_uri_get_param(uri, "server-hostname");
			const gchar *group_type  = gebr_comm_uri_get_param(uri, "group_type");
			const gchar *host        = gebr_comm_uri_get_param(uri, "host");
			const gchar *temp_id     = gebr_comm_uri_get_param(uri, "temp_id");
			const gchar *paths       = gebr_comm_uri_get_param(uri, "paths");

			if (temp_parent) {
				parent_id = gebrm_client_get_job_id_from_temp(client,
									      temp_parent);
				g_debug("Got parent_id %s from temp id %s", parent_id, temp_parent);
			}

			json = gebr_comm_json_content_new(request->content->str);
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

			GebrmJobInfo info = { 0, };
			info.title = g_strdup(title);
			info.temp_id = g_strdup(temp_id);
			info.hostname = g_strdup(host);
			info.parent_id = g_strdup(parent_id);
			info.servers = g_strdup("");
			info.nice = g_strdup(nice);
			info.input = gebr_geoxml_flow_io_get_input(*pflow);
			info.output = gebr_geoxml_flow_io_get_output(*pflow);
			info.error = gebr_geoxml_flow_io_get_error(*pflow);
			if (server_host)
				info.group = g_strdup(server_host);
			else
				info.group = g_strdup(name);
			info.group_type = g_strdup(group_type);
			info.speed = g_strdup(speed);
			info.submit_date = g_strdup(gebr_iso_date());

			GebrmJob *job = gebrm_job_new();
			gebrm_client_add_temp_id(client, temp_id, gebrm_job_get_id(job));
			g_debug("Associating temp_id %s with id %s", temp_id, gebrm_job_get_id(job));

			g_signal_connect(job, "status-change",
					 G_CALLBACK(gebrm_app_job_controller_on_status_change), app);
			g_signal_connect(job, "issued",
					 G_CALLBACK(gebrm_app_job_controller_on_issued), app);
			g_signal_connect(job, "cmd-line-received",
					 G_CALLBACK(gebrm_app_job_controller_on_cmd_line_received), app);
			g_signal_connect(job, "output",
					 G_CALLBACK(gebrm_app_job_controller_on_output), app);

			gebrm_job_init_details(job, &info);
			gebrm_job_info_free(&info);
			gebrm_app_job_controller_add(app, job);

			const gchar *mpi;
			GebrGeoXmlProgram *mpi_prog = gebr_geoxml_flow_get_first_mpi_program(*pflow);
			if (mpi_prog)
				mpi = gebr_geoxml_program_get_mpi(mpi_prog);
			else
				mpi = "";
			gebr_geoxml_object_unref(mpi_prog);

			GList *servers = get_comm_servers_list(app, name, group_type, mpi);
			GebrCommRunner *runner = gebr_comm_runner_new(GEBR_GEOXML_DOCUMENT(*pflow),
			                                              servers, gebrm_job_get_id(job),
								      gid, parent_id, speed, nice,
								      name, paths, validator);
			g_list_free(servers);

			AppAndJob *aap = g_new(AppAndJob, 1);
			aap->app = app;
			aap->job = job;
			gebr_comm_runner_set_ran_func(runner, on_execution_response, aap);

			g_debug("Queue: '%s'", parent_id);

			if (parent_id[0] == '\0') {
				g_debug("Running immediately");
				g_queue_push_head(app->priv->job_def_queue, job);
				if (g_queue_is_empty(app->priv->job_run_queue)
				    && !gebr_comm_runner_run_async(runner))
					gebrm_job_kill_immediately(job);
				g_queue_push_tail(app->priv->job_run_queue, runner);
			} else {
				GebrmJob *parent = gebrm_app_job_controller_find(app, parent_id);
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

			g_free(title);
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
			const gchar *server = gebr_comm_uri_get_param(uri, "server");
			const gchar *is_ac  = gebr_comm_uri_get_param(uri, "ac");

			GebrmDaemon *daemon;
			for (GList *i = app->priv->daemons; i; i = i->next) {
				const gchar *addr = gebrm_daemon_get_address(i->data);
				if (g_strcmp0(addr, server) == 0) {
					daemon = i->data;
					break;
				}
			}
			gebrm_daemon_set_autoconnect(daemon, is_ac);

			if (gebrm_config_set_autoconnect(app, server, is_ac)) {
				for (GList *i = app->priv->connections; i; i = i->next) {
					GebrCommProtocolSocket *socket_client = gebrm_client_get_protocol_socket(i->data);
					gebr_comm_protocol_socket_oldmsg_send(socket_client, FALSE,
					                                      gebr_comm_protocol_defs.ac_def, 2,
					                                      server,
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

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *version = g_list_nth_data(arguments, 0);
			GString *cookie  = g_list_nth_data(arguments, 1);
			GString *gebr_id = g_list_nth_data(arguments, 2);

			g_debug("Maestro received a X11 cookie: %s", cookie->str);

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

			gebrm_client_set_id(client, gebr_id->str);
			gebrm_client_set_magic_cookie(client, cookie->str);

			guint16 client_display_port = gebrm_client_get_display_port(client);
			gchar *port_str = g_strdup_printf("%d", client_display_port);
			gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 1,
							      port_str);

			for (GList *i = app->priv->daemons; i; i = i->next) {
				GebrCommServerState state = gebrm_daemon_get_state(i->data);
				if ( state == SERVER_STATE_LOGGED || state == SERVER_STATE_DISCONNECTED) {
					queue_client_info(app, i->data, client);
					send_server_status_message(app, socket, i->data, gebrm_daemon_get_autoconnect(i->data));
					send_groups_definitions(socket, i->data);
				}
				if ( state == SERVER_STATE_LOGGED) 
					gebrm_app_send_mpi_flavors(socket, i->data);
			}

			if (app->priv->home)
				gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
								      gebr_comm_protocol_defs.home_def, 1,
								      app->priv->home);

			g_free(port_str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.path_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *new_path = g_list_nth_data(arguments, 0);
			GString *old_path = g_list_nth_data(arguments, 1);
			GString *option = g_list_nth_data(arguments, 2);


			GebrmDaemon *daemon = app->priv->daemons->data;

			g_debug("Send to daemon: option: %s, new_path: %s, old_path: %s",
				option->str, new_path->str, old_path->str);

			GebrCommServer *comm_server = gebrm_daemon_get_server(daemon);
			gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, TRUE,
							      gebr_comm_protocol_defs.path_def, 3,
							      new_path->str,
							      old_path->str,
							      option->str);
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
	const gchar *path = gebrm_app_get_servers_file();
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
		const gchar *path = gebrm_app_get_servers_file();
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
	const gchar *path = gebrm_app_get_servers_file();
	gchar *tags = gebrm_daemon_get_tags(daemon);
	const gchar *daemon_addr = gebrm_daemon_get_address(daemon);

	if (g_strcmp0(daemon_addr, "127.0.0.1") == 0 || 
	    g_strcmp0(daemon_addr,"localhost") == 0 )
		daemon_addr = g_get_host_name(); 

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
gebrm_config_delete_server(const gchar *serv)
{
	gchar *server;
	gchar *final_list_str;
	GKeyFile *servers;

	server = g_strcmp0(serv, "127.0.0.1") ? g_strdup(serv): g_strdup("localhost");
	servers = g_key_file_new ();

	const gchar *path = gebrm_app_get_servers_file();

	g_key_file_load_from_file (servers, path, G_KEY_FILE_NONE, NULL);

	if (!g_key_file_has_group(servers, server))
		g_debug("File doesn't have:%s", server);

	if (g_key_file_remove_group(servers, server, NULL))
		g_debug("Remove server %s from file", server);

	final_list_str = g_key_file_to_data(servers, NULL, NULL);
	g_file_set_contents(path, final_list_str, -1, NULL);
	g_free(server);
	g_free(final_list_str);
	g_key_file_free(servers);
}

gboolean
gebrm_config_load_servers(GebrmApp *app, const gchar *path)
{
	GKeyFile *servers = load_servers_keyfile();
	gboolean succ = servers != NULL;

	if (succ) {
		gchar **groups = g_key_file_get_groups(servers, NULL);
		if (!groups)
			succ = FALSE;
		else {
			for (int i = 0; groups[i]; i++) {
				const gchar *tags = g_key_file_get_string(servers, groups[i], "tags", NULL);
				const gchar *ac = g_key_file_get_string(servers, groups[i], "autoconnect", NULL);
				GebrmDaemon *daemon = gebrm_add_server_to_list(app, groups[i], NULL, tags);
				gebrm_daemon_set_autoconnect(daemon, ac);
				if (g_strcmp0(ac, "on") == 0)
					gebrm_daemon_connect(daemon, NULL, NULL);
			}
		}
		g_key_file_free (servers);
		g_strfreev(groups);
	}

	if (!succ) {
		GebrmDaemon *daemon = gebrm_add_server_to_list(app, g_get_host_name(), NULL, "");
		gebrm_daemon_connect(daemon, NULL, NULL);
		gebrm_config_save_server(daemon);
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

	for (GList *i = app->priv->daemons; i; i = i->next)
		gebr_comm_protocol_socket_oldmsg_send(gebrm_daemon_get_server(i->data)->socket, FALSE,
						      gebr_comm_protocol_defs.rmck_def, 2,
						      gebrm_client_get_id(client),
						      gebrm_client_get_magic_cookie(client));
}

static void
send_messages_of_jobs(gpointer key,
                      gpointer value,
                      gpointer data)
{
	GebrCommProtocolSocket *protocol = data;
	const gchar *id = key;
	GebrmJob *job = value;

	gchar *infile, *outfile, *logfile;

	gebrm_job_get_io(job, &infile, &outfile, &logfile);

	const gchar *start_date = gebrm_job_get_start_date(job);
	const gchar *finish_date = gebrm_job_get_finish_date(job);

	/* Job def message */
	gebr_comm_protocol_socket_oldmsg_send(protocol, FALSE,
	                                      gebr_comm_protocol_defs.job_def, 18,
	                                      id,
					      gebrm_job_get_temp_id(job),
	                                      gebrm_job_get_nprocs(job),
	                                      gebrm_job_get_servers_list(job),
	                                      gebrm_job_get_hostname(job),
	                                      gebrm_job_get_title(job),
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
	                                      finish_date? finish_date : "");

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

		for (GList *i = app->priv->daemons; i; i = i->next) {
			if (g_strcmp0(gebrm_daemon_get_autoconnect(i->data), "on") == 0) {
				switch (gebrm_daemon_get_state(i->data)) {
				case SERVER_STATE_DISCONNECTED:
					gebrm_daemon_connect(i->data, NULL, socket);
					break;
				case SERVER_STATE_RUN:
					gebrm_daemon_continue_stuck_connection(i->data, socket);
					break;
				default:
					break;
				}
			}
		}

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

gboolean
gebrm_app_run(GebrmApp *app, int fd)
{
	GError *error = NULL;

	GebrCommSocketAddress address = gebr_comm_socket_address_ipv4_local(0);
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
	gchar *portstr = g_strdup_printf("%d", port);

	g_file_set_contents(gebrm_app_get_lock_file(),
			    portstr, -1, &error);

	if (error) {
		g_critical("Could not create lock: %s", error->message);
		return FALSE;
	}

	/* success, send port */
	gchar *port_str = g_strdup_printf("%u\n", port);
	write(fd, port_str, strlen(port_str));
	g_free(port_str);

	const gchar *path = gebrm_app_get_servers_file();
	gebrm_config_load_servers(app, path);

	g_main_loop_run(app->priv->main_loop);

	return TRUE;
}

static gchar *
get_gebrm_dir_name(void)
{
	gchar *dirname = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", g_get_host_name(), NULL);
	if (!g_file_test(dirname, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dirname, 0755);
	return dirname;
}

const gchar *
gebrm_app_get_lock_file(void)
{
	static gchar *lock = NULL;

	if (!lock) {
		gchar *dirname = get_gebrm_dir_name();
		lock = g_build_filename(dirname, "lock", NULL);
		g_free(dirname);
	}

	return lock;
}

const gchar *
gebrm_app_get_servers_file(void)
{
	static gchar *path = NULL;

	if (!path) {
		gchar *dirname = get_gebrm_dir_name();
		path = g_build_filename(dirname, "servers.conf", NULL);
		g_free(dirname);
	}

	return path;
}
