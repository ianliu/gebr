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

#ifndef __GEBR_COMM_CHANNEL_SOCKET_H
#define __GEBR_COMM_CHANNEL_SOCKET_H

#include "socket.h"
#include "streamsocket.h"
#include "socketaddress.h"

/**
 * \struct GebrCommChannelSocket channelsocket.h comm/channelsocket.h
 * \brief A socket that channel all data to another address
 * \see channelsocket.h
 */ 

/**
 * \file channelsocket.h
 * \brief A socket that channel all data to another address
 *
 * This classes channels all data from a given #GebrCommStreamSocket to another address.
 * At first, a connection is made to this address. If it successful, a pipe-like data flow
 * is established between these two sockets.
 */

G_BEGIN_DECLS GType gebr_comm_channel_socket_get_type(void);

#define GEBR_COMM_CHANNEL_SOCKET_TYPE		(gebr_comm_channel_socket_get_type())
#define GEBR_COMM_CHANNEL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_CHANNEL_SOCKET_TYPE, GebrCommChannelSocket))
#define GEBR_COMM_CHANNEL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_CHANNEL_SOCKET_TYPE, GebrCommChannelSocketClass))
#define GEBR_COMM_IS_CHANNEL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_CHANNEL_SOCKET_TYPE))
#define GEBR_COMM_IS_CHANNEL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_CHANNEL_SOCKET_TYPE))
#define GEBR_COMM_CHANNEL_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_CHANNEL_SOCKET_TYPE, GebrCommChannelSocketClass))

typedef struct _GebrCommChannelSocket GebrCommChannelSocket;
typedef struct _GebrCommChannelSocketClass GebrCommChannelSocketClass;

/**
 * Internal struct data, should not be used directly.
 */
struct _GebrCommChannelSocket {
	GebrCommSocket parent;

	GebrCommSocketAddress forward_address;
};
struct _GebrCommChannelSocketClass {
	GebrCommSocketClass parent;
};

/*
 * user functions
 */

/**
 * Creates a new instance of #GebrCommChannelSocket and return it.
 * \see gebr_comm_channel_socket_start gebr_comm_channel_socket_free 
 */
GebrCommChannelSocket *gebr_comm_channel_socket_new(void);

/**
 * Free an instance of #GebrCommChannelSocket.
 * Closes the socket created from the specified address.
 * \see gebr_comm_channel_socket_new
 */
void gebr_comm_channel_socket_free(GebrCommChannelSocket *);

/**
 * Iniciates connection to \p forward_address 
 */
gboolean
gebr_comm_channel_socket_start(GebrCommChannelSocket * channel_socket, GebrCommSocketAddress * listen_address,
			       GebrCommSocketAddress * forward_address);

GebrCommSocketAddress gebr_comm_channel_socket_get_forward_address(GebrCommChannelSocket * channel_socket);

G_END_DECLS
#endif				//__GEBR_COMM_CHANNEL_SOCKET_H
