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

#define UNIX_PATH_MAX 108
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "gsocketaddress.h"

/*
 * private functions
 */

static const int type_enum_to_size [] = {
	0,
	sizeof(struct sockaddr_in),
	sizeof(struct sockaddr_un)
};

static const int type_enum_to_family [] = {
	0, AF_INET, AF_UNIX
};

GSocketAddress
_g_socket_address_unknown(void)
{
	return (GSocketAddress) {
		.type = G_SOCKET_ADDRESS_TYPE_UNKNOWN
	};
}

gboolean
_g_socket_address_get_sockaddr(GSocketAddress * socket_address, struct sockaddr ** sockaddr, gsize * size)
{
	switch (socket_address->type) {
	case G_SOCKET_ADDRESS_TYPE_UNIX:
		*sockaddr = (struct sockaddr *)&socket_address->address.unix_sockaddr;
		*size = sizeof(socket_address->address.unix_sockaddr);
		return TRUE;
	case G_SOCKET_ADDRESS_TYPE_IPV4:
		*sockaddr = (struct sockaddr *)&socket_address->address.inet_sockaddr;
		*size = sizeof(socket_address->address.inet_sockaddr);
		return TRUE;
	default:
		*sockaddr = NULL;
		*size = 0;
		return FALSE;
	}
}

int
_g_socket_address_get_family(GSocketAddress * socket_address)
{
	return type_enum_to_family[socket_address->type];
}

typedef int (*sockname_function)(int sockfd, struct sockaddr * addr, socklen_t * addrlen);
static int
__g_socket_address_sockname_function(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd,
	sockname_function function)
{
	struct sockaddr *	sockaddr;
	socklen_t		addrlen;

	switch ((socket_address->type = type)) {
	case G_SOCKET_ADDRESS_TYPE_UNIX:
		sockaddr = (struct sockaddr *)&socket_address->address.unix_sockaddr;
		break;
	case G_SOCKET_ADDRESS_TYPE_IPV4:
		sockaddr = (struct sockaddr *)&socket_address->address.inet_sockaddr;
		break;
	default:
		return -1;
	}
	addrlen = type_enum_to_size[type];

	return function(sockfd, sockaddr, &addrlen);
}

int
_g_socket_address_getsockname(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd)
{
	return __g_socket_address_sockname_function(socket_address, type, sockfd, getsockname);
}

int
_g_socket_address_getpeername(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd)
{
	return __g_socket_address_sockname_function(socket_address, type, sockfd, getpeername);
}

int
_g_socket_address_accept(GSocketAddress * socket_address, enum GSocketAddressType type, int sockfd)
{
	return __g_socket_address_sockname_function(socket_address, type, sockfd, accept);
}

/*
 * library functions
 */

GSocketAddress
g_socket_address_unix(const gchar * path)
{
	GSocketAddress	socket_address;

	socket_address = (GSocketAddress) {
		.type = G_SOCKET_ADDRESS_TYPE_UNIX,
		.address.unix_sockaddr.sun_family = AF_UNIX,
	};
	strncpy(socket_address.address.unix_sockaddr.sun_path, path, UNIX_PATH_MAX);

	return socket_address;
}

GSocketAddress
g_socket_address_ipv4(const gchar * string, guint16 port)
{
	GSocketAddress	socket_address;
	struct in_addr	in_addr;

	if (inet_aton(string, &in_addr) == 0) {
		socket_address.type = G_SOCKET_ADDRESS_TYPE_UNKNOWN;
		goto out;
	}

	socket_address = (GSocketAddress) {
		.type = G_SOCKET_ADDRESS_TYPE_IPV4,
		.address.inet_sockaddr = (struct sockaddr_in) {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr = in_addr
		}
	};

out:	return socket_address;
}

GSocketAddress
g_socket_address_ipv4_local(guint16 port)
{
	return g_socket_address_ipv4("127.0.0.1", port);
}

gboolean
g_socket_address_get_is_valid(GSocketAddress * socket_address)
{
	return (gboolean)(socket_address->type != G_SOCKET_ADDRESS_TYPE_UNKNOWN);
}

const gchar *
g_socket_address_get_string(GSocketAddress * socket_address)
{
	switch (socket_address->type) {
	case G_SOCKET_ADDRESS_TYPE_UNIX:
		return inet_ntoa(socket_address->address.inet_sockaddr.sin_addr);
	case G_SOCKET_ADDRESS_TYPE_IPV4:
		return socket_address->address.unix_sockaddr.sun_path;
	default:
		return NULL;
	}
}

guint16
g_socket_address_get_ip_port(GSocketAddress * socket_address)
{
	switch (socket_address->type) {
	case G_SOCKET_ADDRESS_TYPE_IPV4:
		return ntohs(socket_address->address.inet_sockaddr.sin_port);
	default:
		return 0;
	}
}
