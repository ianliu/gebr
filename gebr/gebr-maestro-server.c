/*
 * gebr-maestro-server.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Core team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */


#include "gebr-maestro-server.h"

#include "gebr-marshal.h"
#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gebr-maestro-info.h>
#include <libgebr/gebr-maestro-settings.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <unistd.h>

#include "gebr.h" // for gebr_get_session_id()
#include "project.h" // for populate project/lines

struct MaestroInfoIface {
	GebrMaestroInfo iface;
	GebrMaestroServer *maestro;
};

struct _GebrMaestroServerPriv {
	GebrCommServer *server;
	GtkListStore *store;
	GtkTreeModel *filter;
	GHashTable *jobs;
	GHashTable *temp_jobs;
	gchar *address;
	gchar *error_type;
	gchar *error_msg;
	GtkWindow *window;
	gchar *home;
	gchar *nfsid;
	gchar *nfs_label;
	gint clocks_diff;
	gboolean wizard_setup;

	/* GVFS */
	gboolean has_connected_daemon;
	GFile *mount_location;
	GebrCommPortForward *forward;

	GtkListStore *groups_store;
	GtkListStore *queues_model;
	struct MaestroInfoIface maestro_info_iface;
};

enum {
	JOB_DEFINE,
	GROUP_CHANGED,
	QUESTION_REQUEST,
	PASSWORD_REQUEST,
	DAEMONS_CHANGED,
	STATE_CHANGE,
	AC_CHANGE,
	MPI_CHANGED,
	DAEMON_ERROR,
	MAESTRO_ERROR,
	CONFIRM,
	PATH_ERROR,
	GVFS_MOUNT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_NFSID,
	PROP_HOME,
};

static void mount_enclosing_ready_cb(GFile *location, GAsyncResult *res,
				     GebrMaestroServer *maestro);

static void log_message(GebrCommServer *server, GebrLogMessageType type,
			     const gchar *message, gpointer user_data);

static void state_changed(GebrCommServer *comm_server,
			       gpointer user_data);

static void process_request(GebrCommServer *server, GebrCommHttpMsg *request,
			    gpointer user_data);

static void process_response(GebrCommServer *server, GebrCommHttpMsg *request,
			     GebrCommHttpMsg *response, gpointer user_data);

static void parse_messages(GebrCommServer *comm_server, gpointer user_data);

static void gebr_maestro_server_connectable_init(GebrConnectableIface *iface);

static gchar *gebr_maestro_server_get_user(GebrMaestroServer *maestro);

void gebr_maestro_server_set_window(GebrMaestroServer *maestro, GtkWindow *window);

static void gebr_maestro_server_set_home_dir(GebrMaestroServer *maestro,
					     const gchar *home);

static gchar *gebr_maestro_server_get_home_mount_point(GebrMaestroInfo *iface);

static gchar *gebr_maestro_server_get_home_uri(GebrMaestroInfo *iface);

static void gebr_maestro_server_set_nfs_label_for_jobs(GebrMaestroServer *maestro);

static const struct gebr_comm_server_ops maestro_ops = {
	.log_message      = log_message,
	.state_changed    = state_changed,
	.process_request  = process_request,
	.process_response = process_response,
	.parse_messages   = parse_messages
};

G_DEFINE_TYPE_WITH_CODE(GebrMaestroServer, gebr_maestro_server, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(GEBR_TYPE_CONNECTABLE,
					      gebr_maestro_server_connectable_init));

static void
on_sftp_port_defined(GebrCommPortProvider *self,
		     guint port,
		     GebrMaestroServer *maestro)
{
	maestro->priv->forward = gebr_comm_port_provider_get_forward(self);

	gchar *uri;
	gchar *user = gebr_maestro_server_get_user(maestro);
	if (!*user)
		uri = g_strdup_printf("sftp://localhost:%d", port);
	else
		uri = g_strdup_printf("sftp://%s@localhost:%d", user, port);

	maestro->priv->mount_location = g_file_new_for_commandline_arg(uri);

	g_free(uri);
	g_free(user);

	gebr_add_remove_ssh_key(FALSE);

	GtkWindow *window = NULL;
	GList *windows = gtk_window_list_toplevels();
	for (GList *i = windows; i; i = i->next) {
		GtkWindow *win = i->data;
		if (gtk_window_is_active(win)) {
			window = win;
			g_object_ref(window);
			break;
		}
	}

	if (!window)
		window = GTK_WINDOW(gebr.window);

	GMountOperation *op = gtk_mount_operation_new(window);
	g_file_mount_enclosing_volume(maestro->priv->mount_location, 0, op, NULL,
				      (GAsyncReadyCallback) mount_enclosing_ready_cb,
				      maestro);

	g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_PROGRESS);

	g_object_unref(window);
	g_list_free(windows);
}

static void
on_sftp_port_error(GebrCommPortProvider *self,
		   GError *error,
		   GebrMaestroServer *maestro)
{
	if (error->code == GEBR_COMM_PORT_PROVIDER_ERROR_SFTP_NOT_REQUIRED)
		g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_OK);
}

gboolean
gebr_maestro_server_need_mount_gvfs (GebrMaestroServer *maestro)
{
	if (maestro->priv->has_connected_daemon)
		return FALSE;

	const gchar *client_nfsid = gebr_get_client_nfsid();

	if (!client_nfsid)
		return TRUE;

	const gchar *daemon_nfsid = gebr_maestro_server_get_nfsid(maestro);

	if (g_strcmp0(client_nfsid, daemon_nfsid) == 0)
		return FALSE;

	return TRUE;
}

void
gebr_maestro_server_mount_gvfs(GebrMaestroServer *maestro, const gchar *addr)
{
	if (maestro->priv->has_connected_daemon)
		return;

	maestro->priv->has_connected_daemon = TRUE;

	GebrCommPortProvider *port_provider =
		gebr_comm_server_create_port_provider(maestro->priv->server, GEBR_COMM_PORT_TYPE_SFTP);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_sftp_port_defined), maestro);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_sftp_port_error), maestro);
	g_debug("on %s, address:'%s'", __func__, addr);
	gebr_comm_port_provider_set_sftp_address(port_provider, addr);
	gebr_comm_port_provider_start(port_provider);
}

static void
unmount_gvfs(GebrMaestroServer *maestro,
             gboolean quit)
{
	if (!maestro->priv->has_connected_daemon) {
		if (quit)
			gebr_quit(FALSE);
		return;
	}

	if (maestro->priv->forward)
		gebr_comm_port_forward_close(maestro->priv->forward);

	maestro->priv->has_connected_daemon = FALSE;

	if (!quit)
		g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_NOK);

	if (maestro->priv->mount_location) {
		g_object_unref(maestro->priv->mount_location);
		maestro->priv->mount_location = NULL;
	}

	if (quit)
		gebr_quit(FALSE);
}

