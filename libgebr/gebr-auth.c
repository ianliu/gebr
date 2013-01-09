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
#include "utils.h"
#include <sys/stat.h>

struct _GebrAuth {
	gchar *auth_file;
};

GebrAuth *
gebr_auth_new(void)
{
	GebrAuth *auth = g_new0(GebrAuth, 1);
	auth->auth_file = g_build_filename(g_get_home_dir(), ".gebr",
					   "run", "authorized_cookies", NULL);
	return auth;
}

void
gebr_auth_read_cookie(GebrAuth *self)
{
	gchar *key = g_new0(gchar, GEBR_AUTH_COOKIE_LENGTH + 1);

	if (scanf("%64s", key) != 1) {
		g_warning("Could not retrieve cookie!");
		g_free(key);
		return;
	}

	GEBR_LOCK_FILE(self->auth_file);
	FILE *fp = fopen(self->auth_file, "a");
	chmod(self->auth_file, 0600);
	fprintf(fp, "%s\n", key);
	fclose(fp);
	GEBR_UNLOCK_FILE(self->auth_file);
}

gboolean
gebr_auth_accepts(GebrAuth *self, const gchar *key)
{
	gboolean ret = FALSE;
	FILE *fp = fopen(self->auth_file, "r");

	if (!fp)
		return FALSE;

	gsize len = 0;
	gchar *line = NULL;
	while (getline(&line, &len, fp) != -1) {
		gchar *tmp = g_strstrip(line);
		if (g_strcmp0(key, tmp) == 0) {
			ret = TRUE;
			break;
		}
	}

	if (line)
		g_free(line);

	fclose(fp);
	return ret;
}

void
gebr_auth_remove_cookie(GebrAuth *self, const gchar *key)
{
	GList *list = NULL;
	FILE *fp = fopen(self->auth_file, "r");

	if (!fp)
		return;

	gsize len = 0;
	gchar *line = NULL;
	while (getline(&line, &len, fp) != -1) {
		gchar *tmp = g_strstrip(line);
		if (g_strcmp0(key, tmp) == 0)
			continue;
		list = g_list_prepend(list, g_strdup(line));
	}
	fclose(fp);

	if (line)
		g_free(line);

	list = g_list_reverse(list);

	GEBR_LOCK_FILE(self->auth_file);
	fp = fopen(self->auth_file, "w");
	chmod(self->auth_file, 0600);
	for (GList *i = list; i; i = i->next)
		fprintf(fp, "%s", (gchar*)i->data);
	fclose(fp);
	GEBR_UNLOCK_FILE(self->auth_file);

	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);
}

void
gebr_auth_free(GebrAuth *self)
{
	g_free(self->auth_file);
	g_free(self);
}
