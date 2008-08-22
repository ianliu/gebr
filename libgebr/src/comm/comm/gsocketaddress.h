/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <sys/un.h>
#include <netinet/ip.h>

G_BEGIN_DECLS

typedef struct _GSocketAddress	GSocketAddress;

enum GSocketAddressType {
	G_SOCKET_ADDRESS_TYPE_IPV4,
	G_SOCKET_ADDRESS_TYPE_IPV6,
	G_SOCKET_ADDRESS_TYPE_UNIX
};

struct _GSocketAddress {
	union {
		struct in_addr		in_addr;
		struct sockaddr_un	un_addr;
	} address;
	enum GSocketAddressType	type;
};

GSocketAddress *
g_socket_address_new(const gchar * string, enum GSocketAddressType type);

void
g_socket_address_free(GSocketAddress * socket_address);

enum GSocketAddressType
g_socket_address_get_type(GSocketAddress * socket_address);

gchar *
g_socket_address_get_string(GSocketAddress * socket_address);

void
g_socket_address_set_ipv4(GSocketAddress * socket_address, guint16 address);

void
g_socket_address_set_ipv6(GSocketAddress * socket_address, guint32 address);

void
g_socket_address_set_unix(GSocketAddress * socket_address, const gchar * path);

void
g_socket_address_set_string(GSocketAddress * socket_address, const gchar * string, enum GSocketAddressType type);

G_END_DECLS

#endif // __LIBGEBR_COMM_GSOCKETADDRESS_H
