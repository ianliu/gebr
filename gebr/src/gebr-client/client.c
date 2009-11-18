/*   GeBR - An environment for seismic processing.
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

#include <libgebr/comm/protocol.h>

#include "client.h"
#include "gebrclient.h"
#include "server.h"

gboolean
client_parse_server_messages(struct gebr_comm_server * gebr_comm_server, struct server * server)
{
	GList *			link;
	struct gebr_comm_message *	message;

	while ((link = g_list_last(gebr_comm_server->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;

		if (message->hash == gebr_comm_protocol_defs.ret_def.hash) {
			if (gebr_comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.ini_def.hash) {
				GList *		arguments;
				GString *	hostname;

				/* organize message data */
				arguments = gebr_comm_protocol_split_new(message->argument, 1);
				hostname = g_list_nth_data(arguments, 0);

				/* say we are logged */
				gebr_comm_server->protocol->logged = TRUE;
// 				TODO:
				g_string_assign(gebr_comm_server->protocol->hostname, hostname->str);

				/* request list of jobs */
				gebr_comm_protocol_send_data(gebr_comm_server->protocol, gebr_comm_server->stream_socket,
					gebr_comm_protocol_defs.lst_def, 0);

				gebr_comm_protocol_split_free(arguments);
			} else if (gebr_comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.run_def.hash) {
				GList *		arguments;
				GString *	jid, *status, * title, * start_date, * issues, * cmd_line, * output;

				/* organize message data */
				arguments = gebr_comm_protocol_split_new(message->argument, 7);
				jid = g_list_nth_data(arguments, 0);
				status = g_list_nth_data(arguments, 1);
				title = g_list_nth_data(arguments, 2);
				start_date = g_list_nth_data(arguments, 3);
				issues = g_list_nth_data(arguments, 4);
				cmd_line = g_list_nth_data(arguments, 5);
				output = g_list_nth_data(arguments, 6);

// 				TODO:
// 				job = job_add(server, jid, status, title, start_date, NULL,
// 					NULL, issues, cmd_line, output);
// 				job_set_active(job);
// 				gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), 3);

				gebr_comm_protocol_split_free(arguments);
			} else if (gebr_comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.flw_def.hash) {

			}
		} else if (message->hash == gebr_comm_protocol_defs.job_def.hash) {
			GList *		arguments;
			GString *	jid, * hostname, * status, * title, * start_date, * finish_date, * issues, * cmd_line, * output;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 9);
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			title = g_list_nth_data(arguments, 2);
			start_date = g_list_nth_data(arguments, 3);
			finish_date = g_list_nth_data(arguments, 4);
			hostname = g_list_nth_data(arguments, 5);
			issues = g_list_nth_data(arguments, 6);
			cmd_line = g_list_nth_data(arguments, 7);
			output = g_list_nth_data(arguments, 8);

// 			TODO:
// 			job = job_find(gebr_comm_server->address, jid);
// 			if (job == NULL)
// 				job = job_add(server, jid, status, title, start_date, finish_date,
// 					hostname, issues, cmd_line, output);

			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.out_def.hash) {
			GList *		arguments;
			GString *	jid, * output;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 2);
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);

// 			TODO:
// 			job = job_find(gebr_comm_server->address, jid);
// 			if (job != NULL) {
// 				job_append_output(job, output);
// 			}

			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.fin_def.hash) {
			GList *		arguments;
			GString *	jid, * status, * finish_date;

			/* organize message data */
			arguments = gebr_comm_protocol_split_new(message->argument, 3);
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			finish_date = g_list_nth_data(arguments, 2);

// 			TODO:
// 			job = job_find(gebr_comm_server->address, jid);
// 			if (job != NULL) {
// 				g_string_assign(job->finish_date, finish_date->str);
// 				job->status = job_translate_status(status);
// 				job_update_status(job);
// 			}

			gebr_comm_protocol_split_free(arguments);
		} else {
			/* unknown message! */
			goto err;
		}

		gebr_comm_message_free(message);
		gebr_comm_server->protocol->messages = g_list_delete_link(gebr_comm_server->protocol->messages, link);
	}

	return TRUE;

err:	gebr_comm_message_free(message);
	return FALSE;
}
