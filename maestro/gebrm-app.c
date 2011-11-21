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


/*
 * Global variables to implement GebrmAppSingleton methods.
 */
static GebrmAppFactory __factory = NULL;
static gpointer           __data = NULL;
static GebrmApp           *__app = NULL;



G_DEFINE_TYPE(GebrmApp, gebrm_app, G_TYPE_OBJECT);

struct _GebrmAppPriv {
	gint __reserved;
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
	g_debug("App is up and running!");
	while (1) {
		g_usleep(G_USEC_PER_SEC);
		g_debug("tic...");
	}
}
