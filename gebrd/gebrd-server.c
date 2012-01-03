/*
 * gebrd-server.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <glib/gstdio.h>

#include <libgebr.h>
#include <glib/gi18n.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/geoxml/geoxml.h>

#include "gebrd-server.h"
#include "gebrd.h"


/*
 * Private functions
 */

static gboolean server_run_lock(gboolean *already_running)
{
	*already_running = FALSE;
	/* check if there is another daemon running for this user and hostname */
	g_string_printf(gebrd->run_filename, "%s/.gebr/run/gebrd-%s.run", g_get_home_dir(), gebrd->hostname);
	if (g_file_test(gebrd->run_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == TRUE) {
		gchar *contents;
		GError *error = NULL;
		if (!g_file_get_contents(gebrd->run_filename->str, &contents, NULL, &error))
			g_warning("%s:%d: Failed to retrieve contents of '%s' file",
				  __FILE__, __LINE__, gebrd->run_filename->str);

		guint16 port = atoi(contents);
		g_free(contents);

		/* is this port really being used (open)?? */
		if (gebr_comm_listen_socket_is_local_port_available(port) == FALSE) {
			if (gebrd->options.foreground == TRUE) {
				gebrd_message(GEBR_LOG_ERROR,
					      _("Cannot run interactive server, GêBR daemon is already running"));
			} else {
				gchar buffer[100];
				snprintf(buffer, sizeof(buffer), "%d\n", port);
				if (write(gebrd->finished_starting_pipe[1], buffer, strlen(buffer)+1) < 0)
					g_warning("Failed to write in file with error code %d", errno);
			}

			*already_running = TRUE;
			goto out;
		}
	}
	
	/* init the server socket and listen */
	GebrCommSocketAddress socket_address = gebr_comm_socket_address_ipv4_local(0);
	gebrd->listen_socket = gebr_comm_listen_socket_new();

	if (!gebr_comm_listen_socket_listen(gebrd->listen_socket, &socket_address)) {
		gebrd_message(GEBR_LOG_ERROR, _("Could not listen for connections.\n"));
		goto err;
	}

	gebrd->socket_address = gebr_comm_socket_get_address(GEBR_COMM_SOCKET(gebrd->listen_socket));
	g_signal_connect(gebrd->listen_socket, "new-connection", G_CALLBACK(server_new_connection), NULL);

	GError *error = NULL;
	GString *port_gstring = g_string_new("");
	g_string_printf(port_gstring, "%d\n", gebr_comm_socket_address_get_ip_port(&gebrd->socket_address));
	gboolean ret = g_file_set_contents(gebrd->run_filename->str, port_gstring->str, port_gstring->len, &error);
	g_string_free(port_gstring, TRUE);
	if (!ret) {
		gebrd_message(GEBR_LOG_ERROR, _("Could not write run file."));
		goto err;
	}

out:
	return TRUE;
err:
	return FALSE;
}

static gboolean server_fs_lock(void)
{
	GString *filename = g_string_new("");
	g_string_printf(filename, "%s/.gebr/run/gebrd-fslock.run", g_get_home_dir());
	gchar *lock_hash = gebr_id_random_create(32);
	gchar *fs_lock = gebr_lock_file(filename->str, lock_hash, FALSE);
	g_free(lock_hash);
	g_string_free(filename, TRUE);

	if (fs_lock == NULL) {
		g_warning(_("%s:%d: Could not acquire FS lock.\n"), __FILE__, __LINE__);
		return FALSE;
	}

	gchar * fs_nickname = NULL;
	g_string_assign(gebrd->fs_lock, fs_lock);
	g_object_get(gebrd->user, "fs-nickname", &fs_nickname, NULL);
	if (!strlen(fs_nickname)) {
		fs_nickname = g_strdup_printf(_("FS of %s"), g_get_host_name());	
		g_object_set(gebrd->user, "fs-nickname", fs_nickname, NULL);
	}
	g_free(fs_nickname);
	g_free(fs_lock);

	return TRUE;
}

/*
 * Public
 */

gboolean server_init(void)
{
	/* from libgebr-misc */
	if (gebr_create_config_dirs() == FALSE) {
		g_warning(_("%s:%d: Could not access GêBR configuration directories.\n"), __FILE__, __LINE__);
		goto err;
	}

	/* log */
	GString *log_filename = g_string_new(NULL);
	g_string_printf(log_filename, "%s/.gebr/log/gebrd-%s.log", g_get_home_dir(), gebrd->hostname);
	gebrd->log = gebr_log_open(log_filename->str);
	g_string_free(log_filename, TRUE);

	/* run lock */
	gboolean already_running;
	if (!server_run_lock(&already_running))
		goto err;
	else if (already_running)
		return FALSE;

	/* Daemon Id */
	gchar *contents;
	gchar *gebrd_dir = g_build_filename(g_get_home_dir(), ".gebr", "gebrd", g_get_host_name(), NULL);
	gchar *id_path = g_build_filename(gebrd_dir, "id", NULL);

	if (g_access(id_path, R_OK | W_OK) == 0) {
		if (!g_file_get_contents(id_path, &contents, NULL, NULL)) {
			g_warning("Could not read %s contents", id_path);
			exit(1);
		}
	} else {
		g_mkdir_with_parents(gebrd_dir, 0700);
		contents = gebr_id_random_create(32);
		if (!g_file_set_contents(id_path, contents, -1, NULL)) {
			g_warning("Could not write into %s", id_path);
			exit(1);
		}
	}

	gebrd_user_set_daemon_id(gebrd->user, contents);
	g_free(contents);
	g_free(gebrd_dir);
	g_free(id_path);

	/* fs lock */
	if (!server_fs_lock())
		goto err;

	/* connecting signal TERM */
	struct sigaction act;
	act.sa_sigaction = (typeof(act.sa_sigaction)) & gebrd_quit;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	/* success, send port */
	gebrd_message(GEBR_LOG_START, _("Server started at %u port"),
		      gebr_comm_socket_address_get_ip_port(&gebrd->socket_address));
	gchar buffer[100];
	snprintf(buffer, sizeof(buffer), "%d\n", gebr_comm_socket_address_get_ip_port(&gebrd->socket_address));
	if (write(gebrd->finished_starting_pipe[1], buffer, strlen(buffer)+1) <= 0) {
		g_warning("%s:%d: Failed to write in file with error code %d", __FILE__, __LINE__, errno);
		goto err;
	}

	return TRUE;
err:	
	gebrd_message(GEBR_LOG_ERROR, _("Could not init server. Quiting..."));
	dprintf(gebrd->finished_starting_pipe[1], "0\n");
	server_free();
	return FALSE;
}

void server_free(void)
{
	gebr_log_close(gebrd->log);
	/* delete run lock
	 * fs lock must not be deleted, as it can be used throught multiple sessions */
	g_unlink(gebrd->run_filename->str);
}

void server_quit(void)
{
	server_free();
}

void server_new_connection(void)
{
	GebrCommStreamSocket *socket;
	GebrCommProtocolSocket *client;

	socket = gebr_comm_listen_socket_get_next_pending_connection(gebrd->listen_socket);
	client = gebr_comm_protocol_socket_new_from_socket(socket);

	if (!gebrd_user_has_connection(gebrd->user)) {
		client_add(client);
		gebrd_message(GEBR_LOG_DEBUG, "client_add");
	} else {
		gebr_comm_protocol_socket_oldmsg_send(client, TRUE,
						      gebr_comm_protocol_defs.err_def, 2,
						      "connection-refused",
						      gebrd_user_get_daemon_id(gebrd->user));
		g_object_unref(client);
	}

	gebrd_message(GEBR_LOG_DEBUG, "server_new_connection");
}

