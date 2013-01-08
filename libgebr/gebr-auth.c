/*
 * gebr-auth.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core team (www.gebrproject.com)
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

#include "gebr-auth.h"
#include <stdio.h>

struct _GebrAuth {
	GTree *cookies;
};


GebrAuth *
gebr_auth_new(void)
{
	GebrAuth *auth = g_new0(GebrAuth, 1);
	auth->cookies = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					NULL, g_free, NULL);
	return auth;
}

void
gebr_auth_read_cookie(GebrAuth *self)
{
	gchar *key = g_new(gchar, GEBR_AUTH_COOKIE_LENGTH);

	if (scanf("%64s", key) != 1) {
		g_warning("Could not retrieve cookie!");
		g_free(key);
		return;
	}

	g_message("Got cookie %s", key);

	g_tree_insert(self->cookies, key, GUINT_TO_POINTER(1));
}

gboolean
gebr_auth_accepts(GebrAuth *self, const gchar *key)
{
	return g_tree_lookup_extended(self->cookies, key, NULL, NULL);
}

void
gebr_auth_remove_cookie(GebrAuth *self, const gchar *key)
{
	g_tree_remove(self->cookies, key);
}

void
gebr_auth_free(GebrAuth *self)
{
	g_tree_unref(self->cookies);
	g_free(self);
}
