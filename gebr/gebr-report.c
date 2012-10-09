/*
 * gebr-report.c
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

#ifdef HAVE_CONFIG
# include <config.h>
#endif

#include "gebr-report.h"

gpointer
gebr_report_copy(gpointer boxed)
{
	return NULL;
}

void
gebr_report_free(gpointer boxed)
{
	g_free(boxed);
}

GType
gebr_report_get_type(void)
{
	static GType type_id = 0;

	if (type_id == 0)
		type_id = g_boxed_type_register_static(
				g_intern_static_string("GebrReport"),
				gebr_report_copy,
				gebr_report_free);

	return type_id;
}

GebrReport *
gebr_report_new(void)
{
	return NULL;
}
