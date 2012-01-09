/*
 * gebr-comm-daemon.h
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

#ifndef __GEBR_COMM_DAEMON_H__
#define __GEBR_COMM_DAEMON_H__

#include <glib.h>
#include <glib-object.h>
#include "gebr-comm-server.h"


#define GEBR_COMM_TYPE_DAEMON            (gebr_comm_daemon_get_type ())
#define GEBR_COMM_DAEMON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_TYPE_DAEMON, GebrCommDaemon))
#define GEBR_COMM_DAEMON_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GEBR_COMM_TYPE_DAEMON, GebrCommDaemonIface))
#define GEBR_COMM_IS_DAEMON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_TYPE_DAEMON))
#define GEBR_COMM_DAEMON_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEBR_COMM_TYPE_DAEMON, GebrCommDaemonIface))


typedef struct _GebrCommDaemon GebrCommDaemon;
typedef struct _GebrCommDaemonIface GebrCommDaemonIface;

struct _GebrCommDaemonIface {
	GTypeInterface g_iface;

	GebrCommServer * (*get_server) (GebrCommDaemon *daemon);

	gint (*get_n_running_jobs) (GebrCommDaemon *daemon);
};

GType gebr_comm_daemon_get_type(void) G_GNUC_CONST;

GebrCommServer *gebr_comm_daemon_get_server(GebrCommDaemon *daemon);

gint gebr_comm_daemon_get_n_running_jobs(GebrCommDaemon *daemon);


#endif /* __GEBR_COMM_DAEMON_H__ */