static void
update_groups_store(GebrMaestroServer *maestro)
{
	GtkTreeIter iter; 

	gtk_list_store_clear(maestro->priv->groups_store);

	gtk_list_store_append(maestro->priv->groups_store, &iter);
	gtk_list_store_set(maestro->priv->groups_store, &iter,
	                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_GROUP,
	                   MAESTRO_SERVER_NAME, "",
	                   -1);

	GList *groups = gebr_maestro_server_get_all_tags(maestro);

	for (GList *g = groups; g; g = g->next) {
		const gchar *tag = g->data;

		gtk_list_store_append(maestro->priv->groups_store, &iter);
		gtk_list_store_set(maestro->priv->groups_store, &iter,
		                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_GROUP,
		                   MAESTRO_SERVER_NAME, tag,
		                   -1);
	}

	GtkTreeIter daemons_iter;
	GtkTreeModel *daemons_model = gebr_maestro_server_get_model(maestro, FALSE, NULL);

	gebr_gui_gtk_tree_model_foreach(daemons_iter, daemons_model) {
		GebrDaemonServer *daemon;

		gtk_tree_model_get(daemons_model, &daemons_iter,
		                   0, &daemon, -1);

		gtk_list_store_append(maestro->priv->groups_store, &iter);
		gtk_list_store_set(maestro->priv->groups_store, &iter,
		                   MAESTRO_SERVER_TYPE, MAESTRO_SERVER_TYPE_DAEMON,
		                   MAESTRO_SERVER_NAME, gebr_daemon_server_get_address(daemon),
		                   MAESTRO_SERVER_HOST, gebr_daemon_server_get_hostname(daemon),
		                   -1);
	}
	g_object_unref(daemons_model);
}

void
log_message(GebrCommServer *server,
	    GebrLogMessageType type,
	    const gchar *message,
	    gpointer user_data)
{
}

void
gebr_maestro_server_set_error(GebrMaestroServer *maestro,
			      const gchar *error_type,
			      const gchar *error_msg)
{
	gchar *tmp = g_strdup(error_type);
	if (maestro->priv->error_type)
		g_free(maestro->priv->error_type);
	maestro->priv->error_type = tmp;

	tmp = g_strdup(error_msg);
	if (maestro->priv->error_msg)
		g_free(maestro->priv->error_msg);
	maestro->priv->error_msg = tmp;

	if (maestro->priv->server && g_strcmp0(error_type, "error:protocol") == 0) {
		gebr_comm_server_set_last_error(maestro->priv->server,
						SERVER_ERROR_PROTOCOL_VERSION,
						"Protocol version mismatch");
	}
}

void
gebr_maestro_server_get_error(GebrMaestroServer *maestro,
			      const gchar **error_type,
			      const gchar **error_msg)
{
	if (!maestro || !error_type || !error_msg)
		return;

	*error_type = maestro->priv->error_type;

	*error_msg = maestro->priv->error_msg;
}

void
state_changed(GebrCommServer *comm_server,
	      gpointer user_data)
{
	GebrMaestroServer *maestro = user_data;
	GebrCommServerState state = gebr_comm_server_get_state(comm_server);

	if (state == SERVER_STATE_DISCONNECTED) {
		gtk_list_store_clear(maestro->priv->groups_store);

		if (!gebr.quit)
			gebr_project_line_show(gebr.ui_project_line);

		const gchar *err = gebr_comm_server_get_last_error(maestro->priv->server);
                if (err && *err) {
                	if (comm_server->error == SERVER_ERROR_CONNECT)
                		gebr_maestro_server_connect(maestro, TRUE);
                        gebr_maestro_server_set_error(maestro, "error:ssh", err);
                } else {
                        gebr_maestro_server_set_error(maestro, "error:none", NULL);
                }
	}
	else if (state == SERVER_STATE_CONNECT) {
		if (g_strcmp0(maestro->priv->server->address->str, maestro->priv->address) != 0) {
			g_free(maestro->priv->address);
			maestro->priv->address = g_strdup(maestro->priv->server->address->str);
		}
	}
	else if (state == SERVER_STATE_LOGGED) {
		gebr_maestro_server_set_error(maestro, "error:none", NULL);
		gebr_maestro_controller_clean_potential_maestros(gebr.maestro_controller);

		gebr_project_line_show(gebr.ui_project_line);

		if (!gebr.populate_list) {
			gebr.populate_list = TRUE;
			project_list_populate();
		}

		if (!gebr.restore_selection) {
			gebr.restore_selection = TRUE;
			restore_project_line_flow_selection();
		}
	}

	const gchar *error_type = maestro->priv->error_type;
	const gchar *error_msg = maestro->priv->error_msg;

	if (state == SERVER_STATE_LOGGED
	    || state == SERVER_STATE_DISCONNECTED)
		g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

	g_signal_emit(maestro, signals[MAESTRO_ERROR], 0,
		      maestro->priv->address, error_type, error_msg);
	g_signal_emit(maestro, signals[STATE_CHANGE], 0);
}

void
process_request(GebrCommServer *server,
		GebrCommHttpMsg *request,
		gpointer user_data)
{
	g_debug("[MAESTRO] request");
}

void
process_response(GebrCommServer *server,
		 GebrCommHttpMsg *request,
		 GebrCommHttpMsg *response,
		 gpointer user_data)
{
}

static GebrDaemonServer *
get_daemon_from_address(GebrMaestroServer *server,
			const gchar *address,
			GtkTreeIter *_iter)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(server->priv->store);

	if (!address)
		return NULL;

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		GebrDaemonServer *daemon;
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		if (g_strcmp0(address, gebr_daemon_server_get_address(daemon)) == 0) {
			if (_iter)
				*_iter = iter;
			return daemon;
		}
	}
	return NULL;
}

void
update_queues_model(GebrMaestroServer *maestro, GebrJob *job)
{
	GebrCommJobStatus stat = gebr_job_get_status(job);

	gboolean has_job = FALSE;
	GtkTreeIter iter, found;
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->queues_model);
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		GebrJob *j;
		gtk_tree_model_get(model, &iter, 0, &j, -1);
		if (j == job) {
			has_job = TRUE;
			found = iter;
			break;
		}
	}

	switch (stat) {
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_FAILED:
		if (has_job)
			gtk_list_store_remove(maestro->priv->queues_model, &found);
		break;
	case JOB_STATUS_RUNNING:
	case JOB_STATUS_QUEUED:
		if (!has_job) {
			gtk_list_store_append(maestro->priv->queues_model, &iter);
			gtk_list_store_set(maestro->priv->queues_model, &iter, 0, job, -1);
		}
		break;
	default:
		break;
	}
}

static void
mount_enclosing_ready_cb(GFile *location,
			 GAsyncResult *res,
			 GebrMaestroServer *maestro)
{
	char *uri;
	gboolean success;
	GError *error = NULL;

	uri = g_file_get_uri(location);
	success = g_file_mount_enclosing_volume_finish(location, res, &error);

	if (success || g_error_matches(error, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED)) {
		g_debug("Mounted %s!", uri);
		g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_OK);
	} else {
		g_debug("Not mounted %s :( (%d) %s", uri, error->code, error->message);
		maestro->priv->has_connected_daemon = FALSE;
		g_object_unref(maestro->priv->mount_location);
		maestro->priv->mount_location = NULL;
		g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_NOK);
	}
	gebr_add_remove_ssh_key(TRUE);
}

