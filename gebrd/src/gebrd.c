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
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libgebr/intl.h>

#include "gebrd.h"
#include "server.h"
#include "client.h"

struct gebrd gebrd;

void gebrd_init(void)
{
	gchar buf[10];

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
		read(gebrd.finished_starting_pipe[0], buf, 10);

		fprintf(stdout, "%s", buf);
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

	if (g_find_program_in_path("msub") != NULL && g_find_program_in_path("mcredctl") != NULL)
		server_type = GEBR_COMM_SERVER_TYPE_MOAB;
	else 
		server_type = GEBR_COMM_SERVER_TYPE_REGULAR;

	got_type = TRUE;
	return server_type;
}

