/*   libgebr - GeBR Library
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
 *
 *   Inspired on Qt 4.3 version of QStreamSocket, by Trolltech
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

#include "gstreamsocket.h"
#include "gstreamsocketprivate.h"
#include "gsocketprivate.h"
#include "ghostinfo.h"

/*
 * prototypes
 */

static void
__g_stream_socket_connected(GStreamSocket * stream_socket);

static void
__g_stream_socket_disconnected(GStreamSocket * stream_socket);

/*
 * gobject stuff
 */

enum {
	CONNECTED,
	DISCONNECTED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
g_stream_socket_class_init(GStreamSocketClass * class)
{
	/* virtual */
	class->parent.connected = (typeof(class->parent.connected))__g_stream_socket_connected;
	class->parent.disconnected = (typeof(class->parent.disconnected))__g_stream_socket_disconnected;

	/* signals */
	object_signals[CONNECTED] = g_signal_new ("connected",
		G_STREAM_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GStreamSocketClass, connected),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[DISCONNECTED] = g_signal_new ("disconnected",
		G_STREAM_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GStreamSocketClass, disconnected),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
g_stream_socket_init(GStreamSocket * stream_socket)
{
	stream_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
}

G_DEFINE_TYPE(GStreamSocket, g_stream_socket, G_SOCKET_TYPE)

/*
 * internal functions
 */

static void
__g_stream_socket_init(GStreamSocket * stream_socket, int sockfd, gboolean nonblocking)
{
	/* initialization */
	_g_socket_init(&stream_socket->parent, sockfd);

	if (nonblocking == TRUE) {
		GError *	error;

		error = NULL;
		g_io_channel_set_flags(stream_socket->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
	}
}

static void
__g_stream_socket_connected(GStreamSocket * stream_socket)
{
	stream_socket->parent.state = G_SOCKET_STATE_CONNECTED;
	g_signal_emit(stream_socket, object_signals[CONNECTED], 0);
}

static void
__g_stream_socket_disconnected(GStreamSocket * stream_socket)
{
	stream_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
	g_signal_emit(stream_socket, object_signals[DISCONNECTED], 0);
}

// static void
// __g_stream_socket_lookup(GSocketInfo * host_info, GStreamSocket * stream_socket)
// {
// 	GSocketAddress *	socket_address;
// 
// 	if (g_host_info_error(host_info)) {
// 		_g_socket_emit_error(&stream_socket->parent, G_SOCKET_ERROR_LOOKUP);
// 		stream_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
// 		goto out;
// 	}
// 
// 	socket_address = g_host_info_first_address(host_info);
// 	g_stream_socket_connect(stream_socket, socket_address, ntohs(stream_socket->parent.sockaddr_in.sin_port), FALSE);
// 
// out:	g_host_info_free(host_info);
// }

/*
 * private functions
 */

GStreamSocket *
_g_stream_socket_new_connected(int fd)
{
	GStreamSocket *	stream_socket;

	/* initialization */
	stream_socket = (GStreamSocket*)g_object_new(G_STREAM_SOCKET_TYPE, NULL);
	__g_stream_socket_init(stream_socket, fd, TRUE);
	stream_socket->parent.state = G_SOCKET_STATE_CONNECTED;
	_g_socket_enable_read_watch(&stream_socket->parent);

	return stream_socket;
}

/*
 * user functions
 */

GStreamSocket *
g_stream_socket_new(void)
{
	return (GStreamSocket*)g_object_new(G_STREAM_SOCKET_TYPE, NULL);
}

void
g_stream_socket_connect(GStreamSocket * stream_socket, GSocketAddress * socket_address, guint16 port, gboolean wait)
{
	int	sockfd;

	/* initialization */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	__g_stream_socket_init(stream_socket, sockfd, !wait);
	stream_socket->parent.state = G_SOCKET_STATE_CONNECTING;
	stream_socket->parent.last_error = G_SOCKET_ERROR_NONE;

	/* set address and connect */
	stream_socket->parent.sockaddr_in = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = socket_address->address.in_addr
	};

	/* watches */
	_g_socket_enable_read_watch(&stream_socket->parent);
	_g_socket_enable_write_watch(&stream_socket->parent);

	if (!connect(sockfd, (struct sockaddr *)&stream_socket->parent.sockaddr_in,
			sizeof(stream_socket->parent.sockaddr_in))) {
		stream_socket->parent.state = G_SOCKET_STATE_CONNECTED;
		__g_stream_socket_connected(stream_socket);
	}

	/* no more blocking calls */
	if (wait) {
		GError *	error;

		error = NULL;
		g_io_channel_set_flags(stream_socket->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
	}
}

// void
// g_stream_socket_connect_by_name(GStreamSocket * stream_socket, GString * hostname, guint16 port)
// {
// 	stream_socket->parent.sockaddr_in.sin_port = htons(port);
// 
// 	stream_socket->parent.state = G_SOCKET_STATE_LOOKINGUP;
// 	stream_socket->parent.last_error = G_SOCKET_ERROR_NONE;
// 	g_host_info_lookup(hostname, (GSocketInfoFunc)__g_stream_socket_lookup, stream_socket);
// }

void
g_stream_socket_disconnect(GStreamSocket * stream_socket)
{
	if (stream_socket->parent.state >= G_SOCKET_STATE_CONNECTING)
		_g_socket_close(&stream_socket->parent);
	__g_stream_socket_disconnected(stream_socket);
	stream_socket->parent.last_error = G_SOCKET_ERROR_NONE;
}

GSocketAddress *
g_stream_socket_peer_address(GStreamSocket * stream_socket)
{
	if (stream_socket->parent.state != G_SOCKET_STATE_CONNECTED)
		return NULL;

	GSocketAddress *		socket_address;
	struct sockaddr_in	sockaddr_in;
	socklen_t		namelen;

	namelen = sizeof(sockaddr_in);
	getsockname(_g_socket_get_fd(&stream_socket->parent), (struct sockaddr *)&sockaddr_in, &namelen);

	/* TODO: handle other protocols */
	socket_address = g_socket_address_new("", G_SOCKET_ADDRESS_TYPE_IPV4);
	g_socket_address_set_ipv4(socket_address, sockaddr_in.sin_addr.s_addr);

	return socket_address;
}

guint16
g_stream_socket_peer_port(GStreamSocket * stream_socket)
{
	return stream_socket->parent.sockaddr_in.sin_port;
}
