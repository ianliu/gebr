/*
 * gebrd-client.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2012 - GêBR Core Team (www.gebrproject.com)
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_X11_XAUTH_H
# include <X11/Xauth.h>
#endif

#include "gebrd-client.h"

#include "gebrd-job.h"
#include "gebrd-server.h"
#include "gebrd-sysinfo.h"
#include "gebrd.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <gdome.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/utils.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Private functions
 */

static void client_disconnected(GebrCommProtocolSocket * socket, struct client *client);
static void client_process_request(GebrCommProtocolSocket * socket, GebrCommHttpMsg * request, struct client *client);
static void client_process_response(GebrCommProtocolSocket * socket, GebrCommHttpMsg * request,
				    GebrCommHttpMsg * response, struct client *client);
static void client_old_parse_messages(GebrCommProtocolSocket * socket, struct client *client);


/*
 * Public functions
 */

void client_add(GebrCommProtocolSocket * client)
{
	struct client *c;

	c = g_new(struct client, 1);
	c->socket = client;
	c->display = g_string_new(NULL);

	gebrd_user_set_connection(gebrd->user, c);

	g_signal_connect(c->socket, "disconnected",
			 G_CALLBACK(client_disconnected), c);
	g_signal_connect(c->socket, "process-request",
			 G_CALLBACK(client_process_request), c);
	g_signal_connect(c->socket, "process-response",
			 G_CALLBACK(client_process_response), c);
	g_signal_connect(c->socket, "old-parse-messages",
			 G_CALLBACK(client_old_parse_messages), c);
}

void client_free(struct client *client)
{
	g_object_unref(client->socket);
	g_string_free(client->display, TRUE);
	g_free(client);
}

static void
client_disconnected(GebrCommProtocolSocket * socket,
		    struct client *client)
{
	gebrd_message(GEBR_LOG_DEBUG, "client_disconnected");
	gebrd_quit();
}

static void client_process_request(GebrCommProtocolSocket * socket, GebrCommHttpMsg * request, struct client *client)
{
	//g_message("url '%s'. contents: %s", request->url->str, request->content->str);
	
	if (!g_str_has_prefix(request->url->str, "/"))
		return;
	//TODO: go into object tree to get object/property
	//gchar **splits = g_strsplit(request->url->str+1, "/", 0);
	//for (int i = 0; splits[i]; ++i) {
	//}
	GObject *object = G_OBJECT(gebrd->user);
	const gchar *property = request->url->str+1;

	if (request->method == GEBR_COMM_HTTP_METHOD_GET) {
		GebrCommJsonContent *json = gebr_comm_json_content_new_from_property(object, property);
		gebr_comm_protocol_socket_send_response(socket, 200, json);
		gebr_comm_json_content_free(json);
	} else if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		GebrCommJsonContent *json = gebr_comm_json_content_new(request->content->str);
		gebr_comm_json_content_to_property(json, object, property);
		gebr_comm_json_content_free(json);

		gebr_comm_protocol_socket_send_response(socket, 200, NULL);
	} //else

	//g_strfreev(splits);
}

static void client_process_response(GebrCommProtocolSocket * socket, GebrCommHttpMsg * request,
				    GebrCommHttpMsg * response, struct client *client)
{
}

/**
 * \internal
 * Reads the list of accounts and the list of queue_list returned by the MOAB cluster
 * "mcredctl" command and places them on \p accounts and \p queue_list, respectively.
 */
