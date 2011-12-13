/*
 * gebr-comm-uri.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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
#include "gebr-comm-uri.h"

#include <string.h>

struct _GebrCommUri {
	gchar *prefix;
	GHashTable *params;
};

void
gebr_comm_uri_set_prefix(GebrCommUri *uri, const gchar *prefix)
{
	g_return_if_fail(uri != NULL);
	g_free(uri->prefix);
	uri->prefix = g_strdup(prefix);
}

const gchar *
gebr_comm_uri_get_prefix(GebrCommUri *uri)
{
	return uri->prefix;
}

void
gebr_comm_uri_add_param(GebrCommUri *uri, const gchar *key, const gchar *value)
{
	g_return_if_fail(uri != NULL);
	g_hash_table_insert(uri->params, g_strdup(key), g_strdup(value));
}

gchar *
gebr_comm_uri_to_string(GebrCommUri *uri)
{
	g_return_val_if_fail(uri != NULL, NULL);

	GString *buffer = g_string_new(uri->prefix);

	if (g_hash_table_size(uri->params) > 0)
		g_string_append_c(buffer, '?');

	void concatenate_uri(gpointer key,
			     gpointer value,
			     gpointer user_data)
	{
		gchar *escaped = g_uri_escape_string(value, NULL, TRUE);
		g_string_append_printf(buffer, "%s=%s;", (gchar*)key, escaped);
		g_free(escaped);
	}

	g_hash_table_foreach(uri->params, concatenate_uri, NULL);
	if (buffer->str[buffer->len-1] == ';')
		g_string_erase(buffer, buffer->len-1, 1);

	return g_string_free(buffer, FALSE);
}


void
gebr_comm_uri_parse(GebrCommUri *uri, const gchar *url)
{
	g_return_if_fail(uri != NULL);

	gchar *dup = g_strdup(url);
	gchar *tmp = strchr(dup, '?');

	if (tmp == NULL) {
		gebr_comm_uri_set_prefix(uri, url);
		g_free(dup);
		return;
	}

	tmp[0] = '\0';
	gebr_comm_uri_set_prefix(uri, dup);

	gchar **params = g_strsplit(tmp + 1, ";", -1);
	for (int i = 0; params[i]; i++) {
		tmp = strchr(params[i], '=');
		tmp[0] = '\0';
		const gchar *key = params[i];
		gchar *value = g_uri_unescape_string(tmp + 1, NULL);
		if (tmp)
			gebr_comm_uri_add_param(uri, key, value);
		g_free(value);
	}
	g_free(dup);
}

const gchar *
gebr_comm_uri_get_param(GebrCommUri *uri, const gchar *param)
{
	g_return_val_if_fail(uri != NULL, NULL);
	return g_hash_table_lookup(uri->params, param);
}

GebrCommUri *
gebr_comm_uri_new()
{
	GebrCommUri *uri = g_new0(GebrCommUri, 1); 
	uri->params = g_hash_table_new_full(g_str_hash,
					    g_str_equal, 
					    g_free, 
					    g_free);
	return uri;
}

void
gebr_comm_uri_free(GebrCommUri *uri)
{
	g_return_if_fail(uri != NULL);
	g_hash_table_unref(uri->params);
	g_free(uri->prefix);
	g_free(uri);
}
