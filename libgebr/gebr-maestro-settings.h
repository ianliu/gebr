/*
 * gebr-maestro-settings.h
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

#ifndef __GEBR_MAESTRO_SETTINGS_H__
#define __GEBR_MAESTRO_SETTINGS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrMaestroSettings GebrMaestroSettings;
typedef struct _GebrMaestroSettingsPriv GebrMaestroSettingsPriv;

struct _GebrMaestroSettings {
	GebrMaestroSettingsPriv *priv;
};

/**
 * gebr_maestro_settings_new:
 *
 * Load from file with @path, to memory on keyfile
 */
GebrMaestroSettings * gebr_maestro_settings_new(const gchar *path);

/**
 * gebr_maestro_settings_save:
 *
 * Save the keyfile on file
 */
void gebr_maestro_settings_save(GebrMaestroSettings *ms);

/**
 * gebr_maestro_settings_free:
 *
 * Free the Maestro Settings structure
 */
void gebr_maestro_settings_free(GebrMaestroSettings *ms);

/**
 * gebr_maestro_settings_set_domain:
 *
 * Set on keyfile, the key @domain, with
 * @label to represent that NFS domain, and the @addr
 * to include the address on list of maestros
 */
void gebr_maestro_settings_set_domain(GebrMaestroSettings *ms,
                                      const gchar *domain,
                                      const gchar *label,
                                      const gchar *addr);

/**
 * gebr_maestro_settings_add_address:
 *
 * Add @addr on list of maestro of keyfile, at @domain
 */
void gebr_maestro_settings_add_address_on_domain(GebrMaestroSettings *ms,
                                                 const gchar *domain,
                                                 const gchar *addr);

/**
 * gebr_maestro_settings_change_label:
 *
 * Change label of @domain to @label
 */
void gebr_maestro_settings_change_label(GebrMaestroSettings *ms,
                                        const gchar *domain,
                                        const gchar *label);

/**
 * gebr_maestro_settings_get_addr_for_domain:
 *
 * Get address of the maestro on @domain in @index position
 */
const gchar * gebr_maestro_settings_get_addr_for_domain(GebrMaestroSettings *ms,
                                                        const gchar *domain,
                                                        gint index);

/**
 * gebr_maestro_settings_get_label_for_domain:
 *
 * Get label to represent the @domain
 */
const gchar *gebr_maestro_settings_get_label_for_domain(GebrMaestroSettings *ms,
                                                        const gchar *domain);

G_END_DECLS

#endif /* __GEBR_MAESTRO_SETTINGS_H__ */
