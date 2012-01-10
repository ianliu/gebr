/*
 * gebr-comm-daemon.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

#include "gebr-comm-daemon.h"

GType
gebr_comm_daemon_get_type(void)
{
	static GType type = 0;

	if (!type)
		type = g_type_register_static_simple(G_TYPE_INTERFACE,
						     "GebrCommDaemon",
						     sizeof(GebrCommDaemonIface),
						     NULL, 0, NULL, 0);
	return type;
}

GebrCommServer *
gebr_comm_daemon_get_server(GebrCommDaemon *daemon)
{
	return GEBR_COMM_DAEMON_GET_IFACE(daemon)->get_server(daemon);
}

gint
gebr_comm_daemon_get_n_running_jobs(GebrCommDaemon *daemon)
{
	return GEBR_COMM_DAEMON_GET_IFACE(daemon)->get_n_running_jobs(daemon);
}

const gchar *
gebr_comm_daemon_get_hostname(GebrCommDaemon *daemon)
{
	return GEBR_COMM_DAEMON_GET_IFACE(daemon)->get_hostname(daemon);
}
