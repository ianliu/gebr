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
			if (n == 2)
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

static gboolean
is_maestro_spec_valid(GebrMaestroSettings *ms,
		      const gchar *nfsid,
		      gint index,
		      gchar **curr_version)
{
	const gchar *lock;
	const gchar *version_file;
	gchar *lock_contents;
	GError *lock_error;

	const gchar *local_addr = g_get_host_name();
	const gchar *addr = NULL;
	gboolean same_host = FALSE;

	if (nfsid)
		addr = gebr_maestro_settings_get_addr_for_domain(ms, nfsid, index);

	if (!addr || !*addr) {
		addr = local_addr;
		lock = gebrm_app_get_lock_file();
		version_file  = gebrm_app_get_version_file();
		same_host = TRUE;
	} else {
		lock = gebrm_app_get_lock_file();
		version_file = gebrm_app_get_version_file();

		if (g_strcmp0(addr, local_addr) == 0)
			same_host = TRUE;
	}

	lock_error = NULL;

	*curr_version = g_strdup_printf("%s (%s)\n", GEBR_VERSION NANOVERSION, gebr_version());

	if (g_access(lock, R_OK | W_OK) == 0) { //check lock_file
		GError *version_error = NULL;

		g_file_get_contents(lock, &lock_contents, NULL, &lock_error);
		if (lock_error) {
			if (lock_error)
				g_critical("Error reading lock/version: %s", lock_error->message);
			g_free(lock_contents);
			exit(1);
		}
		g_free(lock_error);

		gchar **lock_array = g_strsplit(lock_contents, ":", -1);
		gint port = 0;

		if (lock_array)
			port = atoi(lock_array[1]);

		if (g_access(version_file, R_OK | W_OK) == 0) { //It has the version file
			gchar *version_contents;
			g_file_get_contents(version_file, &version_contents, NULL, &version_error);
			if (version_error) {
				if (version_error)
					g_critical("Error reading lock/version: %s", version_error->message);
				g_free(version_contents);
				exit(1);
			}
			g_free(version_error);

			if (gebr_comm_listen_socket_listen_on_port(port, addr) || !gebr_comm_listen_socket_is_local_port_available(port)) {
				if (g_strcmp0(*curr_version, version_contents) == 0) { //It is running in the same version
					if (nfsid)
						gebr_maestro_settings_append_address(ms, nfsid, local_addr);
					gebr_maestro_settings_free(ms);

					gchar *new_addr;
					const gchar *username = g_get_user_name();
					if (username && *username)
						new_addr = g_strdup_printf("%s@%s", username, addr);
					else
						new_addr = g_strdup(addr);

					g_print("%s%s\n%s%s\n",
						GEBR_PORT_PREFIX, lock_array[1],
						GEBR_ADDR_PREFIX, new_addr);

					g_free(new_addr);
					exit(0);
				} else {		//It is running in a different version
					gebr_kill_by_port(port);
				}
			}
			g_free(version_contents);

			if (same_host) {
				g_free(lock_contents);
				return FALSE;
			}
		}
		else {		//It does not have version file
			gebr_kill_by_port(port);
		}
		g_strfreev(lock_array);
	} else if (g_access(lock, F_OK) == 0) {
		g_critical("Cannot read/write into %s", lock);
		exit(1);
	} else if (same_host)
		return FALSE;
	return TRUE;
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
	gboolean valid = TRUE;

	for (gint index = -1; valid; index++)
		valid = is_maestro_spec_valid(ms, nfsid, index, &curr_version);

	gebr_maestro_settings_prepend_address(ms, nfsid, local_addr);
	gebr_maestro_settings_free(ms);

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

	gebrm_app_create_folder_for_addr(local_addr);
	gchar *path = gebrm_app_get_log_file_for_address(local_addr);
	gebr_log_set_default(path);
	g_free(path);

	GebrmApp *app = gebrm_app_singleton_get();

	if (!gebrm_app_run(app, output_fd, curr_version))
		exit(EXIT_FAILURE);

	g_free(curr_version);
	g_object_unref(app);
	gebr_geoxml_finalize();

	return EXIT_SUCCESS;
}
