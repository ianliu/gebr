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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <glib/gstdio.h>

#include <libgebr.h>
#include <libgebr/intl.h>
#include <libgebr/comm.h>
#include <libgebr/geoxml.h>

#include <libgebr/utils.h>

#include <gdome.h>

#include "server.h"
#include "gebrd.h"
#include "job.h"
#include "client.h"


/*
 * Private functions
 */

static void server_moab_read_credentials(GString *accounts, GString *classes);


/*
 * Public
 */

gboolean server_init(void)
{
	GebrCommSocketAddress socket_address;
	struct sigaction act;
	gboolean ret;

	GString *log_filename;
	FILE *run_fp;

	gebr_libinit();

	/* from libgebr-misc */
	if (gebr_create_config_dirs() == FALSE) {
		gebrd_message(GEBR_LOG_ERROR, _("Could not access GêBR configuration directories.\n"));
		goto err;
	}

	/* check if there is another daemon running for this user and hostname */
	gethostname(gebrd.hostname, 255);
	gebrd.run_filename = g_string_new(NULL);
	g_string_printf(gebrd.run_filename, "%s/.gebr/run/gebrd-%s.run", getenv("HOME"), gebrd.hostname);
	if (g_file_test(gebrd.run_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == TRUE) {
		/* check if server crashed by trying connecting to it
		 * if the connection is refused, the it *probably* did
		 */
		guint16 port;

		run_fp = fopen(gebrd.run_filename->str, "r");
		fscanf(run_fp, "%hu", &port);
		fclose(run_fp);

		if (gebr_comm_listen_socket_is_local_port_available(port) == FALSE) {
			if (gebrd.options.foreground == TRUE) {
				ret = FALSE;
				gebrd_message(GEBR_LOG_ERROR,
					      _("Cannot run interactive server, GêBR daemon is already running"));
				goto out;
			}
			ret = FALSE;
			dprintf(gebrd.finished_starting_pipe[1], "%d\n", port);
			goto out;
		}
	}

	/* protocol */
	gebr_comm_protocol_init();
	g_random_set_seed((guint32) time(NULL));

	/* init the server socket and listen */
	socket_address = gebr_comm_socket_address_ipv4_local(0);
	gebrd.listen_socket = gebr_comm_listen_socket_new();
	if (gebr_comm_listen_socket_listen(gebrd.listen_socket, &socket_address) == FALSE) {
		gebrd_message(GEBR_LOG_ERROR, _("Could not listen for connections.\n"));
		goto err;
	}
	socket_address = gebr_comm_socket_get_address(GEBR_COMM_SOCKET(gebrd.listen_socket));
	g_signal_connect(gebrd.listen_socket, "new-connection", G_CALLBACK(server_new_connection), NULL);

	/* write on run directory a file with the port */
	if ((run_fp = fopen(gebrd.run_filename->str, "w")) == NULL) {
		gebrd_message(GEBR_LOG_ERROR, _("Could not write run file."));
		goto err;
	}
	fprintf(run_fp, "%d\n", gebr_comm_socket_address_get_ip_port(&socket_address));
	fclose(run_fp);

	/* log */
	log_filename = g_string_new(NULL);
	g_string_printf(log_filename, "%s/.gebr/log/gebrd-%s.log", getenv("HOME"), gebrd.hostname);
	gebrd.log = gebr_log_open(log_filename->str);

	/* connecting signal TERM */
	act.sa_sigaction = (typeof(act.sa_sigaction)) & gebrd_quit;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	/* client list structure */
	gebrd.clients = NULL;

	/* success */
	ret = TRUE;
	gebrd_message(GEBR_LOG_START, _("Server started at %u port"),
		      gebr_comm_socket_address_get_ip_port(&socket_address));
	dprintf(gebrd.finished_starting_pipe[1], "%d\n", gebr_comm_socket_address_get_ip_port(&socket_address));

	/* frees */
	g_string_free(log_filename, TRUE);

	goto out;

 err:	ret = FALSE;
	gebrd_message(GEBR_LOG_ERROR, _("Could not init server. Quiting..."));
	dprintf(gebrd.finished_starting_pipe[1], "0\n");

 out:	return ret;
}

void server_free(void)
{
	/* jobs */
	g_list_foreach(gebrd.jobs, (GFunc) job_free, NULL);
	g_list_free(gebrd.jobs);
	/* client */
	g_list_foreach(gebrd.clients, (GFunc) client_free, NULL);
	g_list_free(gebrd.clients);

	gebr_comm_protocol_destroy();
}

void server_quit(void)
{
	gebr_log_close(gebrd.log);

	/* delete lock */
	g_unlink(gebrd.run_filename->str);
	g_string_free(gebrd.run_filename, TRUE);

	server_free();
}

void server_new_connection(void)
{
	GebrCommStreamSocket *client_socket;

	while ((client_socket = gebr_comm_listen_socket_get_next_pending_connection(gebrd.listen_socket)) != NULL)
		client_add(client_socket);

	gebrd_message(GEBR_LOG_DEBUG, "server_new_connection");
}

gboolean server_parse_client_messages(struct client *client)
{
	GList *link;
	struct gebr_comm_message *message;

	while ((link = g_list_last(client->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;

		/* check login */
		if (message->hash == gebr_comm_protocol_defs.ini_def.hash) {
			GList *arguments;
			GString *version, *hostname, *place, *x11;
			GString *display_port;
			GString *accounts_list;
			GString *classes_list;
			gchar *server_type;

			display_port = g_string_new("");
			accounts_list = g_string_new("");
			classes_list = g_string_new("");

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 4);
			version = g_list_nth_data(arguments, 0);
			hostname = g_list_nth_data(arguments, 1);
			place = g_list_nth_data(arguments, 2);
			x11 = g_list_nth_data(arguments, 3);

			if (strcmp(version->str, PROTOCOL_VERSION)) {
				gebr_comm_protocol_send_data(client->protocol, client->stream_socket,
							     gebr_comm_protocol_defs.err_def, 1,
							     "Client/server version mismatch");
				goto err;
			}

			/* set client info */
			client->protocol->logged = TRUE;
			g_string_assign(client->protocol->hostname, hostname->str);
			if (!strcmp(place->str, "local"))
				client->server_location = GEBR_COMM_SERVER_LOCATION_LOCAL;
			else if (!strcmp(place->str, "remote"))
				client->server_location = GEBR_COMM_SERVER_LOCATION_REMOTE;
			else
				goto err;

			if (client->server_location == GEBR_COMM_SERVER_LOCATION_REMOTE) {
				GString *cmd_line;
				guint16 display;

				/* figure out a free display */
				display = gebrd_get_x11_redirect_display();
				if (display) {
					g_string_printf(display_port, "%d", 6000 + display);
					g_string_printf(client->display, ":%d", display);

					/* add client magic cookie */
					cmd_line = g_string_new(NULL);
					g_string_printf(cmd_line, "xauth add :%d . %s", display, x11->str);
					system(cmd_line->str);

					gebrd_message(GEBR_LOG_DEBUG, "xauth ran: %s", cmd_line->str);

					g_string_free(cmd_line, TRUE);
				} else
					g_string_assign(client->display, "");
			} else {
				/* It is a local server or a MOAB cluster. */
				g_string_assign(client->display, x11->str);
			}

			if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
				/* Get info from the MOAB cluster */
				server_type = "moab";
				server_moab_read_credentials(accounts_list, classes_list);
			} else {
				server_type = "regular";
			}

			/* send return */
			gebr_comm_protocol_send_data(client->protocol, client->stream_socket,
						     gebr_comm_protocol_defs.ret_def, 5,
						     gebrd.hostname, display_port->str,
						     accounts_list->str, classes_list->str, server_type);

			/* frees */
			gebr_comm_protocol_split_free(arguments);
			g_string_free(display_port, TRUE);
			g_string_free(accounts_list, TRUE);
			g_string_free(classes_list, TRUE);

		} else if (client->protocol->logged == FALSE) {
			/* not logged! */
			goto err;
		} else if (message->hash == gebr_comm_protocol_defs.qut_def.hash) {
			client_free(client);
			gebr_comm_message_free(message);
			return TRUE;
		} else if (message->hash == gebr_comm_protocol_defs.lst_def.hash) {
			job_list(client);
		} else if (message->hash == gebr_comm_protocol_defs.run_def.hash) {
			GList *arguments;
			GString *xml;
			struct job *job;
			gboolean success;
			GString *account, *class; 

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 3);
			xml = g_list_nth_data(arguments, 0);
			account = g_list_nth_data(arguments, 1);
			class = g_list_nth_data(arguments, 2);

			/* try to run and send return */
			if ((success = job_new(&job, client, xml, account, class)) == TRUE)
				job_run_flow(job, client);
			gebr_comm_protocol_send_data(client->protocol, client->stream_socket,
						     gebr_comm_protocol_defs.ret_def, 7, job->jid->str,
						     job->status->str, job->title->str, job->start_date->str,
						     job->issues->str, job->cmd_line->str, job->output->str);

			/* notify all clients of this new job */
			if (success == TRUE) {
				/* emit job signal to clients */
				job_send_clients_job_notify(job);
			} else
				job_free(job);

			/* frees */
			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.flw_def.hash) {
			/* TODO: */
		} else if (message->hash == gebr_comm_protocol_defs.clr_def.hash) {
			GList *arguments;
			GString *jid;
			struct job *job;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_clear(job);
			}

			/* frees */
			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.end_def.hash) {
			GList *arguments;
			GString *jid;
			struct job *job;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_end(job);
			}

			/* frees */
			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.kil_def.hash) {
			GList *arguments;
			GString *jid;
			struct job *job;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_kill(job);
			}

			/* frees */
			gebr_comm_protocol_split_free(arguments);
		} else {
			/* unknown message! */
			goto err;
		}

		gebr_comm_message_free(message);
		client->protocol->messages = g_list_delete_link(client->protocol->messages, link);
	}

	return TRUE;

 err:	gebr_comm_message_free(message);
	return FALSE;
}


/**
 * Reads the list of accounts and the list of classes returned by the MOAB cluster
 * "mcredctl" command and places them on \p accounts and \p classes, respectively.
 */
static void server_moab_read_credentials(GString *accounts, GString *classes)
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
	g_string_assign(classes, (gdome_el_getAttribute(element, attribute_name, &exception))->str);
	gdome_str_unref(attribute_name);

	gdome_doc_unref(doc, &exception);
	gdome_di_unref(dom_impl, &exception);

 err:	g_string_free(cmd_line, TRUE);
	g_free(std_out);
	g_free(std_err);
}

