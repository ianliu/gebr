/*   GeBR - An environment for seismic processing.
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

#include <glib.h>

#include "server.h"
#include "gebrclient.h"
#include "support.h"
#include "client.h"

/*
 * Internal functions
 */

static void
server_log_message(enum log_message_type type, const gchar * message)
{
	gebr_client_message(type, message);
}

static GString *
server_ssh_login(const gchar * title, const gchar * message)
{
	GString *	password;

// 	TODO:

	return password;
}

static gboolean
server_ssh_question(const gchar * title, const gchar * message)
{
	gboolean	yes;

// 	TODO:

	return yes;
}

static void
server_disconnected(GTcpSocket * tcp_socket, struct server * server)
{
	server->comm->protocol->logged = FALSE;
// 	TODO:
}

/*
 * Public functions
 */

struct server *
server_new(const gchar * address)
{
	static const struct comm_server_ops	ops = {
		.log_message = server_log_message,
		.ssh_login = server_ssh_login,
		.ssh_question = server_ssh_question,
		.parse_messages = (typeof(ops.parse_messages))client_parse_server_messages
	};
	struct server *				server;

	server = g_malloc(sizeof(struct server));
	server->comm = comm_server_new(address, &ops);
	server->comm->user_data = server;

	g_signal_connect(server->comm->tcp_socket, "disconnected",
		G_CALLBACK(server_disconnected), server);

	comm_server_connect(server->comm);

	return server;
}

void
server_free(struct server * server)
{
	comm_server_free(server->comm);
	g_free(server);
}
