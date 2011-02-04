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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgebr/comm/protocol.h>
#include <libgebr/comm/server.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "client.h"
#include "gebr.h"
#include "server.h"
#include "job.h"

gboolean client_parse_server_messages(struct gebr_comm_server *comm_server, struct server *server)
{
	GList *link;
	struct gebr_comm_message *message;

	while ((link = g_list_last(comm_server->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;

		if (message->hash == gebr_comm_protocol_defs.err_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_split_new(message->argument, 1)) == NULL)
				goto err;
			g_string_assign(server->last_error, ((GString *)g_list_nth_data(arguments, 0))->str);
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Server '%s' reported error '%s'."),
				     comm_server->address->str, server->last_error->str);

			gebr_comm_protocol_split_free(arguments);

		} else if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			if (comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.ini_def.code_hash) {
				GList *arguments;
				GString *hostname;
				GString *display_port;
				gchar ** accounts;
				gchar ** queues;
				GtkTreeIter iter;

				/* organize message data */
				if ((arguments = gebr_comm_protocol_split_new(message->argument, 5)) == NULL)
					goto err;
				hostname = g_list_nth_data(arguments, 0);
				display_port = g_list_nth_data(arguments, 1);
				queues = g_strsplit(((GString *)g_list_nth_data(arguments, 2))->str, ",", 0);
				server->type = gebr_comm_server_get_id(((GString*)g_list_nth_data(arguments, 3))->str);
				accounts = g_strsplit(((GString *)g_list_nth_data(arguments, 4))->str, ",", 0);

				gtk_list_store_clear(server->accounts_model);
				gtk_list_store_clear(server->queues_model);

				for (gint i = 0; accounts[i]; i++) {
					gtk_list_store_append(server->accounts_model, &iter);
					gtk_list_store_set(server->accounts_model, &iter, 0, accounts[i], -1);
				}

				if (server->type == GEBR_COMM_SERVER_TYPE_REGULAR) {
					gtk_list_store_append(server->queues_model, &iter);
					gtk_list_store_set(server->queues_model, &iter,
							   SERVER_QUEUE_TITLE, _("Immediately"),
							   SERVER_QUEUE_ID, "j", 
							   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);
				}
				for (gint i = 0; queues[i]; i++) {
					if (strlen(queues[i])) {
						if (queues[i][0] != 'j') {
							GString *string;

							string = g_string_new("");
							g_string_printf(string, _("At '%s'"),
									server->type == GEBR_COMM_SERVER_TYPE_REGULAR
								       	? queues[i]+1 : queues[i]);
							gtk_list_store_append(server->queues_model, &iter);
							gtk_list_store_set(server->queues_model, &iter,
									   SERVER_QUEUE_TITLE, string->str,
									   SERVER_QUEUE_ID, queues[i],
									   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);

							g_string_free(string, TRUE);
						}

						/* at job control */
						server_queue_find_at_job_control(server, queues[i], NULL);
					}
				}

				gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);

				g_strfreev(accounts);
				g_strfreev(queues);


				/* say we are logged */
				g_string_assign(server->last_error, "");
				comm_server->protocol->logged = TRUE;
				server_list_updated_status(server);
				g_string_assign(comm_server->protocol->hostname, hostname->str);
				if (gebr_comm_server_is_local(comm_server) == TRUE)
					gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Connected to local server."),
						     comm_server->address->str);
				else {
					gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Connected to server '%s'."),
						     comm_server->address->str);
					if (display_port->len) {
						if (!strcmp(display_port->str, "0"))
							gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
								     _("Server '%s' could not add X11 authorization to redirect graphical output."),
								     comm_server->address->str);
						else
							gebr_comm_server_forward_x11(comm_server, atoi(display_port->str));
					} else 
						gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
							     _("Server '%s' could not redirect graphical output."),
							     comm_server->address->str);
				}

				/* request list of jobs */
				gebr_comm_protocol_send_data(comm_server->protocol,
							     comm_server->stream_socket,
							     gebr_comm_protocol_defs.lst_def, 0);

				gebr_comm_protocol_split_free(arguments);
			} else if (comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.run_def.code_hash) {
				GList *arguments;
				GString *jid;
				GString *run_id;

				/* organize message data */
				if ((arguments = gebr_comm_protocol_split_new(message->argument, 2)) == NULL)
					goto err;
				jid = g_list_nth_data(arguments, 0);
				run_id = g_list_nth_data(arguments, 1);

				struct job * job = job_find(comm_server->address, run_id, FALSE);
				if (job != NULL) {
					g_string_assign(job->jid, jid->str);

					/* move it to the end, the right place... */
					gboolean was_selected = job_is_active(job);
					gebr_gui_gtk_tree_store_move_before(gebr.ui_job_control->store, &job->iter, NULL);
					if (was_selected)
						job_set_active(job);
				}

				gebr_comm_protocol_split_free(arguments);
			} else if (comm_server->protocol->waiting_ret_hash == gebr_comm_protocol_defs.flw_def.code_hash) {

			}
		} else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;
			GString *jid, *hostname, *status, *title, *start_date, *finish_date, *issues, *cmd_line,
				*output, *queue, *moab_jid;
			struct job *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_split_new(message->argument, 11)) == NULL)
				goto err;
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			title = g_list_nth_data(arguments, 2);
			start_date = g_list_nth_data(arguments, 3);
			finish_date = g_list_nth_data(arguments, 4);
			hostname = g_list_nth_data(arguments, 5);
			issues = g_list_nth_data(arguments, 6);
			cmd_line = g_list_nth_data(arguments, 7);
			output = g_list_nth_data(arguments, 8);
			queue = g_list_nth_data(arguments, 9);
			moab_jid = g_list_nth_data(arguments, 10);

			job = job_find(comm_server->address, jid, TRUE);
			if (job == NULL)
				job = job_new_from_jid(server, jid, status, title, start_date, finish_date,
						       hostname, issues, cmd_line, output, queue, moab_jid);
			else if (job->waiting_server_details)
				job_init_details(job, status, title, start_date, finish_date,
						 hostname, issues, cmd_line, output, queue, moab_jid);
			else
				gebr_message(GEBR_LOG_DEBUG, FALSE, FALSE, _("Received already listed job %s."), job->jid);

			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *jid, *output;
			struct job *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_split_new(message->argument, 2)) == NULL)
				goto err;
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);

			job = job_find(comm_server->address, jid, TRUE);
			if (job != NULL) {
				job_append_output(job, output);
			}

			gebr_comm_protocol_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;
			GString *jid, *status, *parameter;
			struct job *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_split_new(message->argument, 3)) == NULL)
				goto err;
			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			parameter = g_list_nth_data(arguments, 2);

			job = job_find(comm_server->address, jid, TRUE);
			if (job != NULL) {
				enum JobStatus status_enum;

				status_enum = job_translate_status(status);
				job_status_update(job, status_enum, parameter->str);
			}

			gebr_comm_protocol_split_free(arguments);
		}

		gebr_comm_message_free(message);
		comm_server->protocol->messages = g_list_delete_link(comm_server->protocol->messages, link);
	}

	return TRUE;

err:	gebr_comm_message_free(message);
	comm_server->protocol->messages = g_list_delete_link(comm_server->protocol->messages, link);
	if (gebr_comm_server_is_local(comm_server) == TRUE)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with local server. Please reconnect."));
	else
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with server '%s'. Please reconnect."),
			     comm_server->address->str);
	gebr_comm_server_disconnect(comm_server);

	return FALSE;
}
