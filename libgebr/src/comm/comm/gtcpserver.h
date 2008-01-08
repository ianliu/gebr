/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_COMM_GTCPSERVER_H
#define __LIBGEBR_COMM_GTCPSERVER_H

#include "gsocket.h"
#include "gtcpsocket.h"
#include "ghostaddress.h"

G_BEGIN_DECLS

GType
g_tcp_server_get_type(void);

#define G_TCP_SERVER_TYPE		(g_tcp_server_get_type())
#define G_TCP_SERVER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TCP_SERVER_TYPE, GTcpServer))
#define G_TCP_SERVER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_TCP_SERVER_TYPE, GTcpServerClass))
#define G_IS_TCP_SERVER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TCP_SERVER_TYPE))
#define G_IS_TCP_SERVER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_TCP_SERVER_TYPE))
#define G_TCP_SERVER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TCP_SERVER_TYPE, GTcpServerClass))

typedef struct _GTcpServer	GTcpServer;
typedef struct _GTcpServerClass	GTcpServerClass;

struct _GTcpServer {
	GSocket		parent;

	guint		max_pending_connections;
	GSList *	pending_connections;
};
struct _GTcpServerClass {
	GSocketClass	parent;

	/* signals */
	void		(*new_connection)(GTcpServer * self);
};

/*
 * user functions
 */

gboolean
g_tcp_server_is_local_port_available(guint16 port);

GTcpServer *
g_tcp_server_new(void);

void
g_tcp_server_free(GTcpServer *);

gboolean
g_tcp_server_listen(GTcpServer * tcp_server, GHostAddress * host_address, guint16 port);

guint16
g_tcp_server_server_port(GTcpServer * tcp_server);

void
g_tcp_server_set_max_pending_connections(GTcpServer * tcp_server, guint number);

guint
g_tcp_server_get_max_pending_connections(GTcpServer * tcp_server);

GTcpSocket *
g_tcp_server_get_next_pending_connection(GTcpServer * tcp_server);

gboolean
g_tcp_server_get_has_pending_connections(GTcpServer * tcp_server);

G_END_DECLS

#endif //__LIBGEBR_COMM_GTCPSERVER_H
