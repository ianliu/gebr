/*
 * gebr-daemon-server.c
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

#include "gebr-daemon-server.h"

#include <glib/gi18n.h>
#include <libgebr/comm/gebr-comm.h>
#include "gebr-maestro-server.h"

struct _GebrDaemonServerPriv {
	GebrMaestroServer *maestro;
	gchar *address;
	enum gebr_comm_server_state state;
};

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_MAESTRO,
	PROP_STATE,
};

G_DEFINE_TYPE(GebrDaemonServer, gebr_daemon_server, G_TYPE_OBJECT);

static void
gebr_daemon_server_get(GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GebrDaemonServer *daemon = GEBR_DAEMON_SERVER(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		g_value_set_string(value, daemon->priv->address);
		break;
	case PROP_MAESTRO:
		g_value_take_object(value, daemon->priv->maestro);
		break;
	case PROP_STATE:
		g_value_set_int(value, gebr_daemon_server_get_state(daemon));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_daemon_server_set(GObject      *object,
		       guint         prop_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GebrDaemonServer *daemon = GEBR_DAEMON_SERVER(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		daemon->priv->address = g_value_dup_string(value);
		if (!daemon->priv->address)
			daemon->priv->address = g_strdup("");
		break;
	case PROP_MAESTRO:
		daemon->priv->maestro = g_value_dup_object(value);
		break;
	case PROP_STATE:
		gebr_daemon_server_set_state(daemon, g_value_get_int(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_daemon_server_finalize(GObject *object)
{
	GebrDaemonServer *daemon = GEBR_DAEMON_SERVER(object);

	g_free(daemon->priv->address);
	g_object_unref(daemon->priv->maestro);

	G_OBJECT_CLASS(gebr_daemon_server_parent_class)->finalize(object);
}

static void
gebr_daemon_server_class_init(GebrDaemonServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_daemon_server_get;
	object_class->set_property = gebr_daemon_server_set;
	object_class->finalize = gebr_daemon_server_finalize;

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Server address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(object_class,
					PROP_MAESTRO,
					g_param_spec_object("maestro",
							    "Maestro",
							    "Maestro",
							    GEBR_TYPE_MAESTRO_SERVER,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(object_class,
					PROP_STATE,
					g_param_spec_int("state",
							 "State",
							 "State",
							 0, 100,
							 SERVER_STATE_UNKNOWN,
							 G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(GebrDaemonServerPriv));
}

static void
gebr_daemon_server_init(GebrDaemonServer *daemon)
{
	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon,
						   GEBR_TYPE_DAEMON_SERVER,
						   GebrDaemonServerPriv);
}

GebrDaemonServer *
gebr_daemon_server_new(GObject *maestro,
		       const gchar *address,
		       enum gebr_comm_server_state state)
{
	return g_object_new(GEBR_TYPE_DAEMON_SERVER,
			    "address", address,
			    "maestro", maestro,
			    "state", state,
			    NULL);
}

const gchar *
gebr_daemon_server_get_address(GebrDaemonServer *daemon)
{
	return daemon->priv->address;
}

gchar *
gebr_daemon_server_get_display_address(GebrDaemonServer *daemon)
{
	const gchar *addr = daemon->priv->address;

	if (g_strcmp0(addr, "") == 0)
		return g_strdup(_("Auto-choose"));

	if (g_strcmp0(addr, "127.0.0.1") == 0)
		return g_strdup(_("Local Server"));

	return g_strdup(addr);
}

void
gebr_daemon_server_set_state(GebrDaemonServer *daemon,
			     enum gebr_comm_server_state state)
{
	daemon->priv->state = state;
}

enum gebr_comm_server_state
gebr_daemon_server_get_state(GebrDaemonServer *daemon)
{
	return daemon->priv->state;
}

void
gebr_daemon_server_connect(GebrDaemonServer *daemon)
{
	gchar *url = g_strconcat("/server/", daemon->priv->address, NULL);
	GebrCommServer *server = gebr_maestro_server_get_server(daemon->priv->maestro);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

void
gebr_daemon_server_disconnect(GebrDaemonServer *daemon)
{
	gchar *url = g_strconcat("/disconnect/", daemon->priv->address, NULL);
	GebrCommServer *server = gebr_maestro_server_get_server(daemon->priv->maestro);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT,
					       url, NULL);
	g_free(url);
}

gboolean
gebr_daemon_server_is_autochoose(GebrDaemonServer *daemon)
{
	return g_strcmp0(daemon->priv->address, "") == 0;
}
