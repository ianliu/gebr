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

#include <stdio.h>

#include <libgebr.h>
#include <libgebr/intl.h>

#include "gebrclient.h"
#include "server.h"

struct gebr_client gebr_client;

gboolean
gebr_client_init(const gchar * server_address)
{
	libgebr_init();

	gebr_client.server = server_new(server_address);

	return TRUE;
}

void
gebr_client_quit(void)
{
	server_free(gebr_client.server);

	g_main_loop_quit(gebr_client.main_loop);
}

void
gebr_client_message(enum log_message_type type, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

#ifndef GEBR_DEBUG
	if (type != LOG_DEBUG) {
#endif
	if (type == LOG_ERROR)
		fprintf(stderr, "%s\n", string);
	else
		fprintf(stdout, "%s\n", string);
#ifndef GEBR_DEBUG
	}
#endif

	g_free(string);
}
