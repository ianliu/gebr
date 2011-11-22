/*
 * gebrm-app.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
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

#include <config.h>
#include "gebrm-app.h"

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;



G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GSocketService *listener;
	GSocketAddress *effaddr;
	GList *connections;
};

static gboolean
on_incoming_connection(GSocketService    *service,
		       GSocketConnection *connection,
		       GObject           *source_object,
		       GebrmApp          *app)
{
	g_debug("Incomming connection!");

	app->priv->connections = g_list_prepend(app->priv->connections,
						g_object_ref(connection));

	return TRUE;
}

static void
gebrm_app_class_init(GebrmAppClass *klass)
{
	g_type_class_add_private(klass, sizeof(GebrmAppPriv));
}

static void
gebrm_app_init(GebrmApp *app)
{
	app->priv = G_TYPE_INSTANCE_GET_PRIVATE(app,
						GEBRM_TYPE_APP,
						GebrmAppPriv);
	app->priv->main_loop = g_main_loop_new(NULL, FALSE);
	app->priv->connections = NULL;
	app->priv->listener = g_socket_service_new();

	g_signal_connect(app->priv->listener, "incoming",
			 G_CALLBACK(on_incoming_connection), app);

	GError *error = NULL;
	GInetAddress *loopback = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
	GSocketAddress *address = g_inet_socket_address_new(loopback, 0);
	g_socket_listener_add_address(G_SOCKET_LISTENER(app->priv->listener),
				      address,
				      G_SOCKET_TYPE_STREAM,
				      G_SOCKET_PROTOCOL_TCP,
				      G_OBJECT(app),
				      &app->priv->effaddr,
				      &error);

	if (error) {
		g_critical("Error setting listener: %s", error->message);
		exit(EXIT_FAILURE);
	}

	GInetSocketAddress *inet = G_INET_SOCKET_ADDRESS(app->priv->effaddr);
	GInetAddress *addr = g_inet_socket_address_get_address(inet);

	g_debug("Successfully listening at address %s on port %d",
		g_inet_address_to_string(addr),
		g_inet_socket_address_get_port(inet));

	g_object_unref(loopback);
	g_object_unref(address);
}

void
gebrm_app_singleton_set_factory(GebrmAppFactory fac,
				gpointer data)
{
	__factory = fac;
	__data = data;
}

GebrmApp *
gebrm_app_singleton_get(void)
{
	if (__factory)
		return (*__factory)(__data);

	if (!__app)
		__app = gebrm_app_new();

	return __app;
}

GebrmApp *
gebrm_app_new(void)
{
	return g_object_new(GEBRM_TYPE_APP, NULL);
}

void
gebrm_app_run(GebrmApp *app)
{
	g_socket_service_start(app->priv->listener);
	g_main_loop_run(app->priv->main_loop);
}
