/*   libgebr - GeBR Library
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
 *
 *   Inspired on Qt 4.3 version of QListenServer, by Trolltech
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "listensocket.h"
#include "socketprivate.h"
#include "streamsocketprivate.h"
#include "socketaddressprivate.h"

/*
 * prototypes
 */

static void __gebr_comm_listen_socket_new_connection(GebrCommListenSocket * listen_socket);

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
gebr_comm_listen_socket_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GebrCommListenSocket *self = (GebrCommListenSocket *) object;

	switch (property_id) {
	case MAX_PENDING_CONNECTIONS:
		gebr_comm_listen_socket_set_max_pending_connections(self, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gebr_comm_listen_socket_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	GebrCommListenSocket *self = (GebrCommListenSocket *) object;

	switch (property_id) {
	case MAX_PENDING_CONNECTIONS:
		g_value_set_uint(value, gebr_comm_listen_socket_get_max_pending_connections(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void gebr_comm_listen_socket_class_init(GebrCommListenSocketClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	/* virtual */
	klass->parent.new_connection = (typeof(klass->parent.new_connection)) __gebr_comm_listen_socket_new_connection;

	/* signals */
	object_signals[NEW_CONNECTION] = g_signal_new("new-connection", GEBR_COMM_LISTEN_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommListenSocketClass, new_connection), NULL, NULL,	/* acumulators */
						      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	/* properties */
	gobject_class->set_property = gebr_comm_listen_socket_set_property;
	gobject_class->get_property = gebr_comm_listen_socket_get_property;

	pspec = g_param_spec_uint("max-pending-connections",
				  "Max pending connections",
				  "Set/Get the max pending connections", 1 /* minimum value */ ,
				  UINT_MAX /* maximum value */ ,
				  30 /* default value */ ,
				  G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, MAX_PENDING_CONNECTIONS, pspec);
}

static void gebr_comm_listen_socket_init(GebrCommListenSocket * listen_socket)
{
	listen_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
}

G_DEFINE_TYPE(GebrCommListenSocket, gebr_comm_listen_socket, GEBR_COMM_SOCKET_TYPE)

/*
 * internal functions
 */
static void __gebr_comm_listen_socket_new_connection(GebrCommListenSocket * listen_socket)
{
	GebrCommSocketAddress peer_address;
	int client_sockfd, sockfd;

	sockfd = _gebr_comm_socket_get_fd(&listen_socket->parent);
	while ((client_sockfd = _gebr_comm_socket_address_accept(&peer_address,
								 listen_socket->parent.address_type, sockfd)) != -1) {
		GebrCommStreamSocket *stream_socket;

		if (g_slist_length(listen_socket->pending_connections) > listen_socket->max_pending_connections)
			break;

		/* create GebrCommStreamSocket */
		stream_socket =
		    _gebr_comm_stream_socket_new_connected(client_sockfd, listen_socket->parent.address_type);

		/* add to the list of pending connections and notify user */
		listen_socket->pending_connections = g_slist_append(listen_socket->pending_connections, stream_socket);
		g_signal_emit(listen_socket, object_signals[NEW_CONNECTION], 0);
	}
}

/*
 * user functions
 */

gboolean gebr_comm_listen_socket_is_local_port_available(guint16 port)
{
	int sockfd;
	struct sockaddr_in sockaddr_in;
	gboolean available;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	sockaddr_in = (struct sockaddr_in) {
		.sin_family = AF_INET,.sin_port = htons(port),.sin_addr = {
		INADDR_ANY}
	};
	available = !bind(sockfd, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) ? TRUE : FALSE;

	close(sockfd);
	return available;
}

GebrCommListenSocket *gebr_comm_listen_socket_new(void)
{
	return (GebrCommListenSocket *) g_object_new(GEBR_COMM_LISTEN_SOCKET_TYPE, NULL);
}

void gebr_comm_listen_socket_free(GebrCommListenSocket * listen_socket)
{
	g_slist_foreach(listen_socket->pending_connections, (GFunc) g_object_unref, NULL);
	g_slist_free(listen_socket->pending_connections);
	gebr_comm_socket_close(&listen_socket->parent);
	g_free(listen_socket);
}

gboolean gebr_comm_listen_socket_listen(GebrCommListenSocket * listen_socket, GebrCommSocketAddress * socket_address)
{
	int sockfd;
	struct sockaddr *sockaddr;
	gsize sockaddr_size;

	if (!gebr_comm_socket_address_get_is_valid(socket_address))
		return FALSE;

	/* initialization */
	sockfd = socket(_gebr_comm_socket_address_get_family(socket_address), SOCK_STREAM, 0);
	_gebr_comm_socket_init(&listen_socket->parent, sockfd, socket_address->type);
	listen_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
	gebr_comm_socket_set_blocking(&listen_socket->parent, FALSE);
	_gebr_comm_socket_enable_read_watch(&listen_socket->parent);
	/* pending connections */
	listen_socket->pending_connections = g_slist_alloc();
	gebr_comm_listen_socket_set_max_pending_connections(listen_socket, 30);

	/* bind and listen */
	_gebr_comm_socket_address_get_sockaddr(socket_address, &sockaddr, &sockaddr_size);
	if (bind(sockfd, sockaddr, sockaddr_size))
		return FALSE;
	if (listen(sockfd, listen_socket->max_pending_connections)) {
		listen_socket->parent.state = G_SOCKET_STATE_NOTLISTENING;
		return FALSE;
	}
	listen_socket->parent.state = G_SOCKET_STATE_LISTENING;

	return TRUE;
}

void gebr_comm_listen_socket_set_max_pending_connections(GebrCommListenSocket * listen_socket, guint number)
{
	if (!number)
		return;
	listen_socket->max_pending_connections = number;
}

guint gebr_comm_listen_socket_get_max_pending_connections(GebrCommListenSocket * listen_socket)
{
	return listen_socket->max_pending_connections;
}

GebrCommStreamSocket *gebr_comm_listen_socket_get_next_pending_connection(GebrCommListenSocket * listen_socket)
{
	GebrCommStreamSocket *stream_socket;
	GSList *link;

	/* get the first pending conn. */
	link = g_slist_last(listen_socket->pending_connections);
	if (link == NULL)
		return NULL;
	stream_socket = (GebrCommStreamSocket *) link->data;

	/* remove it from the list of pending */
	listen_socket->pending_connections = g_slist_remove_link(listen_socket->pending_connections, link);

	return stream_socket;
}

gboolean gebr_comm_listen_socket_get_has_pending_connections(GebrCommListenSocket * listen_socket)
{
	return (gboolean) g_slist_length(listen_socket->pending_connections);
}
