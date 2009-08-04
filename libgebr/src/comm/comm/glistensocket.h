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
 *   Inspired on Qt 4.3 version of QListenSocket, by Trolltech
 */

#ifndef __LIBGEBR_COMM_GLISTENSOCKET_H
#define __LIBGEBR_COMM_GLISTENSOCKET_H

#include "gsocket.h"
#include "gstreamsocket.h"
#include "gsocketaddress.h"

G_BEGIN_DECLS

GType
g_listen_socket_get_type(void);

#define G_LISTEN_SOCKET_TYPE		(g_listen_socket_get_type())
#define G_LISTEN_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_LISTEN_SOCKET_TYPE, GListenSocket))
#define G_LISTEN_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_LISTEN_SOCKET_TYPE, GListenSocketClass))
#define G_IS_LISTEN_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_LISTEN_SOCKET_TYPE))
#define G_IS_LISTEN_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_LISTEN_SOCKET_TYPE))
#define G_LISTEN_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_LISTEN_SOCKET_TYPE, GListenSocketClass))

typedef struct _GListenSocket	GListenSocket;
typedef struct _GListenSocketClass	GListenSocketClass;

struct _GListenSocket {
	GSocket		parent;

	guint		max_pending_connections;
	GSList *	pending_connections;
};
struct _GListenSocketClass {
	GSocketClass	parent;

	/* signals */
	void		(*new_connection)(GListenSocket * self);
};

/*
 * user functions
 */

gboolean
g_listen_socket_is_local_port_available(guint16 port);

GListenSocket *
g_listen_socket_new(void);

void
g_listen_socket_free(GListenSocket *);

gboolean
g_listen_socket_listen(GListenSocket * listen_socket, GSocketAddress * socket_address);

void
g_listen_socket_set_max_pending_connections(GListenSocket * listen_socket, guint number);

guint
g_listen_socket_get_max_pending_connections(GListenSocket * listen_socket);

GStreamSocket *
g_listen_socket_get_next_pending_connection(GListenSocket * listen_socket);

gboolean
g_listen_socket_get_has_pending_connections(GListenSocket * listen_socket);

G_END_DECLS

#endif //__LIBGEBR_COMM_GLISTENSOCKET_H
