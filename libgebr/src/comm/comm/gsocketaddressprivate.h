/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
 *   Inspired on Qt 4.3 version of QSocketAddress, by Trolltech
 */

#ifndef __LIBGEBR_COMM_GSOCKETADDRESSPRIVATE_H
#define __LIBGEBR_COMM_GSOCKETADDRESSPRIVATE_H

#include <glib.h>

#include "gsocketaddress.h"

G_BEGIN_DECLS

GSocketAddress
_g_socket_address_unknown(void);

gboolean
_g_socket_address_get_sockaddr(GSocketAddress * socket_address, struct sockaddr ** sockaddr, gsize * size);

int
_g_socket_address_get_family(GSocketAddress * socket_address);

int
_g_socket_address_getsockname(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd);

int
_g_socket_address_getpeername(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd);

int
_g_socket_address_accept(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd);

G_END_DECLS

#endif // 
