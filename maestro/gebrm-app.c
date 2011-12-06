/*
 * gebrm-app.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
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

#include <glib/gprintf.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <glib/gi18n.h>

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GebrCommListenSocket *listener;
	GebrCommSocketAddress address;
	GList *connections;
	GList *daemons;

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

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;

#define GEBRM_LIST_OF_SERVERS_PATH ".gebr/gebrm"
#define GEBRM_LIST_OF_SERVERS_FILENAME "servers.conf"

static void gebrm_config_save_server(GebrmDaemon *daemon);

static gboolean gebrm_config_update_tags_on_server(GebrmApp *app,
                                                   gchar *server,
                                                   gchar *tags);

static gboolean gebrm_config_insert_tag_on_server(GebrmApp *app,
						  gchar *server,
						  gchar *tag,
						  gchar **_tags);

static gboolean gebrm_config_remove_tag_of_server(GebrmApp *app,
                                                  gchar *server,
                                                  gchar *tag,
                                                  gchar **_tags);

static GebrmDaemon *gebrm_add_server_to_list(GebrmApp *app,
					     const gchar *addr,
					     const gchar *pass,
					     const gchar *tags);

static gboolean gebrm_update_tags_on_list_of_servers(GebrmApp *app,
						     const gchar *address,
						     const gchar *tags);

static void gebrm_config_delete_server(const gchar *serv);

static gboolean gebrm_remove_server_from_list(GebrmApp *app, const gchar *address);

gboolean gebrm_config_load_servers(GebrmApp *app, gchar *path);

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
send_server_status_message(GebrmApp *app,
			   GebrCommProtocolSocket *socket,
			   GebrCommServer *server)
{
	const gchar *state = gebr_comm_server_state_to_string(server->state);
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.ssta_def, 2,
					      server->address->str,
					      state);
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
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
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
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
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
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
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
			gebr_comm_runner_run_async(raj->runner, gebrm_job_get_id(raj->job));
		}
		g_list_foreach(children, (GFunc)g_free, NULL);
		g_list_free(children);
		g_object_set_data(G_OBJECT(job), "children", NULL);
	}

	for (GList *i = app->priv->connections; i; i = i->next) {
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.sta_def, 3,
						      gebrm_job_get_id(job),
						      gebr_comm_job_get_string_from_status(new_status),
						      parameter);
	}

}

static void
gebrm_app_daemon_on_state_change(GebrmDaemon *daemon,
				 GebrCommServerState state,
				 GebrmApp *app)
{
	for (GList *i = app->priv->connections; i; i = i->next)
		send_server_status_message(app, i->data, gebrm_daemon_get_server(daemon));
}

static void
gebrm_app_finalize(GObject *object)
{
	GebrmApp *app = GEBRM_APP(object);
	g_hash_table_unref(app->priv->jobs);
	g_list_free(app->priv->connections);
	g_list_free(app->priv->daemons);
	G_OBJECT_CLASS(gebrm_app_parent_class)->finalize(object);
}

static void
gebrm_app_class_init(GebrmAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebrm_app_finalize;
	g_type_class_add_private(klass, sizeof(GebrmAppPriv));
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
get_comm_servers_list(GebrmApp *app, const gchar *address, const gchar *group)
{
	GList *servers = NULL;
	GebrCommServer *server;
	gboolean is_autochoose = (address[0] == '\0');

	if (!is_autochoose) {
		for (GList *i = app->priv->daemons; i; i = i->next) {
			if (!gebrm_daemon_has_group(i->data, group))
				continue;
			if (g_str_equal(gebrm_daemon_get_address(i->data), address)) {
				g_object_get(i->data, "server", &server, NULL);
				servers = g_list_prepend(servers, server);
				break;
			}
		}
	} else {
		for (GList *i = app->priv->daemons; i; i = i->next) {
			if (!gebrm_daemon_has_group(i->data, group))
				continue;
			g_object_get(i->data, "server", &server, NULL);
			if (gebr_comm_server_is_logged(server))
				servers = g_list_prepend(servers, server);
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
		gebr_comm_protocol_socket_oldmsg_send(i->data, FALSE,
						      gebr_comm_protocol_defs.job_def, 17,
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
	gebrm_job_set_nprocs(aap->job, gebr_comm_runner_get_ncores(runner));

	g_debug("Sending job_def to clients");
	send_job_def_to_clients(aap->app, aap->job);

	gebr_validator_free(gebr_comm_runner_get_validator(runner));
	gebr_comm_runner_free(runner);
	g_free(aap);
}

static void
on_client_request(GebrCommProtocolSocket *socket,
		  GebrCommHttpMsg *request,
		  GebrmApp *app)
{
	g_debug("URL: %s", request->url->str);

	if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		if (g_str_has_prefix(request->url->str, "/server")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *addr = strchr(params[0], '=') + 1;
			gchar *pass = strchr(params[1], '=') + 1;

			GebrmDaemon *d = gebrm_add_server_to_list(app, addr, pass, NULL);
			gebrm_daemon_connect(d, pass, socket);
			gebrm_config_save_server(d);
			g_strfreev(params);
		}
		else if (g_str_has_prefix(request->url->str, "/disconnect/")) {
			const gchar *addr = request->url->str + strlen("/disconnect/");
			gebrm_remove_server_from_list(app, addr);
			gebrm_config_delete_server(addr);
			g_debug(">> on %s, disconecting %s", __func__, addr) 	;
		}
		else if (g_str_has_prefix(request->url->str, "/close")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar *id = strchr(tmp, '=') + 1;

			GebrmJob *job = g_hash_table_lookup(app->priv->jobs, id);
			if (job) {
				gebrm_job_close(job);
				g_hash_table_remove(app->priv->jobs, id);
				g_debug("CLOSE JOB WITH ID %s", id);
			}
		}
		else if (g_str_has_prefix(request->url->str, "/kill")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar *id = strchr(tmp, '=') + 1;

			GebrmJob *job = g_hash_table_lookup(app->priv->jobs, id);
			if (job) {
				gebrm_job_kill(job);
				g_debug("KILL JOB WITH ID %s", id);
			}
		}
		else if (g_str_has_prefix(request->url->str, "/run")) {
			GebrCommJsonContent *json;
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *address, *parent_id, *speed, *nice, *group;
			gchar *host, *temp_id;

			g_debug("I will run this flow:");

			address   = strchr(params[0], '=') + 1;
			parent_id = strchr(params[1], '=') + 1;
			speed     = strchr(params[2], '=') + 1;
			nice      = strchr(params[3], '=') + 1;
			group     = strchr(params[4], '=') + 1;
			host      = strchr(params[5], '=') + 1;
			temp_id   = strchr(params[6], '=') + 1;

			json = gebr_comm_json_content_new(request->content->str);
			GString *value = gebr_comm_json_content_to_gstring(json);
			write(STDOUT_FILENO, value->str, MIN(value->len, 100));
			puts("");

			GebrGeoXmlProject **pproj = g_new(GebrGeoXmlProject*, 1);
			GebrGeoXmlLine **pline = g_new(GebrGeoXmlLine*, 1);
			GebrGeoXmlFlow **pflow = g_new(GebrGeoXmlFlow*, 1);

			gebr_geoxml_document_load_buffer((GebrGeoXmlDocument **)pflow, value->str);
			*pproj = gebr_geoxml_project_new();
			*pline = gebr_geoxml_line_new();

			GebrValidator *validator = gebr_validator_new((GebrGeoXmlDocument **)pflow,
								      (GebrGeoXmlDocument **)pline,
								      (GebrGeoXmlDocument **)pproj);

			GList *servers = get_comm_servers_list(app, address, group);
			GebrCommRunner *runner = gebr_comm_runner_new(GEBR_GEOXML_DOCUMENT(*pflow),
								      servers,
								      parent_id, speed, nice, group,
								      validator);
			g_list_free(servers);

			gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(*pflow));

			GebrmJobInfo info = { 0, };
			info.title = title;
			info.temp_id = temp_id;
			info.hostname = host;
			info.parent_id = parent_id;
			info.servers = "";
			info.nice = nice;
			info.input = gebr_geoxml_flow_io_get_input(*pflow);
			info.output = gebr_geoxml_flow_io_get_output(*pflow);
			info.error = gebr_geoxml_flow_io_get_error(*pflow);
			info.group = group;
			info.speed = speed;
			info.submit_date = gebr_iso_date();

			GebrmJob *job = gebrm_job_new();

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

			AppAndJob *aap = g_new(AppAndJob, 1);
			aap->app = app;
			aap->job = job;
			gebr_comm_runner_set_ran_func(runner, on_execution_response, aap);

			g_debug("Queue: '%s'", parent_id);

			if (parent_id[0] == '\0') {
				g_debug("Running immediately");
				gebr_comm_runner_run_async(runner, gebrm_job_get_id(job));
			} else {
				GebrmJob *parent = gebrm_app_job_controller_find(app, parent_id);
				GList *l = g_object_get_data(G_OBJECT(parent), "children");
				RunnerAndJob *raj = g_new(RunnerAndJob, 1);
				raj->runner = runner;
				raj->job = job;
				l = g_list_prepend(l, raj);
				g_object_set_data(G_OBJECT(parent), "children", l);
				send_job_def_to_clients(app, job);
			}

			g_strfreev(params);
			g_free(title);
		} else if (g_str_has_prefix(request->url->str, "/server-tags")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *server, *tags;

			server = strchr(params[0], '=') + 1;
			tags   = strchr(params[1], '=') + 1;

			g_debug("RECEIVED GROUPS %s of SERVER %s", tags, server);

			if (gebrm_config_update_tags_on_server(app, server, tags)) {
				gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
								      gebr_comm_protocol_defs.agrp_def, 2,
								      server, tags);
				gebrm_update_tags_on_list_of_servers(app, server, tags);
			}
			g_strfreev(params);
		} else if (g_str_has_prefix(request->url->str, "/tag-insert")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *server, *tag;

			server = strchr(params[0], '=') + 1;
			tag    = strchr(params[1], '=') + 1;

			g_debug("Inserting tag '%s' on server '%s'", tag, server);

			gchar *tags;
			if (gebrm_config_insert_tag_on_server(app, server, tag, &tags)) {
				gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
				                                      gebr_comm_protocol_defs.agrp_def, 2,
				                                      server, tags);
				gebrm_update_tags_on_list_of_servers(app, server, tags);
				g_free(tags);
			}
			g_strfreev(params);
		} else if (g_str_has_prefix(request->url->str, "/tag-remove")) {
			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *server, *tag;

			server = strchr(params[0], '=') + 1;
			tag    = strchr(params[1], '=') + 1;

			g_debug("Removing server'%s' of tag '%s'", server, tag);

			gchar *tags;
			if (gebrm_config_remove_tag_of_server(app, server, tag, &tags)) {
				gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
				                                      gebr_comm_protocol_defs.agrp_def, 2,
				                                      server, tags);
				gebrm_update_tags_on_list_of_servers(app, server, tags);
				g_free(tags);
			}
			g_strfreev(params);
		}
	}
}

static GKeyFile *
load_servers_keyfile(void)
{
	gchar *path = g_build_filename(g_get_home_dir(),
	                               GEBRM_LIST_OF_SERVERS_PATH,
	                               GEBRM_LIST_OF_SERVERS_FILENAME, NULL);

	GKeyFile *keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
		g_key_file_free(keyfile);
		g_free(path);
		return NULL;
	}

	g_free(path);

	return keyfile;
}

static void
save_servers_keyfile(GKeyFile *keyfile)
{
	gchar *content = g_key_file_to_data(keyfile, NULL, NULL);

	if (content) {
		gchar *path = g_build_filename(g_get_home_dir(),
					       GEBRM_LIST_OF_SERVERS_PATH,
					       GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
		g_file_set_contents(path, content, -1, NULL);
		g_free(content);
		g_free(path);
	}
}

static gboolean
gebrm_config_update_tags_on_server(GebrmApp *app,
                                   gchar *server,
                                   gchar *tags)
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
				  gchar *server,
				  gchar *tag,
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
                                  gchar *server,
                                  gchar *tag,
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

static void
gebrm_config_save_server(GebrmDaemon *daemon)
{
	GKeyFile *servers = g_key_file_new ();
	gchar *dir = g_build_filename(g_get_home_dir(),
				      GEBRM_LIST_OF_SERVERS_PATH, NULL);
	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 755);

	gchar *path = g_build_filename(dir, GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	gchar *tags = gebrm_daemon_get_tags(daemon);

	g_key_file_load_from_file(servers, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(servers, gebrm_daemon_get_address(daemon),
			      "tags", tags);

	gchar *content = g_key_file_to_data(servers, NULL, NULL);
	if (content)
		g_file_set_contents(path, content, -1, NULL);

	g_free(content);
	g_free(path);
	g_free(tags);
	g_key_file_free(servers);
}

static void
gebrm_config_delete_server(const gchar *serv)
{
	gchar *dir, *path, *subdir, *server;
	gchar *final_list_str;
	GKeyFile *servers;
	gboolean ret;

	server = g_strcmp0(serv, "127.0.0.1") ? g_strdup(serv): g_strdup("localhost");
	g_debug("Adding server %s to file", server);

	servers = g_key_file_new ();

	dir = g_build_path ("/", g_get_home_dir (), GEBRM_LIST_OF_SERVERS_PATH, NULL);
	if(!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 755);

	subdir = g_strconcat(GEBRM_LIST_OF_SERVERS_PATH, GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	path = g_build_path ("/", g_get_home_dir (), subdir, NULL);

	g_key_file_load_from_file (servers, path, G_KEY_FILE_NONE, NULL);

	if(g_key_file_has_group(servers,server))
		g_debug("File doesn't have:%s", server);

	g_key_file_remove_group(servers, server, NULL);
	   
	if(g_key_file_has_group(servers,server))
		g_debug("Already has:%s", server);


	final_list_str= g_key_file_to_data (servers, NULL, NULL);
	ret = g_file_set_contents (path, final_list_str, -1, NULL);
	g_free (server);
	g_free (final_list_str);
	g_free (path);
	g_free (subdir);
	g_key_file_free (servers);
}

gboolean
gebrm_config_load_servers(GebrmApp *app, gchar *path)
{

	GKeyFile *servers = g_key_file_new();
	gchar **groups;
	gboolean succ = g_key_file_load_from_file(servers, path, G_KEY_FILE_NONE, NULL);
	if (succ) {
		groups = g_key_file_get_groups(servers, NULL);
		for (int i = 0; groups && groups[i]; i++) {
			gchar *tags = g_key_file_get_string(servers, groups[i], "tags", NULL);
			GebrmDaemon *daemon = gebrm_add_server_to_list(app, groups[i], NULL, tags);
			gebrm_daemon_connect(daemon, NULL, NULL);
		}
		g_key_file_free (servers);
		g_strfreev(groups);
	}
	return TRUE;
}

static void
on_client_disconnect(GebrCommProtocolSocket *socket,
		     GebrmApp *app)
{
	g_debug("Client disconnected!");
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
	                                      gebr_comm_protocol_defs.job_def, 17,
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
on_new_connection(GebrCommListenSocket *listener,
		  GebrmApp *app)
{
	GebrCommStreamSocket *client;

	g_debug("New connection!");

	while ((client = gebr_comm_listen_socket_get_next_pending_connection(listener))) {
		GebrCommProtocolSocket *socket =
			gebr_comm_protocol_socket_new_from_socket(client);

		app->priv->connections = g_list_prepend(app->priv->connections, socket);

		for (GList *i = app->priv->daemons; i; i = i->next) {
			GebrCommServer *server;
			g_object_get(i->data, "server", &server, NULL);
			send_server_status_message(app, socket, server);
			send_groups_definitions(socket, i->data);
		}

		g_signal_connect(socket, "disconnected",
				 G_CALLBACK(on_client_disconnect), app);
		g_signal_connect(socket, "process-request",
				 G_CALLBACK(on_client_request), app);

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

	g_file_set_contents(gebrm_main_get_lock_file(),
			    portstr, -1, &error);

	if (error) {
		g_critical("Could not create lock: %s", error->message);
		return FALSE;
	}

	/* success, send port */
	gchar *port_str = g_strdup_printf("%u\n", port);
	write(fd, port_str, strlen(port_str));
	g_free(port_str);

	gchar *path = g_build_filename(g_get_home_dir(),
	                               GEBRM_LIST_OF_SERVERS_PATH,
	                               GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	gebrm_config_load_servers(app, path);
	g_free(path);

	g_main_loop_run(app->priv->main_loop);

	return TRUE;
}

const gchar *
gebrm_main_get_lock_file(void)
{
	static gchar *lock = NULL;

	if (!lock) {
		gchar *fname = g_strconcat("lock-", g_get_host_name(), NULL);
		gchar *dirname = g_build_filename(g_get_home_dir(), ".gebr",
						  "gebrm", NULL);
		if(!g_file_test(dirname, G_FILE_TEST_EXISTS))
			g_mkdir_with_parents(dirname, 0755);
		lock = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", fname, NULL);
		g_free(dirname);
		g_free(fname);
	}

	return lock;
}
