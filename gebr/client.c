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
#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/comm/gebr-comm-server.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "client.h"
#include "gebr.h"
#include "server.h"
#include "gebr-task.h"
#include "gebr-job.h"

void client_process_server_request(struct gebr_comm_server *comm_server, GebrCommHttpMsg *request, GebrServer *server)
{
}

void client_process_server_response(struct gebr_comm_server *comm_server, GebrCommHttpMsg *request,
				   GebrCommHttpMsg *response, GebrServer *server)
{
}

/**
 * This is a callback function for the "response-recived" signal.
 * Explanation: First the client sends a message to the server,
 * (\see gebr_comm_protocol_socket_send_request). Then, after
 * processsing the message, the server sends an response to the
 * client. When the client recieves this response, it will trigger
 * the "response-recieved" signal (this signal is emitted by the 
 * http request).
 *
 * @Parameters:
 * request: This is the http-request who emitted the signal.
 * response: This is an http-response with the data sent by the server.
 *
 */
void on_fs_nickname_msg(GebrCommHttpMsg *request, GebrCommHttpMsg *response, GebrServer *server)
{
	if (request->method == GEBR_COMM_HTTP_METHOD_GET) 
	{
		GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
		GString *value = gebr_comm_json_content_to_gstring(json);
		gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter, SERVER_FS, value->str, -1);
		ui_server_update_tags_combobox ();
		g_string_free(value, TRUE);
		gebr_comm_json_content_free(json);
	} 
	else if (request->method == GEBR_COMM_HTTP_METHOD_PUT) 
	{
		gchar *value = g_object_get_data(G_OBJECT(request), "value");
		if (response->status_code == 200)
			gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter, SERVER_FS, value, -1);
		//else
		g_free(value);
	}
}