static gboolean
have_logged_daemon(GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *m = gebr_maestro_server_get_model(maestro, FALSE, NULL);
	gebr_gui_gtk_tree_model_foreach(iter, m) {
		GebrDaemonServer *daemon;
		gtk_tree_model_get(m, &iter, 0, &daemon, -1);
		if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
			return TRUE;
	}
	return FALSE;
}

static void
add_nfs_label(const gchar *key,
              GebrJob *job,
              GebrMaestroServer *maestro)
{
	const gchar *nfs_label = gebr_maestro_server_get_nfs_label(maestro);
	gebr_job_set_nfs_label(job, nfs_label);
}

static void
gebr_maestro_server_set_nfs_label_for_jobs(GebrMaestroServer *maestro)
{
	g_hash_table_foreach(maestro->priv->jobs, (GHFunc)add_nfs_label, maestro);
	gebr_job_control_update_servers_model(gebr.job_control);
}

static gboolean
check_client_is_in_the_same_nfs_as_daemons(GString *nfsid)
{
	gboolean same_nfs = FALSE;
	gchar *gebrd_lock_filename = g_strdup_printf("%s/.gebr/run/gebrd-fslock.run", g_get_home_dir());
	gchar *gebrd_nfsid = NULL;

	gchar *gebrd_bin_path = g_find_program_in_path("gebrd");
	gboolean lock_exists = g_access(gebrd_lock_filename, R_OK) == 0;
	gboolean read_ok = g_file_get_contents(gebrd_lock_filename, &gebrd_nfsid, NULL, NULL);
	gboolean has_gebrd = gebrd_bin_path ? TRUE : FALSE;

	if (has_gebrd && lock_exists && read_ok && g_strcmp0(gebrd_nfsid, nfsid->str) == 0)
		same_nfs = TRUE;

	g_free(gebrd_bin_path);
	g_free(gebrd_nfsid);

	return same_nfs;
}

