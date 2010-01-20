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

#include "socketaddress.h"

/*
 * private functions
 */

static const int type_enum_to_size[] = {
	0,
	sizeof(struct sockaddr_in),
	sizeof(struct sockaddr_un)
};

static const int type_enum_to_family[] = {
	0, AF_INET, AF_UNIX
};

GebrCommSocketAddress _gebr_comm_socket_address_unknown(void)
{
	return (GebrCommSocketAddress) {
	.type = GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN};
}

gboolean
_gebr_comm_socket_address_get_sockaddr(GebrCommSocketAddress * socket_address, struct sockaddr ** sockaddr,
				       gsize * size)
{
	switch (socket_address->type) {
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_UNIX:
		*sockaddr = (struct sockaddr *)&socket_address->address.unix_sockaddr;
		*size = sizeof(socket_address->address.unix_sockaddr);
		return TRUE;
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4:
		*sockaddr = (struct sockaddr *)&socket_address->address.inet_sockaddr;
		*size = sizeof(socket_address->address.inet_sockaddr);
		return TRUE;
	default:
		*sockaddr = NULL;
		*size = 0;
		return FALSE;
	}
}

int _gebr_comm_socket_address_get_family(GebrCommSocketAddress * socket_address)
{
	return type_enum_to_family[socket_address->type];
}

typedef int (*sockname_function) (int sockfd, struct sockaddr * addr, socklen_t * addrlen);
static int
__gebr_comm_socket_address_sockname_function(GebrCommSocketAddress * socket_address,
					     enum GebrCommSocketAddressType type, int sockfd,
					     sockname_function function)
{
	struct sockaddr *sockaddr;
	socklen_t addrlen;

	switch ((socket_address->type = type)) {
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_UNIX:
		sockaddr = (struct sockaddr *)&socket_address->address.unix_sockaddr;
		break;
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4:
		sockaddr = (struct sockaddr *)&socket_address->address.inet_sockaddr;
		break;
	default:
		return -1;
	}
	addrlen = type_enum_to_size[type];

	return function(sockfd, sockaddr, &addrlen);
}

int
_gebr_comm_socket_address_getsockname(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type,
				      int sockfd)
{
	return __gebr_comm_socket_address_sockname_function(socket_address, type, sockfd, getsockname);
}

int
_gebr_comm_socket_address_getpeername(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type,
				      int sockfd)
{
	return __gebr_comm_socket_address_sockname_function(socket_address, type, sockfd, getpeername);
}

int
_gebr_comm_socket_address_accept(GebrCommSocketAddress * socket_address, enum GebrCommSocketAddressType type,
				 int sockfd)
{
	return __gebr_comm_socket_address_sockname_function(socket_address, type, sockfd, accept);
}

/*
 * library functions
 */

GebrCommSocketAddress gebr_comm_socket_address_unix(const gchar * path)
{
	GebrCommSocketAddress socket_address;

	socket_address = (GebrCommSocketAddress) {
	.type = GEBR_COMM_SOCKET_ADDRESS_TYPE_UNIX,.address.unix_sockaddr.sun_family = AF_UNIX,};
	strncpy(socket_address.address.unix_sockaddr.sun_path, path, UNIX_PATH_MAX);

	return socket_address;
}

GebrCommSocketAddress gebr_comm_socket_address_ipv4(const gchar * string, guint16 port)
{
	GebrCommSocketAddress socket_address;
	struct in_addr in_addr;

	if (inet_aton(string, &in_addr) == 0) {
		socket_address.type = GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN;
		goto out;
	}

	socket_address = (GebrCommSocketAddress) {
		.type = GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4,.address.inet_sockaddr = (struct sockaddr_in) {
		.sin_family = AF_INET,.sin_port = htons(port),.sin_addr = in_addr}
	};

 out:	return socket_address;
}

GebrCommSocketAddress gebr_comm_socket_address_ipv4_local(guint16 port)
{
	return gebr_comm_socket_address_ipv4("127.0.0.1", port);
}

gboolean gebr_comm_socket_address_get_is_valid(GebrCommSocketAddress * socket_address)
{
	return (gboolean) (socket_address->type != GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN);
}

const gchar *gebr_comm_socket_address_get_string(GebrCommSocketAddress * socket_address)
{
	switch (socket_address->type) {
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_UNIX:
		return inet_ntoa(socket_address->address.inet_sockaddr.sin_addr);
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4:
		return socket_address->address.unix_sockaddr.sun_path;
	default:
		return NULL;
	}
}

guint16 gebr_comm_socket_address_get_ip_port(GebrCommSocketAddress * socket_address)
{
	switch (socket_address->type) {
	case GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4:
		return ntohs(socket_address->address.inet_sockaddr.sin_port);
	default:
		return 0;
	}
}
