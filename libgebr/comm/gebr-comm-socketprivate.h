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
 *   Inspired on Qt 4.3 version of QAbstractSocket, by Trolltech
 */

#ifndef __GEBR_COMM_SOCKETPRIVATE_H
#define __GEBR_COMM_SOCKETPRIVATE_H

#include "gebr-comm-socket.h"
#include "gebr-comm-socketaddress.h"

G_BEGIN_DECLS

void _gebr_comm_socket_init(GebrCommSocket * socket, int fd, enum GebrCommSocketAddressType address_type);

void _gebr_comm_socket_close(GebrCommSocket * socket);

int _gebr_comm_socket_get_fd(GebrCommSocket * socket);

void _gebr_comm_socket_enable_read_watch(GebrCommSocket * socket);

void _gebr_comm_socket_enable_write_watch(GebrCommSocket * socket);

void _gebr_comm_socket_emit_error(GebrCommSocket * socket, enum GebrCommSocketError error);

G_END_DECLS
#endif				//__GEBR_COMM_SOCKETPRIVATE_H