void
parse_messages(GebrCommServer *comm_server,
	       gpointer user_data)
{
	GList *link;
	struct gebr_comm_message *message;

	GebrMaestroServer *maestro = user_data;

	while ((link = g_list_last(comm_server->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;
		if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			guint ret_hash = GPOINTER_TO_UINT(g_queue_pop_head(comm_server->socket->protocol->waiting_ret_hashs));
			if (ret_hash == gebr_comm_protocol_defs.ini_def.code_hash) {
				GList *arguments;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
					goto err;

				GString *clocks_diff = g_list_nth_data(arguments, 0);

				gebr_maestro_server_set_clocks_diff(maestro, atoi(clocks_diff->str));

				gebr_comm_server_set_logged(comm_server);

				gboolean use_key = gebr_comm_server_get_use_public_key(comm_server);
				if (use_key)
					gebr_comm_server_append_key(comm_server, gebr_maestro_server_append_key_finished, NULL);
				else
					gebr_maestro_server_connect_on_daemons(maestro);

				gebr_comm_server_forward_x11(maestro->priv->server);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			} else if (ret_hash == gebr_comm_protocol_defs.path_def.code_hash) {
				GList *arguments;
				GString *status_id;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
					goto err;

				status_id = g_list_nth_data(arguments, 1);

				gint ret_id = atoi(status_id->str);
				g_signal_emit(maestro, signals[PATH_ERROR], 0, ret_id);

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
		} else if (message->hash == gebr_comm_protocol_defs.err_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 4)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *prog = g_list_nth_data(arguments, 1);
			GString *type = g_list_nth_data(arguments, 2);
			GString *msg  = g_list_nth_data(arguments, 3);

			g_debug("Error from %s: %s reported an error of type %s : %s",
				prog->str, addr->str, type->str, msg->str);


			if (g_strcmp0(prog->str, "daemon") == 0) {
				g_signal_emit(maestro, signals[DAEMON_ERROR], 0,
					      addr->str, type->str, msg->str);
			} else {
				gebr_maestro_server_set_error(maestro, type->str, msg->str);
				g_signal_emit(maestro, signals[MAESTRO_ERROR], 0,
					      maestro->priv->address, type->str, msg->str);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.ssta_def.code_hash) {
			GList *arguments;
			GString *addr, *ssta, *ac, *hostname, *ncores, *cpu_clock, *cpu_model, *memory;
			const gchar *maestro_addr = gebr_maestro_server_get_address(maestro);

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 8)) == NULL)
				goto err;

			hostname =  g_list_nth_data(arguments, 0);
			addr =      g_list_nth_data(arguments, 1);
			ssta =      g_list_nth_data(arguments, 2);
			ac =        g_list_nth_data(arguments, 3);
			ncores =    g_list_nth_data(arguments, 4);
			cpu_clock = g_list_nth_data(arguments, 5);
			cpu_model = g_list_nth_data(arguments, 6);
			memory =    g_list_nth_data(arguments, 7);

			g_debug("Daemon state change (%s) %s", addr->str, ssta->str);

			GtkTreeIter iter;
			GebrCommServerState state = gebr_comm_server_state_from_string(ssta->str);
			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, &iter);

			if (!daemon) {
				daemon = gebr_daemon_server_new(GEBR_CONNECTABLE(maestro),
								addr->str, state, maestro_addr);
				gebr_maestro_server_add_daemon(maestro, daemon);
			} else {
				GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(maestro->priv->store), &iter);
				gebr_daemon_server_set_state(daemon, state);
				gtk_tree_model_row_changed(GTK_TREE_MODEL(maestro->priv->store), path, &iter);
				gtk_tree_path_free(path);
			}

			gebr_daemon_server_set_hostname(daemon, hostname->str);
			gboolean is_ac = g_strcmp0(ac->str, "on") == 0 ? TRUE : FALSE;

			gebr_daemon_server_set_ncores(daemon, atoi(ncores->str));
			gebr_daemon_server_set_cpu_clock(daemon, cpu_clock->str);
			gebr_daemon_server_set_cpu_model(daemon, cpu_model->str);
			gebr_daemon_server_set_memory(daemon, memory->str);

			if (maestro->priv->has_connected_daemon && !have_logged_daemon(maestro))
				unmount_gvfs(maestro, FALSE);

			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);
			g_signal_emit(maestro, signals[DAEMONS_CHANGED], 0);
			g_signal_emit(maestro, signals[AC_CHANGE], 0, is_ac, daemon);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.job_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 26)) == NULL)
				goto err;

			GString *id		    = g_list_nth_data(arguments, 0);
			GString *temp_id	    = g_list_nth_data(arguments, 1);
			GString *flow_id	    = g_list_nth_data(arguments, 2);
			GString *nprocs		    = g_list_nth_data(arguments, 3);
			GString *server_list	    = g_list_nth_data(arguments, 4);
			GString *hostname	    = g_list_nth_data(arguments, 5);
			GString *title		    = g_list_nth_data(arguments, 6);
			GString *job_counter	    = g_list_nth_data(arguments, 7);
			GString *description	    = g_list_nth_data(arguments, 8);
			GString *snapshot_title     = g_list_nth_data(arguments, 9);
			GString *snapshot_id	    = g_list_nth_data(arguments, 10);
			GString *parent_id	    = g_list_nth_data(arguments, 11);
			GString *nice		    = g_list_nth_data(arguments, 12);
			GString *input		    = g_list_nth_data(arguments, 13);
			GString *output		    = g_list_nth_data(arguments, 14);
			GString *error		    = g_list_nth_data(arguments, 15);
			GString *submit_date	    = g_list_nth_data(arguments, 16);
			GString *group		    = g_list_nth_data(arguments, 17);
			GString *group_type	    = g_list_nth_data(arguments, 18);
			GString *speed		    = g_list_nth_data(arguments, 19);
			GString *status		    = g_list_nth_data(arguments, 20);
			GString *start_date	    = g_list_nth_data(arguments, 21);
			GString *finish_date	    = g_list_nth_data(arguments, 22);
			GString *run_type	    = g_list_nth_data(arguments, 23);
			GString *mpi_owner	    = g_list_nth_data(arguments, 24);
			GString *mpi_flavor	    = g_list_nth_data(arguments, 25);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gboolean prev_exist = FALSE;

			if (!job) {
				job = g_hash_table_lookup(maestro->priv->temp_jobs, temp_id->str);
				if (job) {
					g_hash_table_remove(maestro->priv->temp_jobs, temp_id->str);
					gebr_job_set_runid(job, id->str);
					g_hash_table_insert(maestro->priv->jobs, g_strdup(id->str), job);
					prev_exist = TRUE;
				}
			}

			gboolean init = (job == NULL);

			if (!job)
				job = gebr_job_new_with_id(id->str, parent_id->str, run_type->str);

			/* These properties are computed after Maestro sent the
			 * tasks to its Daemons, so they can not be set before
			 * that. The interface should update this properties.
			 */
			gebr_job_set_servers(job, server_list->str);
			gebr_job_set_server_list(job, server_list->str);
			gebr_job_set_submit_date(job, submit_date->str);
			gebr_job_set_nprocs(job, nprocs->str);
			gebr_job_set_io(job, input->str, output->str, error->str);
			gebr_job_set_mpi_owner(job, mpi_owner->str);
			gebr_job_set_mpi_flavor(job, mpi_flavor->str);
			gebr_job_set_flow_id(job, flow_id->str);
			gebr_job_set_snapshot_title(job, snapshot_title->str);
			gebr_job_set_snapshot_id(job, snapshot_id->str);
			gebr_job_set_description(job, description->str);
			gebr_job_set_job_counter(job, job_counter->str);
			gebr_job_set_static_status(job, gebr_comm_job_get_status_from_string(status->str));

			if (g_strcmp0(gebr_job_get_queue(job), parent_id->str) != 0) {
				gebr_job_set_queue(job, parent_id->str);
				prev_exist = TRUE;
			}

			if (init) {
				/* Creates a job and populates some of its information.
				 * This condition happens when GeBR connects to Maestro.
				 */
				gebr_job_set_is_fake(job, FALSE);
				gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
				gebr_job_set_hostname(job, hostname->str);
				gebr_job_set_title(job, title->str);
				gebr_job_set_nice(job, nice->str);
				gebr_job_set_server_group(job, group->str);
				gebr_job_set_server_group_type(job, group_type->str);
				gebr_job_set_exec_speed(job, atof(speed->str));
				gebr_job_set_static_status(job, gebr_comm_job_get_status_from_string(status->str));

				if (start_date->len > 0)
					gebr_job_set_start_date(job, start_date->str);
				if (finish_date->len > 0)
					gebr_job_set_finish_date(job, finish_date->str);

				g_hash_table_insert(maestro->priv->jobs, g_strdup(id->str), job);
			}
			
			if (init || prev_exist)
				g_signal_emit(maestro, signals[JOB_DEFINE], 0, job);

			update_queues_model(maestro, job);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.iss_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *issues = g_list_nth_data(arguments, 1);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_set_issues(job, issues->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.cmd_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *cmd = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_set_cmd_line(job, atoi(frac->str) - 1, cmd->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.out_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *frac = g_list_nth_data(arguments, 1);
			GString *output = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_append_output(job, atoi(frac->str) - 1, output->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.sta_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);
			GString *status = g_list_nth_data(arguments, 1);
			GString *parameter = g_list_nth_data(arguments, 2);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);
			gebr_job_set_status(job, gebr_comm_job_get_status_from_string(status->str), parameter->str);

			update_queues_model(maestro, job);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.jcl_def.code_hash) {
			GList *arguments;

			/* organize message data */
			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *id = g_list_nth_data(arguments, 0);

			GebrJob *job = g_hash_table_lookup(maestro->priv->jobs, id->str);

			gebr_job_remove(job);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.agrp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *tags = g_list_nth_data(arguments, 1);
			gchar **tagsv = g_strsplit(tags->str, ",", -1);

			GtkTreeIter iter;
			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, &iter);
			gebr_daemon_server_set_tags(daemon, tagsv);

			GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(maestro->priv->store), &iter);
			gtk_tree_model_row_changed(GTK_TREE_MODEL(maestro->priv->store), path, &iter);
			gtk_tree_path_free(path);

			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

			g_strfreev(tagsv);
			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.qst_def.code_hash) {
			GList *arguments, *i;

			if ((i = arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *addr = i->data; i = i->next;
			GString *title = i->data; i = i->next;
			GString *question = i->data; i = i->next;

			gboolean response;

			g_signal_emit(maestro, signals[QUESTION_REQUEST], 0,
				      addr->str, title->str, question->str, &response);

			const gchar *resp = response ? "true" : "false";

			GebrCommUri *uri = gebr_comm_uri_new();
			gebr_comm_uri_set_prefix(uri, "/ssh-answer");
			gebr_comm_uri_add_param(uri, "address", addr->str);
			gebr_comm_uri_add_param(uri, "response", resp);
			gchar *url = gebr_comm_uri_to_string(uri);
			gebr_comm_uri_free(uri);

			gebr_comm_protocol_socket_send_request(comm_server->socket,
							       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
			g_free(url);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.pss_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *acpkey = g_list_nth_data(arguments, 1);
			GString *retry = g_list_nth_data(arguments, 2);

			PasswordKeys *pk;

			gboolean accepts_key = g_strcmp0(acpkey->str, "yes") == 0;
			gboolean retry_pass = g_strcmp0(retry->str, "yes") == 0;

			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, NULL);
			g_signal_emit(maestro, signals[PASSWORD_REQUEST], 0, daemon, accepts_key, retry_pass, &pk);

			if (pk) {
				GebrCommUri *uri = gebr_comm_uri_new();
				gebr_comm_uri_set_prefix(uri, "/set-password");
				gebr_comm_uri_add_param(uri, "address", addr->str);
				if (pk->password)
					gebr_comm_uri_add_param(uri, "pass", pk->password);
				gebr_comm_uri_add_param(uri, "haskey", pk->use_public_key? "yes" : "no");

				gchar *url = gebr_comm_uri_to_string(uri);
				gebr_comm_uri_free(uri);

				gebr_comm_protocol_socket_send_request(comm_server->socket,
								       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
				g_free(url);
			}

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.ac_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *ac = g_list_nth_data(arguments, 1);
			gboolean is_ac = g_strcmp0(ac->str, "on") == 0 ? TRUE : FALSE;

			GebrDaemonServer *daemon = get_daemon_from_address(maestro, addr->str, NULL);

			g_signal_emit(maestro, signals[AC_CHANGE], 0, is_ac, daemon);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.mpi_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *daemon_addr = g_list_nth_data(arguments, 0);
			GString *mpi_flavors = g_list_nth_data(arguments, 1);

			GebrDaemonServer *daemon = gebr_maestro_server_get_daemon(maestro, daemon_addr->str);
			if (!daemon)
				goto err;
			
			g_signal_emit(maestro, signals[MPI_CHANGED], 0, daemon, mpi_flavors->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.srm_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);

			g_debug("Removing server %s", addr->str);

			GtkTreeIter iter;
			GebrDaemonServer *daemon;
			GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

			gebr_gui_gtk_tree_model_foreach(iter, model) {
				gtk_tree_model_get(model, &iter, 0, &daemon, -1);
				if (g_strcmp0(addr->str, gebr_daemon_server_get_address(daemon)) == 0)
					gtk_list_store_remove(maestro->priv->store, &iter);
			}

			g_signal_emit(maestro, signals[DAEMONS_CHANGED], 0);
			g_signal_emit(maestro, signals[GROUP_CHANGED], 0);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.cfrm_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *addr = g_list_nth_data(arguments, 0);
			GString *type = g_list_nth_data(arguments, 1);

			g_signal_emit(maestro, signals[CONFIRM], 0, addr->str, type->str);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.home_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
				goto err;

			GString *home = g_list_nth_data(arguments, 0);

			g_debug("HOME: %s", home->str);

			gebr_maestro_server_set_home_dir(maestro, home->str);
			flow_browse_validate_io(gebr.ui_flow_browse);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.nfsid_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 3)) == NULL)
				goto err;

			GString *nfsid = g_list_nth_data(arguments, 0);
			GString *hosts = g_list_nth_data(arguments, 1);
			GString *label = g_list_nth_data(arguments, 2);

			g_debug("NFSID = %s / LABEL = %s", nfsid->str, label->str);

			/*
			 * Send NFS Label to Maestro
			 */

			gchar *nfslabel;
			if (label->len == 0) {
				nfslabel = gebr_maestro_settings_get_label_for_domain(gebr.config.maestro_set,
				                                                      nfsid->str,
				                                                      TRUE);
				gebr_maestro_server_send_nfs_label(maestro,
				                                   nfsid->str,
				                                   nfslabel);
			} else {
				nfslabel = g_strdup(label->str);
			}

			gebr_config_set_current_nfs_info(nfsid->str,
			                                 hosts->str,
			                                 nfslabel);

			gtk_label_set_markup(GTK_LABEL(gebr.ui_log->maestro_label), nfslabel);

			gebr_maestro_server_set_nfs_label(maestro, nfslabel);
			gebr_maestro_server_set_nfs_label_for_jobs(maestro);

			gebr_maestro_server_set_nfsid(maestro, nfsid->str);


			if (check_client_is_in_the_same_nfs_as_daemons(nfsid))
				gebr_maestro_controller_server_list_add(gebr.maestro_controller, g_get_host_name(), TRUE);

			// Mount SFTP if needed
			if (gebr_maestro_server_need_mount_gvfs (maestro) && !maestro->priv->wizard_setup) {
				gchar *addr;

				gchar *comma = strstr(hosts->str, ",");
				if (comma)
					addr = g_strndup(hosts->str, (comma - hosts->str)/sizeof(gchar));
				else
					addr = g_strdup(hosts->str);

				gebr_maestro_server_mount_gvfs(maestro, addr);

				g_free(addr);
			} else {
				g_signal_emit(maestro, signals[GVFS_MOUNT], 0, STATUS_MOUNT_OK);
			}

			g_free(nfslabel);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}

		gebr_comm_message_free(message);
		comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	}

	return;

