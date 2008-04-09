/*   GeBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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
#include <string.h>
#include <unistd.h>

#include <comm/protocol.h>

#include "client.h"
#include "gebrd.h"
#include "support.h"
#include "server.h"

/*
 * Private functions
 */

static void
client_disconnected(GTcpSocket * tcp_socket, struct client * client);

static void
client_read(GTcpSocket * tcp_socket, struct client * client);

static void
client_error(GTcpSocket * tcp_socket, enum GSocketError error, struct client * client);

/*
 * Public functions
 */

void
client_add(GTcpSocket * tcp_socket)
{
	struct client *	client;

	client = g_malloc(sizeof(struct client));
	*client = (struct client) {
		.tcp_socket = tcp_socket,
		.protocol = protocol_new(),
		.display = g_string_new(NULL),
		.mcookie = g_string_new(NULL),
		.address = g_string_new(NULL)
	};

	gebrd.clients = g_list_prepend(gebrd.clients, client);
	g_signal_connect(tcp_socket, "disconnected",
			G_CALLBACK(client_disconnected), client);
	g_signal_connect(tcp_socket, "error",
			 G_CALLBACK(client_error), client);
	g_signal_connect(tcp_socket, "ready-read",
			G_CALLBACK(client_read), client);

	gebrd_message(LOG_DEBUG, "client_add");
}

void
client_free(struct client * client)
{
	g_socket_close(G_SOCKET(client->tcp_socket));
	protocol_free(client->protocol);
	g_string_free(client->display, TRUE);
	g_string_free(client->mcookie, TRUE);
	g_string_free(client->address, TRUE);
	g_free(client);
}

gboolean
client_is_local(struct client * client)
{
	return strcmp(client->address->str, "127.0.0.1") == 0
		? TRUE : FALSE;
}

static void
client_disconnected(GTcpSocket * tcp_socket, struct client * client)
{
	gebrd_message(LOG_DEBUG, "client_disconnected");

	gebrd.clients = g_list_remove(gebrd.clients, client);
	client_free(client);
}

static void
client_read(GTcpSocket * tcp_socket, struct client * client)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));
	if (protocol_receive_data(client->protocol, data) == FALSE) {
		client_disconnected(tcp_socket, client);
		goto out;
	}
	if (server_parse_client_messages(client) == FALSE) {
		client_disconnected(tcp_socket, client);
		goto out;
	}

	gebrd_message(LOG_DEBUG, "client_read %s", data->str);

out:	g_string_free(data, TRUE);
}

static void
client_error(GTcpSocket * tcp_socket, enum GSocketError error, struct client * client)
{
	if (error == G_SOCKET_ERROR_UNKNOWN)
		gebrd_message(LOG_ERROR, _("unk"), error, client->address->str);
	gebrd_message(LOG_ERROR, _("Connection error '%s' on server '%s'"), error, client->address->str);
}