static void server_moab_read_credentials(GString *accounts, GString *queue_list)
{
	GString *cmd_line = NULL;
	gchar *std_out = NULL;
	gchar *std_err = NULL;
	gint exit_status;

	GdomeDOMImplementation *dom_impl = NULL;
	GdomeDocument *doc = NULL;
	GdomeElement *element = NULL;
	GdomeException exception;
	GdomeDOMString *attribute_name;

	cmd_line = g_string_new("");
	g_string_printf(cmd_line, "mcredctl -q accessfrom user:%s --format=xml", getenv("USER"));
	if (g_spawn_command_line_sync(cmd_line->str, &std_out, &std_err, &exit_status, NULL) == FALSE)
		goto err;

	if ((dom_impl = gdome_di_mkref()) == NULL)
		goto err;

	if ((doc = gdome_di_createDocFromMemory(dom_impl, std_out, GDOME_LOAD_PARSING, &exception)) == NULL) {
		gdome_di_unref(dom_impl, &exception);
		goto err;
	}

	if ((element = gdome_doc_documentElement(doc, &exception)) == NULL) {
		gdome_doc_unref(doc, &exception);
		gdome_di_unref(dom_impl, &exception);
		goto err;
	}

	if ((element = (GdomeElement *) gdome_el_firstChild(element, &exception)) == NULL) {
		gdome_doc_unref(doc, &exception);
		gdome_di_unref(dom_impl, &exception);
		goto err;
	}

	attribute_name = gdome_str_mkref("AList");
	g_string_assign(accounts, (gdome_el_getAttribute(element, attribute_name, &exception))->str);
	gdome_str_unref(attribute_name);

	attribute_name = gdome_str_mkref("CList");
	g_string_assign(queue_list, (gdome_el_getAttribute(element, attribute_name, &exception))->str);
	gdome_str_unref(attribute_name);

	gdome_doc_unref(doc, &exception);
	gdome_di_unref(dom_impl, &exception);

 err:	g_string_free(cmd_line, TRUE);
	g_free(std_out);
	g_free(std_err);
}

static gboolean
run_xauth_command(gchar **argv,
		  gchar **out)
{
	int fd;
	struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
	gchar *xauth = g_build_filename(g_get_home_dir(), ".gebr", "Xauthority", NULL);
	gchar *xauthority = g_strconcat("XAUTHORITY=", xauth, NULL);
	gchar *envp[] = {xauthority, NULL};

	fl.l_pid = getpid();

	if ((fd = open(xauth, O_RDWR | O_APPEND | O_CREAT, 0600)) == -1) {
		perror("open");
		return FALSE;
	}

	if (fcntl(fd, F_SETLKW, &fl) == -1) {
		perror("fcntl");
		close(fd);
		return FALSE;
	}

	gebrd_message(GEBR_LOG_DEBUG, "got lock");

	GError *error = NULL;
	gchar *err = NULL;
	gint status;
	gint exit_status;
	gint tries = 0;
	gboolean retval = FALSE;

	do {
		g_spawn_sync(g_get_current_dir(), argv, envp,
			     G_SPAWN_SEARCH_PATH, NULL, NULL,
			     out, &err, &status, &error);
		exit_status = WEXITSTATUS(status);

		if (error) {
			gebrd_message(GEBR_LOG_ERROR, "Error running `xauth': %s", error->message);
			g_error_free(error);
			exit_status = 1;
		} else if (exit_status != 0) {
			gebrd_message(GEBR_LOG_ERROR, "xauth command exited with %d: %s",
				      exit_status, err);
		} else {
			gebrd_message(GEBR_LOG_INFO, "xauth command succeeded");
			retval = TRUE;
		}
		g_usleep(500000);
		tries++;
	} while (tries < 15 && exit_status);

	fl.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &fl) == -1) {
		perror("fcntl");
		exit(1);
	}

	gebrd_message(GEBR_LOG_DEBUG, "release lock");

	close(fd);
	g_free(err);

	return retval;
}

