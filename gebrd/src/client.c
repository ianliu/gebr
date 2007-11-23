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
#include <unistd.h>

#include <misc/protocol.h>

#include "client.h"
#include "gebrd.h"
#include "server.h"

void
client_add(GTcpSocket * tcp_socket)
{
	struct client *	client;

	client = g_malloc(sizeof(struct client));
	if (client == NULL)
		goto err;

	*client = (struct client) {
		.tcp_socket = tcp_socket,
		.protocol = protocol_new()
	};
	if (client->protocol == NULL)
		goto err;
	gebrd.clients = g_list_prepend(gebrd.clients, client);

	g_signal_connect(tcp_socket, "disconnected",
			G_CALLBACK(client_disconnected), client);
	g_signal_connect(tcp_socket, "ready-read",
			G_CALLBACK(client_read), client);
g_print("client_add\n");
	return;

err:	g_socket_close(G_SOCKET(tcp_socket));
}

void
client_free(struct client * client)
{
	g_socket_close(G_SOCKET(client->tcp_socket));
	protocol_free(client->protocol);
	g_string_free(client->display, TRUE);
	g_string_free(client->mcookie, TRUE);
	g_free(client);
}

void
client_disconnected(GTcpSocket * tcp_socket, struct client * client)
{
g_print("client_disconnected\n");
	gebrd.clients = g_list_remove(gebrd.clients, client);
	client_free(client);
}

void
client_read(GTcpSocket * tcp_socket, struct client * client)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));g_print("client_read %s\n", data->str);
	if (protocol_receive_data(client->protocol, data) == FALSE) {
		client_disconnected(tcp_socket, client);
		goto out;
	}
	if (server_parse_client_messages(client) == FALSE) {
		client_disconnected(tcp_socket, client);
		goto out;
	}

out:	g_string_free(data, TRUE);
}
