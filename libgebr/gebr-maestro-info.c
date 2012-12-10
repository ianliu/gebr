/*
 * gebr-maestro-info.c
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

#include "gebr-maestro-info.h"

gchar *
gebr_maestro_info_get_home_uri(GebrMaestroInfo *self)
{
	return self->get_home_uri(self);
}

gchar *
gebr_maestro_info_get_home_mount_point(GebrMaestroInfo *self)
{
	return self->get_home_mount_point(self);
}

gboolean
gebr_maestro_info_get_need_gvfs(GebrMaestroInfo *self)
{
	return self->get_need_gvfs(self);
}
