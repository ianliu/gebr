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
#include <libgebr/utils.h>
#include "gebr-comm-listensocket.h"

#include <stdlib.h>
#include <string.h>

void
gebr_comm_get_display(gchar **x11_file, guint *port, gchar **host)
{
	const gchar *display = g_getenv("DISPLAY");

	*x11_file = NULL;
	*port = 0;
	*host = NULL;

	if (!display || !*display)
		return;

	gchar **v = g_strsplit(display, ":", -1);

	if (g_strv_length(v) != 2)
		return;

	const gchar *addr = v[0];
	const gchar *numb = v[1];

	gchar *tmp;
	const gchar *dot = strchr(numb, '.');
	if (dot)
		tmp = g_strndup(numb, (dot - numb) / sizeof(gchar));
	else
		tmp = g_strdup(numb);

	if (strlen(addr) == 0) {
		*x11_file = g_strdup_printf("/tmp/.X11-unix/X%s", tmp);
		*host = g_strdup("127.0.0.1");

		if (!g_file_test(*x11_file, G_FILE_TEST_EXISTS)) {
			g_free(*x11_file);
			*x11_file = NULL;
			*port = atoi(tmp) + 6000;
		}
	} else {
		*port = atoi(tmp) + 6000;
		*host = g_strdup(addr);
	}

	g_free(tmp);
	g_strfreev(v);
}

gboolean
gebr_comm_is_address_equal(const gchar *_addr1, const gchar *_addr2)
{
	gchar *u1, *u2;
	gchar *a1, *a2;

	a1 = strchr(_addr1, '@');
	a2 = strchr(_addr2, '@');

	if (!a1) {
		u1 = g_strdup(g_get_user_name());
		a1 = g_strdup(_addr1);
	} else {
		u1 = g_strndup(_addr1, (a1 - _addr1) / sizeof(gchar));
		a1 = g_strdup(a1 + 1);
	}

	if (!a2) {
		u2 = g_strdup(g_get_user_name());
		a2 = g_strdup(_addr2);
	} else {
		u2 = g_strndup(_addr2, (a2 - _addr2) / sizeof(gchar));
		a2 = g_strdup(a2 + 1);
	}

	gboolean ret = g_strcmp0(a1, a2) == 0 && g_strcmp0(u1, u2) == 0;

	g_free(a1);
	g_free(a2);
	g_free(u1);
	g_free(u2);

	return ret;
}

static gchar *
gebr_comm_gebr_get_dafault_keys(void)
{
	const gchar *default_keys[] = {"id_rsa", "id_dsa", "identity", NULL};
	GString *keys = g_string_new(NULL);

	for (gint i = 0; default_keys[i]; i++) {
		gchar *default_key = g_build_filename(g_get_home_dir(), ".ssh", default_keys[i], NULL);

		if (g_file_test(default_key, G_FILE_TEST_EXISTS)) {
			gchar *cmd = g_strdup_printf(" -i %s", default_key);
			keys = g_string_append(keys, cmd);
			g_free(cmd);
		}

		g_free(default_key);
	}

	return g_string_free(keys, FALSE);
}

gchar *
gebr_comm_get_ssh_command_with_key(void)
{
	const gchar *default_keys = gebr_comm_gebr_get_dafault_keys();
	gchar *basic_cmd;
	if (default_keys)
		basic_cmd = g_strdup_printf("ssh -o NoHostAuthenticationForLocalhost=yes %s", default_keys);
	else
		basic_cmd = g_strdup("ssh -o NoHostAuthenticationForLocalhost=yes");

	gchar *path = gebr_key_filename(FALSE);
	gchar *ssh_cmd;

	if (g_file_test(path, G_FILE_TEST_EXISTS))
		ssh_cmd = g_strconcat(basic_cmd, " -i ", path, NULL);
	else
		ssh_cmd = g_strdup(basic_cmd);

	g_free(path);
	g_free(basic_cmd);

	return ssh_cmd;
}

guint
gebr_comm_get_available_port(guint start)
{
	guint port = start;
	while (!gebr_comm_listen_socket_is_local_port_available(port))
		port++;
	return port;
}

gboolean
gebr_comm_is_local_address(const gchar *addr)
{
	if (g_strcmp0(g_get_host_name(), addr) == 0
	    || g_strcmp0(addr, "localhost") == 0
	    || g_strcmp0(addr, "127.0.0.1") == 0)
		return TRUE;
	return FALSE;
}
