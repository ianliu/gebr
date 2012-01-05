/*
 * gebrm-client.h
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

#ifndef __GEBRM_CLIENT_H__
#define __GEBRM_CLIENT_H__


#include <glib.h>
#include <glib-object.h>
#include <libgebr/comm/gebr-comm.h>


#define GEBRM_TYPE_CLIENT		(gebrm_client_get_type())
#define GEBRM_CLIENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRM_TYPE_CLIENT, GebrmClient))
#define GEBRM_CLIENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRM_TYPE_CLIENT, GebrmClientClass))
#define GEBRM_IS_CLIENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRM_TYPE_CLIENT))
#define GEBRM_IS_CLIENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRM_TYPE_CLIENT))
#define GEBRM_CLIENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRM_TYPE_CLIENT, GebrmClientClass))


G_BEGIN_DECLS


typedef struct _GebrmClient GebrmClient;
typedef struct _GebrmClientPriv GebrmClientPriv;
typedef struct _GebrmClientClass GebrmClientClass;

struct _GebrmClient {
	GObject parent;
	GebrmClientPriv *priv;
};

struct _GebrmClientClass {
	GObjectClass parent_class;
};

GType gebrm_client_get_type(void) G_GNUC_CONST;

/**
 * gebrm_client_new:
 *
 * Returns: A newly created client from @stream with reference count of 1.
 */
GebrmClient *gebrm_client_new(GebrCommStreamSocket *stream);

/**
 * gebrm_client_get_protocol_socket:
 *
 * Returns: The protocol socket of this client, do not unref it.
 */
GebrCommProtocolSocket *gebrm_client_get_protocol_socket(GebrmClient *client);

/**
 * gebrm_client_get_display_port:
 *
 * The display is used to redirect X11 through the socket connection.
 *
 * Returns: The display port for this client.
 */
guint16 gebrm_client_get_display_port(GebrmClient *client);

void gebrm_client_set_id(GebrmClient *client, const gchar *id);

const gchar *gebrm_client_get_id(GebrmClient *client);

/**
 * gebrm_client_add_forward:
 */
void gebrm_client_add_forward(GebrmClient *client,
			      GebrCommServer *server,
			      guint16 remote_port);

void gebrm_client_remove_forwards(GebrmClient *client);


G_END_DECLS


#endif /* __GEBRM_CLIENT_H__ */
