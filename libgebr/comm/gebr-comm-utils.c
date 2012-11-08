/*
 * gebr-comm-utils.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

#include "gebr-comm-utils.h"

#include <stdlib.h>
#include <string.h>

void
gebr_comm_get_display(gchar **x11_file, guint *port, gchar **host)
{
	const gchar *display = g_getenv("DISPLAY");

	if (!display || !*display) {
		*x11_file = NULL;
		*port = 0;
		*host = NULL;
		return;
	}

	gchar **v = g_strsplit(display, ":", -1);

	if (g_strv_length(v) != 2) {
		*x11_file = NULL;
		*port = 0;
		*host = NULL;
		return;
	}

	const gchar *addr = v[0];
	const gchar *numb = v[1];

	gchar *tmp;
	const gchar *dot = strchr(numb, '.');
	if (dot)
		tmp = g_strndup(numb, (dot - numb) / sizeof(gchar));
	else
		tmp = g_strdup(numb);

	if (strlen(addr) == 0) {
		*port = 0;
		*host = g_strdup("127.0.0.1");
		*x11_file = NULL;
	} else {
		*port = atoi(tmp) + 6000;
		*host = g_strdup(addr);
		*x11_file = g_strdup_printf("/tmp/.X11-unix/X%d", *port);

		if (!g_file_test(*x11_file, G_FILE_TEST_EXISTS)) {
			g_free(*x11_file);
			*x11_file = NULL;
		}
	}

	g_free(tmp);
	g_strfreev(v);
}
