/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#ifndef __GSOCKETPRIVATE_H
#define __GSOCKETPRIVATE_H

void
_g_socket_init(GSocket * socket, int fd);

void
_g_socket_close(GSocket * socket);

int
_g_socket_get_fd(GSocket * socket);

void
_g_socket_enable_read_watch(GSocket * socket);

void
_g_socket_enable_write_watch(GSocket * socket);

void
_g_socket_emit_error(GSocket * socket, enum GSocketError error);

#endif //__GSOCKETPRIVATE_H
