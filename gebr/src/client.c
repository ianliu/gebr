/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <misc/protocol.h>

#include "client.h"
#include "server.h"
#include "cb_job.h"
#include "gebr.h"

gboolean
client_parse_server_messages(struct server * server)
{
	GList *			link;
	struct message *	message;

	while ((link = g_list_last(server->protocol->messages)) != NULL) {
		message = (struct message *)link->data;

		if (message->hash == protocol_defs.ret_def.hash) {
			if (server->protocol->waiting_ret_hash == protocol_defs.ini_def.hash) {
				GList *		arguments;
				GString *	hostname;

				/* organize message data */
				arguments = protocol_split_new(message->argument, 1);
				hostname = g_list_nth_data(arguments, 0);

				/* say we are logged */
				server->protocol->logged = TRUE;
				g_string_assign(server->protocol->hostname, hostname->str);
				/* request list of jobs */
				protocol_send_data(server->protocol, server->tcp_socket,
					protocol_defs.lst_def, 0);

				protocol_split_free(arguments);
			} else if (server->protocol->waiting_ret_hash == protocol_defs.run_def.hash) {
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
					NULL, issues, cmd_line, output, TRUE);
				if (job->status != JOB_STATUS_RUNNING) {
					g_string_assign(job->finish_date, job->start_date->str);
					job_clicked();
				}

				protocol_split_free(arguments);
			} else if (server->protocol->waiting_ret_hash == protocol_defs.flw_def.hash) {

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

			job = job_find(server->address, jid);
			if (job == NULL) {
				job = job_add(server, jid, status, title, start_date, finish_date,
					hostname, issues, cmd_line, output, FALSE);
			}

			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.out_def.hash) {
			GList *		arguments;
			GString *	jid, * output;
			struct job *	job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 2);
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);

			job = job_find(server->address, jid);
			if (job != NULL) {
				job_append_output(job, output->str);
			}

			protocol_split_free(arguments);
		} else if (message->hash == protocol_defs.fin_def.hash) {
			GList *		arguments;
			GString *	jid, * status;
			struct job *	job;

			/* organize message data */
			arguments = protocol_split_new(message->argument, 2);
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);

			job = job_find(server->address, jid);
			if (job != NULL) {
				job->status = job_translate_status(status);
				job_update_status(job);
			}

			protocol_split_free(arguments);
		} else {
			/* unknown message! */
			goto err;
		}

		message_free(message);
		server->protocol->messages = g_list_delete_link(server->protocol->messages, link);
	}

	return TRUE;

err:	message_free(message);
	return FALSE;
}
