/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_COMM_G_STREAM_SOCKET_H
#define __LIBGEBR_COMM_G_STREAM_SOCKET_H

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
	GSocket		parent;
};
struct _GStreamSocketClass {
	GSocketClass	parent;

	/* signals */
	void		(*connected)(GStreamSocket * self);
	void		(*disconnected)(GStreamSocket * self);
};

/*
 * user functions
 */

GStreamSocket *
g_stream_socket_new(void);

void
g_stream_socket_connect(GStreamSocket * stream_socket, GSocketAddress * socket_address, guint16 port, gboolean wait);

GSocketAddress *
g_stream_socket_peer_address(GStreamSocket * stream_socket);

guint16
g_stream_socket_peer_port(GStreamSocket * stream_socket);

G_END_DECLS

#endif //__LIBGEBR_COMM_G_STREAM_SOCKET_H