#if HAVE_X11_XAUTH_H
static Xauth *
create_xauth_from_data(const gchar *port,
		       const gchar *cookie)
{
	Xauth *auth = g_new(Xauth, 1);
	auth->family = 256;
	auth->address = (gchar *)g_get_host_name();
	auth->address_length = strlen(auth->address);
	auth->number = (gchar *)port;
	auth->number_length = strlen(auth->number);
	auth->name = "MIT-MAGIC-COOKIE-1";
	auth->name_length = strlen(auth->name);

	gsize len = strlen(cookie);
	gchar *tmp = g_strdup(cookie);
	gchar *data = g_new(gchar, len / 2);

	for (int i = 0; tmp[i]; i++) {
		if (tmp[i] >= 'a')
			tmp[i] -= 'a' + 10;
		else if (tmp[i] >= '0')
			tmp[i] -= '0';
		if (i % 2 == 0)
			tmp[i] <<= 4;
		else
			data[i/2] = tmp[i-1] | tmp[i];
	}
	auth->data = data;
	auth->data_length = len / 2;
	return auth;
}

static GList *
build_xauth_list(FILE *xauth_file, Xauth *auth)
{
	GList *list = NULL;
	gboolean subst = FALSE;

	if (!xauth_file)
		return NULL;

	Xauth *i = XauReadAuth(xauth_file);
	while (i) {
		if (g_strcmp0(i->address, auth->address) == 0
		    && g_strcmp0(i->number, auth->number) == 0
		    && g_strcmp0(i->name, auth->name) == 0) {
			list = g_list_prepend(list, auth);
			XauDisposeAuth(i);
			subst = TRUE;
		} else {
			list = g_list_prepend(list, i);
		}
		i = XauReadAuth(xauth_file);
	}

	if (!subst)
		list = g_list_prepend(list, auth);
	return list;
}

static gboolean
run_lib_xauth_command(const gchar *port, const gchar *cookie)
{
	gchar *path = g_build_filename(g_get_home_dir(), ".gebr", "Xauthority", NULL);
	FILE *xauth = fopen(path, "r");
	Xauth *auth = create_xauth_from_data(port, cookie);
	GList *list = build_xauth_list(xauth, auth);

	if (xauth)
		fclose(xauth);

	xauth = fopen(path, "w");
	for (GList *i = list; i; i = i->next)
		XauWriteAuth(xauth, i->data);

//	g_list_foreach(list, (GFunc)XauDisposeAuth, NULL);
	g_list_free(list);
	fclose(xauth);

	return TRUE;
}
#endif

