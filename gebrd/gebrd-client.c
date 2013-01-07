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

void
client_disconnected(GebrCommProtocolSocket * socket,
		    struct client *client)
{
	gebrd_message(GEBR_LOG_DEBUG, "client_disconnected");
	gebrd_quit();
}

static void client_process_request(GebrCommProtocolSocket * socket, GebrCommHttpMsg * request, struct client *client)
{
	if (!g_str_has_prefix(request->url->str, "/"))
		return;

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
	}
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
run_xauth_command(const gchar *cmd,
                  const gchar *display,
                  const gchar *cookie)
{
	gchar *xauth_file = g_build_filename(g_get_home_dir(), ".gebr", "gebrd", gebrd->hostname, "Xauthority", NULL);

	if (!display || !cookie)
		return FALSE;

	gchar *cmd_line;
	cmd_line = g_strdup_printf("xauth -f %s add %s . %s", xauth_file, display, cookie);

	GError *error = NULL;
	gchar *err = NULL;
	gint tries = 0;
	gboolean retval = FALSE;
	gint status;

	gebrd_message(GEBR_LOG_INFO, "COMMAND LINE FOR XAUTH: %s", cmd_line);

	do {
		g_spawn_command_line_sync(cmd_line, NULL, &err, &status, &error);

		if (error) {
			gebrd_message(GEBR_LOG_ERROR, "Error running `xauth': %s", error->message);
			g_error_free(error);
		} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			gebrd_message(GEBR_LOG_ERROR, "xauth command exited with %d: %s",
			              WEXITSTATUS(status), err);
		} else {
			gebrd_message(GEBR_LOG_INFO, "xauth command succeeded");
			retval = TRUE;
			break;
		}
		g_usleep(500000);
		tries++;
	} while (tries < 15 && !retval);

	g_free(xauth_file);
	g_free(cmd_line);
	g_free(err);

	return retval;
}

GList *
parse_comma_separated_string(gchar *str)
{
	gint i = 0;
	GString *path = g_string_new(NULL);
	GList *list = NULL;
	while (str[i]) {
		if (str[i] == ',' && str[i+1] == ',') {
			g_string_append_c(path, ',');
			i += 2;
		} else if (str[i] == ',' || str[i+1] == '\0') {
			if (str[i] != ',')
				g_string_append_c(path, str[i]);
			gebr_path_resolve_home_variable(path);
			g_debug("Got path %s", path->str);
			list = g_list_append(list, g_string_new(path->str));
			g_string_assign(path, "");
			i++;
		} else {
			g_string_append_c(path, str[i]);
			i++;
		}
	}
	return list;
}