err:
	gebr_comm_message_free(message);
	comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	gebr_comm_server_disconnect(comm_server);
}


static void
gebr_maestro_server_get(GObject    *object,
			guint       prop_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	switch (prop_id)
	{
	case PROP_NFSID:
		g_value_set_string(value, gebr_maestro_server_get_nfsid(maestro));
		break;
	case PROP_HOME:
		g_value_set_string(value, gebr_maestro_server_get_home_dir(maestro));
		break;
	case PROP_ADDRESS:
		g_value_set_string(value, maestro->priv->server->address->str);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_maestro_server_set(GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	switch (prop_id)
	{
	case PROP_NFSID:
		gebr_maestro_server_set_nfsid(maestro, g_value_get_string(value));
		break;
	case PROP_ADDRESS:
		maestro->priv->address = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_maestro_server_finalize(GObject *object)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(object);

	gebr_comm_server_free(maestro->priv->server);
	gtk_list_store_clear(maestro->priv->store);
	g_object_unref(maestro->priv->store);
	g_hash_table_unref(maestro->priv->jobs);
	g_hash_table_unref(maestro->priv->temp_jobs);
	g_free(maestro->priv->nfsid);
	g_free(maestro->priv->home);
	unmount_gvfs(maestro, FALSE);

	G_OBJECT_CLASS(gebr_maestro_server_parent_class)->finalize(object);
}

static void
gebr_maestro_server_group_changed_real(GebrMaestroServer *maestro)
{
	update_groups_store(maestro);
}

static void
gebr_maestro_server_state_change_real(GebrMaestroServer *maestro)
{
	update_groups_store(maestro);

	if (maestro->priv->nfsid)
		gebr_config_maestro_save();
}

static void
gebr_maestro_server_class_init(GebrMaestroServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_maestro_server_get;
	object_class->set_property = gebr_maestro_server_set;
	object_class->finalize = gebr_maestro_server_finalize;

	klass->group_changed = gebr_maestro_server_group_changed_real;
	klass->state_change  = gebr_maestro_server_state_change_real;

	signals[JOB_DEFINE] =
		g_signal_new("job-define",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, job_define),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__OBJECT,
			     G_TYPE_NONE, 1, GEBR_TYPE_JOB);

	signals[GROUP_CHANGED] =
		g_signal_new("group-changed",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, group_changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	signals[QUESTION_REQUEST] =
		g_signal_new("question-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, question_request),
			     NULL, NULL,
			     gebr_cclosure_marshal_BOOL__STRING_STRING_STRING,
			     G_TYPE_BOOLEAN, 3,
			     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[PASSWORD_REQUEST] =
		g_signal_new("password-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, password_request),
			     NULL, NULL,
			     gebr_cclosure_marshal_POINTER__OBJECT_BOOL_BOOL,
			     G_TYPE_POINTER, 3,
			     G_TYPE_OBJECT,
			     G_TYPE_BOOLEAN,
			     G_TYPE_BOOLEAN);

	signals[DAEMONS_CHANGED] =
			g_signal_new("daemons-changed",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, daemons_changed),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__VOID,
			             G_TYPE_NONE, 0);

	signals[STATE_CHANGE] =
		g_signal_new("state-change",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, state_change),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	signals[AC_CHANGE] =
			g_signal_new("ac-change",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, ac_change),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__INT_OBJECT,
			             G_TYPE_NONE, 2, G_TYPE_INT, GEBR_TYPE_DAEMON_SERVER);

	signals[MPI_CHANGED] =
			g_signal_new("mpi-changed",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, mpi_changed),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__OBJECT_STRING,
			             G_TYPE_NONE, 2, GEBR_TYPE_DAEMON_SERVER, G_TYPE_STRING);

	signals[DAEMON_ERROR] =
			g_signal_new("daemon-error",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, daemon_error),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__STRING_STRING_STRING,
			             G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[MAESTRO_ERROR] =
			g_signal_new("maestro-error",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, maestro_error),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__STRING_STRING_STRING,
			             G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[CONFIRM] =
			g_signal_new("confirm",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, confirm),
			             NULL, NULL,
			             gebr_cclosure_marshal_VOID__STRING_STRING,
			             G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

	signals[PATH_ERROR] =
			g_signal_new("path-error",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, path_error),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__INT,
			             G_TYPE_NONE, 1, G_TYPE_INT);

	signals[GVFS_MOUNT] =
			g_signal_new("gvfs-mount",
			             G_OBJECT_CLASS_TYPE(object_class),
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrMaestroServerClass, gvfs_mount),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__INT,
			             G_TYPE_NONE, 1, G_TYPE_INT);

	g_object_class_install_property(object_class,
					PROP_NFSID,
					g_param_spec_string("nfsid",
							    "Nfsid",
							    "NFS identification",
							    NULL,
							    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
					PROP_HOME,
					g_param_spec_string("home",
							    "Home",
							    "Home directory",
							    NULL,
							    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Server address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private(klass, sizeof(GebrMaestroServerPriv));
}

static void
gebr_maestro_server_init(GebrMaestroServer *maestro)
{
	maestro->priv = G_TYPE_INSTANCE_GET_PRIVATE(maestro,
						    GEBR_TYPE_MAESTRO_SERVER,
						    GebrMaestroServerPriv);

	maestro->priv->store = gtk_list_store_new(1, G_TYPE_POINTER);
	maestro->priv->jobs = g_hash_table_new(g_str_hash, g_str_equal);
	maestro->priv->temp_jobs = g_hash_table_new(g_str_hash, g_str_equal);
	maestro->priv->queues_model = gtk_list_store_new(1, GEBR_TYPE_JOB);
	maestro->priv->has_connected_daemon = FALSE;
	maestro->priv->window = NULL;
	maestro->priv->clocks_diff = 0;
	maestro->priv->wizard_setup = FALSE;

	maestro->priv->maestro_info_iface.maestro = maestro;
	maestro->priv->maestro_info_iface.iface.get_home_uri = gebr_maestro_server_get_home_uri;
	maestro->priv->maestro_info_iface.iface.get_home_mount_point = gebr_maestro_server_get_home_mount_point;
	maestro->priv->maestro_info_iface.iface.get_need_gvfs = gebr_maestro_server_get_need_gvfs;

	gebr_maestro_server_set_error(maestro, "error:none", NULL);

	// Insert queue 'Immediately'
	GtkTreeIter iter;
	gtk_list_store_append(maestro->priv->queues_model, &iter);
	gtk_list_store_set(maestro->priv->queues_model, &iter, 0, NULL, -1);

	maestro->priv->groups_store = gtk_list_store_new(MAESTRO_SERVER_N,
							 G_TYPE_INT,
							 G_TYPE_STRING,
							 G_TYPE_STRING);

	GebrDaemonServer *autochoose =
		gebr_daemon_server_new(GEBR_CONNECTABLE(maestro), 
				       NULL, 
				       SERVER_STATE_LOGGED, 
				       gebr_maestro_server_get_address(maestro));
	gebr_maestro_server_add_daemon(maestro, autochoose);
}

/* Implementation of GebrConnectable interface {{{ */
static void
gebr_maestro_server_connectable_connect(GebrConnectable *connectable,
					const gchar *address)
{
	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/server");
	gebr_comm_uri_add_param(uri, "address", address);
	gebr_comm_uri_add_param(uri, "respect-ac", "0");
	gchar *url = gebr_comm_uri_to_string(uri);

	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_connectable_disconnect(GebrConnectable *connectable,
                                           const gchar *address,
                                           const gchar *confirm)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/disconnect");
	gebr_comm_uri_add_param(uri, "address", address);
	gebr_comm_uri_add_param(uri, "confirm", confirm);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_remove(GebrConnectable *connectable,
                           const gchar *address)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/remove");
	gebr_comm_uri_add_param(uri, "address", address);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_stop(GebrConnectable *connectable,
                         const gchar *address,
                         const gchar *confirm)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/stop");
	gebr_comm_uri_add_param(uri, "address", address);
	gebr_comm_uri_add_param(uri, "confirm", confirm);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	GebrMaestroServer *maestro = GEBR_MAESTRO_SERVER(connectable);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
	                                       GEBR_COMM_HTTP_METHOD_PUT,
	                                       url, NULL);
	g_free(url);
}

static void
gebr_maestro_server_connectable_init(GebrConnectableIface *iface)
{
	iface->connect = gebr_maestro_server_connectable_connect;
	iface->disconnect = gebr_maestro_server_connectable_disconnect;
	iface->remove = gebr_maestro_server_remove;
	iface->stop = gebr_maestro_server_stop;
}
/* }}} */

GebrMaestroServer *
gebr_maestro_server_new(const gchar *addr)
{
	return g_object_new(GEBR_TYPE_MAESTRO_SERVER,
			    "address", addr,
			    NULL);
}

void
gebr_maestro_server_add_daemon(GebrMaestroServer *maestro,
			       GebrDaemonServer *daemon)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));
	g_return_if_fail(GEBR_IS_DAEMON_SERVER(daemon));

	GtkTreeIter iter;
	gtk_list_store_prepend(maestro->priv->store, &iter);
	gtk_list_store_set(maestro->priv->store, &iter, 0, daemon, -1);
}

