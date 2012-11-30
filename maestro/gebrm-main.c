/*
 * gebrm-main.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
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

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/log.h>
#include <fcntl.h>
#include <gio/gio.h>

#include "gebrm-app.h"

#include <libgebr/gebr-version.h>
#include <libgebr/gebr-maestro-settings.h>

#define GETTEXT_PACKAGE "gebrm"

static gboolean interactive;
static gboolean show_version;
static int output_fd = STDOUT_FILENO;

static GOptionEntry entries[] = {
	{"interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive,
		"Run server in interactive mode, not as a daemon", NULL},
	{"version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
		"Show GeBR daemon version", NULL},
	{NULL}
};

void
fork_and_exit_half_main(void)
{
	int fd[2];

	if (pipe(fd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	if (pid == (pid_t)-1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid != 0)
		exit(EXIT_SUCCESS);

	umask(0);

	if (setsid() == (pid_t)-1) {
		perror("setsid");
		exit(EXIT_FAILURE);
	}

	if (chdir("/") == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	output_fd = fd[1];
	int null = open("/dev/null", O_RDWR);
	(void)dup2(null, STDIN_FILENO);
	(void)dup2(null, STDOUT_FILENO);
	(void)dup2(null, STDERR_FILENO);

	/* Gebr opens the file des number 3 when calling
	 * Maestro's binary. We close it as a workaround
	 * to the double-connection bug.
	 */
	close(3);
}