static void
get_mpi_flavors(GebrdMpiConfig *mpi_config, GString *mpi_flavors) {
	mpi_flavors = g_string_prepend_c(mpi_flavors, ',');
	mpi_flavors = g_string_prepend(mpi_flavors, mpi_config->name->str);
	g_debug("%s", mpi_config->name->str);
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
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *version = g_list_nth_data(arguments, 0);
			GString *hostname = g_list_nth_data(arguments, 1);
			GString *gebr_cookie = g_list_nth_data(arguments, 2);

			g_debug("Current protocol version is: %s", gebr_comm_protocol_get_version());
			g_debug("Received protocol version:   %s", version->str);
			g_debug("Received GeBR cookie: %s", gebr_cookie->str);

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

			GString *mpi_flavors = g_string_new("");
			g_list_foreach(gebrd->mpi_flavors, (GFunc) get_mpi_flavors, mpi_flavors);
			if (mpi_flavors->len > 0)
				mpi_flavors = g_string_erase(mpi_flavors, mpi_flavors->len-1, 1);

			gchar *cpu_clock;
			const gchar *model_name;
			const gchar *total_memory;
			GebrdCpuInfo *cpuinfo = gebrd_cpu_info_new();
			GebrdMemInfo *meminfo = gebrd_mem_info_new();
			model_name = gebrd_cpu_info_get (cpuinfo, 0, "model name");
			cpu_clock = gebrd_cpu_info_get_clock (cpuinfo, 0, "cpu MHz");
			total_memory = gebrd_mem_info_get (meminfo, "MemTotal");
			gchar *ncores = g_strdup_printf("%d", gebrd_cpu_info_n_procs(cpuinfo));
			gchar *gebrm_path = g_find_program_in_path("gebrm");
			const gchar *has_maestro = gebrm_path ? "1" : "0";
			g_free(gebrm_path);

			gebr_comm_protocol_socket_return_message(client->socket, FALSE,
								 gebr_comm_protocol_defs.ini_def, 12,
								 gebrd->hostname,
								 server_type,
								 accounts_list->str,
								 model_name,
								 total_memory,
								 gebrd->fs_lock->str,
								 ncores,
								 cpu_clock,
								 gebrd_user_get_daemon_id(gebrd->user),
								 g_get_home_dir(),
								 mpi_flavors->str,
								 has_maestro);
			gebrd_cpu_info_free(cpuinfo);
			gebrd_mem_info_free(meminfo);
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			g_string_free(accounts_list, TRUE);
			g_string_free(queue_list, TRUE);
			g_string_free(display_port, TRUE);
			g_free(ncores);
			g_free(cpu_clock);
		}
		else if (message->hash == gebr_comm_protocol_defs.gid_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;

			GString *gid = g_list_nth_data(arguments, 0);
			GString *cookie = g_list_nth_data(arguments, 1);
			GString *display_host = g_list_nth_data(arguments, 2);
			GString *disp_str = g_list_nth_data(arguments, 3);

			guint display_port = atoi(disp_str->str) - 6000;
			gchar *display = g_strdup_printf("%s:%d", display_host->str, display_port);

			g_hash_table_insert(gebrd->display_ports, g_strdup(gid->str), display);

			g_debug("Received gid %s with cookie %s, DISPLAY=%s", gid->str, cookie->str, display);

			if (cookie->len && gebrd_get_server_type() != GEBR_COMM_SERVER_TYPE_MOAB) {
				gebrd_message(GEBR_LOG_DEBUG, "Authorizing with system(xauth)");
				gchar *tmp = g_strdup_printf(":%d", display_port);
				if (!run_xauth_command("add", tmp, cookie->str))
					display_port = 0;
				g_free(tmp);
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
			GString *numproc = g_list_nth_data(arguments, 3);
			GString *nice = g_list_nth_data(arguments, 4);
			GString *flow_xml = g_list_nth_data(arguments, 5);
			GString *paths = g_list_nth_data(arguments, 6);

			/* Moab & MPI settings */
			GString *account = g_list_nth_data(arguments, 7);
			GString *servers_mpi = g_list_nth_data(arguments, 8);

			g_debug("SERVERS MPI %s", servers_mpi->str);

			/* try to run and send return */
			job_new(&job, client, gid, id, frac, numproc, nice, flow_xml, account, paths, servers_mpi);

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
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *new_path = g_list_nth_data(arguments, 0);
			GString *old_path = g_list_nth_data(arguments, 1);
			GString *opt = g_list_nth_data(arguments, 2);

			g_debug("new_path:%s, old_path:%s, opt:%s", new_path->str, old_path->str, opt->str);

			GList *new_paths= parse_comma_separated_string(new_path->str);

			gint option = gebr_comm_protocol_path_str_to_enum(opt->str);
			gint status_id = -1;
			gboolean flag_exists = FALSE;
			gboolean flag_error = FALSE;


			switch (option) {
			case GEBR_COMM_PROTOCOL_PATH_CREATE:
				for (GList *j = new_paths; j; j = j->next) {
					GString *path = j->data;
					if (g_file_test(path->str, G_FILE_TEST_IS_DIR)){
						flag_exists = TRUE;
					}
					else if (*(path->str) && g_mkdir_with_parents(path->str, 0700)) {
						flag_error = TRUE;
						break;
					}
					if (g_access(path->str, W_OK)==-1){
						flag_error = TRUE;
						break;
					}
				}

				if (flag_error)
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
				else if (flag_exists)
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS;
				else
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
				break;
			case GEBR_COMM_PROTOCOL_PATH_RENAME:
				g_debug("Renaming %s to %s", old_path->str, new_path->str);
				gboolean dir_exist = g_file_test(new_path->str, G_FILE_TEST_IS_DIR);
				gboolean create_dir = g_rename(old_path->str, new_path->str) == 0;

				if (!dir_exist && create_dir)
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
				else if (dir_exist && !create_dir)
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS;
				else if (!dir_exist && !create_dir)
					status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
				else
					status_id = -1;
				break;
			case GEBR_COMM_PROTOCOL_PATH_DELETE:
				for (GList *j = new_paths; j; j = j->next) {
					GString *path = j->data;
					if (g_rmdir(path->str))
						status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
					else
						status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
				}
				break;
			default:
				g_warn_if_reached();
				break;
			}
			g_debug("on %s, new_path:'%s', old_path:'%s', status_id: '%d'", __func__, new_path->str, old_path->str, status_id);

			/* frees */
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);

			gebr_comm_protocol_socket_return_message(socket, FALSE,
								 gebr_comm_protocol_defs.path_def, 2,
								 gebrd->hostname,
								 g_strdup_printf("%d", status_id));
		} else if (message->hash == gebr_comm_protocol_defs.harakiri_def.code_hash) {
			/* Maestro wants me killed */
			g_debug("Harakiri");
			gebrd_quit();
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
