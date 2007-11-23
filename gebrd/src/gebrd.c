/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "gebrd.h"
#include "server.h"
#include "client.h"

struct gebrd	gebrd;

void
gebrd_init(void)
{
	GHostAddress *	host_address;

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* server */
	if (!server_init()) {
		fprintf(stderr, "Could not init server. Quiting...\n");

		server_free();
		g_main_loop_quit(gebrd.main_loop);
		g_main_loop_unref(gebrd.main_loop);
		return;
	}

	g_print("Server started at %u port\n", g_tcp_server_server_port(gebrd.tcp_server));
}

void
gebrd_quit(void)
{
	/* TODO: someway we have to make the user reach this function */

	server_quit();
	g_main_loop_quit(gebrd.main_loop);
	g_main_loop_unref(gebrd.main_loop);
}
