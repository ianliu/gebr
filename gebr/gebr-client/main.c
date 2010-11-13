/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <stdlib.h>

#include <glib.h>

#include <libgebr/intl.h>
#include <libgebr/libgebr.h>
#include <libgebr/comm/comm.h>

#include "gebrclient.h"
#include "../defines.h"

int main(int argc, char **argv, char **env)
{
	gboolean show_version;
	GOptionEntry entries[] = {
		{"version", 0, 0, G_OPTION_ARG_NONE, &show_version,
		 "Show GeBR client version", NULL},
		{NULL}
	};
	GError *error = NULL;
	GOptionContext *context;

	gebr_libinit(GETTEXT_PACKAGE, argv[0]);

	context = g_option_context_new(_("serveraddress command [args]}"));
	g_option_context_set_summary(context, _("GeBR commandline client")
	    );
	g_option_context_set_description(context, _("a seismic processing environment")
	    );
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE || argv == NULL) {
		gebr_client_message(GEBR_LOG_ERROR, _("%s: syntax error"), argv[0]);
		gebr_client_message(GEBR_LOG_ERROR, _("Try %s --­­help"), argv[0]);
		return -1;
	}
	if (show_version == TRUE) {
		gebr_client_message(GEBR_LOG_INFO, "%s", GEBR_VERSION);
		return 0;
	}
	if (argc < 2) {
		gebr_client_message(GEBR_LOG_ERROR, _("No server address specified"));
		return -1;
	}

	gebr_client.main_loop = g_main_loop_new(NULL, FALSE);
	g_type_init();

	if (gebr_client_init(argv[1]) == TRUE) {

		g_main_loop_run(gebr_client.main_loop);
		g_main_loop_unref(gebr_client.main_loop);
	}

	return 0;
}
