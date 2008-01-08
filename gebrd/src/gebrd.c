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

#include <stdio.h>
#include <stdlib.h>

#include <misc/utils.h>

#include "gebrd.h"
#include "support.h"
#include "server.h"
#include "client.h"

struct gebrd	gebrd;

void
gebrd_init(void)
{
	GString *	log_filename;
	GHostAddress *	host_address;

	/* initialization */
	log_filename = g_string_new(NULL);

	/* from libgebr-misc */
	if (gebr_create_config_dirs() == FALSE) {
		gebrd_message(ERROR, TRUE, TRUE, _("Could not access GÍBR configuration directories."));
		goto out;
	}

	/* log */
	g_string_printf(log_filename, "%s/.gebr/gebrd.log", getenv("HOME"));
	gebrd.log = log_open(log_filename->str);

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* server */
	if (!server_init()) {
		gebrd_message(ERROR, TRUE, TRUE, _("Could not init server. Quiting..."));

		server_free();
		g_main_loop_quit(gebrd.main_loop);
		g_main_loop_unref(gebrd.main_loop);
		goto out;
	}

	gebrd_message(START, TRUE, TRUE, _("Server started at %u port"), g_tcp_server_server_port(gebrd.tcp_server));

	/* frees */
out:	g_string_free(log_filename, TRUE);
}

void
gebrd_quit(void)
{
	gebrd_message(END, TRUE, TRUE, _("Server quited"), g_tcp_server_server_port(gebrd.tcp_server));
	log_close(gebrd.log);

	server_quit();
	g_main_loop_quit(gebrd.main_loop);
	g_main_loop_unref(gebrd.main_loop);
}

/*
 * Function: gebrd_message
 * Log a message. If in_stdout is TRUE it is writen to standard output.
 *
 */
void
gebrd_message(enum log_message_type type, gboolean in_stdout, gboolean in_log_file, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	if (in_stdout)
		g_print("%s\n", string);
	if (in_log_file)
		log_add_message(gebrd.log, type, string);

	g_free(string);
}
