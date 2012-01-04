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

#include "gebrm-client.h"

G_DEFINE_TYPE(GebrmClient, gebrm_client, G_TYPE_OBJECT);

struct _GebrmClientPriv {
	guint16 display_port;
	GebrCommProtocolSocket *socket;
};

enum {
	PROP_0,
	PROP_STREAM,
	PROP_PROTOCOL_SOCKET,
	PROP_DISPLAY_PORT,
};

static void gebrm_client_set_property(GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);

static void gebrm_client_get_property(GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec);

static void gebrm_client_set_stream_socket(GebrmClient *client,
					   GebrCommStreamSocket *stream);

static void
gebrm_client_class_init(GebrmClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gebrm_client_set_property;
	gobject_class->get_property = gebrm_client_get_property;

	g_object_class_install_property(gobject_class,
					PROP_STREAM,
					g_param_spec_object("stream",
							    "Socket stream",
							    "A socket stream to communicate with this client",
							    GEBR_COMM_STREAM_SOCKET_TYPE,
							    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(gobject_class,
					PROP_PROTOCOL_SOCKET,
					g_param_spec_object("protocol-socket",
							    "Protocol socket",
							    "A protocol socket to communicate with this client",
							    GEBR_COMM_PROTOCOL_SOCKET_TYPE,
							    G_PARAM_READABLE));

	g_object_class_install_property(gobject_class,
					PROP_DISPLAY_PORT,
					g_param_spec_int("display-port",
							 "Display port",
							 "Port that will be used to forward X11 display",
							 0, G_MAXUINT16, 0,
							 G_PARAM_READABLE));

	g_type_class_add_private(klass, sizeof(GebrmClientPriv));
}

static void
gebrm_client_init(GebrmClient *client)
{
	client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client,
						   GEBRM_TYPE_CLIENT,
						   GebrmClientPriv);
}

static void
gebrm_client_set_property(GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GebrmClient *client = GEBRM_CLIENT(object);

	switch (prop_id)
	{
	case PROP_STREAM:
		gebrm_client_set_stream_socket(client, g_value_dup_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
gebrm_client_get_property(GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GebrmClient *client = GEBRM_CLIENT(object);

	switch (prop_id)
	{
	case PROP_DISPLAY_PORT:
		g_value_set_int(value, gebrm_client_get_display_port(client));
		break;
	case PROP_PROTOCOL_SOCKET:
		g_value_set_object(value, gebrm_client_get_protocol_socket(client));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

GebrmClient *
gebrm_client_new(GebrCommStreamSocket *stream)
{
	return g_object_new(GEBRM_TYPE_CLIENT,
			    "stream", stream,
			    NULL);
}

static void
gebrm_client_set_stream_socket(GebrmClient *client,
			       GebrCommStreamSocket *stream)
{
	client->priv->socket = gebr_comm_protocol_socket_new_from_socket(stream);
}

GebrCommProtocolSocket *
gebrm_client_get_protocol_socket(GebrmClient *client)
{
	return client->priv->socket;
}

guint16
gebrm_client_get_display_port(GebrmClient *client)
{
	return client->priv->display_port;
}

void
gebrm_client_send_display_port(GebrmClient *client)
{
}