GebrCommServerState
gebr_maestro_server_get_state(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), SERVER_STATE_DISCONNECTED);

	return maestro->priv->server->state;
}

GebrCommServer *
gebr_maestro_server_get_server(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return maestro->priv->server;
}

typedef struct {
	gboolean include_auto;
	gchar *group;
} GroupAndAutochoose;

static void
free_group_and_autochoose(gpointer data)
{
	GroupAndAutochoose *gaa = data;
	g_free(gaa->group);
	g_free(gaa);
}

static gboolean
visible_servers_func(GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
	GroupAndAutochoose *gaa = data;
	GebrDaemonServer *daemon;

	gtk_tree_model_get(model, iter, 0, &daemon, -1);

	if (!daemon)
		return FALSE;

	if (gebr_daemon_server_is_autochoose(daemon))
		return gaa->include_auto;

	return gebr_daemon_server_has_tag(daemon, gaa->group);
}

GtkTreeModel *
gebr_maestro_server_get_model(GebrMaestroServer *maestro,
			      gboolean include_autochoose,
			      const gchar *group)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

	if (include_autochoose && (!group || !*group))
		return g_object_ref(model);

	GroupAndAutochoose *gaa = g_new(GroupAndAutochoose, 1);
	gaa->include_auto = include_autochoose;
	gaa->group = g_strdup(group);

	GtkTreeModel *filter = gtk_tree_model_filter_new(model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       visible_servers_func, gaa,
					       free_group_and_autochoose);

	return filter;
}

