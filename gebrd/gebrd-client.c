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
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <glib/gi18n.h>

#include <libgebr/comm/gebr-comm-protocol.h>

#include "gebrd-client.h"
#include "gebrd.h"
#include "gebrd-server.h"

/*
 * Private functions
 */

static void client_disconnected(GebrCommProtocolSocket * socket, struct client *client);
static void client_old_parse_messages(GebrCommProtocolSocket * socket, struct client *client);


/*
 * Public functions
 */

void client_add(GebrCommStreamSocket * socket)
{
	struct client *client;

	client = g_new(struct client, 1);
	client->socket = gebr_comm_protocol_socket_new_from_socket(socket);
	client->display = g_string_new(NULL);

	gebrd.clients = g_list_prepend(gebrd.clients, client);
	g_signal_connect(client->socket, "disconnected", G_CALLBACK(client_disconnected), client);
	g_signal_connect(client->socket, "old-parse-messages", G_CALLBACK(client_old_parse_messages), client);

	gebrd_message(GEBR_LOG_DEBUG, "client_add");
}

void client_free(struct client *client)
{
	g_object_unref(client->socket);
	g_string_free(client->display, TRUE);
	g_free(client);
}

static void client_disconnected(GebrCommProtocolSocket * socket, struct client *client)
{
	gebrd_message(GEBR_LOG_DEBUG, "client_disconnected");

	gebrd.clients = g_list_remove(gebrd.clients, client);
	client_free(client);
}

static void client_old_parse_messages(GebrCommProtocolSocket * socket, struct client *client)
{
	if (server_parse_client_messages(client) == FALSE)
		client_disconnected(socket, client);
}
