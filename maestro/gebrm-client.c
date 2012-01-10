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
	gchar *id;
	gchar *cookie;
	GList *forwards;
	GHashTable *job_ids;
};

enum {
	PROP_0,
	PROP_STREAM,
	PROP_PROTOCOL_SOCKET,
	PROP_DISPLAY_PORT,
};

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

	client->priv->job_ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void
gebrm_client_finalize(GObject *object)
{
	GebrmClient *client = GEBRM_CLIENT(object);
	
	g_list_foreach(client->priv->forwards,
		       (GFunc)gebr_comm_terminal_process_free, NULL);
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
	if (client->priv->display_port == 0) {
		static guint16 p = 3000;
		while (!gebr_comm_listen_socket_is_local_port_available(p))
			p++;
		client->priv->display_port = p;
	}

	return client->priv->display_port;
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

void
gebrm_client_add_forward(GebrmClient *client,
			 GebrCommServer *server,
			 guint16 remote_port)
{
	GebrCommTerminalProcess *proc;
	guint16 cp = gebrm_client_get_display_port(client);
	proc = gebr_comm_server_forward_remote_port(server, remote_port, cp);
	client->priv->forwards = g_list_prepend(client->priv->forwards, proc);

	gchar *address = g_strdup(server->address->str);
	g_object_weak_ref(G_OBJECT(proc), (GWeakNotify)g_free, address);
	g_object_set_data(G_OBJECT(proc), "address", address);
}

void
gebrm_client_kill_forward_by_address(GebrmClient *client,
				     const gchar *addr)
{
	for (GList *i = client->priv->forwards; i; i = i->next) {
		GebrCommTerminalProcess *proc = GEBR_COMM_TERMINAL_PROCESS(i->data);
		const gchar *address = g_object_get_data(G_OBJECT(proc), "address");
		if (g_strcmp0(address, addr) == 0)
			gebr_comm_terminal_process_free(i->data);
	}
}

void
gebrm_client_remove_forwards(GebrmClient *client)
{
	g_debug("Removing client %s forwards...", client->priv->id);

	g_list_foreach(client->priv->forwards,
		       (GFunc)gebr_comm_terminal_process_free, NULL);
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