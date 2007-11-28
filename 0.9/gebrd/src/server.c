/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <misc.h>
#include <geoxml.h>

#include "server.h"
#include "gebrd.h"
#include "job.h"
#include "client.h"

gboolean
server_init(void)
{
	GHostAddress *	  host_address;
	gboolean	  ret;
	GString *	  run_filename;
	FILE *		  run_fp;
	struct sigaction  act;

	/* run path */
	run_filename = g_string_new(getenv("HOME"));

	/* protocol */
	protocol_init();

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* init the server socket and listen */
	gebrd.tcp_server = g_tcp_server_new();
	ret = g_tcp_server_listen(gebrd.tcp_server, host_address, 0);
	if (ret == FALSE) {
		g_print("Could not listen for connections.\n");
		goto out;
	}
	g_signal_connect(gebrd.tcp_server, "new-connection",
			G_CALLBACK(server_new_connection), NULL);

	/* Test for gebr conf dir */
	g_string_append(run_filename, "/.gebr");
	if (g_file_test(run_filename->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
	   struct stat	home_stat;
	   gchar *	home = getenv("HOME");
	   stat(home, &home_stat);

	   if (mkdir(run_filename->str, home_stat.st_mode)){
	      ret = FALSE;
	      goto out;
	   }
	}

	/* Test for .gebr/run conf dir */
	g_string_append(run_filename, "/run");
	if (g_file_test(run_filename->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
	   struct stat	home_stat;
	   gchar *	home = getenv("HOME");
	   stat(home, &home_stat);

	   if (mkdir(run_filename->str, home_stat.st_mode)){
	      ret = FALSE;
	      goto out;
	   }
	}

	g_string_append(run_filename, "/gebrd.run");
	/* write on user's home directory a file with a port */
	if (g_file_test(run_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == TRUE) {
		g_print("Run file (~/.gebr/run/gebrd.run) already exists or is not a regular file\n");
		ret = FALSE;
		goto out;
	}
	if ((run_fp = fopen(run_filename->str, "w")) == NULL) {
		g_print("Could not write run file\n");
		ret = FALSE;
		goto out;
	}
	fprintf(run_fp, "%d\n", g_tcp_server_server_port(gebrd.tcp_server));
	fclose(run_fp);


	/* connecting signals TERM */
	act.sa_sigaction = &gebrd_quit;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);

	/* client list structure */
	gebrd.clients = NULL;

out:	g_string_free(run_filename, TRUE);
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
	GString *	run_filename;

	/* delete lock */
	run_filename = g_string_new(getenv("HOME"));
	g_string_append(run_filename, "/.gebr/run/gebrd.run");
	unlink(run_filename->str);
	g_string_free(run_filename, TRUE);

	server_free();
}

void
server_new_connection(void)
{
	GTcpSocket *	client_socket;
g_print("new connection\n");
	while ((client_socket = g_tcp_server_get_next_pending_connection(gebrd.tcp_server)) != NULL)
		client_add(client_socket);
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
			GString *	version, * hostname, * display, * mcookie;
			gchar		server_hostname[100];
			GString *       cmd_line;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 4);
			version = g_list_nth_data(arguments, 0);
			hostname = g_list_nth_data(arguments, 1);
			display = g_list_nth_data(arguments, 2);
			mcookie = g_list_nth_data(arguments, 3);

			/* set client info */
			client->protocol->logged = TRUE;
			g_string_assign(client->protocol->hostname, hostname->str);
			g_string_assign(client->display, display->str);
			g_string_assign(client->mcookie, mcookie->str);

			/* add client magic cookie */
			cmd_line = g_string_new(NULL);
			g_string_printf(cmd_line, "xauth add %s%s . %s",
					hostname->str,
					display->str,
					mcookie->str);
			printf("%s\n", cmd_line->str);
			system(cmd_line->str);
			g_string_free(cmd_line, TRUE);

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

