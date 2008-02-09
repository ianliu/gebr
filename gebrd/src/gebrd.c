/*   GÍBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <sys/types.h>
#include <unistd.h>

#include "gebrd.h"
#include "support.h"
#include "server.h"
#include "client.h"

struct gebrd	gebrd;

void
gebrd_init(void)
{
	gchar	buf[2];

	pipe(gebrd.finished_starting_pipe);
	if (fork() == 0) {
		gebrd.main_loop = g_main_loop_new(NULL, FALSE);
		g_type_init();

		if (server_init() == TRUE) {
			g_main_loop_run(gebrd.main_loop);
			g_main_loop_unref(gebrd.main_loop);
		}

		return;
	}

	/* wait for server_init sign that it finished */
	read(gebrd.finished_starting_pipe[0], buf, 2);
	close(gebrd.finished_starting_pipe[0]);
	close(gebrd.finished_starting_pipe[1]);
}

void
gebrd_quit(void)
{
	gebrd_message(LOG_END, _("Server quited"), g_tcp_server_server_port(gebrd.tcp_server));

	server_quit();
	g_main_loop_quit(gebrd.main_loop);
}

/*
 * Function: gebrd_message
 * Log a message. If in_stdout is TRUE it is writen to standard output.
 *
 */
void
gebrd_message(enum log_message_type type, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	log_add_message(gebrd.log, type, string);

	g_free(string);
}
