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
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgebr/comm/gebr-comm.h>

/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;



G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

struct _GebrmAppPriv {
	GMainLoop *main_loop;
	GebrCommListenSocket *listener;
	GebrCommSocketAddress address;
	GList *connections;

	gint finished_starting_pipe[2];
	gchar buffer[1024];
};

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

static void
on_client_request(GebrCommProtocolSocket *socket,
		  GebrCommHttpMsg *request,
		  GebrmApp *app)
{
	g_debug("URL: '%s' CONTENTS: '%s' TYPE: '%d'",
		request->url->str,
		request->content->str,
		request->method);

	if (request->method == GEBR_COMM_HTTP_METHOD_PUT) {
		if (g_str_has_prefix(request->url->str, "/server/"))
			g_debug("Adding server %s", request->url->str + strlen("/server/"));
	}
}

static void
on_client_disconnect(GebrCommProtocolSocket *socket,
		     GebrmApp *app)
{
	g_debug("Client disconnected!");
}

static void
on_new_connection(GebrCommListenSocket *listener,
		  GebrmApp *app)
{
	GebrCommStreamSocket *client;

	g_debug("New connection!");

	while ((client = gebr_comm_listen_socket_get_next_pending_connection(listener))) {
		GebrCommProtocolSocket *protocol = gebr_comm_protocol_socket_new_from_socket(client);
		g_signal_connect(protocol, "disconnected", G_CALLBACK(on_client_disconnect), app);
		g_signal_connect(protocol, "process-request", G_CALLBACK(on_client_request), app);
	}
}

GebrmApp *
gebrm_app_new(void)
{
	return g_object_new(GEBRM_TYPE_APP, NULL);
}

gboolean
gebrm_app_run(GebrmApp *app)
{
	GError *error = NULL;

	GebrCommSocketAddress address = gebr_comm_socket_address_ipv4_local(0);
	app->priv->listener = gebr_comm_listen_socket_new();

	g_signal_connect(app->priv->listener, "new-connection",
			 G_CALLBACK(on_new_connection), app);
	
	if (!gebr_comm_listen_socket_listen(app->priv->listener, &address)) {
		g_critical("Failed to start listener");
		return FALSE;
	}

	app->priv->address = gebr_comm_socket_get_address(GEBR_COMM_SOCKET(app->priv->listener));
	guint16 port = gebr_comm_socket_address_get_ip_port(&app->priv->address);
	gchar *portstr = g_strdup_printf("%d", port);

	g_file_set_contents(gebrm_main_get_lock_file(),
			    portstr, -1, &error);

	if (error) {
		g_critical("Could not create lock: %s", error->message);
		return FALSE;
	}

	/* success, send port */
	g_debug("Server started at %u port",
		gebr_comm_socket_address_get_ip_port(&app->priv->address));

	g_main_loop_run(app->priv->main_loop);

	return TRUE;
}

const gchar *
gebrm_main_get_lock_file(void)
{
	static gchar *lock = NULL;

	if (!lock) {
		gchar *fname = g_strconcat("lock-", g_get_host_name(), NULL);
		gchar *dirname = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", NULL);
		if(!g_file_test(dirname, G_FILE_TEST_EXISTS))
			g_mkdir_with_parents(dirname, 0755);
		lock = g_build_filename(g_get_home_dir(), ".gebr", "gebrm", fname, NULL);
		g_free(dirname);
		g_free(fname);
	}

	return lock;
}
