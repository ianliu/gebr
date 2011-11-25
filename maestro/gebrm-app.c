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

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/utils.h>
#include <glib/gi18n.h>

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;

#define GEBRM_LIST_OF_SERVERS_PATH ".gebr/gebrm"
#define GEBRM_LIST_OF_SERVERS_FILENAME "servers.conf"

static void gebrm_config_save_server(GebrmDaemon *daemon);

static GebrmDaemon *gebrm_add_server_to_list(GebrmApp *app,
					     const gchar *addr,
					     const gchar *tags);

static void gebrm_config_delete_server(const gchar *serv);

static gboolean gebrm_remove_server_from_list(GebrmApp *app, const gchar *address);

gboolean gebrm_config_load_servers(GebrmApp *app, gchar *path);

G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GebrCommListenSocket *listener;
	GebrCommSocketAddress address;
	GList *connections;
	GList *daemons;

	gint finished_starting_pipe[2];
	gchar buffer[1024];
};

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
send_server_status_message(GebrCommProtocolSocket *socket,
			   GebrCommServer *server)
{
	const gchar *state = gebr_comm_server_state_to_string(server->state);
	gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
					      gebr_comm_protocol_defs.ssta_def, 2,
					      server->address->str,
					      state);
}

static void
gebrm_server_op_state_changed(GebrCommServer *server,
			      gpointer user_data)
{
	GebrmApp *app = user_data;
	const gchar *state = gebr_comm_server_state_to_string(server->state);
	g_debug("[DAEMON] %s: State %s", __func__, state);

	for (GList *i = app->priv->connections; i; i = i->next)
		send_server_status_message(i->data, server);
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
			else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
				GList *arguments;
				GString *hostname, *status, *title, *start_date, *finish_date, *issues, *cmd_line,
					*output, *queue, *moab_jid, *rid, *frac, *server_list, *n_procs, *niceness,
					*input_file, *output_file, *log_file, *last_run_date, *server_group_name,
					*job_percentage, *exec_speed;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 23)) == NULL)
					goto err;

				status = g_list_nth_data(arguments, 1);
				title = g_list_nth_data(arguments, 2);
				start_date = g_list_nth_data(arguments, 3);
				finish_date = g_list_nth_data(arguments, 4);
				hostname = g_list_nth_data(arguments, 5);
				issues = g_list_nth_data(arguments, 6);
				cmd_line = g_list_nth_data(arguments, 7);
				output = g_list_nth_data(arguments, 8);
				queue = g_list_nth_data(arguments, 9);
				moab_jid = g_list_nth_data(arguments, 10);
				rid = g_list_nth_data(arguments, 11);
				frac = g_list_nth_data(arguments, 12);
				server_list = g_list_nth_data(arguments, 13);
				n_procs = g_list_nth_data(arguments, 14);
				niceness = g_list_nth_data(arguments, 15);
				input_file = g_list_nth_data(arguments, 16);
				output_file= g_list_nth_data(arguments, 17);
				log_file = g_list_nth_data(arguments, 18);
				last_run_date = g_list_nth_data(arguments, 19);
				server_group_name = g_list_nth_data(arguments, 20);
				exec_speed = g_list_nth_data(arguments, 21);
				job_percentage = g_list_nth_data(arguments, 22);

				g_debug("[%s] JOB DEF! %s", server->address->str, title->str);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
		} else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *output, *rid, *frac;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;

			output = g_list_nth_data(arguments, 1);
			rid = g_list_nth_data(arguments, 2);
			frac = g_list_nth_data(arguments, 3);

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
gebrm_app_class_init(GebrmAppClass *klass)
{
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
	gchar *path = g_build_filename(g_get_home_dir(),
				       GEBRM_LIST_OF_SERVERS_PATH,
				       GEBRM_LIST_OF_SERVERS_FILENAME, NULL);
	gebrm_config_load_servers(app, path);
	g_free(path);

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
			 const gchar *tags)
{
	GebrmDaemon *daemon;

	for (GList *i = app->priv->daemons; i; i = i->next) {
		daemon = i->data;
		if (g_strcmp0(address, gebrm_daemon_get_address(daemon)) == 0)
			return NULL;
	}

	GebrCommServer *server = gebr_comm_server_new(address, &daemon_ops);
	server->user_data = app;
	daemon = gebrm_daemon_new(server);

	gchar **tagsv = tags ? g_strsplit(tags, ",", -1) : NULL;
	if (tagsv) {
		for (int i = 0; tagsv[i]; i++)
			gebrm_daemon_add_tag(daemon, tagsv[i]);
	}

	app->priv->daemons = g_list_prepend(app->priv->daemons, daemon);
	gebrm_daemon_connect(daemon);

	return daemon;
}