static void client_old_parse_messages(GebrCommProtocolSocket * socket, struct client *client)
{
	GList *link;
	struct gebr_comm_message *message;

	while ((link = g_list_last(client->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;

		/* check login */
		if (message->hash == gebr_comm_protocol_defs.ini_def.code_hash) {
			GList *arguments;

			GString *accounts_list = g_string_new("");
			GString *queue_list = g_string_new("");
			GString *display_port = g_string_new("");

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *version = g_list_nth_data(arguments, 0);
			GString *hostname = g_list_nth_data(arguments, 1);

			g_debug("Current protocol version is: %s", gebr_comm_protocol_get_version());
			g_debug("Received protocol version:   %s", version->str);

			if (strcmp(version->str, gebr_comm_protocol_get_version())) {
				gebr_comm_protocol_socket_oldmsg_send(client->socket, TRUE,
								      gebr_comm_protocol_defs.err_def, 2,
								      "protocol",
								      gebr_comm_protocol_get_version());
				goto err;
			}

			/* set client info */
			client->socket->protocol->logged = TRUE;
			g_string_assign(client->socket->protocol->hostname, hostname->str);
			client->server_location = GEBR_COMM_SERVER_LOCATION_REMOTE;

			const gchar *server_type;
			if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
				/* Get info from the MOAB cluster */
				server_moab_read_credentials(accounts_list, queue_list);
				server_type = "moab";
			} else
				server_type = "regular";

			/* send return */
			const gchar *model_name;
			const gchar *cpu_clock;
			const gchar *total_memory;
			GebrdCpuInfo *cpuinfo = gebrd_cpu_info_new();
			GebrdMemInfo *meminfo = gebrd_mem_info_new();
			model_name = gebrd_cpu_info_get (cpuinfo, 0, "model name");
			cpu_clock = gebrd_cpu_info_get (cpuinfo, 0, "cpu MHz");
			total_memory = gebrd_mem_info_get (meminfo, "MemTotal");
			gchar *ncores = g_strdup_printf("%d", gebrd_cpu_info_n_procs(cpuinfo));
			gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 9,
							      gebrd->hostname,
							      server_type,
							      accounts_list->str,
							      model_name,
							      total_memory,
							      gebrd->fs_lock->str,
							      ncores,
							      cpu_clock,
							      gebrd_user_get_daemon_id(gebrd->user));
			gebrd_cpu_info_free (cpuinfo);
			gebrd_mem_info_free (meminfo);
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			g_string_free(accounts_list, TRUE);
			g_string_free(queue_list, TRUE);
			g_string_free(display_port, TRUE);
			g_free(ncores);
		}
		else if (message->hash == gebr_comm_protocol_defs.gid_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *gid = g_list_nth_data(arguments, 0);
			GString *cookie = g_list_nth_data(arguments, 1);
			guint16 display = gebrd_get_x11_redirect_display();
			g_hash_table_insert(gebrd->display_ports, g_strdup(gid->str), GINT_TO_POINTER(display));

			g_debug("Received gid %s with cookie %s", gid->str, cookie->str);

			if (cookie->len && gebrd_get_server_type() != GEBR_COMM_SERVER_TYPE_MOAB) {
#if !HAVE_X11_XAUTH_H
				gebrd_message(GEBR_LOG_DEBUG, "Authorizing with system(xauth)");
				gchar *tmp = g_strdup_printf(":%d", display);
				gchar *argv[] = {"xauth", "-i", "add", tmp, ".", cookie->str, NULL};
				if (!run_xauth_command(argv, NULL))
					display = 0;
				g_free(tmp);
#else
				gebrd_message(GEBR_LOG_DEBUG, "Authorizing with libXau");
				gchar *tmp = g_strdup_printf("%d", display);
				if (!run_lib_xauth_command(tmp, cookie->str))
					display = 0;
				g_free(tmp);
#endif
			}

			gchar *display_str = g_strdup_printf("%d", display ? display + 6000 : 0);
			gebrd_message(GEBR_LOG_INFO, "Sending port %s to client %s!", display_str, gid->str);
			gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 2,
							      gid->str, display_str);
			g_free(display_str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.rmck_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *gid = g_list_nth_data(arguments, 0);
			GString *cookie = g_list_nth_data(arguments, 1);
			guint16 display = GPOINTER_TO_UINT(g_hash_table_lookup(gebrd->display_ports, gid->str));

			if (display) {
				gchar *output = NULL;
				gchar *args[] = {"xauth", "list", NULL};

				if (run_xauth_command(args, &output)) {
					gchar **lines = g_strsplit(output, "\n", 0);
					guint len = g_strv_length(lines);
					gchar **args = g_new0(gchar *, len + 3);
					args[0] = "xauth";
					args[1] = "remove";

					for (int i = 0; lines[i]; i++) {
						if (strstr(lines[i], cookie->str)) {
							args[i + 2] = lines[i];
							gchar *ptr = strchr(lines[i], ' ');
							ptr[0] = '\0';
							gebrd_message(GEBR_LOG_DEBUG, "Got display %s for cookie %s",
								      args[i+2], cookie->str);
						}
					}

					if (args[2])
						run_xauth_command(args, NULL);

					g_free(args);
					g_strfreev(lines);
				}
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (client->socket->protocol->logged == FALSE) {
			/* not logged! */
			goto err;
		} else if (message->hash == gebr_comm_protocol_defs.qut_def.code_hash) {
			client_free(client);
			gebr_comm_message_free(message);
			return;
		} else if (message->hash == gebr_comm_protocol_defs.lst_def.code_hash) {
			job_list(client);
		} else if (message->hash == gebr_comm_protocol_defs.run_def.code_hash) {
			GList *arguments;
			GebrdJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 9)) == NULL)
				goto err;

			GString *gid = g_list_nth_data(arguments, 0);
			GString *id = g_list_nth_data(arguments, 1);
			GString *frac = g_list_nth_data(arguments, 2);
			GString *speed = g_list_nth_data(arguments, 3);
			GString *nice = g_list_nth_data(arguments, 4);
			GString *flow_xml = g_list_nth_data(arguments, 5);
			GString *paths = g_list_nth_data(arguments, 6);

			/* Moab & MPI settings */
			GString *account = g_list_nth_data(arguments, 6);
			GString *num_processes = g_list_nth_data(arguments, 7);

			/* try to run and send return */
			job_new(&job, client, gid, id, frac, speed, nice, flow_xml, account, num_processes, paths);

#ifdef DEBUG
			gchar *env_delay = getenv("GEBRD_RUN_DELAY_SEC");
			if (env_delay != NULL)
				sleep(atoi(env_delay));
#endif

			if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {
				/* send job message (job is created -promoted from waiting server response- at the client) */
				g_debug("RUN_DEF: run task with rid %s", id->str);
				job_send_clients_job_notify(job);
				job_run_flow(job);
			} else {
				/* ask moab to run */
				job_run_flow(job);
				/* send job message (job is created -promoted from waiting server response- at the client)
				 * at moab we must run the process before sending the JOB message, because at
				 * job_run_flow moab_jid is acquired.
				 */
				job_send_clients_job_notify(job);
			}

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.rnq_def.code_hash) {
		} else if (message->hash == gebr_comm_protocol_defs.flw_def.code_hash) {
		} else if (message->hash == gebr_comm_protocol_defs.clr_def.code_hash) {
			GList *arguments;
			GString *rid;
			GebrdJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;
			rid = g_list_nth_data(arguments, 0);

			job = job_find(rid);
			if (job != NULL)
				job_clear(job);

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.end_def.code_hash) {
			GList *arguments;
			GString *rid;
			GebrdJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;
			rid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(rid);
			if (job != NULL) {
				job_end(job);
			}

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.kil_def.code_hash) {
			GList *arguments;
			GString *rid;
			GebrdJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;
			rid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(rid);
			if (job != NULL) {
				job_kill(job);
			}

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.path_def.code_hash) {
			g_debug("------------------------on %s",__func__);
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;

			//GString *server = g_list_nth_data(arguments, 0);
			GString *new_path = g_list_nth_data(arguments, 1);
			GString *old_path = g_list_nth_data(arguments, 2);
			GString *opt = g_list_nth_data(arguments, 3);

			gint success = 0; 
			gint option = atoi(opt->str);

			switch (option){
			case GEBR_COMM_PROTOCOL_PATH_CREATE:
				if (g_file_test(new_path->str, G_FILE_TEST_IS_DIR))
					errno = 666;
				else
					success = g_mkdir_with_parents(new_path->str, 0700);
				break;
			case GEBR_COMM_PROTOCOL_PATH_RENAME:
				success = g_rename(old_path->str, new_path->str);
				break;
			case GEBR_COMM_PROTOCOL_PATH_DELETE:
				success = g_rmdir(new_path->str);
				break;
			default:
				g_warn_if_reached();
				break;
			}

			g_debug("erro numero:%d", errno);

			if (!success)
				g_debug("on %s, new_path:'%s', old_path:'%s' sucessfull operation", __func__, new_path->str, old_path->str);

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);

			gebr_comm_protocol_socket_oldmsg_send(socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 2,
							      gebrd->hostname,
							      g_strdup_printf("%d", errno));
		} else {
			/* unknown message! */
			goto err;
		}

		gebr_comm_message_free(message);
		client->socket->protocol->messages = g_list_delete_link(client->socket->protocol->messages, link);
	}

	return;

err:	gebr_comm_message_free(message);
	client->socket->protocol->messages = g_list_delete_link(client->socket->protocol->messages, link);
	client_disconnected(socket, client);
}
