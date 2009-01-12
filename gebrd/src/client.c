/*   GeBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <comm/protocol.h>

#include "client.h"
#include "gebrd.h"
#include "support.h"
#include "server.h"

/*
 * Private functions
 */

static void
client_disconnected(GStreamSocket * stream_socket, struct client * client);

static void
client_read(GStreamSocket * stream_socket, struct client * client);

static void
client_error(GStreamSocket * stream_socket, enum GSocketError error, struct client * client);

/*
 * Public functions
 */

void
client_add(GStreamSocket * stream_socket)
{
	struct client *	client;

	client = g_malloc(sizeof(struct client));
	*client = (struct client) {
		.stream_socket = stream_socket,
		.protocol = protocol_new(),
		.display = g_string_new(NULL),
	};

	gebrd.clients = g_list_prepend(gebrd.clients, client);
	g_signal_connect(stream_socket, "disconnected",
			G_CALLBACK(client_disconnected), client);
	g_signal_connect(stream_socket, "error",
			 G_CALLBACK(client_error), client);
	g_signal_connect(stream_socket, "ready-read",
			G_CALLBACK(client_read), client);

	gebrd_message(LOG_DEBUG, "client_add");
}

void
client_free(struct client * client)
{
	g_socket_close(G_SOCKET(client->stream_socket));
	protocol_free(client->protocol);
	g_string_free(client->display, TRUE);
	g_free(client);
}

static void
client_disconnected(GStreamSocket * stream_socket, struct client * client)
{
	gebrd_message(LOG_DEBUG, "client_disconnected");

	gebrd.clients = g_list_remove(gebrd.clients, client);
	client_free(client);
}

static void
client_read(GStreamSocket * stream_socket, struct client * client)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(stream_socket));
	if (protocol_receive_data(client->protocol, data) == FALSE) {
		client_disconnected(stream_socket, client);
		goto out;
	}
	if (server_parse_client_messages(client) == FALSE) {
		client_disconnected(stream_socket, client);
		goto out;
	}

	gebrd_message(LOG_DEBUG, "client_read %s", data->str);

out:	g_string_free(data, TRUE);
}

static void
client_error(GStreamSocket * stream_socket, enum GSocketError error, struct client * client)
{
	gebrd_message(LOG_ERROR, _("Connection error '%s' on client '%s'"), error, strerror(error), client->protocol->hostname->str);
}
