/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_COMM_G_TCP_SOCKET_H
#define __LIBGEBR_COMM_G_TCP_SOCKET_H

#include "gsocket.h"
#include "ghostaddress.h"

G_BEGIN_DECLS

GType
g_tcp_socket_get_type(void);

#define G_TCP_SOCKET_TYPE		(g_tcp_socket_get_type())
#define G_TCP_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TCP_SOCKET_TYPE, GTcpSocket))
#define G_TCP_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_TCP_SOCKET_TYPE, GTcpSocketClass))
#define G_IS_TCP_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TCP_SOCKET_TYPE))
#define G_IS_TCP_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_TCP_SOCKET_TYPE))
#define G_TCP_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TCP_SOCKET_TYPE, GTcpSocketClass))

typedef struct _GTcpSocket	GTcpSocket;
typedef struct _GTcpSocketClass	GTcpSocketClass;

struct _GTcpSocket {
	GSocket		parent;
};
struct _GTcpSocketClass {
	GSocketClass	parent;

	/* signals */
	void		(*connected)(GTcpSocket * self);
	void		(*disconnected)(GTcpSocket * self);
};

/*
 * user functions
 */

GTcpSocket *
g_tcp_socket_new(void);

void
g_tcp_socket_connect(GTcpSocket * tcp_socket, GHostAddress * host_address, guint16 port);

void
g_tcp_socket_connect_by_name(GTcpSocket * tcp_socket, GString * hostname, guint16 port);

GHostAddress *
g_tcp_socket_peer_address(GTcpSocket * tcp_socket);

guint16
g_tcp_socket_peer_port(GTcpSocket * tcp_socket);

G_END_DECLS

#endif //__LIBGEBR_COMM_G_TCP_SOCKET_H
