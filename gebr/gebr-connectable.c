/*
 * gebr-connectable.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-connectable.h"

GType
gebr_connectable_get_type(void)
{
	static GType connectable_type = 0;

	if (!connectable_type)
		connectable_type =
			g_type_register_static_simple(G_TYPE_INTERFACE, "GebrConnectable",
						      sizeof(GebrConnectableIface),
						      NULL, 0, NULL, 0);
	return connectable_type;
}

void
gebr_connectable_connect(GebrConnectable *connectable,
			 const gchar     *address)
{
	GEBR_CONNECTABLE_GET_IFACE(connectable)->connect(connectable, address);
}

void
gebr_connectable_disconnect(GebrConnectable *connectable,
			    const gchar     *address)
{
	GEBR_CONNECTABLE_GET_IFACE(connectable)->disconnect(connectable, address);
}
