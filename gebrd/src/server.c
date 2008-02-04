/*   G�BR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

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
 * Internal
 */

struct try_connect {
	GHostAddress *	host_address;
	guint16		port;

	gboolean	refused;
};

static gpointer
__try_connect(struct try_connect * try_connect)
{
	GTcpSocket *	tcp_socket;

	tcp_socket = g_tcp_socket_new();
	g_tcp_socket_connect(tcp_socket, try_connect->host_address, try_connect->port, TRUE);
// 	try_connect->refused = g_socket_last_error(G_SOCKET(tcp_socket)) == G_SOCKET_ERROR_CONNECTION_REFUSED;
	try_connect->refused = g_socket_get_state(G_SOCKET(tcp_socket)) != G_SOCKET_STATE_CONNECTED;

	g_socket_close(G_SOCKET(tcp_socket));

	return NULL;
}

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
	GString *		run_filename;
	FILE *			run_fp;

	gchar *			ssh_env;
	GString *		client_ip;

	/* run path */
	log_filename = g_string_new(NULL);
	run_filename = g_string_new(NULL);
	client_ip = g_string_new("");

	/* if we were ran by ssh, then use IP to change log and run filenames */
	ssh_env = getenv("SSH_CLIENT");
	if (ssh_env != NULL) {
		gchar **	splits;

		splits = g_strsplit(ssh_env, " ", 3);
		g_string_printf(client_ip, "-%s", splits[0]);

		g_strfreev(splits);
	}

	/* from libgebr-misc */
	if (gebr_create_config_dirs() == FALSE) {
		gebrd_message(ERROR, _("Could not access G�BR configuration directories.\n"));
		goto err;
	}

	/* log */
	g_string_printf(log_filename, "%s/.gebr/log/gebrd%s.log", getenv("HOME"), client_ip->str);
	gebrd.log = log_open(log_filename->str);

	/* protocol */
	protocol_init();

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* init the server socket and listen */
	gebrd.tcp_server = g_tcp_server_new();
	if (g_tcp_server_listen(gebrd.tcp_server, host_address, 0) == FALSE) {
		gebrd_message(ERROR, _("Could not listen for connections.\n"));
		goto err;
	}
	g_signal_connect(gebrd.tcp_server, "new-connection",
			G_CALLBACK(server_new_connection), NULL);

	/* write on user's home directory a file with a port */
	g_string_printf(run_filename, "%s/.gebr/run/gebrd%s.run", getenv("HOME"), client_ip->str);
	if (g_file_test(run_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == TRUE) {
		/* check if server crashed by trying connecting to it
		 * if the connection is refused, the it *probably* did
		 */
		struct try_connect	try_connect;
		GThread *		thread;
		GError *		error;

		if (!g_thread_supported())
			g_thread_init(NULL);

		try_connect.host_address = host_address;
		run_fp = fopen(run_filename->str, "r");
		fscanf(run_fp, "%hu", &try_connect.port);
		fclose(run_fp);

		error = NULL;
		thread = g_thread_create((GThreadFunc)__try_connect, &try_connect, TRUE, &error);
		g_thread_join(thread);

		if (try_connect.refused == FALSE) {
			gebrd_message(ERROR, _("Run file (~/.gebr/run/gebrd.run) already exists or is not a regular file."));
			goto err;
		}
	}
	if ((run_fp = fopen(run_filename->str, "w")) == NULL) {
		gebrd_message(ERROR, _("Could not write run file."));
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

	/* sucess */
	ret = TRUE;
	gebrd_message(START, _("Server started at %u port"), g_tcp_server_server_port(gebrd.tcp_server));
	goto out;

err:	ret = FALSE;
	gebrd_message(ERROR, _("Could not init server. Quiting..."));

out:	g_string_free(log_filename, TRUE);
	g_string_free(run_filename, TRUE);
	g_host_address_free(host_address);

	/* report to the parent process we finished to start */
	write(gebrd.finished_starting_pipe[1], "1", 2);

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
	GString *	run_filename;

	log_close(gebrd.log);

	/* delete lock */
	run_filename = g_string_new(NULL);
	g_string_printf(run_filename, "%s/.gebr/run/gebrd.run", getenv("HOME"));
	g_unlink(run_filename->str);
	g_string_free(run_filename, TRUE);

	server_free();
}

void
server_new_connection(void)
{
	GTcpSocket *	client_socket;

	while ((client_socket = g_tcp_server_get_next_pending_connection(gebrd.tcp_server)) != NULL)
		client_add(client_socket);

	gebrd_message(DEBUG, "server_new_connection");
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
			GString *	version, * hostname, * address, * display, * mcookie;
			gchar		server_hostname[100];

			/* organize message data */
			arguments = protocol_split_new(message->argument, 5);
			version = g_list_nth_data(arguments, 0);
			hostname = g_list_nth_data(arguments, 1);
			address = g_list_nth_data(arguments, 2);
			display = g_list_nth_data(arguments, 3);
			mcookie = g_list_nth_data(arguments, 4);

			/* set client info */
			client->protocol->logged = TRUE;
			g_string_assign(client->protocol->hostname, hostname->str);
			g_string_assign(client->address, address->str);
			g_string_assign(client->display, display->str);
			g_string_assign(client->mcookie, mcookie->str);

			if (client_is_local(client) == FALSE) {
				GString *	cmd_line;

				/* add client magic cookie */
				cmd_line = g_string_new(NULL);
				g_string_printf(cmd_line, "xauth add %s%s . %s",
						client->address->str,
						display->str,
						mcookie->str);
				system(cmd_line->str);

				g_string_free(cmd_line, TRUE);
			}

			/* send return */
			gethostname(server_hostname, 100);
			protocol_send_data(client->protocol, client->tcp_socket,
				protocol_defs.ret_def, 1, server_hostname);

			/* frees */
			protocol_split_free(arguments);
		} else if (client->protocol->logged == FALSE) {
			/* not logged! */
			goto err;
		} else if (message->hash == protocol_defs.qut_def.hash) {
			if (g_list_length(gebrd.clients) == 1) {
				/* FIXME: seg. fault here */
				gebrd_quit();
			} else {
				client_free(client);
			}

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

