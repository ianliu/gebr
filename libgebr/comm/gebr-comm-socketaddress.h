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

#ifndef __GEBR_COMM_SOCKET_ADDRESS_H
#define __GEBR_COMM_SOCKET_ADDRESS_H

#include <glib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>

G_BEGIN_DECLS

typedef struct _GebrCommSocketAddress GebrCommSocketAddress;

enum GebrCommSocketAddressType {
	GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN = 0,
	GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4,
	GEBR_COMM_SOCKET_ADDRESS_TYPE_UNIX
};

struct _GebrCommSocketAddress {
	enum GebrCommSocketAddressType type;
	union {
		struct sockaddr_un unix_sockaddr;
		struct sockaddr_in inet_sockaddr;
	} address;
};

GebrCommSocketAddress gebr_comm_socket_address_unix(const gchar * filepath);

GebrCommSocketAddress gebr_comm_socket_address_ipv4(const gchar * ip, guint16 port);

GebrCommSocketAddress gebr_comm_socket_address_ipv4_local(guint16 port);

GebrCommSocketAddress gebr_comm_socket_address_parse_from_string(const gchar *address);

enum GebrCommSocketAddressType gebr_comm_socket_address_get_type(GebrCommSocketAddress * socket_address);

gboolean gebr_comm_socket_address_get_is_valid(GebrCommSocketAddress * socket_address);

const gchar *gebr_comm_socket_address_get_string(GebrCommSocketAddress * socket_address);

guint16 gebr_comm_socket_address_get_ip_port(GebrCommSocketAddress * socket_address);

G_END_DECLS
#endif				// __GEBR_COMM_SOCKET_ADDRESS_H
