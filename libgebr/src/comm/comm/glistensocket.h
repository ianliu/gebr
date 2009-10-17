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
gebr_comm_listen_socket_get_type(void);

#define GEBR_COMM_LISTEN_SOCKET_TYPE		(gebr_comm_listen_socket_get_type())
#define GEBR_COMM_LISTEN_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_LISTEN_SOCKET_TYPE, GebrCommListenSocket))
#define GEBR_COMM_LISTEN_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_LISTEN_SOCKET_TYPE, GebrCommListenSocketClass))
#define GEBR_COMM_IS_LISTEN_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_LISTEN_SOCKET_TYPE))
#define GEBR_COMM_IS_LISTEN_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_LISTEN_SOCKET_TYPE))
#define GEBR_COMM_LISTEN_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_LISTEN_SOCKET_TYPE, GebrCommListenSocketClass))

typedef struct _GebrCommListenSocket	GebrCommListenSocket;
typedef struct _GebrCommListenSocketClass	GebrCommListenSocketClass;

struct _GebrCommListenSocket {
	GebrCommSocket		parent;

	guint		max_pending_connections;
	GSList *	pending_connections;
};
struct _GebrCommListenSocketClass {
	GebrCommSocketClass	parent;

	/* signals */
	void		(*new_connection)(GebrCommListenSocket * self);
};

/*
 * user functions
 */

gboolean
gebr_comm_listen_socket_is_local_port_available(guint16 port);

GebrCommListenSocket *
gebr_comm_listen_socket_new(void);

void
gebr_comm_listen_socket_free(GebrCommListenSocket *);

gboolean
gebr_comm_listen_socket_listen(GebrCommListenSocket * listen_socket, GebrCommSocketAddress * socket_address);

void
gebr_comm_listen_socket_set_max_pending_connections(GebrCommListenSocket * listen_socket, guint number);

guint
gebr_comm_listen_socket_get_max_pending_connections(GebrCommListenSocket * listen_socket);

GStreamSocket *
gebr_comm_listen_socket_get_next_pending_connection(GebrCommListenSocket * listen_socket);

gboolean
gebr_comm_listen_socket_get_has_pending_connections(GebrCommListenSocket * listen_socket);

G_END_DECLS

#endif //__LIBGEBR_COMM_GLISTENSOCKET_H
