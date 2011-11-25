/*
 * gebr-connectable.h
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

#ifndef __GEBR_CONNECTABLE_H__
#define __GEBR_CONNECTABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEBR_TYPE_CONNECTABLE            (gebr_connectable_get_type ())
#define GEBR_CONNECTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_CONNECTABLE, GebrConnectable))
#define GEBR_CONNECTABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GEBR_TYPE_CONNECTABLE, GebrConnectableIface))
#define GEBR_IS_CONNECTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_CONNECTABLE))
#define GEBR_CONNECTABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEBR_TYPE_CONNECTABLE, GebrConnectableIface))

typedef struct _GebrConnectable GebrConnectable;
typedef struct _GebrConnectableIface GebrConnectableIface;

struct _GebrConnectableIface {
	GTypeInterface g_iface;

	/* virtual methods */
	void (*connect)    (GebrConnectable *connectable,
			    const gchar     *address);

	void (*disconnect) (GebrConnectable *connectable,
			    const gchar     *address);
};

GType gebr_connectable_get_type      (void) G_GNUC_CONST;

/**
 * gebr_connectable_connect:
 *
 * Asks @connectable to connect with @address.
 */
void  gebr_connectable_connect       (GebrConnectable *connectable,
				      const gchar     *address);

/**
 * gebr_connectable_disconnect:
 *
 * Asks @connectable to disconnect from @address.
 */
void  gebr_connectable_disconnect    (GebrConnectable *connectable,
				      const gchar     *address);

G_END_DECLS

#endif /* __GEBR_CONNECTABLE_H__ */
