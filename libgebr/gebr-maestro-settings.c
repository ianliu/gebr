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

struct _GebrMaestroSettingsPriv {
	const gchar *path;
	GKeyFile *maestro_key;
};

GebrMaestroSettings *
gebr_maestro_settings_new(const gchar *path)
{
	return NULL;
}

void
gebr_maestro_settings_save(GebrMaestroSettings *ms)
{

}

void
gebr_maestro_settings_free(GebrMaestroSettings *ms)
{

}

void
gebr_maestro_settings_set_domain(GebrMaestroSettings *ms,
                                 const gchar *domain,
                                 const gchar *label,
                                 const gchar *addr)
{

}

void
gebr_maestro_settings_add_address_on_domain(GebrMaestroSettings *ms,
                                            const gchar *domain,
                                            const gchar *addr)
{

}

const gchar *
gebr_maestro_settings_get_addr_for_domain(GebrMaestroSettings *ms,
                                          const gchar *domain)
{
	return NULL;
}

const gchar *
gebr_maestro_settings_get_label_for_domain(GebrMaestroSettings *ms,
                                           const gchar *domain)
{
	return NULL;
}
