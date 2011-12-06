/*
 * gebr-server.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR core team (www.gebrproject.com)
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

#ifndef __GEBR_SERVER_H__
#define __GEBR_SERVER_H__

#include <glib-object.h>
#include "gebr-connectable.h"
#include <libgebr/comm/gebr-comm.h>

G_BEGIN_DECLS

#define GEBR_TYPE_DAEMON_SERVER            (gebr_daemon_server_get_type ())
#define GEBR_DAEMON_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_DAEMON_SERVER, GebrDaemonServer))
#define GEBR_DAEMON_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_DAEMON_SERVER, GebrDaemonServerClass))
#define GEBR_IS_DAEMON_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_DAEMON_SERVER))
#define GEBR_IS_DAEMON_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_DAEMON_SERVER))
#define GEBR_DAEMON_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_DAEMON_SERVER, GebrDaemonServerClass))

typedef struct _GebrDaemonServer GebrDaemonServer;
typedef struct _GebrDaemonServerPriv GebrDaemonServerPriv;
typedef struct _GebrDaemonServerClass GebrDaemonServerClass;

struct _GebrDaemonServer {
	GObject parent;
	GebrDaemonServerPriv *priv;
};

struct _GebrDaemonServerClass {
	GObjectClass parent_class;
};

GType gebr_daemon_server_get_type(void) G_GNUC_CONST;

GebrDaemonServer *gebr_daemon_server_new(GebrConnectable *connectable,
					 const gchar *address,
					 GebrCommServerState state);

/**
 * gebr_daemon_server_get_address:
 *
 * Returns: The raw address of this @server, do not free this string. See
 * gebr_daemon_server_get_display_address().
 */
const gchar *gebr_daemon_server_get_address(GebrDaemonServer *server);

/**
 * gebr_daemon_server_get_display_address:
 *
 * The display address is a function that translate "special" addresses like
 * 127.0.0.1 to "Local server" and the empty address to "Auto-choose". This
 * method should be used when the address is to be shown to the user.
 *
 * Returns: A newly allocated string, free with g_free().
 */
gchar *gebr_daemon_server_get_display_address(GebrDaemonServer *server);

void gebr_daemon_server_set_state(GebrDaemonServer *server,
				  GebrCommServerState state);

GebrCommServerState gebr_daemon_server_get_state(GebrDaemonServer *server);

/**
 * gebr_daemon_server_connect:
 *
 * Asynchronously connect to @server.
 */
void gebr_daemon_server_connect(GebrDaemonServer *server);

/**
 * gebr_daemon_server_disconnect:
 *
 * Asynchronously disconnect from @server.
 */
void gebr_daemon_server_disconnect(GebrDaemonServer *server);

gboolean gebr_daemon_server_is_autochoose(GebrDaemonServer *daemon);

void gebr_daemon_server_set_tags(GebrDaemonServer *daemon, gchar **tags);

GList *gebr_daemon_server_get_tags(GebrDaemonServer *daemon);

gboolean gebr_daemon_server_has_tag(GebrDaemonServer *daemon, const gchar *tag);

gboolean gebr_daemon_server_get_ac(GebrDaemonServer *daemon);

gboolean gebr_daemon_server_set_ac(GebrDaemonServer *daemon, 
				   gboolean ac);
G_END_DECLS

#endif /* __GEBR_SERVER_H__ */
