/*
 * gebrm-app.h
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

#ifndef __GEBRM_APP_H__
#define __GEBRM_APP_H__

#include <glib-object.h>

#define GEBRM_TYPE_APP            (gebrm_app_get_type())
#define GEBRM_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GEBRM_TYPE_APP, GebrmApp))
#define GEBRM_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GEBRM_TYPE_APP, GebrmAppClass))
#define GEBRM_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBRM_TYPE_APP))
#define GEBRM_APP_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GEBRM_TYPE_APP))
#define GEBRM_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GEBRM_TYPE_APP, GebrmAppClass))

G_BEGIN_DECLS

typedef struct _GebrmApp GebrmApp;
typedef struct _GebrmAppPriv GebrmAppPriv;
typedef struct _GebrmAppClass GebrmAppClass;

struct _GebrmApp {
	GObject parent;
	GebrmAppPriv *priv;
};

struct _GebrmAppClass {
	GObjectClass parent_class;
};

/* GebrmAppSingleton methods {{{1 */
/**
 * GebrmAppFactory:
 *
 * A function that constructs a #GebrmApp. See
 * gebrm_app_singleton_set_factory().
 */
typedef GebrmApp* (*GebrmAppFactory) (gpointer data);

/**
 * gebrm_app_singleton_set_factory:
 *
 * Changes the method that is used to create a #GebrmApp in
 * gebrm_app_singleton_get(). The default method creates a #GebrmApp per
 * program execution. You should use this method in unit tests to prevent one
 * test from interfering with another. See the example below.
 *
 * Note that this method changes global behavior and calling it on different
 * threads is probably an error.
 *
 * \[
 * GebrApp *my_app_factory(GebrApp **app)
 * {
 *   if (!*app)
 *     *app = gebrm_app_new();
 *   return *app;
 * }
 *
 * void my_funky_test()
 * {
 *   static GebrApp *app = NULL;
 *   gebrm_app_singleton_set_factory(my_app_factory, &app);
 *   // Now the functions which use gebrm_app_singleton_get
 *   // will use my_app_factory instead, which will not interfere
 *   // with other tests.
 * }
 * \]
 */
void gebrm_app_singleton_set_factory(GebrmAppFactory factory,
				     gpointer data);

/**
 * gebrm_app_singleton_get:
 *
 * Gets the #GebrmApp singleton instance. The instance returned is calculated
 * by the method set with gebrm_app_singleton_set_factory(). If no method was
 * set, then a default one is used, which creates one #GebrmApp for
 * application.
 */
GebrmApp *gebrm_app_singleton_get(void);
/* }}} GebrmAppSingleton methods */

/* GebrmApp methods {{{ */
GType gebrm_app_get_type(void) G_GNUC_CONST;

/**
 * gebrm_app_new:
 *
 * Creates a new #GebrmApp with reference count of one. Free with
 * g_object_unref().
 */
GebrmApp *gebrm_app_new(void);

/**
 * gebrm_app_run:
 *
 * Starts the application main loop.
 */
gboolean gebrm_app_run(GebrmApp *app);

const gchar *gebrm_main_get_lock_file(void);

/* }}} GebrmApp methods */

G_END_DECLS

#endif /* __GEBRM_APP_H__ */
