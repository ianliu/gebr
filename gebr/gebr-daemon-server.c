/*
 * gebr-daemon-server.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR core team (www.gebrproject.com)
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

struct _GebrDaemonServerPriv {
	GebrConnectable *connectable;
	gchar *address;
	gchar *hostname;
	gboolean ac;
	gchar *error;
	GebrCommServerState state;
	GList *tags;
	gchar *maestro_addr;
};

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_AC,
	PROP_CONNECTABLE,
	PROP_STATE,
	PROP_MAESTRO_ADDR,
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
	case PROP_AC:
		g_value_set_boolean(value, daemon->priv->ac);
		break;
	case PROP_CONNECTABLE:
		g_value_take_object(value, daemon->priv->connectable);
		break;
	case PROP_STATE:
		g_value_set_int(value, gebr_daemon_server_get_state(daemon));
		break;
	case PROP_MAESTRO_ADDR:
		g_value_set_string(value, daemon->priv->maestro_addr);
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
	case PROP_CONNECTABLE:
		daemon->priv->connectable = g_value_dup_object(value);
		break;
	case PROP_STATE:
		gebr_daemon_server_set_state(daemon, g_value_get_int(value));
		break;
	case PROP_MAESTRO_ADDR:
		daemon->priv->maestro_addr= g_value_dup_string(value);
		if (!daemon->priv->maestro_addr)
			daemon->priv->maestro_addr = g_strdup("");
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
	g_free(daemon->priv->hostname);
	g_object_unref(daemon->priv->connectable);
	g_list_foreach(daemon->priv->tags, (GFunc)g_free, NULL);
	g_list_free(daemon->priv->tags);
	g_free(daemon->priv->maestro_addr);

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
					PROP_CONNECTABLE,
					g_param_spec_object("connectable",
							    "Connectable",
							    "Connectable",
							    G_TYPE_OBJECT,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property(object_class,
					PROP_STATE,
					g_param_spec_int("state",
							 "State",
							 "State",
							 0, 100,
							 SERVER_STATE_UNKNOWN,
							 G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
					PROP_MAESTRO_ADDR,
					g_param_spec_string("maestro_addr",
							    "Maestro_addr",
							    "Maestro address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private(klass, sizeof(GebrDaemonServerPriv));
}

static void
gebr_daemon_server_init(GebrDaemonServer *daemon)
{
	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon,
						   GEBR_TYPE_DAEMON_SERVER,
						   GebrDaemonServerPriv);
	daemon->priv->tags = NULL;
	daemon->priv->maestro_addr = NULL;
	daemon->priv->ac = TRUE;
}

GebrDaemonServer *
gebr_daemon_server_new(GebrConnectable *connectable,
		       const gchar *address,
		       GebrCommServerState state,
		       const gchar *maestro_addr)
{
	return g_object_new(GEBR_TYPE_DAEMON_SERVER,
			    "address", address,
			    "connectable", connectable,
			    "state", state,
			    "maestro_addr", maestro_addr,
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
	g_return_val_if_fail(GEBR_IS_DAEMON_SERVER(daemon), NULL);

	const gchar *addr = daemon->priv->address;

	if (g_strcmp0(addr, "") == 0)
		return g_strdup(_("Auto-choose"));

	if (g_strcmp0(addr, "127.0.0.1") == 0 || g_strcmp0(addr, "localhost") == 0 )
		//return g_strdup(_("Local Server"));
		return g_strdup(daemon->priv->maestro_addr);

	return g_strdup(addr);
}

void
gebr_daemon_server_set_state(GebrDaemonServer *daemon,
			     GebrCommServerState state)
{
	daemon->priv->state = state;
}

GebrCommServerState
gebr_daemon_server_get_state(GebrDaemonServer *daemon)
{
	return daemon->priv->state;
}

void
gebr_daemon_server_connect(GebrDaemonServer *daemon)
{
	gebr_connectable_connect(daemon->priv->connectable,
				 daemon->priv->address);
}

void
gebr_daemon_server_disconnect(GebrDaemonServer *daemon)
{
	gebr_connectable_disconnect(daemon->priv->connectable,
				    daemon->priv->address,
				    "");
}

gboolean
gebr_daemon_server_is_autochoose(GebrDaemonServer *daemon)
{
	return g_strcmp0(daemon->priv->address, "") == 0;
}

void
gebr_daemon_server_set_tags(GebrDaemonServer *daemon,
			    gchar **tags)
{
	g_list_foreach(daemon->priv->tags, (GFunc)g_free, NULL);
	g_list_free(daemon->priv->tags);

	daemon->priv->tags = NULL;
	for (int i = 0; tags[i]; i++)
		daemon->priv->tags = g_list_prepend(daemon->priv->tags,
						    g_strdup(tags[i]));
}

GList *
gebr_daemon_server_get_tags(GebrDaemonServer *daemon)
{
	return daemon->priv->tags;
}

gboolean
gebr_daemon_server_has_tag(GebrDaemonServer *daemon, const gchar *tag)
{
	if (!tag || !*tag)
		return TRUE;

	for (GList *i = daemon->priv->tags; i; i = i->next)
		if (g_strcmp0(i->data, tag) == 0)
			return TRUE;
	return FALSE;
}

gboolean
gebr_daemon_server_get_ac(GebrDaemonServer *daemon)
{
	return daemon->priv->ac;
}

void
gebr_daemon_server_set_ac(GebrDaemonServer *daemon, gboolean ac)
{
	daemon->priv->ac = ac;
}
void
gebr_daemon_server_set_error(GebrDaemonServer *daemon, const gchar *error)
{
	if (daemon->priv->error)
		g_free(daemon->priv->error);
	daemon->priv->error = g_strdup(error);
}
const gchar *gebr_daemon_server_get_error(GebrDaemonServer *daemon)
{
	return daemon->priv->error;
}

void
gebr_daemon_server_set_hostname(GebrDaemonServer *daemon, const gchar *hostname)
{
	daemon->priv->hostname = g_strdup(hostname);
}

const gchar *
gebr_daemon_server_get_hostname(GebrDaemonServer *daemon)
{
	return daemon->priv->hostname;
}
