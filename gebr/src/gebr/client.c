/*   GeBR - An environment for seismic processing.
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

#include <stdlib.h>

#include <comm/protocol.h>

#include "client.h"
#include "gebr.h"
#include "support.h"
#include "server.h"
#include "job.h"

gboolean
client_parse_server_messages(struct comm_server * comm_server, struct server * server)
{
	GList *			link;
	struct message *	message;

	while ((link = g_list_last(comm_server->protocol->messages)) != NULL) {
		message = (struct message *)link->data;

		if (message->hash == protocol_defs.ret_def.hash) {
			if (comm_server->protocol->waiting_ret_hash == protocol_defs.ini_def.hash) {
				GList *		arguments;
				GString *	hostname, * display_port;

				/* organize message data */
				arguments = protocol_split_new(message->argument, 2);
				hostname = g_list_nth_data(arguments, 0);
				display_port = g_list_nth_data(arguments, 1);

				/* say we are logged */
				comm_server->protocol->logged = TRUE;
				server_list_updated_status(server);
				g_string_assign(comm_server->protocol->hostname, hostname->str);
				if (comm_server_is_local(comm_server) == TRUE)
					gebr_message(LOG_INFO, TRUE, TRUE, _("Connected to local server"),
						comm_server->address->str);
				else {
					gebr_message(LOG_INFO, TRUE, TRUE, _("Connected to server '%s'"),
						comm_server->address->str);
					comm_server_forward_x11(comm_server, atoi(display_port->str));
				}

				/* request list of jobs */
				protocol_send_data(comm_server->protocol, comm_server->tcp_socket,
					protocol_defs.lst_def, 0);

				protocol_split_free(arguments);
			} else if (comm_server->protocol->waiting_ret_hash == protocol_defs.run_def.hash) {
				GList *		arguments;
				GString *	jid, *status, * title, * start_date, * issues, * cmd_line, * output;
				struct job *	job;

				/* organize message data */
				arguments = protocol_split_new(message->argument, 7);
				jid = g_list_nth_data(arguments, 0);
				status = g_list_nth_data(arguments, 1);
				title = g_list_nth_data(arguments, 2);
				start_date = g_list_nth_data(arguments, 3);
				issues = g_list_nth_data(arguments, 4);
				cmd_line = g_list_nth_data(arguments, 5);
				output = g_list_nth_data(arguments, 6);

				job = job_add(server, jid, status, title, start_date, NULL,
					NULL, issues, cmd_line, output);
				job_set_active(job);
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), 3);

				protocol_split_free(arguments);
			} else if (comm_server->protocol->waiting_ret_hash == protocol_defs.flw_def.hash) {

			}
		} else if (message->hash == protocol_defs.job_def.hash) {
			GList *		arguments;
			GString *	jid, * hostname, * status, * title, * start_date, * finish_date, * issues, * cmd_line, * output;
			struct job *	job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 9);
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			title = g_list_nth_data(arguments, 2);
			start_date = g_list_nth_data(arguments, 3);
			finish_date = g_list_nth_data(arguments, 4);
			hostname = g_list_nth_data(arguments, 5);
			issues = g_list_nth_data(arguments, 6);
			cmd_line = g_list_nth_data(arguments, 7);
			output = g_list_nth_data(arguments, 8);

			job = job_find(comm_server->address, jid);
			if (job == NULL)
				job = job_add(server, jid, status, title, start_date, finish_date,
					hostname, issues, cmd_line, output);

			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.out_def.hash) {
			GList *		arguments;
			GString *	jid, * output;
			struct job *	job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 2);
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);

			job = job_find(comm_server->address, jid);
			if (job != NULL) {
				job_append_output(job, output);
			}

			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.fin_def.hash) {
			GList *		arguments;
			GString *	jid, * status, * finish_date;
			struct job *	job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 3);
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			finish_date = g_list_nth_data(arguments, 2);

			job = job_find(comm_server->address, jid);
			if (job != NULL) {
				g_string_assign(job->finish_date, finish_date->str);
				job->status = job_translate_status(status);
				job_update_status(job);
			}

			protocol_split_free(arguments);
		} else {
			/* unknown message! */
			goto err;
		}

		message_free(message);
		comm_server->protocol->messages = g_list_delete_link(comm_server->protocol->messages, link);
	}

	return TRUE;

err:	message_free(message);
	return FALSE;
}
