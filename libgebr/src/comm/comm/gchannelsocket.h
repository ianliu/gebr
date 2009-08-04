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
 *   Inspired on Qt 4.3 version of QChannelSocket, by Trolltech
 */

#ifndef __LIBGEBR_COMM_GCHANNELSOCKET_H
#define __LIBGEBR_COMM_GCHANNELSOCKET_H

#include "gsocket.h"
#include "gstreamsocket.h"
#include "gsocketaddress.h"

G_BEGIN_DECLS

GType
g_channel_socket_get_type(void);

#define G_CHANNEL_SOCKET_TYPE		(g_channel_socket_get_type())
#define G_CHANNEL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_CHANNEL_SOCKET_TYPE, GChannelSocket))
#define G_CHANNEL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_CHANNEL_SOCKET_TYPE, GChannelSocketClass))
#define G_IS_CHANNEL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_CHANNEL_SOCKET_TYPE))
#define G_IS_CHANNEL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_CHANNEL_SOCKET_TYPE))
#define G_CHANNEL_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_CHANNEL_SOCKET_TYPE, GChannelSocketClass))

typedef struct _GChannelSocket	GChannelSocket;
typedef struct _GChannelSocketClass	GChannelSocketClass;

struct _GChannelSocket {
	GSocket			parent;

	GSocketAddress		forward_address;
};
struct _GChannelSocketClass {
	GSocketClass		parent;
};

/*
 * user functions
 */

GChannelSocket *
g_channel_socket_new(void);

void
g_channel_socket_free(GChannelSocket *);

gboolean
g_channel_socket_start(GChannelSocket * channel_socket, GSocketAddress * listen_address,
	GSocketAddress * forward_address);

GSocketAddress
g_channel_socket_get_forward_address(GChannelSocket * channel_socket);

G_END_DECLS

#endif //__LIBGEBR_COMM_GCHANNELSOCKET_H
