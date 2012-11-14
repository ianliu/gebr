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
	GebrCommProtocolSocket *socket;
	gchar *id;
	gchar *cookie;
	GList *forwards;
	GHashTable *job_ids;
	guint x11_port;
};

enum {
	PROP_0,
	PROP_STREAM,
	PROP_PROTOCOL_SOCKET,
	PROP_DISPLAY_PORT,
};

typedef struct {
	GebrmClient *client;
	GebrCommServer *server;
	GebrCommPortForward *forward;
} X11ForwardData;

static void gebrm_client_finalize(GObject *object);

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
	gobject_class->finalize = gebrm_client_finalize;
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

	g_type_class_add_private(klass, sizeof(GebrmClientPriv));
}

static void
gebrm_client_init(GebrmClient *client)
{
	client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client,
						   GEBRM_TYPE_CLIENT,
						   GebrmClientPriv);

	client->priv->job_ids = g_hash_table_new_full(g_str_hash, g_str_equal,
						      g_free, g_free);
}

static void
x11_forward_data_close_and_free(X11ForwardData *data)
{
	gebr_comm_port_forward_close(data->forward);
	g_free(data);
}

static void
gebrm_client_finalize(GObject *object)
{
	GebrmClient *client = GEBRM_CLIENT(object);
	
	g_list_foreach(client->priv->forwards,
		       (GFunc)x11_forward_data_close_and_free, NULL);
	g_list_free(client->priv->forwards);
	g_object_unref(client->priv->socket);
	g_free(client->priv->id);
	g_hash_table_destroy(client->priv->job_ids);

	G_OBJECT_CLASS(gebrm_client_parent_class)->finalize(object);
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

	client->priv->x11_port = gebr_comm_get_available_port(6010);
	gchar *str = g_strdup_printf("%d", client->priv->x11_port);
	gebr_comm_protocol_socket_oldmsg_send(client->priv->socket, FALSE,
					      gebr_comm_protocol_defs.prt_def, 1,
					      str);
	g_free(str);
}

GebrCommProtocolSocket *
gebrm_client_get_protocol_socket(GebrmClient *client)
{
	return client->priv->socket;
}

void
gebrm_client_set_id(GebrmClient *client,
		    const gchar *id)
{
	g_return_if_fail(client->priv->id == NULL);
	client->priv->id = g_strdup(id);
}

const gchar *
gebrm_client_get_id(GebrmClient *client)
{
	return client->priv->id;
}

void
gebrm_client_set_magic_cookie(GebrmClient *client, const gchar *cookie)
{
	if (client->priv->cookie)
		g_free(client->priv->cookie);
	client->priv->cookie = g_strdup(cookie);
}

const gchar *
gebrm_client_get_magic_cookie(GebrmClient *client)
{
	return client->priv->cookie;
}

static void
on_x11_port_defined(GebrCommPortProvider *self,
		    guint port,
		    X11ForwardData *data)
{
	data->forward = gebr_comm_port_provider_get_forward(self);
	data->client->priv->forwards = g_list_prepend(data->client->priv->forwards, data);
}

static void
on_x11_error(GebrCommPortProvider *self,
	     GError *error,
	     GebrmClient *client)
{
	g_debug("%s", error->message);
}

void
gebrm_client_add_forward(GebrmClient *client,
			 GebrCommServer *server,
			 guint16 remote_port)
{
	X11ForwardData *data = g_new(X11ForwardData, 1);
	data->client = client;
	data->server = server;

	GebrCommPortProvider *port_provider =
		gebr_comm_server_create_port_provider(server, GEBR_COMM_PORT_TYPE_X11);
	gebr_comm_port_provider_set_display(port_provider, remote_port);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_x11_port_defined), data);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_x11_error), client);
	gebr_comm_port_provider_start_with_port(port_provider, client->priv->x11_port);
}

void
gebrm_client_kill_forward_by_address(GebrmClient *client,
				     const gchar *addr)
{
	for (GList *i = client->priv->forwards; i; i = i->next) {
		X11ForwardData *data = i->data;
		if (g_strcmp0(addr, data->server->address->str) == 0) {
			gebr_comm_port_forward_close(data->forward);
			client->priv->forwards = g_list_delete_link(client->priv->forwards, i);
			g_free(data);
			return;
		}
	}
}

void
gebrm_client_remove_forwards(GebrmClient *client)
{
	g_debug("Removing client %s forwards...", client->priv->id);

	g_list_foreach(client->priv->forwards,
		       (GFunc)x11_forward_data_close_and_free, NULL);
	g_list_free(client->priv->forwards);
	client->priv->forwards = NULL;
}

void
gebrm_client_add_temp_id(GebrmClient *client,
			 const gchar *temp_id,
			 const gchar *job_id)
{
	g_hash_table_insert(client->priv->job_ids,
			    g_strdup(temp_id),
			    g_strdup(job_id));
}

const gchar *
gebrm_client_get_job_id_from_temp(GebrmClient *client,
				  const gchar *temp_id)
{
	return g_hash_table_lookup(client->priv->job_ids, temp_id);
}

guint
gebrm_client_get_display_port(GebrmClient *self,
			      const gchar *addr)
{
	if (gebr_comm_is_local_address(addr))
		return self->priv->x11_port;
	else
		return 0;
}
