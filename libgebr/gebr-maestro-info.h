/*
 * gebr-maestro-info.h
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

#ifndef __GEBR_MAESTRO_INFO_H
#define __GEBR_MAESTRO_INFO_H
#include <glib.h>
G_BEGIN_DECLS

typedef struct _GebrMaestroInfo GebrMaestroInfo;
struct _GebrMaestroInfo {
	gchar *(*get_home_uri)(GebrMaestroInfo *self);
	gchar *(*get_home_mount_point)(GebrMaestroInfo *self);
	gboolean (*get_need_gvfs)(GebrMaestroInfo *self);
};

gchar *gebr_maestro_info_get_home_uri(GebrMaestroInfo *self);

gboolean gebr_maestro_info_get_need_gvfs(GebrMaestroInfo *self);

G_END_DECLS

#endif
