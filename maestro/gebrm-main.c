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
#include "gebrm-proxy.h"

#include <libgebr/gebr-version.h>
#include <libgebr/gebr-maestro-settings.h>
#include <libgebr/gebr-auth.h>

#define GETTEXT_PACKAGE "gebrm"

static gboolean interactive;
static gboolean show_version;
static gboolean force_init;
static gboolean nocookie;
static int output_fd = STDOUT_FILENO;
static GebrAuth *auth;

static GOptionEntry entries[] = {
	{"interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive,
		"Run server in interactive mode, not as a daemon", NULL},
	{"version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
		"Show GeBR daemon version", NULL},
	{"force", 'f', 0, G_OPTION_ARG_NONE, &force_init,
		"Force to run server, ignoring lock", NULL},
	{"nocookie", 'n', 0, G_OPTION_ARG_NONE, &nocookie,
		"Do not ask for authorization cookie when launching", NULL},
	{NULL}
};

static const gchar *
get_version(void)
{
	static gchar *version = NULL;
	if (!version)
		version = g_strdup_printf("%s (%s)", GEBR_VERSION NANOVERSION, gebr_version());
	return version;
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
	gchar *contents;
	if (!g_file_get_contents(version_file, &contents, NULL, NULL))
		return NULL;
	return g_strstrip(contents);
}

static gboolean
need_cleanup(const gchar *v1,
             const gchar *v2)
{
	if (g_strcmp0(v1, v2) == 0)
		return FALSE;

	return TRUE;
}

static void
register_local_daemon(void)
{
	GebrMaestroSettings *ms = gebrm_app_create_configuration();
	const gchar *nfsid = gebrm_app_get_nfsid(ms);
	if (nfsid) {
		const gchar *gebrd_location = g_find_program_in_path("gebrd");
		gchar *addr = g_strdup_printf("%s@%s", g_get_user_name(), g_get_host_name());
		gebr_maestro_settings_append_address(ms, nfsid, addr);
		if (gebrd_location)
			gebr_maestro_settings_add_node(ms, g_get_host_name(), "", "on");
		g_free(addr);
	}
	gebr_maestro_settings_free(ms);
}

static void
start_proxy_maestro(const gchar *address,
		    guint port)
{
	GebrmProxy *proxy = gebrm_proxy_new(address, port);

	//Generate gebr.key
	gebr_generate_key();
	gebrm_app_append_key();

	gebrm_proxy_run(proxy, output_fd);
	gebrm_proxy_free(proxy);
}

static void
start_maestro_app(void)
{
	struct sigaction sa;
	sa.sa_handler = gebrm_remove_lock_and_quit;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, NULL)
	    || sigaction(SIGINT, &sa, NULL)
	    || sigaction(SIGQUIT, &sa, NULL)
	    || sigaction(SIGSEGV, &sa, NULL))
		perror("sigaction");

	register_local_daemon();
	gebrm_app_create_folder_for_addr(g_get_host_name());
	const gchar *path = gebrm_app_get_log_file_for_address(g_get_host_name());
	gebr_log_set_default(path);

	GebrmApp *app = gebrm_app_singleton_get();

	if (!gebrm_app_run(app, output_fd, get_version(), auth))
		exit(EXIT_FAILURE);

	g_object_unref(app);
}

static gboolean
get_current_maestro_info(gchar **addr,
			 guint *port,
			 gchar **version)
{
	const gchar *lock = gebrm_app_get_lock_file();
	const gchar *version_file  = gebrm_app_get_version_file();
	gchar *contents;

	if (!g_file_get_contents(lock, &contents, NULL, NULL))
		return FALSE;

	gchar **parts = g_strsplit(contents, ":", -1);

	if (parts) {
		*version = get_version_contents_from_file(version_file);
		*addr = g_strdup(parts[0]);
		*port = atoi(parts[1]);
		g_strfreev(parts);
		return TRUE;
	}

	g_free(parts);
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
	gebr_geoxml_init();

	if (show_version) {
		fprintf(stdout, "%s\n", get_version());
		exit(EXIT_SUCCESS);
	}

	gboolean has_lock;
	gboolean cleanup;
	gboolean is_local;

	gchar *addr;
	gchar *version;
	guint port;

	has_lock = get_current_maestro_info(&addr, &port, &version);
	if (has_lock) {
		is_local = g_strcmp0(addr, g_get_host_name()) == 0;
		cleanup = need_cleanup(get_version(), version);
	}
	auth = gebr_auth_new();

	if (force_init)
		has_lock = FALSE;

	gboolean start_maestro = !has_lock || (has_lock && cleanup);
	gboolean start_proxy = has_lock && !cleanup && !is_local;

	if (has_lock && cleanup)
		gebr_kill_by_port(port);

	if (!nocookie)
		gebr_auth_read_cookie(auth);

	if (start_maestro) {
		if (!interactive)
			fork_and_exit_main();
		start_maestro_app();
	} else if (start_proxy) {
		fork_and_exit_main();
		start_proxy_maestro(addr, port);
	} else {
		g_print("%s%d\n", GEBR_PORT_PREFIX, port);
	}

	gebr_geoxml_finalize();

	return EXIT_SUCCESS;
}
