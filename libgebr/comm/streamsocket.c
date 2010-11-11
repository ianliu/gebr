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
 *   Inspired on Qt 4.3 version of QStreamSocket, by Trolltech
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

#include "streamsocket.h"
#include "streamsocketprivate.h"
#include "socketprivate.h"
#include "hostinfo.h"
#include "socketaddressprivate.h"

/*
 * prototypes
 */

static void __gebr_comm_stream_socket_connected(GebrCommStreamSocket * stream_socket);

static void __gebr_comm_stream_socket_disconnected(GebrCommStreamSocket * stream_socket);

/*
 * gobject stuff
 */

enum {
	CONNECTED,
	DISCONNECTED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void gebr_comm_stream_socket_class_init(GebrCommStreamSocketClass * class)
{
	/* virtual */
	class->parent.connected = (typeof(class->parent.connected)) __gebr_comm_stream_socket_connected;
	class->parent.disconnected = (typeof(class->parent.disconnected)) __gebr_comm_stream_socket_disconnected;

	/* signals */
	object_signals[CONNECTED] = g_signal_new("connected", GEBR_COMM_STREAM_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommStreamSocketClass, connected), NULL, NULL,	/* acumulators */
						 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[DISCONNECTED] = g_signal_new("disconnected", GEBR_COMM_STREAM_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommStreamSocketClass, disconnected), NULL, NULL,	/* acumulators */
						    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gebr_comm_stream_socket_init(GebrCommStreamSocket * stream_socket)
{
	stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
}

G_DEFINE_TYPE(GebrCommStreamSocket, gebr_comm_stream_socket, GEBR_COMM_SOCKET_TYPE)

/*
 * internal functions
 */
static void
__gebr_comm_stream_socket_init(GebrCommStreamSocket * stream_socket, int sockfd, enum GebrCommSocketAddressType type,
			       gboolean nonblocking)
{
	/* initialization */
	_gebr_comm_socket_init(&stream_socket->parent, sockfd, type);

	gebr_comm_socket_set_blocking(&stream_socket->parent, !nonblocking);
}

static void __gebr_comm_stream_socket_connected(GebrCommStreamSocket * stream_socket)
{
	stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_CONNECTED;
	g_signal_emit(stream_socket, object_signals[CONNECTED], 0);
}

static void __gebr_comm_stream_socket_disconnected(GebrCommStreamSocket * stream_socket)
{
	stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
	g_signal_emit(stream_socket, object_signals[DISCONNECTED], 0);
}

// static void
// __gebr_comm_stream_socket_lookup(GSocketInfo * host_info, GebrCommStreamSocket * stream_socket)
// {
//      GebrCommSocketAddress * socket_address;
// 
//      if (gebr_comm_host_info_error(host_info)) {
//              _gebr_comm_socket_emit_error(&stream_socket->parent, GEBR_COMM_SOCKET_ERROR_LOOKUP);
//              stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
//              goto out;
//      }
// 
//      socket_address = gebr_comm_host_info_first_address(host_info);
//      gebr_comm_stream_socket_connect(stream_socket, socket_address, ntohs(stream_socket->parent.sockaddr_in.sin_port), FALSE);
// 
// out: gebr_comm_host_info_free(host_info);
// }

/*
 * private functions
 */

GebrCommStreamSocket *_gebr_comm_stream_socket_new_connected(int fd, enum GebrCommSocketAddressType address_type)
{
	GebrCommStreamSocket *stream_socket;

	/* initialization */
	stream_socket = (GebrCommStreamSocket *) g_object_new(GEBR_COMM_STREAM_SOCKET_TYPE, NULL);
	__gebr_comm_stream_socket_init(stream_socket, fd, address_type, TRUE);
	stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_CONNECTED;
	_gebr_comm_socket_enable_read_watch(&stream_socket->parent);

	return stream_socket;
}

/*
 * user functions
 */

GebrCommStreamSocket *gebr_comm_stream_socket_new(void)
{
	return (GebrCommStreamSocket *) g_object_new(GEBR_COMM_STREAM_SOCKET_TYPE, NULL);
}

gboolean
gebr_comm_stream_socket_connect(GebrCommStreamSocket * stream_socket, GebrCommSocketAddress * socket_address, gboolean wait)
{
	g_return_val_if_fail(GEBR_COMM_IS_STREAM_SOCKET(stream_socket), FALSE);

	struct sockaddr *sockaddr;
	gsize sockaddr_size;
	int sockfd;
	gboolean ret;

	if (!gebr_comm_socket_address_get_is_valid(socket_address))
		return FALSE;

	/* initialization */
	ret = TRUE;
	sockfd = socket(_gebr_comm_socket_address_get_family(socket_address), SOCK_STREAM, 0);
	__gebr_comm_stream_socket_init(stream_socket, sockfd, socket_address->type, !wait);
	stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
	stream_socket->parent.last_error = GEBR_COMM_SOCKET_ERROR_NONE;

	/* watches */
	_gebr_comm_socket_enable_read_watch(&stream_socket->parent);
	_gebr_comm_socket_enable_write_watch(&stream_socket->parent);

	_gebr_comm_socket_address_get_sockaddr(socket_address, &sockaddr, &sockaddr_size);
	if (!connect(sockfd, sockaddr, sockaddr_size)) {
		stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_CONNECTED;
		__gebr_comm_stream_socket_connected(stream_socket);
	} else {
		switch (errno) {
		case EINPROGRESS:
			stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_CONNECTING;
			break;
		case ECONNREFUSED:
			stream_socket->parent.last_error = GEBR_COMM_SOCKET_ERROR_CONNECTION_REFUSED;
			ret = FALSE;
			break;
		case ETIMEDOUT:
			stream_socket->parent.last_error = GEBR_COMM_SOCKET_ERROR_SERVER_TIMED_OUT;
			ret = FALSE;
		}
	}

	gebr_comm_socket_set_blocking(&stream_socket->parent, wait);

	return ret;
}

// void
// gebr_comm_stream_socket_connect_by_name(GebrCommStreamSocket * stream_socket, GString * hostname, guint16 port)
// {
//	g_return_if_fail(GEBR_COMM_IS_STREAM_SOCKET(stream_socket));
//
//      stream_socket->parent.sockaddr_in.sin_port = htons(port);
// 
//      stream_socket->parent.state = GEBR_COMM_SOCKET_STATE_LOOKINGUP;
//      stream_socket->parent.last_error = GEBR_COMM_SOCKET_ERROR_NONE;
//      gebr_comm_host_info_lookup(hostname, (GSocketInfoFunc)__gebr_comm_stream_socket_lookup, stream_socket);
// }

void gebr_comm_stream_socket_disconnect(GebrCommStreamSocket * stream_socket)
{
	g_return_if_fail(GEBR_COMM_IS_STREAM_SOCKET(stream_socket));

	if (stream_socket->parent.state >= GEBR_COMM_SOCKET_STATE_CONNECTING)
		_gebr_comm_socket_close(&stream_socket->parent);
	__gebr_comm_stream_socket_disconnected(stream_socket);
	stream_socket->parent.last_error = GEBR_COMM_SOCKET_ERROR_NONE;
}

GebrCommSocketAddress gebr_comm_stream_socket_peer_address(GebrCommStreamSocket * stream_socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_STREAM_SOCKET(stream_socket), _gebr_comm_socket_address_unknown());

	if (stream_socket->parent.state != GEBR_COMM_SOCKET_STATE_CONNECTED)
		return _gebr_comm_socket_address_unknown();

	GebrCommSocketAddress peer_address;
	_gebr_comm_socket_address_getpeername(&peer_address, stream_socket->parent.address_type,
					      _gebr_comm_socket_get_fd(&stream_socket->parent));
	return peer_address;
}
