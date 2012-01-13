/*   GeBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
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
#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/utils.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
				struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
				int fd;
				gchar *xauth_dir = g_build_filename(g_get_home_dir(), "Xauthority", NULL);

				fl.l_pid = getpid();

				if ((fd = open(xauth_dir, O_RDWR | O_CREAT, 0600)) == -1) {
					perror("open");
					g_free(xauth_dir);
					exit(1);
				}

				g_free(xauth_dir);

				if (fcntl(fd, F_SETLKW, &fl) == -1) {
					perror("fcntl");
					exit(1);
				}

				gebrd_message(GEBR_LOG_DEBUG, "got lock");

				/* add client magic cookie */
				gchar *cmd = g_strdup_printf("XAUTHORITY=$HOME/.gebr/Xauthority xauth add :%d . %s",
							     display, cookie->str);
				g_debug("%s", cmd);
				if (gebr_system(cmd))
					gebrd_message(GEBR_LOG_ERROR, "Failed to add X11 authorization.");
				else
					gebrd_message(GEBR_LOG_ERROR, "X authorized!");
				g_free(cmd);

				fl.l_type = F_UNLCK;

				if (fcntl(fd, F_SETLK, &fl) == -1) {
					perror("fcntl");
					exit(1);
				}

				gebrd_message(GEBR_LOG_DEBUG, "release lock");

				close(fd);
			}

			gchar *display_str = g_strdup_printf("%d", display + 6000);
			gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
							      gebr_comm_protocol_defs.ret_def, 2,
							      gid->str, display_str);
			g_free(display_str);

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
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 8)) == NULL)
				goto err;

			GString *gid = g_list_nth_data(arguments, 0);
			GString *id = g_list_nth_data(arguments, 1);
			GString *frac = g_list_nth_data(arguments, 2);
			GString *speed = g_list_nth_data(arguments, 3);
			GString *nice = g_list_nth_data(arguments, 4);
			GString *flow_xml = g_list_nth_data(arguments, 5);

			/* Moab & MPI settings */
			GString *account = g_list_nth_data(arguments, 6);
			GString *num_processes = g_list_nth_data(arguments, 7);

			/* try to run and send return */
			job_new(&job, client, gid, id, frac, speed, nice, flow_xml, account, num_processes);

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
