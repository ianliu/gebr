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
 *   Inspired on Qt 4.3 version of QTcpSocket, by Trolltech
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

#include "gtcpsocket.h"
#include "gtcpsocketprivate.h"
#include "gsocketprivate.h"
#include "ghostinfo.h"

/*
 * prototypes
 */

static void
__g_tcp_socket_connected(GTcpSocket * tcp_socket);

static void
__g_tcp_socket_disconnected(GTcpSocket * tcp_socket);

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
g_tcp_socket_class_init(GTcpSocketClass * class)
{
	/* virtual */
	class->parent.connected = __g_tcp_socket_connected;
	class->parent.disconnected = __g_tcp_socket_disconnected;

	/* signals */
	object_signals[CONNECTED] = g_signal_new ("connected",
		G_TCP_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GTcpSocketClass, connected),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[DISCONNECTED] = g_signal_new ("disconnected",
		G_TCP_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GTcpSocketClass, disconnected),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
g_tcp_socket_init(GTcpSocket * tcp_socket)
{
	tcp_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
}

G_DEFINE_TYPE(GTcpSocket, g_tcp_socket, G_SOCKET_TYPE)

/*
 * internal functions
 */

static void
__g_tcp_socket_init(GTcpSocket * tcp_socket, int sockfd)
{
	GError *	error;

	/* initialization */
	_g_socket_init(&tcp_socket->parent, sockfd);

	/* for nonblocking call of connect */
	error = NULL;
	g_io_channel_set_flags(tcp_socket->parent.io_channel, G_IO_FLAG_NONBLOCK, &error);
}

static void
__g_tcp_socket_connected(GTcpSocket * tcp_socket)
{
	tcp_socket->parent.state = G_SOCKET_STATE_CONNECTED;
	g_signal_emit(tcp_socket, object_signals[CONNECTED], 0);
}

static void
__g_tcp_socket_disconnected(GTcpSocket * tcp_socket)
{
	tcp_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
	g_signal_emit(tcp_socket, object_signals[DISCONNECTED], 0);
}

static void
__g_tcp_socket_lookup(GHostInfo * host_info, GTcpSocket * tcp_socket)
{
	GHostAddress *	host_address;

	if (g_host_info_ok(host_info) == FALSE) {
		_g_socket_emit_error(&tcp_socket->parent, G_SOCKET_ERROR_LOOKUP);
		tcp_socket->parent.state = G_SOCKET_STATE_UNCONNECTED;
		goto out;
	}

	host_address = g_host_info_first_address(host_info);
g_print("address: %s\n", g_host_address_to_string(host_address));
	g_tcp_socket_connect(tcp_socket, host_address, ntohs(tcp_socket->parent.sockaddr_in.sin_port));

out:	g_host_info_free(host_info);
}

/*
 * private functions
 */

GTcpSocket *
_g_tcp_socket_new_connected(int fd)
{
	GTcpSocket *	tcp_socket;

	/* initialization */
	tcp_socket = (GTcpSocket*)g_object_new(G_TCP_SOCKET_TYPE, NULL);
	__g_tcp_socket_init(tcp_socket, fd);
	tcp_socket->parent.state = G_SOCKET_STATE_CONNECTED;
	_g_socket_enable_read_watch(&tcp_socket->parent);

	return tcp_socket;
}

/*
 * user functions
 */

GTcpSocket *
g_tcp_socket_new(void)
{
	return (GTcpSocket*)g_object_new(G_TCP_SOCKET_TYPE, NULL);
}

void
g_tcp_socket_connect(GTcpSocket * tcp_socket, GHostAddress * host_address, guint16 port)
{
	int	sockfd;

	/* initialization */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	__g_tcp_socket_init(tcp_socket, sockfd);
	tcp_socket->parent.state = G_SOCKET_STATE_CONNECTING;

	/* set address and connect */
	tcp_socket->parent.sockaddr_in = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = host_address->in_addr
	};

	/* watches */
	_g_socket_enable_read_watch(&tcp_socket->parent);
	_g_socket_enable_write_watch(&tcp_socket->parent);

	if (!connect(sockfd, (struct sockaddr *)&tcp_socket->parent.sockaddr_in,
			sizeof(tcp_socket->parent.sockaddr_in))) {
		tcp_socket->parent.state = G_SOCKET_STATE_CONNECTED;
		__g_tcp_socket_connected(tcp_socket);
	}
}

void
g_tcp_socket_connect_by_name(GTcpSocket * tcp_socket, GString * hostname, guint16 port)
{
	tcp_socket->parent.sockaddr_in.sin_port = htons(port);

	tcp_socket->parent.state = G_SOCKET_STATE_LOOKINGUP;
	g_host_info_lookup(hostname, (GHostInfoFunc)__g_tcp_socket_lookup, tcp_socket);
}

GHostAddress *
g_tcp_socket_peer_address(GTcpSocket * tcp_socket)
{
	if (tcp_socket->parent.state != G_SOCKET_STATE_CONNECTED)
		return NULL;

	GHostAddress *		host_address;
	struct sockaddr_in	sockaddr_in;
	socklen_t		namelen;

	namelen = sizeof(sockaddr_in);
	getsockname(_g_socket_get_fd(&tcp_socket->parent), (struct sockaddr *)&sockaddr_in, &namelen);

	/* TODO: handle other protocols */
	host_address = g_host_address_new();
	*host_address = (struct _GHostAddress) {
		.type = G_HOST_ADDRESS_TYPE_IPV4,
		.in_addr.s_addr = sockaddr_in.sin_addr.s_addr
	};

	return host_address;
}

guint16
g_tcp_socket_peer_port(GTcpSocket * tcp_socket)
{
	return tcp_socket->parent.sockaddr_in.sin_port;
}
