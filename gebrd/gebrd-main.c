/*   GeBR Daemon - Process and control execution of flows
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gebrd-gettext.h"

#include <locale.h>
#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#endif

#include <glib.h>

#include <glib/gi18n.h>
#include <libgebr/libgebr.h>
#include <libgebr/gebr-version.h>
#include <stdio.h>

#include "gebrd.h"

int main(int argc, char **argv)
{
	gboolean show_version = FALSE;
	gboolean foreground = FALSE;
	gboolean nocookie = FALSE;
	const GOptionEntry entries[] = {
		{"interactive", 'i', 0, G_OPTION_ARG_NONE, &foreground,
		 N_("Run server in interactive mode, not as a daemon"), NULL},
		{"version", 0, 0, G_OPTION_ARG_NONE, &show_version,
		 N_("Show GeBR daemon version"), NULL},
		{"nocookie", 'n', 0, G_OPTION_ARG_NONE, &nocookie,
		 N_("Do not ask for authorization cookie when launching"), NULL},
		{NULL}
	};
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	gebr_libinit(GETTEXT_PACKAGE);
	gebr_geoxml_init();

	context = g_option_context_new(_("GeBR daemon"));
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	g_option_context_set_summary(context, _("Run flows and manage jobs of GeBR."));
	g_option_context_set_description(context, _(
					 "Debug-compiled environment variables:\n"
					 "  GEBRD_RUN_DELAY_SEC   Delay in seconds to wait before sending response of a flow run request\n"
					 "  GEBRD_LOG_LEVEL       If equal to 'verbose', prints extra information in stdout"
					 ));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		fprintf(stderr, _("%s: syntax error\n"), argv[0]);
		fprintf(stderr, _("Try %s --help\n"), argv[0]);
		return -1;
	}

	if (show_version == TRUE) {
		fprintf(stdout, "%s (%s)\n", GEBR_VERSION NANOVERSION, gebr_version());
		return 0;
	}

	GebrAuth *auth;
	auth = gebr_auth_new();
	if (!nocookie)
		gebr_auth_read_cookie(auth);

	gebrd = gebrd_app_new();
	gebrd->options.foreground = foreground;

	gebrd_config_load(auth);
	gebrd_init();
	gebr_geoxml_finalize();

	return 0;
}
