/*
 * gebr-comm-utils.c
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

#include "gebr-comm-utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

gchar *
gebr_get_xauth_cookie(const gchar *display_number)
{
	if (!display_number)
		return g_strdup("");

	gchar *mcookie_str = g_new(gchar, 33);
	GString *cmd_line = g_string_new(NULL);

	g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display_number);

	g_debug("GET XATUH COOKIE WITH COMMAND: %s", cmd_line->str);

	/* WORKAROUND: if xauth is already executing it will lock
	 * the auth file and it will fail to retrieve the m-cookie.
	 * So, as a workaround, we try to get the m-cookie many times.
	 */
	gint i;
	for (i = 0; i < 5; i++) {
		FILE *output_fp = popen(cmd_line->str, "r");
		if (fscanf(output_fp, "%32s", mcookie_str) != 1)
			usleep(100*1000);
		else {
			pclose(output_fp);
			break;
		}
		pclose(output_fp);
	}

	if (i == 5)
		strcpy(mcookie_str, "");

	g_debug("===== COOKIE ARE %s", mcookie_str);

	g_string_free(cmd_line, TRUE);

	return mcookie_str;
}

GByteArray *
gebr_convert_xauth_cookie_to_binary(const gchar *xauth_str)
{
	gint i;
	guint value;
	guint data_len = (guint) strlen(xauth_str) / 2;
	GByteArray *ba = g_byte_array_new();
	for (i = 0; i < data_len; i++) {
		if (sscanf(xauth_str + 2 * i, "%2x", &value) != 1) {
			perror("sscanf");
			exit(-1);
		}
		g_byte_array_append(ba, (guint8*)&value, 1);
	}
	return NULL;
}

gchar *
gebr_get_display(void)
{
	const gchar *display = getenv("DISPLAY");

	if (!display)
		return g_strdup("");
	else
		return g_strdup(strchr(display, ':'));
}