const gchar *
gebr_maestro_server_get_address(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return maestro->priv->address;
}

gchar *
gebr_maestro_server_get_display_address(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	const gchar *addr = maestro->priv->address;

	if (g_strcmp0(addr, "127.0.0.1") == 0)
		return g_strdup(_("Local domain"));

	return g_strdup_printf(_("Maestro %s"), addr);
}

static gboolean
foreach_maestro_server_func(gpointer key, gpointer value, gpointer data)
{
	GList **groups = data;
	*groups = g_list_prepend(*groups, g_strdup((gchar*)(key)));
	return FALSE;
}

GList *
gebr_maestro_server_get_all_tags(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	GtkTreeIter iter;
	GtkTreeModel *daemons = gebr_maestro_server_get_model(maestro, FALSE, NULL);
	GTree *tree = g_tree_new((GCompareFunc)g_strcmp0);
	GList *groups = NULL;

	gebr_gui_gtk_tree_model_foreach(iter, daemons) {
		GebrDaemonServer *daemon;
		gtk_tree_model_get(daemons, &iter, 0, &daemon, -1);

		GList *tags = gebr_daemon_server_get_tags(daemon);
		for (GList *i = tags; i; i = i->next)
			g_tree_insert(tree, i->data, GUINT_TO_POINTER(TRUE));
	}

	g_tree_foreach(tree, foreach_maestro_server_func, &groups);
	g_object_unref(daemons);
	g_tree_unref(tree);
	return groups;
}

GtkTreeModel *
gebr_maestro_server_get_queues_model(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return GTK_TREE_MODEL(maestro->priv->queues_model);
}

void
gebr_maestro_server_disconnect(GebrMaestroServer *maestro,
                               gboolean quit)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	gebr_comm_server_disconnect(maestro->priv->server);
	unmount_gvfs(maestro, quit);
}

static void
on_password_request(GebrCommServer *server,
                    gboolean retry,
                    gpointer user_data)
{
	PasswordKeys *pk;
	GebrMaestroServer *maestro = user_data;

	g_signal_emit(maestro, signals[PASSWORD_REQUEST], 0,
		      maestro,
		      gebr_comm_server_get_accepts_key(server),
		      retry,
		      &pk);

	if (!pk)
		return;

	gebr_comm_server_set_use_public_key(server, pk->use_public_key);
	gebr_comm_server_set_password(server, pk->password);
}

static void
on_question_request(GebrCommServer *server,
                    const gchar *title,
                    const gchar *question,
                    gpointer user_data)
{
	gboolean response;
	GebrMaestroServer *maestro = user_data;

	g_signal_emit(maestro, signals[QUESTION_REQUEST], 0,
		      gebr_maestro_server_get_display_address(maestro),
		      title, question, &response);

	gebr_comm_server_answer_question(server, response);
}

void
gebr_maestro_server_connect(GebrMaestroServer *maestro,
                            gboolean force_init)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	maestro->priv->server = gebr_comm_server_new(maestro->priv->address,
						     gebr_get_session_id(),
						     &maestro_ops);

	g_signal_connect(maestro->priv->server, "server-password-request",
	                 G_CALLBACK(on_password_request), maestro);
	g_signal_connect(maestro->priv->server, "question-request",
	                 G_CALLBACK(on_question_request), maestro);

	maestro->priv->server->user_data = maestro;
	gebr_comm_server_connect(maestro->priv->server, TRUE, force_init);
}

void
gebr_maestro_server_add_temporary_job(GebrMaestroServer *maestro, 
				      GebrJob *job)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	g_hash_table_insert(maestro->priv->temp_jobs,
			    g_strdup(gebr_job_get_id(job)), job);
}

void
gebr_maestro_server_add_tag_to(GebrMaestroServer *maestro,
			       GebrDaemonServer *daemon,
			       const gchar *tag)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/tag-insert");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "tag", tag);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

void
gebr_maestro_server_remove_tag_from(GebrMaestroServer *maestro,
                                    GebrDaemonServer *daemon,
                                    const gchar *tag)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/tag-remove");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "tag", tag);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

void
gebr_maestro_server_set_autoconnect(GebrMaestroServer *maestro,
                                    GebrDaemonServer *daemon,
                                    gboolean ac)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/autoconnect");
	gebr_comm_uri_add_param(uri, "server", gebr_daemon_server_get_address(daemon));
	gebr_comm_uri_add_param(uri, "ac", ac? "on" : "off");
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);
	gebr_comm_protocol_socket_send_request(maestro->priv->server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
}

void 
gebr_maestro_server_set_window(GebrMaestroServer *maestro, GtkWindow *window)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	maestro->priv->window = window;
}

GtkTreeModel *
gebr_maestro_server_get_groups_model(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return GTK_TREE_MODEL(maestro->priv->groups_store);
}

/*
 * Keep the order of this list equal the enums at GebrMaestroServerGroupType!
 */
static const gchar *groups_str_types[] = {
	"group",
	"daemon",
	NULL
};

GebrMaestroServerGroupType
gebr_maestro_server_group_str_to_enum(const gchar *str)
{
	for (int i = 0; groups_str_types[i]; i++)
		if (g_strcmp0(groups_str_types[i], str) == 0)
			return (GebrMaestroServerGroupType) i;

	g_return_val_if_reached(MAESTRO_SERVER_TYPE_GROUP);
}

const gchar *
gebr_maestro_server_group_enum_to_str(GebrMaestroServerGroupType type)
{
	return groups_str_types[type];
}

GebrDaemonServer *
gebr_maestro_server_get_daemon(GebrMaestroServer *server,
			       const gchar *address)
{
	return get_daemon_from_address(server, address, NULL);
}

gchar *
gebr_maestro_server_get_browse_prefix(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	if (!maestro->priv->mount_location)
		return NULL;

	return g_file_get_uri(maestro->priv->mount_location);
}

void
gebr_maestro_server_set_nfsid(GebrMaestroServer *maestro,
                              const gchar *nfsid)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	if (maestro->priv->nfsid)
		g_free(maestro->priv->nfsid);
	maestro->priv->nfsid = g_strdup(nfsid);
	g_object_notify(G_OBJECT(maestro), "nfsid");
}

const gchar *
gebr_maestro_server_get_nfsid(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	if (maestro->priv->nfsid && strlen(maestro->priv->nfsid))
		return maestro->priv->nfsid;

	return NULL;
}

void
gebr_maestro_server_set_nfs_label(GebrMaestroServer *maestro,
                                  const gchar *nfs_label)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	if (maestro->priv->nfs_label)
		g_free(maestro->priv->nfs_label);
	maestro->priv->nfs_label = g_strdup(nfs_label);
}

const gchar *
gebr_maestro_server_get_nfs_label(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return maestro->priv->nfs_label;
}

static void
gebr_maestro_server_set_home_dir(GebrMaestroServer *maestro,
				 const gchar *home)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	if (maestro->priv->home)
		g_free(maestro->priv->home);
	maestro->priv->home = g_strdup(home);
	g_object_notify(G_OBJECT(maestro), "home");
}

