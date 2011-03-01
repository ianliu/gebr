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
#include "job.h"

void client_process_server_request(struct gebr_comm_server *comm_server, GebrCommHttpMsg *request, GebrServer *server)
{
}

void client_process_server_response(struct gebr_comm_server *comm_server, GebrCommHttpMsg *request,
				   GebrCommHttpMsg *response, GebrServer *server)
{
}

void on_fs_nickname_msg(GebrCommHttpMsg *request, GebrCommHttpMsg *response, GebrServer *server)
{
	GString *dialog(void)
       	{
		GString *nickname = g_string_new("");
		GtkWidget *dialog = gtk_dialog_new_with_buttons(_("File-system label"), GTK_WINDOW(gebr.window),
								(GtkDialogFlags)(GTK_DIALOG_MODAL |
										 GTK_DIALOG_DESTROY_WITH_PARENT),
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							       	GTK_STOCK_OK, GTK_RESPONSE_OK,
							       	NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

		GtkWidget *label = gtk_label_new(_("This server is at a new file system.\n"
						   "You can label this file system\n"
						   "so you can group them."));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

		GtkWidget *entry = gtk_entry_new();
		gchar *def_nickname = g_strdup_printf(_("File system of %s"), server->comm->address->str);
		gtk_entry_set_text(GTK_ENTRY(entry), def_nickname);
		g_free(def_nickname);
		gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);

		gtk_widget_show_all(dialog);
		gboolean confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;
		g_string_assign(nickname, !confirmed ? "" : gtk_entry_get_text(GTK_ENTRY(entry)));
		gtk_widget_destroy(dialog);

		return nickname;
	}

	if (request->method == GEBR_COMM_HTTP_METHOD_GET) {
		GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
		GString *value = gebr_comm_json_content_to_gstring(json);

		if (!value->len) {
			GString *nickname = dialog();
			GebrCommJsonContent *json = gebr_comm_json_content_new_from_string(nickname->str);
			GebrCommHttpMsg *req = gebr_comm_protocol_socket_send_request(server->comm->socket,
										      GEBR_COMM_HTTP_METHOD_PUT,
										      "/fs-nickname", json);
			g_object_set_data(G_OBJECT(req), "value", g_strdup(nickname->str));
			g_signal_connect(req, "response-received", G_CALLBACK(on_fs_nickname_msg), server);
			g_string_free(nickname, TRUE);
			gebr_comm_json_content_free(json);
		} else {
			gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter, SERVER_FS, value->str, -1);
			ui_server_update_tags_combobox ();
		}

		g_string_free(value, TRUE);
		gebr_comm_json_content_free(json);
	} else if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
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
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Server '%s' reported error '%s'."),
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
				GtkTreeIter iter;

				/* organize message data */
				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 8)) == NULL)
					goto err;
				hostname = g_list_nth_data(arguments, 0);
				display_port = g_list_nth_data(arguments, 1);
				queues = g_strsplit(((GString *)g_list_nth_data(arguments, 2))->str, ",", 0);
				server->type = gebr_comm_server_get_id(((GString*)g_list_nth_data(arguments, 3))->str);
				accounts = g_strsplit(((GString *)g_list_nth_data(arguments, 4))->str, ",", 0);
				model_name = g_list_nth_data (arguments, 5);
				total_memory = g_list_nth_data (arguments, 6);
				nfsid = g_list_nth_data (arguments, 7);

				g_string_assign (server->nfsid, nfsid->str);

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

						/* at job control */
						server_queue_find_at_job_control(server, queues[i], NULL);
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
				gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, FALSE,
								      gebr_comm_protocol_defs.lst_def, 0);
				/* set out nickname */
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

				/* organize message data */
				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
					goto err;
				jid = g_list_nth_data(arguments, 0);
				run_id = g_list_nth_data(arguments, 1);
				GebrJob * job = job_find(comm_server->address, run_id, FALSE);
				if (job != NULL) {
					g_string_assign(job->parent.jid, jid->str);

					/* move it to the end, the right place... */
					gboolean was_selected = job_is_active(job);
					gebr_gui_gtk_tree_store_move_before(gebr.ui_job_control->store, &job->iter, NULL);
					if (was_selected)
						job_set_active(job);
				}

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			} else if (comm_server->socket->protocol->waiting_ret_hash == gebr_comm_protocol_defs.flw_def.code_hash) {

			}
		} else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;
			GString *jid, *hostname, *status, *title, *start_date, *finish_date, *issues, *cmd_line,
				*output, *queue, *moab_jid;
			GebrJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 11)) == NULL)
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
			else if (job->parent.status == JOB_STATUS_INITIAL)
				job_init_details(job, status, title, start_date, finish_date,
						 hostname, issues, cmd_line, output, queue, moab_jid);
			else
				gebr_message(GEBR_LOG_DEBUG, FALSE, FALSE, _("Received already listed job %s."), job->parent.jid);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;
			GString *jid, *output;
			GebrJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;
			jid = g_list_nth_data(arguments, 0);
			output = g_list_nth_data(arguments, 1);

			job = job_find(comm_server->address, jid, TRUE);
			if (job != NULL) {
				job_append_output(job, output);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		} else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;
			GString *jid, *status, *parameter;
			GebrJob *job;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
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

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	}

	return TRUE;

err:	gebr_comm_message_free(message);
	comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	if (gebr_comm_server_is_local(comm_server) == TRUE)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with local server. Please reconnect."));
	else
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Error communicating with server '%s'. Please reconnect."),
			     comm_server->address->str);
	gebr_comm_server_disconnect(comm_server);

	return FALSE;
}