void
fork_and_exit_main(void)
{
	int fd[2];

	if (pipe(fd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	if (pid == (pid_t)-1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid != 0) {
		char c;
		int n = 0;
		GString *buf = g_string_new(NULL);

		while (read(fd[0], &c, 1) != 0) {
			if (c == '\n')
				n++;
			if (n == 1)
				break;
			g_string_append_c(buf, c);
		}
		puts(buf->str);
		g_string_free(buf, TRUE);
		exit(EXIT_SUCCESS);
	}

	umask(0);

	if (setsid() == (pid_t)-1) {
		perror("setsid");
		exit(EXIT_FAILURE);
	}

	if (chdir("/") == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	output_fd = fd[1];
	int null = open("/dev/null", O_RDWR);
	(void)dup2(null, STDIN_FILENO);
	(void)dup2(null, STDOUT_FILENO);
	(void)dup2(null, STDERR_FILENO);

	/* Gebr opens the file des number 3 when calling
	 * Maestro's binary. We close it as a workaround
	 * to the double-connection bug.
	 */
	close(3);
}

static void
gebrm_remove_lock_and_quit(int sig)
{
	g_unlink(gebrm_app_get_lock_file());
	g_unlink(gebrm_app_get_version_file());
	exit(0);
}

static gchar *
get_version_contents_from_file(const gchar *version_file)
{
	GError *version_error = NULL;
	gchar *version_contents;

	g_file_get_contents(version_file, &version_contents, NULL, &version_error);
	if (version_error) {
		if (version_error)
			g_critical("Error reading lock/version: %s", version_error->message);
		g_error_free(version_error);
		g_free(version_contents);
		return NULL;
	}
	g_free(version_error);

	return version_contents;
}

typedef struct {
	gchar *addr;
	guint port;
} ForwardData;

static void
start_forward(gint fd1,
              gint fd2)
{
	gchar buf[1024];
	GByteArray *buff_fd1 = g_byte_array_new();
	GByteArray *buff_fd2 = g_byte_array_new();
	fd_set readfds, writefds;

	for ( ; ; ) {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(fd1, &readfds);
		FD_SET(fd2, &readfds);

		FD_SET(fd1, &writefds);
		FD_SET(fd2, &writefds);

		gint nfds = MAX(fd1, fd2);

		gint ret = select (nfds + 1, &readfds, &writefds, NULL, NULL);

		if (ret == -1) {
			perror("select");
			exit(0);
		}

		if (FD_ISSET(fd1, &readfds)) {
			gssize len;
			len = read(fd1, &buf, sizeof(buf));
			g_byte_array_append(buff_fd1, (const guint8*) &buf, len);
		}

		if (FD_ISSET(fd2, &readfds)) {
			gssize len;
			len = read(fd2, &buf, sizeof(buf));
			g_byte_array_append(buff_fd2, (const guint8*) &buf, len);
		}

		if (FD_ISSET(fd1, &writefds) && buff_fd2->len > 0) {
			gssize len;
			len = write(fd1, buff_fd2->data, buff_fd2->len);

			if (len > 0)
				g_byte_array_remove_range(buff_fd2, 0, len);
		}

		if (FD_ISSET(fd2, &writefds) && buff_fd1->len > 0) {
			gssize len;
			len = write(fd2, buff_fd1->data, buff_fd1->len);

			if (len > 0)
				g_byte_array_remove_range(buff_fd1, 0, len);
		}
	}
}

static gboolean
on_client_listener_connect(GThreadedSocketService *service,
                           GSocketConnection      *connection,
                           GObject                *source_object,
                           ForwardData		  *data)
{
	gint fd1, fd2;
	GSocketConnectable *addr;
	GSocketAddressEnumerator *enumerator;
	GSocketAddress *sockaddr;

	GSocket *socket = g_socket_new(G_SOCKET_FAMILY_IPV4,
	                               G_SOCKET_TYPE_STREAM,
	                               G_SOCKET_PROTOCOL_TCP,
	                               NULL);

	if (!socket)
		return FALSE;

	addr = g_network_address_new (data->addr, data->port);
	enumerator = g_socket_connectable_enumerate (addr);
	g_object_unref (addr);

	gboolean valid = FALSE;
	while (!valid && (sockaddr = g_socket_address_enumerator_next (enumerator, NULL, NULL)))
	{
		valid = g_socket_connect(socket, sockaddr, NULL, NULL);
		g_object_unref (sockaddr);
	}
	g_object_unref (enumerator);

	GSocket *user_socket = g_socket_connection_get_socket(connection);

	fd1 = g_socket_get_fd(user_socket);
	fd2 = g_socket_get_fd(socket);

	start_forward(fd1, fd2);

	return FALSE;
}

static guint16
start_forward_listener(const gchar *remote_addr,
                       guint remote_port)
{
	GInetAddress *loopback = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
	GSocketAddress *addr = g_inet_socket_address_new(loopback, 0);

	GSocket *socket = g_socket_new(G_SOCKET_FAMILY_IPV4,
	                               G_SOCKET_TYPE_STREAM,
	                               G_SOCKET_PROTOCOL_TCP,
	                               NULL);

	if (!g_socket_bind(socket, addr, TRUE, NULL))
		return -1;

	if (!g_socket_listen(socket, NULL))
		return -1;

	GSocketAddress *local_addr = g_socket_get_local_address(socket, NULL);
	guint16 port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(local_addr));

	GSocketService *service = g_threaded_socket_service_new(-1);

	ForwardData *data = g_new0(ForwardData, 1);
	data->addr = g_strdup(remote_addr);
	data->port = remote_port;

	g_signal_connect(service, "run", G_CALLBACK(on_client_listener_connect), data);

	g_socket_listener_add_socket(G_SOCKET_LISTENER(service), socket, NULL, NULL);
	g_socket_service_start(service);

	return port;
}

static gboolean
check_if_port_is_busy(const gchar *addr,
                      gint port,
                      const gchar *curr_version,
                      const gchar *version_contents)
{
	if (!gebr_comm_listen_socket_is_local_port_available(port) || gebr_comm_listen_socket_listen_on_port(port, addr)) {
		if (g_strcmp0(curr_version, version_contents) == 0) { //It is running in the same version
			guint16 local_port;

			fork_and_exit_main();

			local_port = start_forward_listener(addr, port);

			/* success, send port */
			gchar *port_str = g_strdup_printf("%s%u\n",
			                                  GEBR_PORT_PREFIX, port);

			ssize_t s = 0;
			do {
				s = write(output_fd, port_str + s, strlen(port_str) - s);
				if (s == -1)
					exit(-1);
			} while(s != 0);

			g_free(port_str);

			GMainLoop *loop;
			loop = g_main_loop_new(NULL, FALSE);
			g_main_loop_run(loop);

			return TRUE;
		} else {		//It is running in a different version
			gebr_kill_by_port(port);
		}
	}

	return FALSE;
}

static gboolean
get_singleton_lock_contents(const gchar *lock,
                            gchar **addr,
                            gint *port)
{
	GError *lock_error = NULL;
	gchar *lock_contents;

	g_file_get_contents(lock, &lock_contents, NULL, &lock_error);
	if (lock_error) {
		g_critical("Error reading lock/version: %s", lock_error->message);

		g_error_free(lock_error);
		g_free(lock_contents);

		return FALSE;
	}

	gchar **lock_array = g_strsplit(lock_contents, ":", -1);

	if (lock_array) {
		*port = atoi(lock_array[1]);
		*addr = g_strdup(lock_array[0]);
		g_strfreev(lock_array);

		return TRUE;
	}

	g_critical("Error on singleton lock file");
	g_free(lock_contents);

	return FALSE;
}

static gboolean
verify_singleton_lock(const gchar *curr_version)
{
	const gchar *lock;
	const gchar *version_file;

	lock = gebrm_app_get_lock_file();
	version_file  = gebrm_app_get_version_file();

	if (g_access(lock, R_OK | W_OK) == 0) { //check lock_file
		gchar *addr;
		gint port;

		if (!get_singleton_lock_contents(lock, &addr, &port))
			exit(1);

		if (g_access(version_file, R_OK | W_OK) == 0) { //It has the version file
			gchar *version_contents = get_version_contents_from_file(version_file);
			if (!version_contents)
				exit(1);

			if (check_if_port_is_busy(addr, port, curr_version, version_contents)) {
				g_free(version_contents);
				return TRUE;
			}

			g_free(version_contents);
		}
	}
	else if (g_access(lock, F_OK) == 0) {
		g_critical("Cannot read/write into %s", lock);
		exit(1);
	}

	return FALSE;
}

int
main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	context = g_option_context_new("GêBR Maestro");
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	g_type_init();
	g_thread_init(NULL);

	if (show_version) {
		fprintf(stdout, "%s (%s)\n", GEBR_VERSION NANOVERSION, gebr_version());
		exit(EXIT_SUCCESS);
	}

	GebrMaestroSettings *ms = gebrm_app_create_configuration();
	const gchar *nfsid = gebrm_app_get_nfsid(ms);
	const gchar *local_addr = g_get_host_name();

	gchar *curr_version = NULL;
	curr_version = g_strdup_printf("%s (%s)\n", GEBR_VERSION NANOVERSION, gebr_version());

	gebrm_app_create_folder_for_addr(local_addr);
	const gchar *path = gebrm_app_get_log_file_for_address(local_addr);
	gebr_log_set_default(path);

	gboolean valid;
	valid = verify_singleton_lock(curr_version);

	const gchar *gebrd_location = g_find_program_in_path("gebrd");

	if (nfsid) {
		gchar *addr = g_strdup_printf("%s@%s", g_get_user_name(), local_addr);
		gebr_maestro_settings_append_address(ms, nfsid, addr);
		if (gebrd_location)
			gebr_maestro_settings_add_node(ms, g_get_host_name(), "", "on");
		g_free(addr);
	}

	gebr_maestro_settings_free(ms);

	// Singleton lock is valid and suggest the correct maestro
	if (valid) {
		g_free(curr_version);
		exit(0);
	}

	if (!interactive)
		fork_and_exit_main();

	struct sigaction sa;
	sa.sa_handler = gebrm_remove_lock_and_quit;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, NULL)
	    || sigaction(SIGINT, &sa, NULL)
	    || sigaction(SIGQUIT, &sa, NULL)
	    || sigaction(SIGSEGV, &sa, NULL))
		perror("sigaction");

	gebr_geoxml_init();

	GebrmApp *app = gebrm_app_singleton_get();

	if (!gebrm_app_run(app, output_fd, curr_version))
		exit(EXIT_FAILURE);

	g_free(curr_version);
	g_object_unref(app);
	gebr_geoxml_finalize();

	return EXIT_SUCCESS;
}
