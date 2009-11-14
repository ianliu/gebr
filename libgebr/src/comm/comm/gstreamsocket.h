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

#ifndef __COMM_G_STREAM_SOCKET_H
#define __COMM_G_STREAM_SOCKET_H

#include "gsocket.h"
#include "gsocketaddress.h"

G_BEGIN_DECLS

GType
g_stream_socket_get_type(void);

#define G_STREAM_SOCKET_TYPE		(g_stream_socket_get_type())
#define G_STREAM_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_STREAM_SOCKET_TYPE, GStreamSocket))
#define G_STREAM_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_STREAM_SOCKET_TYPE, GStreamSocketClass))
#define G_IS_STREAM_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_STREAM_SOCKET_TYPE))
#define G_IS_STREAM_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_STREAM_SOCKET_TYPE))
#define G_STREAM_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_STREAM_SOCKET_TYPE, GStreamSocketClass))

typedef struct _GStreamSocket	GStreamSocket;
typedef struct _GStreamSocketClass	GStreamSocketClass;

struct _GStreamSocket {
	GebrCommSocket		parent;
};
struct _GStreamSocketClass {
	GebrCommSocketClass	parent;

	/* signals */
	void		(*connected)(GStreamSocket * self);
	void		(*disconnected)(GStreamSocket * self);
};

/*
 * user functions
 */

GStreamSocket *
g_stream_socket_new(void);

gboolean
g_stream_socket_connect(GStreamSocket * stream_socket, GebrCommSocketAddress * socket_address, gboolean wait);

void
g_stream_socket_disconnect(GStreamSocket * stream_socket);

GebrCommSocketAddress
g_stream_socket_peer_address(GStreamSocket * stream_socket);

G_END_DECLS

#endif //__COMM_G_STREAM_SOCKET_H
