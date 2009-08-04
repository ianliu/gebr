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
 *   Inspired on Qt 4.3 version of QSocketAddress, by Trolltech
 */

#ifndef __LIBGEBR_COMM_GSOCKETADDRESS_H
#define __LIBGEBR_COMM_GSOCKETADDRESS_H

#include <glib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>

G_BEGIN_DECLS

typedef struct _GSocketAddress	GSocketAddress;

enum GSocketAddressType {
	G_SOCKET_ADDRESS_TYPE_UNKNOWN = 0,
	G_SOCKET_ADDRESS_TYPE_IPV4,
	G_SOCKET_ADDRESS_TYPE_UNIX
};

struct _GSocketAddress {
	enum GSocketAddressType		type;
	union {
		struct sockaddr_un	unix_sockaddr;
		struct sockaddr_in	inet_sockaddr;
	} address;
};

GSocketAddress
g_socket_address_unix(const gchar * string);

GSocketAddress
g_socket_address_ipv4(const gchar * string, guint16 port);

GSocketAddress
g_socket_address_ipv4_local(guint16 port);

gboolean
g_socket_address_get_is_valid(GSocketAddress * socket_address);

const gchar *
g_socket_address_get_string(GSocketAddress * socket_address);

guint16
g_socket_address_get_ip_port(GSocketAddress * socket_address);

G_END_DECLS

#endif // __LIBGEBR_COMM_GSOCKETADDRESS_H
