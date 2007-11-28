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
 *
 *   Inspired on Qt 4.3 version of QTcpServer, by Trolltech
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "gtcpserver.h"
#include "gsocketprivate.h"
#include "gtcpsocketprivate.h"

/*
 * prototypes
 */

static void
__g_tcp_server_new_connection(GTcpServer * tcp_server);

/*
 * gobject stuff
 */

enum {
	MAX_PENDING_CONNECTIONS = 1
};

enum {
	NEW_CONNECTION,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
g_tcp_server_set_property(GObject *	object,
			guint		property_id,
			const GValue *	value,
			GParamSpec *	pspec)
{
	GTcpServer *	self = (GTcpServer *)object;

	switch (property_id) {
	case MAX_PENDING_CONNECTIONS:
		g_tcp_server_set_max_pending_connections(self, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
g_tcp_server_get_property(GObject *	object,
			guint		property_id,
			GValue *	value,
			GParamSpec *	pspec)
{
	GTcpServer *	self = (GTcpServer *)object;

	switch (property_id) {
	case MAX_PENDING_CONNECTIONS:
		g_value_set_uint(value, g_tcp_server_get_max_pending_connections(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
g_tcp_server_class_init(GTcpServerClass * class)
{
	GObjectClass *	gobject_class = G_OBJECT_CLASS(class);
	GParamSpec *	pspec;

	/* virtual */
	class->parent.new_connection = __g_tcp_server_new_connection;

	/* signals */
	object_signals[NEW_CONNECTION] = g_signal_new("new-connection",
		G_TCP_SERVER_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GTcpServerClass, new_connection),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	/* properties */
	gobject_class->set_property = g_tcp_server_set_property;
	gobject_class->get_property = g_tcp_server_get_property;

	pspec = g_param_spec_uint("max-pending-connections",
				"Max pending connections",
				"Set/Get the max pending connections",
				1		/* minimum value */,
				UINT_MAX	/* maximum value */,
				30		/* default value */,
				G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class,
					MAX_PENDING_CONNECTIONS,
					pspec);
}

static void
g_tcp_server_init(GTcpServer * tcp_server)
{
	tcp_server->parent.state = G_SOCKET_STATE_NOTLISTENING;
}

G_DEFINE_TYPE(GTcpServer, g_tcp_server, G_SOCKET_TYPE)

/*
 * internal functions
 */

static void
__g_tcp_server_new_connection(GTcpServer * tcp_server)
{
	struct sockaddr_in	peer_sockaddr_in;
	socklen_t		peer_socklen;
	int			client_sockfd, sockfd;

	sockfd = _g_socket_get_fd(&tcp_server->parent);
	peer_socklen = sizeof(peer_sockaddr_in);
	while ((client_sockfd = accept(sockfd, (struct sockaddr *)&peer_sockaddr_in, &peer_socklen)) != -1) {
		GTcpSocket *	tcp_socket;

		if (g_slist_length(tcp_server->pending_connections) > tcp_server->max_pending_connections)
			break;

		/* create GTcpSocket */
		tcp_socket = _g_tcp_socket_new_connected(client_sockfd);

		/* add to the list of pending connections and notify user */
		tcp_server->pending_connections = g_slist_append(tcp_server->pending_connections, tcp_socket);
		g_signal_emit(tcp_server, object_signals[NEW_CONNECTION], 0);
	}
}

/*
 * user functions
 */

gboolean
g_tcp_server_is_local_port_available(guint16 port)
{
	int			sockfd;
	struct sockaddr_in	sockaddr_in;
	gboolean		available;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	sockaddr_in = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = {INADDR_ANY}
	};
	available = !bind(sockfd, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) ? TRUE : FALSE;

	close(sockfd);
	return available;
}

GTcpServer *
g_tcp_server_new(void)
{
	return (GTcpServer*)g_object_new(G_TCP_SERVER_TYPE, NULL);
}

void
g_tcp_server_free(GTcpServer * tcp_server)
{
	g_slist_foreach(tcp_server->pending_connections, (GFunc)g_object_unref, NULL);
	g_slist_free(tcp_server->pending_connections);
	g_socket_close(&tcp_server->parent);
	g_free(tcp_server);
}

gboolean
g_tcp_server_listen(GTcpServer * tcp_server, GHostAddress * host_address, guint16 port)
{
	int		sockfd;
	GError *	error;
	socklen_t	namelen;

	/* initialization */
	error = NULL;
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	_g_socket_init(&tcp_server->parent, sockfd);
	tcp_server->parent.state = G_SOCKET_STATE_NOTLISTENING;
	/* for nonblocking call of accept */
	g_io_channel_set_flags(tcp_server->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
	/* pending connections */
	tcp_server->pending_connections = g_slist_alloc();
	g_tcp_server_set_max_pending_connections(tcp_server, 30);
	/* watch */
	_g_socket_enable_read_watch(&tcp_server->parent);

	/* bind and listen */
	tcp_server->parent.sockaddr_in = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = host_address->in_addr
	};
	if (bind(sockfd, (struct sockaddr *)&tcp_server->parent.sockaddr_in, sizeof(tcp_server->parent.sockaddr_in)))
		return FALSE;
	if (listen(sockfd, tcp_server->max_pending_connections)) {
		tcp_server->parent.state = G_SOCKET_STATE_NOTLISTENING;
		return FALSE;
	}

	/* get "real" server address */
	namelen = sizeof(tcp_server->parent.sockaddr_in);
	getsockname(sockfd, (struct sockaddr *)&tcp_server->parent.sockaddr_in, &namelen);

	tcp_server->parent.state = G_SOCKET_STATE_LISTENING;
	return TRUE;
}

guint16
g_tcp_server_server_port(GTcpServer * tcp_server)
{
	return (guint16)ntohs(tcp_server->parent.sockaddr_in.sin_port);
}

void
g_tcp_server_set_max_pending_connections(GTcpServer * tcp_server, guint number)
{
	if (!number)
		return;
	tcp_server->max_pending_connections = number;
}

guint
g_tcp_server_get_max_pending_connections(GTcpServer * tcp_server)
{
	return tcp_server->max_pending_connections;
}

GTcpSocket *
g_tcp_server_get_next_pending_connection(GTcpServer * tcp_server)
{
	GTcpSocket *	tcp_socket;
	GSList *	link;

	/* get the first pending conn. */
	link = g_slist_last(tcp_server->pending_connections);
	if (link == NULL)
		return NULL;
	tcp_socket = (GTcpSocket*)link->data;

	/* remove it from the list of pending */
	tcp_server->pending_connections = g_slist_remove_link(tcp_server->pending_connections, link);

	return tcp_socket;
}

gboolean
g_tcp_server_get_has_pending_connections(GTcpServer * tcp_server)
{
	return (gboolean)g_slist_length(tcp_server->pending_connections);
}
