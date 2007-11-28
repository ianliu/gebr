/*   G�BR Daemon - Process and control execution of flows
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
 *   Inspired on Qt 4.3 version of QTcpSocket, by Trolltech
 */

#ifndef __G_TCP_SOCKET_PRIVATE_H
#define __G_TCP_SOCKET_PRIVATE_H

/**
 * Create an already connected socket
 * Used by GTcpServer when a new connection is accepted.
 */
GTcpSocket *
_g_tcp_socket_new_connected(int fd);

#endif //__G_TCP_SOCKET_PRIVATE_H
