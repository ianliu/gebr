/*
 * gebr-maestro-settings.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Core team (www.gebrproject.com)
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


#include "gebr-maestro-settings.h"
#include <unistd.h>

#include <glib/gstdio.h>
#include <glib/gkeyfile.h>
#include <glib/gi18n.h>

#include "libgebr/utils.h"

struct _GebrMaestroSettingsPriv {
	gchar *path;
	GKeyFile *maestro_key;
};

GebrMaestroSettings *
gebr_maestro_settings_new(const gchar *path)
{
	GebrMaestroSettings *ms = g_new(GebrMaestroSettings, 1);
	ms->priv = g_new0(GebrMaestroSettingsPriv, 1);

	ms->priv->maestro_key = g_key_file_new();
	ms->priv->path = g_strdup(path);

	/* For the sake of backwards compatibility, load deprecated gengetopt file format ... */
	g_key_file_load_from_file(ms->priv->maestro_key, path, G_KEY_FILE_NONE, NULL);

	return ms;
}

void
gebr_maestro_settings_save(GebrMaestroSettings *ms)
{
	gsize length;
	gchar *string;
	FILE *configfp;

	string = g_key_file_to_data(ms->priv->maestro_key, &length, NULL);

	configfp = fopen(ms->priv->path, "w");
	if (configfp == NULL) {
		g_debug(_("Could not save configuration."));
		goto out;
	}

	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);

out:
	g_free(string);
	return;
}

void
gebr_maestro_settings_free(GebrMaestroSettings *ms)
{
	g_free(ms->priv->path);
	g_key_file_free(ms->priv->maestro_key);
	g_free(ms);
}

void
gebr_maestro_settings_set_domain(GebrMaestroSettings *ms,
                                 const gchar *domain,
                                 const gchar *label,
                                 const gchar *addr)
{
	if (!label || !*label)
		label = gebr_generate_nfs_label();

	g_key_file_set_string(ms->priv->maestro_key, domain, "label", label);

	gebr_maestro_settings_add_address_on_domain(ms, domain, addr);
}

void
gebr_maestro_settings_add_addresses_on_domain(GebrMaestroSettings *ms,
                                              const gchar *domain,
                                              const gchar *addrs)
{
	if (g_key_file_has_group(ms->priv->maestro_key, domain))
		g_key_file_set_string(ms->priv->maestro_key, domain, "maestro", addrs);
}

void
gebr_maestro_settings_add_address_on_domain(GebrMaestroSettings *ms,
                                            const gchar *domain,
                                            const gchar *addr)
{
	gboolean has_addr = FALSE;
	GString *maestro;

	if (g_key_file_has_group(ms->priv->maestro_key, domain))
		maestro = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");
	else
		maestro = g_string_new(NULL);

	gchar **list = g_strsplit(maestro->str, ",", -1);
	if (list) {
		for (gint i = 0; list[i] && !has_addr; i++)
			has_addr = g_strcmp0(list[i], addr) == 0;
	}

	if (!has_addr) {
		if (maestro->len > 1)
			maestro = g_string_append_c(maestro, ',');
		maestro = g_string_append(maestro, addr);

		g_key_file_set_string(ms->priv->maestro_key, domain, "maestro", g_string_free(maestro, FALSE));
	}

	g_strfreev(list);
}

void
gebr_maestro_settings_change_label(GebrMaestroSettings *ms,
                                   const gchar *domain,
                                   const gchar *label)
{
	if (g_key_file_has_group(ms->priv->maestro_key, domain))
		g_key_file_set_string(ms->priv->maestro_key, domain, "label", label);
}

const gchar *
gebr_maestro_settings_get_addrs(GebrMaestroSettings *ms,
                                const gchar *domain)
{
	GString *maestros;

	maestros = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");

	return g_string_free(maestros, FALSE);
}

const gchar *
gebr_maestro_settings_get_addr_for_domain(GebrMaestroSettings *ms,
                                          const gchar *domain,
                                          gint index)
{
	const gchar *maestro;
	GString *maestros;

	maestros = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "maestro", "");

	const gchar *list = g_string_free(maestros, FALSE);

	gchar **addrs = g_strsplit(list, ",", -1);

	if (addrs)
		maestro = g_strdup(addrs[index]);

	g_strfreev(addrs);

	return maestro;
}

const gchar *
gebr_maestro_settings_get_label_for_domain(GebrMaestroSettings *ms,
                                           const gchar *domain)
{
	GString *label;

	label = gebr_g_key_file_load_string_key(ms->priv->maestro_key, domain, "label", gebr_generate_nfs_label());

	return g_string_free(label, FALSE);
}

GKeyFile *
gebr_maestro_settings_get_key_file(GebrMaestroSettings *ms)
{
	return ms->priv->maestro_key;
}