static GList *
get_comm_servers_list(GebrmApp *app)
{
	GList *servers = NULL;
	GebrCommServer *server;
	for (GList *i = app->priv->daemons; i; i = i->next) {
		g_object_get(i->data, "server", &server, NULL);
		servers = g_list_prepend(servers, server);
	}

	return servers;
}

static gboolean
gebrm_remove_server_from_list(GebrmApp *app, const gchar *addr)
{
	gboolean succ = FALSE;

	gint compare_daemons(GebrCommServer *a, GebrCommServer *b){
		return g_strcmp0(a->address->str, b->address->str);
	}

	if (!g_list_find_custom(app->priv->daemons, daemon, (GCompareFunc)compare_daemons )){
		GebrCommServer *daemon = gebr_comm_server_new(addr, &daemon_ops);
		daemon->user_data = app;
		app->priv->daemons = g_list_remove(app->priv->daemons,
						    daemon);
		gebr_comm_server_disconnect(daemon);
		gebr_comm_server_free(daemon);
		g_debug("Removed server %s", addr);
		succ =  TRUE;
	}
	return succ;
}

static void
on_client_request(GebrCommProtocolSocket *socket,
		  GebrCommHttpMsg *request,
		  GebrmApp *app)
{
	g_debug("URL: %s", request->url->str);

	if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		if (g_str_has_prefix(request->url->str, "/server/")) {
			gchar *addr = request->url->str + strlen("/server/");
			GebrmDaemon *d = gebrm_add_server_to_list(app, addr, NULL);
			gebrm_config_save_server(d);
		}
		else if (g_str_has_prefix(request->url->str, "/disconnect/")) {
			const gchar *addr = request->url->str + strlen("/server/");
			gebrm_remove_server_from_list(app, addr);
			gebrm_config_delete_server(addr);
			g_debug(">> on %s, disconecting %s", __func__, addr) 	;
		}
		else if (g_str_has_prefix(request->url->str, "/run")) {
			GebrCommJsonContent *json;
			static gint id = 0;

			gchar *tmp = strchr(request->url->str, '?') + 1;
			gchar **params = g_strsplit(tmp, ";", -1);
			gchar *parent_rid, *speed, *nice, *group;

			g_debug("I will run this flow:");

			parent_rid = strchr(params[0], '=') + 1;
			speed      = strchr(params[1], '=') + 1;
			nice       = strchr(params[2], '=') + 1;
			group      = strchr(params[3], '=') + 1;

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

			GList *servers = get_comm_servers_list(app);
			GebrCommRunner *runner = gebr_comm_runner_new(GEBR_GEOXML_DOCUMENT(*pflow),
								      servers,
								      parent_rid, speed, nice, group,
								      validator);
			g_list_free(servers);

			gchar *idstr = g_strdup_printf("%d", id++);
			gebr_comm_runner_run_async(runner, idstr);
			//gebr_validator_free(validator);
			g_free(idstr);
		}
	}
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
		for (int i = 0; groups[i]; i++) {
			gchar *tags = g_key_file_get_string(servers, groups[i], "tags", NULL);
			gebrm_add_server_to_list(app, groups[i], tags);
		}
		g_key_file_free (servers);
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
on_new_connection(GebrCommListenSocket *listener,
		  GebrmApp *app)
{
	GebrCommStreamSocket *client;

	g_debug("New connection!");

	while ((client = gebr_comm_listen_socket_get_next_pending_connection(listener))) {
		GebrCommProtocolSocket *protocol =
			gebr_comm_protocol_socket_new_from_socket(client);

		app->priv->connections = g_list_prepend(app->priv->connections, protocol);

		for (GList *i = app->priv->daemons; i; i = i->next) {
			GebrCommServer *server;
			g_object_get(i->data, "server", &server, NULL);
			send_server_status_message(protocol, server);
		}

		g_signal_connect(protocol, "disconnected",
				 G_CALLBACK(on_client_disconnect), app);
		g_signal_connect(protocol, "process-request",
				 G_CALLBACK(on_client_request), app);
	}
}

GebrmApp *
gebrm_app_new(void)
{
	return g_object_new(GEBRM_TYPE_APP, NULL);
}

gboolean
gebrm_app_run(GebrmApp *app)
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
	g_debug("Server started at %u port",
		gebr_comm_socket_address_get_ip_port(&app->priv->address));

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
