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
 *   Inspired on Qt 4.3 version of QHostAddress, by Trolltech
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "ghostaddress.h"

GHostAddress *
g_host_address_new(void)
{
	GHostAddress *	new;

	new = g_malloc(sizeof(struct _GHostAddress));
	if (new == NULL)
		goto out;
	*new = (struct _GHostAddress) {
		.in_addr.s_addr = 0,
		.type = G_HOST_ADDRESS_TYPE_UNKNOWN
	};

out:	return new;
}

void
g_host_address_free(GHostAddress * host_address)
{
	g_free(host_address);
}

enum GHostAddressType
g_host_address_get_type(GHostAddress * host_address)
{
	return host_address->type;
}

gboolean
g_host_address_valid(GHostAddress * host_address)
{
	return (host_address->in_addr.s_addr) ? TRUE : FALSE;
}

gchar *
g_host_address_to_string(GHostAddress * host_address)
{
	if (!host_address->type == G_HOST_ADDRESS_TYPE_UNKNOWN)
		return NULL;
	return inet_ntoa(host_address->in_addr);
}

void
g_host_address_set_ipv4(GHostAddress * host_address, guint16 address)
{
	host_address->type = G_HOST_ADDRESS_TYPE_IPV4;
	host_address->in_addr.s_addr = htons(address);
}

void
g_host_address_set_ipv6(GHostAddress * host_address, guint32 address)
{
	host_address->type = G_HOST_ADDRESS_TYPE_IPV6;
	host_address->in_addr.s_addr = htonl(address);
}

void
g_host_address_set_ipv4_string(GHostAddress * host_address, const gchar * string)
{
	host_address->type = G_HOST_ADDRESS_TYPE_IPV4;
	if (inet_aton(string, &host_address->in_addr))
		host_address->in_addr.s_addr = 0;
}

void
g_host_address_set_ipv6_string(GHostAddress * host_address, const gchar * string)
{
	host_address->type = G_HOST_ADDRESS_TYPE_IPV6;
	if (inet_aton(string, &host_address->in_addr))
		host_address->in_addr.s_addr = 0;
}
