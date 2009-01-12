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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "gsocketaddress.h"

GSocketAddress *
g_socket_address_new(const gchar * string, enum GSocketAddressType type)
{
	GSocketAddress *	new;

	new = g_malloc(sizeof(struct _GSocketAddress));
	if (new == NULL)
		goto out;
	g_socket_address_set_string(new, string, type);

out:	return new;
}

void
g_socket_address_free(GSocketAddress * socket_address)
{
	if (socket_address->type == G_SOCKET_ADDRESS_TYPE_UNIX)
		g_free(socket_address->address.un_addr.sun_path);
	g_free(socket_address);
}

enum GSocketAddressType
g_socket_address_get_type(GSocketAddress * socket_address)
{
	return socket_address->type;
}

gchar *
g_socket_address_get_string(GSocketAddress * socket_address)
{
	if (socket_address->type != G_SOCKET_ADDRESS_TYPE_UNIX)
		return inet_ntoa(socket_address->address.in_addr);
	else
		return socket_address->address.un_addr.sun_path;
}

void
g_socket_address_set_ipv4(GSocketAddress * socket_address, guint16 address)
{
	socket_address->type = G_SOCKET_ADDRESS_TYPE_IPV4;
	socket_address->address.in_addr.s_addr = htons(address);
}

void
g_socket_address_set_ipv6(GSocketAddress * socket_address, guint32 address)
{
	socket_address->type = G_SOCKET_ADDRESS_TYPE_IPV6;
	socket_address->address.in_addr.s_addr = htonl(address);
}

void
g_socket_address_set_unix(GSocketAddress * socket_address, const gchar * path)
{
	socket_address->address.un_addr.sun_family = AF_UNIX;
	strcpy(socket_address->address.un_addr.sun_path, path);
}

void
g_socket_address_set_string(GSocketAddress * socket_address, const gchar * string, enum GSocketAddressType type)
{
	socket_address->type = type;
	if (type != G_SOCKET_ADDRESS_TYPE_UNIX) {
		if (inet_aton(string, &socket_address->address.in_addr))
			socket_address->address.in_addr.s_addr = 0;
	} else
		g_socket_address_set_unix(socket_address, string);
}