const gchar *
gebr_maestro_server_get_home_dir(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	if (maestro->priv->home && strlen(maestro->priv->home))
		return maestro->priv->home;

	return g_strdup(g_get_home_dir());
}

gboolean
gebr_maestro_server_has_home_dir(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), FALSE);

	if (maestro->priv->home && strlen(maestro->priv->home))
		return TRUE;

	return FALSE;
}

gchar *
gebr_maestro_server_get_sftp_root(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	GMount *mount = NULL;
	if (maestro->priv->mount_location)
		mount = g_file_find_enclosing_mount(maestro->priv->mount_location, NULL, NULL);
	if (!mount)
		return NULL;
	GFile *root = g_mount_get_root(mount);
	gchar *ret = g_file_get_path(root);
	g_object_unref(mount);
	g_object_unref(root);
	return ret;
}

static gchar *
gebr_maestro_server_get_home_mount_point(GebrMaestroInfo *iface)
{
	struct MaestroInfoIface *self = (struct MaestroInfoIface*) iface;
	return gebr_maestro_server_get_sftp_root(self->maestro);
}

gboolean
gebr_maestro_server_get_need_gvfs(GebrMaestroInfo *iface)
{
	struct MaestroInfoIface *self = (struct MaestroInfoIface*) iface;
	return gebr_maestro_server_need_mount_gvfs(self->maestro);
}

void 
gebr_maestro_server_set_clocks_diff(GebrMaestroServer *maestro, gint secs)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	maestro->priv->clocks_diff = secs;
}

gint
gebr_maestro_server_get_clocks_diff(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), 0);

	return maestro->priv->clocks_diff;
}

static gchar *
gebr_maestro_server_get_home_uri(GebrMaestroInfo *iface)
{
	struct MaestroInfoIface *self = (struct MaestroInfoIface*) iface;
	return gebr_maestro_server_get_browse_prefix(self->maestro);
}

GebrMaestroInfo *
gebr_maestro_server_get_info(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	return (GebrMaestroInfo *) &maestro->priv->maestro_info_iface;
}

gint
gebr_maestro_server_get_ncores_for_group(GebrMaestroServer *maestro,
					 const gchar *mpi_flavor,
					 const gchar *group,
					 GebrMaestroServerGroupType type)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), 0);

	GebrDaemonServer *daemon;

	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		daemon = get_daemon_from_address(maestro, group, NULL);
		if (!mpi_flavor || gebr_daemon_server_accepts_mpi(daemon, mpi_flavor))
			return gebr_daemon_server_get_ncores(daemon);
		else
			return 0;
	}

	gint sum = 0;
	GtkTreeIter iter;
	GtkTreeModel *m = gebr_maestro_server_get_model(maestro, FALSE, group);

	gebr_gui_gtk_tree_model_foreach(iter, m) {
		gtk_tree_model_get(m, &iter, 0, &daemon, -1);
		if (!mpi_flavor || gebr_daemon_server_accepts_mpi(daemon, mpi_flavor))
			sum += gebr_daemon_server_get_ncores(daemon);
	}

	return sum;
}

gboolean
gebr_maestro_server_has_servers(GebrMaestroServer *maestro,
                                gboolean connected_servers)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), FALSE);

	gboolean valid;
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

	 valid = gtk_tree_model_get_iter_first(model, &iter);
	 while (valid) {
		 gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		 if (daemon) {
			 const gchar *addr = gebr_daemon_server_get_address(daemon);
			 if (connected_servers && *addr) {
				 if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
					 return TRUE;
			 } else if (*addr)
				 return TRUE;
		 }

		 valid = gtk_tree_model_iter_next(model, &iter);
	 }

	 return FALSE;
}
gboolean
gebr_maestro_server_has_connected_daemon(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), FALSE);

	return maestro->priv->has_connected_daemon;
}

static gchar *
gebr_maestro_server_get_user(GebrMaestroServer *maestro)
{
	g_return_val_if_fail(GEBR_IS_MAESTRO_SERVER(maestro), NULL);

	const gchar *addr = gebr_maestro_server_get_address(maestro);

	gchar *find_str = g_strrstr(addr, "@");
	if (find_str) {
		gint n = find_str - addr;
		gchar *user = g_strndup(addr, n);
		return user;
	}
	return g_strdup(g_get_user_name());
}

void
gebr_maestro_server_set_wizard_setup(GebrMaestroServer *maestro,
                                     gboolean is_wizard_setup)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	maestro->priv->wizard_setup = is_wizard_setup;
}

void
gebr_maestro_server_connect_on_daemons(GebrMaestroServer *maestro)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	if (g_strcmp0(maestro->priv->error_type, "error:none") != 0)
		return;

	g_debug("SEND CONNECT DAEMONS MESSAGE");
	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/connect-daemons");
	gchar *addr_without_user = gebr_get_host_from_address(server->address->str);

	gebr_comm_uri_add_param(uri, "address", addr_without_user);
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(server->socket,
	                                       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	g_free(url);
	g_free(addr_without_user);
}

void
gebr_maestro_server_append_key_finished()
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro->priv->wizard_setup)
		return;

	gebr_maestro_server_connect_on_daemons(maestro);
}

void
gebr_maestro_server_reset_daemons_timeout(GebrMaestroServer *maestro)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	gboolean valid;
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = GTK_TREE_MODEL(maestro->priv->store);

	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		if (daemon) {
			guint timeout = gebr_daemon_server_get_timeout(daemon);
			if (timeout != -1)
				g_source_remove(timeout);
			gebr_daemon_server_set_timeout(daemon, -1);
		}

		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

gchar *
gebr_maestro_server_translate_error(const gchar *error_type,
                                    const gchar *error_msg)
{
	gchar *message = NULL;

	if (g_strcmp0(error_type, "error:protocol") == 0)
		message = g_strdup_printf(_("Domain protocol version mismatch: %s"), error_msg);
	else if (g_strcmp0(error_type, "error:ssh") == 0)
		message = g_strdup(error_msg);

	return message;
}

GtkTreeModel *
gebr_maestro_server_copy_queues_model(GtkTreeModel *orig_model)
{
	GtkTreeIter iter, new_iter;
	GtkListStore *new_model;
	gboolean valid = FALSE;

	new_model = gtk_list_store_new(1,
	                               GEBR_TYPE_JOB);

	 valid = gtk_tree_model_get_iter_first(orig_model, &iter);

	 while (valid) {
		 GebrJob *job;
		 gtk_tree_model_get(orig_model, &iter, 0, &job, -1);

		 gtk_list_store_append(new_model, &new_iter);
		 gtk_list_store_set(new_model, &new_iter,
		                    0, job,
		                    -1);

		 valid = gtk_tree_model_iter_next(orig_model, &iter);
	 }
	 return GTK_TREE_MODEL(new_model);
}

void
gebr_maestro_server_send_nfs_label(GebrMaestroServer *maestro,
                                   const gchar *nfsid,
                                   const gchar *label)
{
	g_return_if_fail(GEBR_IS_MAESTRO_SERVER(maestro));

	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

	gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
	                                      gebr_comm_protocol_defs.nfsid_def, 3,
	                                      nfsid,
	                                      "",
	                                      label);
}
