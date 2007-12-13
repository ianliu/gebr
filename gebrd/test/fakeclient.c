/*   GÃªBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

#include <comm/ghostaddress.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fakeclient.h"

struct fake_client	fake_client;

void
fake_client_connect_to_server(void)
{
	GTcpSocket * 	tcp_socket;
	GHostAddress *	host_address;

	/* local address used for listening */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	/* create socket and connect to server */
	tcp_socket = g_tcp_socket_new();
	g_signal_connect(tcp_socket, "connected",
			G_CALLBACK(fake_client_connected), NULL);
	g_signal_connect(tcp_socket, "disconnected",
			G_CALLBACK(fake_client_disconnected), NULL);
	g_signal_connect(tcp_socket, "ready-read",
			G_CALLBACK(fake_client_read), NULL);
	g_signal_connect(tcp_socket, "ready-write",
			G_CALLBACK(fake_client_write), NULL);
	g_signal_connect(tcp_socket, "error",
			G_CALLBACK(fake_client_error), NULL);
	g_tcp_socket_connect(tcp_socket, host_address, 2125);

	g_host_address_free(host_address);
	/* i know, there is a memleak (on tcp_socket) ;) */
}

void
fake_client_connected(GTcpSocket * tcp_socket)
{
	g_print("fake_client_connected ");

	GString *	message;
	gchar		hostname[100];

	gethostname(hostname, 100);
	message = g_string_new("");
	g_string_printf(message, "INI %lu %s", strlen(hostname), hostname);

	g_print("%s\n", message->str);

	g_socket_write_string(G_SOCKET(tcp_socket), message);
	g_string_free(message, TRUE);
}

void
fake_client_disconnected(GTcpSocket * tcp_socket)
{
	g_print("fake_client_disconnected\n");
	g_main_loop_quit(fake_client.main_loop);
}

void
fake_client_read(GTcpSocket * tcp_socket)
{
	g_print("fake_client_read\n");
}

void
fake_client_write(GTcpSocket * tcp_socket)
{
	g_print("fake_client_write\n");
}

void
fake_client_error(GTcpSocket * tcp_socket, enum GSocketError error)
{
	g_print("fake_client_error = %d\n", error);
	g_main_loop_quit(fake_client.main_loop);
}
