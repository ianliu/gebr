/*
 * gebr-maestro-server.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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

#ifndef __GEBR_MAESTRO_H__
#define __GEBR_MAESTRO_H__

#include <glib-object.h>

#include "gebr-daemon-server.h"

G_BEGIN_DECLS

#define GEBR_TYPE_MAESTRO_SERVER            (gebr_maestro_server_get_type ())
#define GEBR_MAESTRO_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServer))
#define GEBR_MAESTRO_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServerClass))
#define GEBR_IS_MAESTRO_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_MAESTRO_SERVER))
#define GEBR_IS_MAESTRO_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_MAESTRO_SERVER))
#define GEBR_MAESTRO_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServerClass))

typedef struct _GebrMaestroServer GebrMaestroServer;
typedef struct _GebrMaestroServerPriv GebrMaestroServerPriv;
typedef struct _GebrMaestroServerClass GebrMaestroServerClass;

struct _GebrMaestroServerClass {
	GObjectClass parent_class;
};

struct _GebrMaestroServer {
	GObject parent;
	GebrMaestroServerPriv *priv;
};

GType gebr_maestro_server_get_type(void) G_GNUC_CONST;

GebrMaestroServer *gebr_maestro_server_new(const gchar *addr);

void gebr_maestro_add_daemon(GebrMaestroServer *maestro,
			     GebrDaemonServer *daemon);

GebrDaemonServer *gebr_maestro_get_daemon(GebrMaestroServer *maestro,
					  const gchar *addr);

GebrCommServer *gebr_maestro_server_get_server(GebrMaestroServer *maestro);

G_END_DECLS

#endif /* __GEBR_MAESTRO_H__ */
