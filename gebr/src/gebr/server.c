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
#include <libgebr/comm/streamsocket.h>
#include <libgebr/gui/utils.h>

#include "server.h"
#include "gebr.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

/**
 * \internal
 */
static void server_log_message(enum gebr_log_message_type type, const gchar * message)
{
	gebr_message(type, TRUE, TRUE, message);
}

/**
 * \internal
 */
static GString *server_ssh_login(const gchar * title, const gchar * message)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *entry;

	GString *password;

	dialog = gtk_dialog_new_with_buttons(title,
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
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

/**
 * \internal
 */
static gboolean server_ssh_question(const gchar * title, const gchar * message)
{
	GtkWidget *dialog;
	gboolean yes;

	dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
					(GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
					GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("SSH inquiry"));

	yes = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) ? TRUE : FALSE;
	gtk_widget_destroy(dialog);

	return yes;
}

/**
 * \internal
 */
static void server_clear_jobs(struct server * server)
{
	gboolean server_free_foreach_job(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		struct job *job;
		gchar *server_address;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_SERVER_ADDRESS, &server_address,
				   JC_STRUCT, &job, JC_IS_JOB, &is_job, -1);
		if (strcmp(server_address, server->comm->address->str)) {
			g_free(server_address);
			return FALSE;	
		}
		if (!is_job)
			gtk_tree_store_remove(gebr.ui_job_control->store, iter);
		else if (job != NULL)
			job_delete(job);

		g_free(server_address);
		return FALSE;
	}
	/* delete all jobs at server */
	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)server_free_foreach_job, NULL); 
}

/**
 * \internal
 */
static void server_disconnected(GebrCommStreamSocket * stream_socket, struct server *server)
{
	server_list_updated_status(server);
	gtk_list_store_clear(server->accounts_model);
	gtk_list_store_clear(server->queues_model);
	server_clear_jobs(server);
}

gboolean server_find_address(const gchar * address, GtkTreeIter * iter)
{
	GtkTreeIter i;

	gebr_gui_gtk_tree_model_foreach(i, GTK_TREE_MODEL(gebr.ui_server_list->common.store)) {
		struct server *server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &i, SERVER_POINTER, &server, -1);
		if (!strcmp(address, server->comm->address->str)) {
			*iter = i;
			return TRUE;
		}
	}

	return FALSE;
}

const gchar *server_get_name_from_address(const gchar * address)
{
	return !strcmp(address, "127.0.0.1") ? _("Local server") : address;
}

struct server *server_new(const gchar * address, gboolean autoconnect)
{
	static const struct gebr_comm_server_ops ops = {
		.log_message = server_log_message,
		.ssh_login = server_ssh_login,
		.ssh_question = server_ssh_question,
		.parse_messages = (typeof(ops.parse_messages)) client_parse_server_messages
	};
	GtkTreeIter iter;
	struct server *server;

	gtk_list_store_append(gebr.ui_server_list->common.store, &iter);
	server = g_new(struct server, 1);
	*server = (struct server) {
		.comm = gebr_comm_server_new(address, &ops),
		.iter = iter,
		.last_error = g_string_new(""),
		.type = GEBR_COMM_SERVER_TYPE_UNKNOWN,
		.accounts_model = gtk_list_store_new(1, G_TYPE_STRING),
		.queues_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER),
		.ran_jid = g_string_new(""),
	};

	server->comm->user_data = server;
	gtk_list_store_set(gebr.ui_server_list->common.store, &iter,
			   SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
			   SERVER_NAME, server_get_name_from_address(address),
			   SERVER_POINTER, server, SERVER_AUTOCONNECT, autoconnect, -1);

	g_signal_connect(server->comm->stream_socket, "disconnected", G_CALLBACK(server_disconnected), server);

	if (autoconnect)
		gebr_comm_server_connect(server->comm);

	return server;
}

gboolean server_find(struct server * server, GtkTreeIter * iter)
{
	GtkTreeIter i;

	gebr_gui_gtk_tree_model_foreach(i, GTK_TREE_MODEL(gebr.ui_server_list->common.store)) {
		struct server *i_server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &i,
				   SERVER_POINTER, &i_server, -1);
		if (i_server == server) {
			*iter = i;
			return TRUE;
		}
	}

	return FALSE;
}

void server_free(struct server *server)
{
	server_clear_jobs(server);

	gtk_list_store_remove(gebr.ui_server_list->common.store, &server->iter);
	gtk_list_store_clear(server->accounts_model);
	gtk_list_store_clear(server->queues_model);

	gebr_comm_server_free(server->comm);
	g_string_free(server->last_error, TRUE);
	g_string_free(server->ran_jid, TRUE);
	g_free(server);
}

gboolean server_queue_find(struct server * server, const gchar * name, GtkTreeIter * _iter)
{
	GtkTreeIter iter;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(server->queues_model)) {
		gchar * i_name;

		gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, 1, &i_name, -1);
		if (strcmp(name, i_name) == 0) {
			if (_iter != NULL)
				*_iter = iter;
			g_free(i_name);
			return TRUE;
		}

		g_free(i_name);
	}

	return FALSE;
}

void server_queue_find_at_job_control(struct server * server, const gchar * name, GtkTreeIter * _iter)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(gebr.ui_job_control->store)) {
		gchar *i_name;
		gchar *i_address;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				   JC_SERVER_ADDRESS, &i_address,
				   JC_QUEUE_NAME, &i_name,
				   JC_IS_JOB, &is_job,
				   -1);
		if (is_job)
			continue;
		if (strcmp(server->comm->address->str, i_address))
			continue;
		if (server->type == GEBR_COMM_SERVER_TYPE_REGULAR) {
		    if (!((name[0] == 'j' && i_name[0] == 'j') || !strcmp(name, i_name)))
		    	continue;
		} else if (strcmp(name, i_name))
			continue;
		
		if (_iter != NULL)
			*_iter = iter;
		g_free(i_name);
		g_free(i_address);
		return;
	}

	GString *title = g_string_new(NULL);

	g_string_printf(title, "%s at %s",
			(server->type == GEBR_COMM_SERVER_TYPE_MOAB) ? name : (name[0] == 'j') ? _("Immediately") : name + 1,
			server_get_name_from_address(server->comm->address->str));
	gtk_tree_store_append(gebr.ui_job_control->store, &iter, NULL);
	if (_iter != NULL)
		*_iter = iter;
	gtk_tree_store_set(gebr.ui_job_control->store, &iter,
			   JC_SERVER_ADDRESS, server->comm->address->str,
			   JC_QUEUE_NAME, name,
			   JC_TITLE, title->str,
			   JC_STRUCT, NULL,
			   JC_IS_JOB, FALSE,
			   -1);

	g_string_free(title, TRUE);
}

