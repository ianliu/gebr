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

#ifndef __GHOSTADDRESS_H
#define __GHOSTADDRESS_H

#include <glib.h>

#include <netinet/ip.h>

G_BEGIN_DECLS

typedef struct _GHostAddress	GHostAddress;

enum GHostAddressType {
	G_HOST_ADDRESS_TYPE_IPV4,
	G_HOST_ADDRESS_TYPE_IPV6,
	G_HOST_ADDRESS_TYPE_UNKNOWN,
};

struct _GHostAddress {
	struct in_addr		in_addr;
	enum GHostAddressType	type;
};

GHostAddress *
g_host_address_new(void);

void
g_host_address_free(GHostAddress * host_address);

enum GHostAddressType
g_host_address_get_type(GHostAddress * host_address);

gboolean
g_host_address_valid(GHostAddress * host_address);

gchar *
g_host_address_to_string(GHostAddress * host_address);

void
g_host_address_set_ipv4(GHostAddress * host_address, guint16 address);

void
g_host_address_set_ipv6(GHostAddress * host_address, guint32 address);

void
g_host_address_set_ipv4_string(GHostAddress * host_address, const gchar * string);

void
g_host_address_set_ipv6_string(GHostAddress * host_address, const gchar * string);

G_END_DECLS

#endif // __GHOSTADDRESS_H
