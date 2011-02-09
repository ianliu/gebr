/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2008  Br ulio Barros de Oliveira (brauliobo@gmail.com)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <glib/gi18n.h>

#include "libgebr.h"
#include "../comm/gebr-comm-channelsocket.h"

gint channel_do(const gchar * source, const gchar * destination);

void cmdline_print_error(char **argv)
{
	fprintf(stderr, _("%s: syntax error\n"), argv[0]);
	fprintf(stderr, _("Try %s --help\n"), argv[0]);
}

int main(int argc, char **argv)
{
	static gchar **host_pair = NULL;
	static GOptionEntry entries[] = {
		{G_OPTION_REMAINING, 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &host_pair, "",
			"192.168.0.1:6010 /tmp/.X11-unix/X0"},
		{NULL}
	};
	gint ret;
	GError *error = NULL;
	GOptionContext *context;

	setlocale (LC_ALL, "");
	gebr_libinit("libgebr", argv[0]);

	context = g_option_context_new(NULL);
	g_option_context_set_summary(context, _("LibGebrComm socket channelling"));
	g_option_context_set_description(context, _("You can use this to do socket channelling, e.g., from a tcp host-port to another tcp host-port or an unix socket"));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	/* Parse command line */
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		cmdline_print_error(argv);
		ret = -1;
		goto out;
	}
	if (host_pair == NULL || g_strv_length(host_pair) != 2) {
		cmdline_print_error(argv);
		ret = -1;
		if (host_pair != NULL)
			g_strfreev(host_pair);
		goto out;
	}

	ret = channel_do(host_pair[0], host_pair[1]);

	g_strfreev(host_pair);
out:	g_option_context_free(context);

	return ret;
}

gint channel_do(const gchar * source, const gchar * destination)
{
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_type_init();

	GebrCommSocketAddress sourceaddress = gebr_comm_socket_address_parse_from_string(source);
	if (gebr_comm_socket_address_get_type(&sourceaddress) == GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN) {
		fprintf(stderr, _("Unknown source address\n"));
		return -2;
	}
	GebrCommSocketAddress destinationaddress = gebr_comm_socket_address_parse_from_string(destination);
	if (gebr_comm_socket_address_get_type(&destinationaddress) == GEBR_COMM_SOCKET_ADDRESS_TYPE_UNKNOWN) {
		fprintf(stderr, _("Unknown destination address\n"));
		return -3;
	}
	GebrCommChannelSocket *channel = gebr_comm_channel_socket_new();
	gebr_comm_channel_socket_start(channel, &sourceaddress, &destinationaddress);
	//	fprintf(stderr, _("Could not create channel\n"));
	//	return -4;
	//}
	fprintf(stdout, _("Channel created successful. Forwarding connections...\n"));

	g_main_loop_run(loop);

	return 0;
	}

