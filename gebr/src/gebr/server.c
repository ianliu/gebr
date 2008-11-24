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

#include <string.h>

#include <gtk/gtk.h>

#include <comm/gstreamsocket.h>

#include "server.h"
#include "gebr.h"
#include "support.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

/*
 * Internal functions
 */

static void
server_log_message(enum log_message_type type, const gchar * message)
{
	gebr_message(type, TRUE, TRUE, message);
}

static GString *
server_ssh_login(const gchar * title, const gchar * message)
{
	GtkWidget *	dialog;
	GtkWidget *	label;
	GtkWidget *	entry;

	GString *	password;

	dialog = gtk_dialog_new_with_buttons(title,
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		password = NULL;
	else
		password = g_string_new(gtk_entry_get_text(GTK_ENTRY(entry)));

	gtk_widget_destroy(dialog);

	return password;
}

static gboolean
server_ssh_question(const gchar * title, const gchar * message)
{
	GtkWidget *	dialog;
	gboolean	yes;

	dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("SSH question:"));

	yes = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) ? TRUE : FALSE;
	gtk_widget_destroy(dialog);

	return yes;
}

static void
server_disconnected(GStreamSocket * stream_socket, struct server * server)
{
	server_list_updated_status(server);
}

/*
 * Public functions
 */

struct server *
server_new(const gchar * address)
{
	static const struct comm_server_ops	ops = {
		.log_message = server_log_message,
		.ssh_login = server_ssh_login,
		.ssh_question = server_ssh_question,
		.parse_messages = (typeof(ops.parse_messages))client_parse_server_messages
	};
	GtkTreeIter				iter;
	struct server *				server;

	server = g_malloc(sizeof(struct server));
	server->comm = comm_server_new(address, &ops);
	server->comm->user_data = server;
	/* fill iter */
	gtk_list_store_append(gebr.ui_server_list->common.store, &iter);
	gtk_list_store_set(gebr.ui_server_list->common.store, &iter,
		SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
		SERVER_ADDRESS, !strcmp(address, "127.0.0.1") ? _("Local server") : address,
		SERVER_POINTER, server,
		-1);
	server->iter = iter;

	g_signal_connect(server->comm->stream_socket, "disconnected",
		G_CALLBACK(server_disconnected), server);

	comm_server_connect(server->comm);

	return server;
}

void
server_free(struct server * server)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* delete all jobs at server */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);

		if (job->server == server)
			job_delete(job);
	}

	comm_server_free(server->comm);
	g_free(server);
}