gboolean client_parse_server_messages(struct gebr_comm_server *comm_server, GebrServer *server)
{
	GList *link;
	struct gebr_comm_message *message;

	while ((link = g_list_last(comm_server->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;

		if (message->hash == gebr_comm_protocol_defs.err_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;
			g_string_assign(server->last_error, ((GString *)g_list_nth_data(arguments, 0))->str);
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Server '%s' reported the error '%s'."),
				     comm_server->address->str, server->last_error->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);

		} else if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			if (comm_server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.ini_def.code_hash) {
				GList *arguments;
				GString *hostname;
				GString *display_port;
				gchar ** accounts;
				gchar ** queues;
				GString *model_name;
				GString *total_memory;
				GString *nfsid;
				GString *ncores;
				GString *clock_cpu;
				GtkTreeIter iter;

				/* organize message data */
				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 10)) == NULL)
					goto err;
				hostname = g_list_nth_data(arguments, 0);
				display_port = g_list_nth_data(arguments, 1);
				queues = g_strsplit(((GString *)g_list_nth_data(arguments, 2))->str, ",", 0);
				server->type = gebr_comm_server_get_id(((GString*)g_list_nth_data(arguments, 3))->str);
				accounts = g_strsplit(((GString *)g_list_nth_data(arguments, 4))->str, ",", 0);
				model_name = g_list_nth_data (arguments, 5);
				total_memory = g_list_nth_data (arguments, 6);
				nfsid = g_list_nth_data (arguments, 7);
				ncores = g_list_nth_data (arguments, 8);
				clock_cpu = g_list_nth_data (arguments, 9);

				g_string_assign (server->nfsid, nfsid->str);
				server->ncores = atoi(ncores->str);
				server->clock_cpu = atof(clock_cpu->str);

				gtk_list_store_set (gebr.ui_server_list->common.store, &server->iter,
						    SERVER_CPU, model_name->str,
						    SERVER_MEM, total_memory->str,
						    -1);

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
							   SERVER_QUEUE_LAST_RUNNING_JOB, NULL,
							   -1);
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
					}
				}

				gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);

				g_strfreev(accounts);
				g_strfreev(queues);


				/* say we are logged */
				g_string_assign(server->last_error, "");
				comm_server->socket->protocol->logged = TRUE;
				server_list_updated_status(server);
				g_string_assign(comm_server->socket->protocol->hostname, hostname->str);
				if (gebr_comm_server_is_local(comm_server) == TRUE)
					gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Connected to the local server."),
						     comm_server->address->str);
				else {
					gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Connected to the server '%s'."),
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
				gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, FALSE,
								      gebr_comm_protocol_defs.lst_def, 0);
				/* set out nickname */
				/** [Json test protocol]
				 * Our server communicates with the client using a http-like protocol.
				 * The variable "req" is a http-request, that gets the data via "GET"
				 * (much like an html form).
				 *
				 * Then, we connect an callback on the request (req), this callback
				 * is activated when the client recieves a server response.
				 *
				 * For this communication between server and client we use a json
				 * serialized object.
				 */
				GebrCommHttpMsg *req = gebr_comm_protocol_socket_send_request(comm_server->socket,
											      GEBR_COMM_HTTP_METHOD_GET,
											      "/fs-nickname", NULL);
				g_signal_connect(req, "response-received", G_CALLBACK(on_fs_nickname_msg), server);


				gebr_comm_protocol_socket_oldmsg_split_free(arguments);

				// Emits the GebrServer::initialized signal
				gebr_server_emit_initialized (server);

			} else if (comm_server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.run_def.code_hash) {
				GList *arguments;
				GString *jid;
				GString *run_id;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
					goto err;

				jid = g_list_nth_data(arguments, 0);
				run_id = g_list_nth_data(arguments, 1);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			} else if (comm_server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.flw_def.code_hash) {

			}
		} else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;
			GString *jid, *hostname, *status, *title, *start_date, *finish_date, *issues, *cmd_line,
				*output, *queue, *moab_jid, *run_id, *frac, *server_list;
			GebrJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 14)) == NULL)
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
			run_id = g_list_nth_data(arguments, 11);
			frac = g_list_nth_data(arguments, 12);
			server_list = g_list_nth_data(arguments, 13);

			g_debug("JOB_DEF: Received task %s frac %s status %s", run_id->str, frac->str, status->str);
			GebrTask *task = gebr_task_new(server, run_id->str, frac->str);
			gebr_task_init_details(task, status, start_date, finish_date, issues, cmd_line, queue, moab_jid);

			job = gebr_job_find(run_id->str);

			g_debug("Job found is: %p", job);

			if (!job) {
				gchar **servers = g_strsplit(server_list->str, ",", 0);
				gint length = 0;

				while (servers[length])
					length++;

				gint cmpfun(gconstpointer a, gconstpointer b) {
					const gchar *aa = *(gchar * const *)a;
					const gchar *bb = *(gchar * const *)b;

					g_debug("Comparing %s to %s", aa, bb);
					if (g_strcmp0(aa, "127.0.0.1") == 0)
						return -1;

					if (g_strcmp0(bb, "127.0.0.1") == 0)
						return 1;

					return g_strcmp0(aa, bb);
				}

				qsort(servers, length, sizeof(servers[0]), cmpfun);
				gchar *servers_str = g_strjoinv(", ", servers);

				job = gebr_job_new_with_id(gebr.ui_job_control->store, run_id->str, queue->str, servers_str);
				gebr_job_set_title(job, title->str);
				gebr_job_show(job);
				g_free(servers_str);
			}

			gebr_job_append_task(job, task);
			gebr_task_emit_output_signal(task, output->str);
			gebr_task_emit_status_changed_signal(task, job_translate_status(status), "");
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *jid, *output, *rid, *frac;
			GebrTask *task;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);
			rid = g_list_nth_data(arguments, 2);
			frac = g_list_nth_data(arguments, 3);

			task = gebr_task_find(rid->str, frac->str);
			g_debug("OUT_DEF: Found task %p rid %s frac %s output %s", task, rid->str, frac->str, output->str);

			if (task != NULL) {
				gebr_task_emit_output_signal(task, output->str);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;
			GString *jid, *status, *parameter, *rid, *frac;
			GebrTask *task;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 5)) == NULL)
				goto err;

			jid = g_list_nth_data(arguments, 0);
			status = g_list_nth_data(arguments, 1);
			parameter = g_list_nth_data(arguments, 2);
			rid = g_list_nth_data(arguments, 3);
			frac = g_list_nth_data(arguments, 4);

			g_debug("STA_DEF: Task %s frac %s status %s param %s", rid->str, frac->str, status->str, parameter->str);

			task = gebr_task_find(rid->str, frac->str);

			if (task != NULL) {
				enum JobStatus status_enum;

				status_enum = job_translate_status(status);
				gebr_task_emit_status_changed_signal(task, status_enum, parameter->str);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	}

	return TRUE;

err:	gebr_comm_message_free(message);
	comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	if (gebr_comm_server_is_local(comm_server) == TRUE)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with the local server. Please reconnect."));
	else
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with the server '%s'. Please reconnect."),
			     comm_server->address->str);
	gebr_comm_server_disconnect(comm_server);

	return FALSE;
}
