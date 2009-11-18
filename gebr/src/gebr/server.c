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

#include <string.h>

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/comm/gstreamsocket.h>
#include <libgebr/gui/utils.h>

#include "server.h"
#include "gebr.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

/*
 * Internal functions
 */

static void
server_log_message(enum gebr_log_message_type type, const gchar * message)
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
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
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

/**
 * server_new:
 * @address: The server address.
 * @autoconnect: The server will connect whenever GêBR starts.
 *
 * Creates a new server.
 *
 * Returns: A server structure.
 */
struct server *
server_new(const gchar * address, gboolean autoconnect)
{
	static const struct gebr_comm_server_ops	ops = {
		.log_message = server_log_message,
		.ssh_login = server_ssh_login,
		.ssh_question = server_ssh_question,
		.parse_messages = (typeof(ops.parse_messages))client_parse_server_messages
	};
	GtkTreeIter				iter;
	struct server *				server;

	gtk_list_store_append(gebr.ui_server_list->common.store, &iter);
	server = g_malloc(sizeof(struct server));
	*server = (struct server){
		.gebr_comm = gebr_comm_server_new(address, &ops),
		.iter = iter,
		.last_error = g_string_new("")
	};
	server->gebr_comm->user_data = server;
	gtk_list_store_set(gebr.ui_server_list->common.store, &iter,
		SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
		SERVER_ADDRESS, !strcmp(address, "127.0.0.1") ? _("Local server") : address,
		SERVER_POINTER, server,
		SERVER_AUTOCONNECT, autoconnect,
		-1);

	g_signal_connect(server->gebr_comm->stream_socket, "disconnected",
		G_CALLBACK(server_disconnected), server);

	if (autoconnect)
		gebr_comm_server_connect(server->gebr_comm);

	return server;
}

/**
 * server_find_address:
 * @address: The server to be found.
 * @iter: A #GtkTreeIter that will hold the corresponding iterator.
 *
 * Searches for the @address server and fill @iter with the correct
 * iterator for the gebr.ui_server_list->common.store model.
 *
 * Returns: %TRUE if the server was found, %FALSE otherwise.
 */
gboolean
server_find_address(const gchar * address, GtkTreeIter * iter)
{
	GtkTreeIter	i;

	if (!address)
		return FALSE;

	gebr_gui_gtk_tree_model_foreach(i, GTK_TREE_MODEL(gebr.ui_server_list->common.store)) {
		gchar * addr;
		gtk_tree_model_get(
			GTK_TREE_MODEL(gebr.ui_server_list->common.store), &i,
			SERVER_ADDRESS, &addr,
			-1);
		if (!strcmp(address, addr)) {
			g_free(addr);
			*iter = i;
			return TRUE;
		}
		g_free(addr);
	}

	return FALSE;
}

void
server_free(struct server * server)
{
	GtkTreeIter	iter;
	gboolean	valid;

	gtk_list_store_remove(gebr.ui_server_list->common.store, &server->iter);

	/* delete all jobs at server */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);

		if (job->server == server)
			job_delete(job);
	}

	gebr_comm_server_free(server->gebr_comm);
	g_string_free(server->last_error, TRUE);
	g_free(server);
}
