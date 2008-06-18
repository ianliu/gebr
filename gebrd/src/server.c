/*   GeBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <glib/gstdio.h>

#include <comm.h>
#include <geoxml.h>

#include <misc/utils.h>

#include "server.h"
#include "gebrd.h"
#include "support.h"
#include "job.h"
#include "client.h"

/*
 * Public
 */

gboolean
server_init(void)
{
	GHostAddress *		host_address;
	struct sigaction	act;
	gboolean		ret;

	GString *		log_filename;
	FILE *			run_fp;

	/* initialization */
	gebrd.run_filename = g_string_new(NULL);
	gethostname(gebrd.hostname, 255);
	log_filename = g_string_new(NULL);

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* from libgebr-misc */
	if (gebr_create_config_dirs() == FALSE) {
		gebrd_message(LOG_ERROR, _("Could not access GêBR configuration directories.\n"));
		goto err;
	}

	/* log */
	g_string_printf(log_filename, "%s/.gebr/log/gebrd-%s.log", getenv("HOME"), gebrd.hostname);
	gebrd.log = log_open(log_filename->str);

	/* protocol */
	protocol_init();

	/* init the server socket and listen */
	gebrd.tcp_server = g_tcp_server_new();
	if (g_tcp_server_listen(gebrd.tcp_server, host_address, 0) == FALSE) {
		gebrd_message(LOG_ERROR, _("Could not listen for connections.\n"));
		goto err;
	}
	g_signal_connect(gebrd.tcp_server, "new-connection",
		G_CALLBACK(server_new_connection), NULL);

	/* write on user's home directory a file with a port */
	g_string_printf(gebrd.run_filename, "%s/.gebr/run/gebrd-%s.run", getenv("HOME"), gebrd.hostname);
	if (g_file_test(gebrd.run_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == TRUE) {
		/* check if server crashed by trying connecting to it
		 * if the connection is refused, the it *probably* did
		 */
		guint16		port;

		run_fp = fopen(gebrd.run_filename->str, "r");
		fscanf(run_fp, "%hu", &port);
		fclose(run_fp);

		if (g_tcp_server_is_local_port_available(port) == FALSE) {
			if (gebrd.options.foreground == TRUE) {
				ret = FALSE;
				gebrd_message(LOG_ERROR,
					_("Cannot run interactive server, GeBR daemon is already running"));
				goto out;
			}
			ret = FALSE;
			dprintf(gebrd.finished_starting_pipe[1], "%d\n", port);
			goto out;
		}
	}
	if ((run_fp = fopen(gebrd.run_filename->str, "w")) == NULL) {
		gebrd_message(LOG_ERROR, _("Could not write run file."));
		goto err;
	}
	fprintf(run_fp, "%d\n", g_tcp_server_server_port(gebrd.tcp_server));
	fclose(run_fp);

	/* connecting signal TERM */
	act.sa_sigaction = (typeof(act.sa_sigaction))&gebrd_quit;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	/* client list structure */
	gebrd.clients = NULL;

	/* success */
	ret = TRUE;
	gebrd_message(LOG_START, _("Server started at %u port"), g_tcp_server_server_port(gebrd.tcp_server));
	dprintf(gebrd.finished_starting_pipe[1], "%d\n", g_tcp_server_server_port(gebrd.tcp_server));
	goto out;

err:	ret = FALSE;
	gebrd_message(LOG_ERROR, _("Could not init server. Quiting..."));
	dprintf(gebrd.finished_starting_pipe[1], "0\n");

out:	g_string_free(log_filename, TRUE);
	g_host_address_free(host_address);

	return ret;
}

void
server_free(void)
{
	/* jobs */
	g_list_foreach(gebrd.jobs, (GFunc)job_free, NULL);
	g_list_free(gebrd.jobs);
	/* client */
	g_list_foreach(gebrd.clients, (GFunc)client_free, NULL);
	g_list_free(gebrd.clients);
}

void
server_quit(void)
{
	log_close(gebrd.log);

	/* delete lock */
	g_unlink(gebrd.run_filename->str);
	g_string_free(gebrd.run_filename, TRUE);

	server_free();
}

void
server_new_connection(void)
{
	GTcpSocket *	client_socket;

	while ((client_socket = g_tcp_server_get_next_pending_connection(gebrd.tcp_server)) != NULL)
		client_add(client_socket);

	gebrd_message(LOG_DEBUG, "server_new_connection");
}

gboolean
server_parse_client_messages(struct client * client)
{
	GList *			link;
	struct message *	message;

	while ((link = g_list_last(client->protocol->messages)) != NULL) {
		message = (struct message *)link->data;

		/* check login */
		if (message->hash == protocol_defs.ini_def.hash) {
			GList *		arguments;
			GString *	version, * hostname, * mcookie;
			GString *	display_port;

			display_port = g_string_new("");
			/* organize message data */
			arguments = protocol_split_new(message->argument, 3);
			version = g_list_nth_data(arguments, 0);
			hostname = g_list_nth_data(arguments, 1);
			mcookie = g_list_nth_data(arguments, 2);

			/* set client info */
			client->protocol->logged = TRUE;
			g_string_assign(client->protocol->hostname, hostname->str);

			if (mcookie->len) {
				GString *	cmd_line;
				guint16		display;

				/* figure out a free display */
				display = gebrd_get_x11_redirect_display();
				if (display) {
					g_string_printf(display_port, "%d", 6000+display);
					g_string_printf(client->display, ":%d", display);

					/* add client magic cookie */
					cmd_line = g_string_new(NULL);
					g_string_printf(cmd_line, "xauth add :%d . %s",
						display, mcookie->str);
					system(cmd_line->str);

					gebrd_message(LOG_DEBUG, "xauth ran: %s", cmd_line->str);

					g_string_free(cmd_line, TRUE);
				} else
					g_string_assign(client->display, "");
			} else
				g_string_assign(client->display, "");

			/* send return */
			protocol_send_data(client->protocol, client->tcp_socket,
				protocol_defs.ret_def, 2, gebrd.hostname, display_port->str);

			/* frees */
			protocol_split_free(arguments);
			g_string_free(display_port, TRUE);
		} else if (client->protocol->logged == FALSE) {
			/* not logged! */
			goto err;
		} else if (message->hash == protocol_defs.qut_def.hash) {
			client_free(client);
			message_free(message);
			return TRUE;
		} else if (message->hash == protocol_defs.lst_def.hash) {
			job_list(client);
		} else if (message->hash == protocol_defs.run_def.hash) {
			GList *			arguments;
			GString *		xml;
			struct job *		job;
			gboolean		success;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 1);
			xml = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			if ((success = job_new(&job, client, xml)) == TRUE)
				job_run_flow(job, client);
			protocol_send_data(client->protocol, client->tcp_socket, protocol_defs.ret_def,
				7, job->jid->str, job->status->str, job->title->str,
				job->start_date->str, job->issues->str,
				job->cmd_line->str, job->output->str);

			/* notify all clients of this new job */
			if (success == TRUE) {
				/* emit job signal to clients */
				job_send_clients_job_notify(job);
			} else
				job_free(job);

			/* frees */
			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.flw_def.hash) {
			/* TODO: */
		} else if (message->hash == protocol_defs.clr_def.hash) {
			GList *			arguments;
			GString *		jid;
			struct job *		job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_clear(job);
			}

			/* frees */
			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.end_def.hash) {
			GList *			arguments;
			GString *		jid;
			struct job *		job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_end(job);
			}

			/* frees */
			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.kil_def.hash) {
			GList *			arguments;
			GString *		jid;
			struct job *		job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 1);
			jid = g_list_nth_data(arguments, 0);

			/* try to run and send return */
			job = job_find(jid);
			if (job != NULL) {
				job_kill(job);
			}

			/* frees */
			protocol_split_free(arguments);
		} else {
			/* unknown message! */
			goto err;
		}

		message_free(message);
		client->protocol->messages = g_list_delete_link(client->protocol->messages, link);
	}

	return TRUE;

err:	message_free(message);
	return FALSE;
}

