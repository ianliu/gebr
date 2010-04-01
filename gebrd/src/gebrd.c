/*   GeBR Daemon - Process and control execution of flows
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>

#include "gebrd.h"
#include "server.h"
#include "client.h"

struct gebrd gebrd;

void gebrd_init(void)
{
	gchar buffer[100];

	pipe(gebrd.finished_starting_pipe);
	if (gebrd.options.foreground == FALSE) {
		if (fork() == 0) {
			int i;
			gboolean ret;

			/* daemon stuff */
			setsid();
			umask(027);
			close(0);
			close(1);
			close(2);
			i = open("/dev/null", O_RDWR);	/* open stdin */
			dup(i);	/* stdout */
			dup(i);	/* stderr */
			signal(SIGCHLD, SIG_IGN);

			gebrd.main_loop = g_main_loop_new(NULL, FALSE);
			g_type_init();

			ret = server_init();
			close(gebrd.finished_starting_pipe[0]);
			close(gebrd.finished_starting_pipe[1]);
			if (ret == TRUE) {
				g_main_loop_run(gebrd.main_loop);
				g_main_loop_unref(gebrd.main_loop);
			}

			return;
		}

		/* wait for server_init sign that it finished */
		read(gebrd.finished_starting_pipe[0], buffer, 10);

		fprintf(stdout, "%s", buffer);
	} else {
		gebrd.main_loop = g_main_loop_new(NULL, FALSE);
		g_type_init();

		if (server_init() == TRUE) {
			g_main_loop_run(gebrd.main_loop);
			g_main_loop_unref(gebrd.main_loop);
		}
	}
}

void gebrd_quit(void)
{
	gebrd_message(GEBR_LOG_END, _("Server quited"));

	g_list_foreach(gebrd.mpi_flavors, (GFunc)gebrd_mpi_config_free, NULL);
	g_list_free(gebrd.mpi_flavors);

	server_quit();
	g_main_loop_quit(gebrd.main_loop);
}

void gebrd_message(enum gebr_log_message_type type, const gchar * message, ...)
{
	gchar *string;
	va_list argp;

#ifndef GEBRD_DEBUG
	if (type == GEBR_LOG_DEBUG)
		return;
#endif

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	gebr_log_add_message(gebrd.log, type, string);
	if (gebrd.options.foreground == TRUE) {
		if (type != GEBR_LOG_ERROR)
			fprintf(stdout, "%s\n", string);
		else
			fprintf(stderr, "%s\n", string);
	}

	g_free(string);
}

guint8 gebrd_get_x11_redirect_display(void)
{
	static guint8 display = 10;

	while (gebr_comm_listen_socket_is_local_port_available(6000 + display) == FALSE) {
		if (display == 255) {
			display = 10;
			return 0;
		}
		display++;
	}

	return display++;
}

GebrCommServerType gebrd_get_server_type(void)
{
	static gboolean got_type = FALSE;
	static GebrCommServerType server_type;

	if (got_type)
		return server_type;

	if (g_find_program_in_path("msub") != NULL &&
	    g_find_program_in_path("mcredctl") != NULL &&
	    g_find_program_in_path("checkjob") != NULL)
		server_type = GEBR_COMM_SERVER_TYPE_MOAB;
	else 
		server_type = GEBR_COMM_SERVER_TYPE_REGULAR;

	got_type = TRUE;
	return server_type;
}

void gebrd_config_load(void)
{
	gchar ** groups;
	gchar * config_path;
	GKeyFile * key_file;
	GError * error;

	error = NULL;
	gebrd.mpi_flavors = NULL;
	config_path = g_strdup_printf("%s/.gebr/gebrd/gebrd.conf", getenv("HOME"));
	key_file = g_key_file_new();

	g_key_file_load_from_file(key_file, config_path, G_KEY_FILE_NONE, &error);

	if (error) {
		g_warning("Error reading ~/gebr/gebrd/gebrd.conf: %s\n", error->message);
		return;
	}

	groups = g_key_file_get_groups(key_file, NULL);

	for (int i = 0; groups[i]; i++) {
		if (!g_str_has_prefix(groups[i], "mpi-"))
			continue;
		GebrdMpiConfig * mpi_config;
		mpi_config = g_new(GebrdMpiConfig, 1);

		mpi_config->name = g_string_new(groups[i] + 4);
		mpi_config->libpath = gebr_g_key_file_load_string_key(key_file, groups[i], "libpath", NULL);
		mpi_config->binpath = gebr_g_key_file_load_string_key(key_file, groups[i], "binpath", NULL);
		mpi_config->init_script = gebr_g_key_file_load_string_key(key_file, groups[i], "init_script", NULL);
		mpi_config->end_script = gebr_g_key_file_load_string_key(key_file, groups[i], "end_script", NULL);

		gebrd.mpi_flavors = g_list_prepend(gebrd.mpi_flavors, mpi_config);
	}

	g_free(config_path);
	g_key_file_free(key_file);
}

const GebrdMpiConfig * gebrd_get_mpi_config_by_name(const gchar * name)
{
	GList * iter;
	for (iter = gebrd.mpi_flavors; iter; iter = iter->next)
		if (g_strcmp0(((GebrdMpiConfig*)iter->data)->name->str, name) == 0)
			return (GebrdMpiConfig*)iter->data;
	return NULL;
}
