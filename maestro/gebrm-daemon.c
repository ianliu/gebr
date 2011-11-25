/*
 * gebrm-daemon.c
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

#include "gebrm-daemon.h"

struct _GebrmDaemonPriv {
	GTree *tags;
	GebrCommServer *server;
};

enum {
	PROP_0,
	PROP_ADDRESS,
	PROP_SERVER,
};

G_DEFINE_TYPE(GebrmDaemon, gebrm_daemon, G_TYPE_OBJECT);

static void
gebrm_daemon_get(GObject    *object,
		 guint       prop_id,
		 GValue     *value,
		 GParamSpec *pspec)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	switch (prop_id)
	{
	case PROP_ADDRESS:
		g_value_set_string(value, gebrm_daemon_get_address(daemon));
		break;
	case PROP_SERVER:
		g_value_set_pointer(value, daemon->priv->server);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebrm_daemon_set(GObject      *object,
		 guint         prop_id,
		 const GValue *value,
		 GParamSpec   *pspec)
{
	GebrmDaemon *daemon = GEBRM_DAEMON(object);

	switch (prop_id)
	{
	case PROP_SERVER:
		daemon->priv->server = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebrm_daemon_class_init(GebrmDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebrm_daemon_get;
	object_class->set_property = gebrm_daemon_set;

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Server address",
							    NULL,
							    G_PARAM_READABLE));

	g_object_class_install_property(object_class,
					PROP_SERVER,
					g_param_spec_pointer("server",
							     "Server",
							     "Server",
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private(klass, sizeof(GebrmDaemonPriv));
}

static void
gebrm_daemon_init(GebrmDaemon *daemon)
{
	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon,
						   GEBRM_TYPE_DAEMON,
						   GebrmDaemonPriv);
	daemon->priv->tags = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					     NULL, g_free, NULL);
}

GebrmDaemon *
gebrm_daemon_new(GebrCommServer *server)
{
	return g_object_new(GEBRM_TYPE_DAEMON,
			    "server", server,
			    NULL);
}

const gchar *
gebrm_daemon_get_address(GebrmDaemon *daemon)
{
	return daemon->priv->server->address->str;
}

void
gebrm_daemon_add_tag(GebrmDaemon *daemon,
		     const gchar *tag)
{
	g_debug("Adding %s", tag);
	g_tree_insert(daemon->priv->tags, g_strdup(tag),
		      GINT_TO_POINTER(1));
}

gboolean
gebrm_daemon_has_tag(GebrmDaemon *daemon,
		     const gchar *tag)
{
	return g_tree_lookup(daemon->priv->tags, tag) != NULL;
}

gchar *
gebrm_daemon_get_tags(GebrmDaemon *daemon)
{
	gboolean traverse(gpointer key,
			  gpointer value,
			  gpointer data)
	{
		GString *b = data;
		gchar *tag = key;
		g_string_append_printf(b, "%s,", tag);
		return FALSE;
	}

	GString *buf = g_string_new("");
	g_tree_foreach(daemon->priv->tags, traverse, buf);
	g_string_erase(buf, buf->len - 1, 1);
	return g_string_free(buf, FALSE);
}

void
gebrm_daemon_connect(GebrmDaemon *daemon)
{
	gebr_comm_server_connect(daemon->priv->server, FALSE);
}
