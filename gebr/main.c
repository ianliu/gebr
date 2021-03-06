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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gebr-gettext.h"

#include <stdlib.h>
#include <locale.h>

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-icons.h>
#include <libgebr/libgebr.h>
#include <libgebr/gebr-version.h>

#include "gebr.h"
#include "interface.h"


int main(int argc, char **argv, char **env)
{
	gboolean show_version = FALSE;
	gboolean show_sys_dir = FALSE;
	GOptionEntry entries[] = {
		{"query-system-menu", 's', 0, G_OPTION_ARG_NONE, &show_sys_dir,
		 _("Returns the users menu directory"), NULL},
		{"version", 'V', 0, G_OPTION_ARG_NONE, &show_version,
		 _("Show GeBR's version"), NULL},
		{NULL}
	};
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();
	g_thread_init(NULL);
	gdk_threads_init();

	gebr_libinit(GETTEXT_PACKAGE);

	gebr_geoxml_init();

	context = g_option_context_new(_(" - GeBR, a seismic processing environment"));
	g_option_context_set_summary(context,
				     _("GeBR is a free-software which provides an environment to process seismic\n"
				       "data, designed to easily assemble and run processing\n"
				       "flows, as well as manage jobs."));
	g_option_context_set_description(context, _("GeBR Project - http://www.gebrproject.com/"));
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

	if (show_sys_dir == TRUE) {
		GKeyFile *file;
		GString *path;
		GError *error;
		gchar *usermenus;

		file = g_key_file_new();
		path = g_string_new(NULL);
		error = NULL;

		g_string_printf(path, "%s/.gebr/gebr/gebr.conf", g_get_home_dir());
		g_key_file_load_from_file(file, path->str, G_KEY_FILE_NONE, &error);

		if (error) {
			fprintf(stderr, _("ERROR: Could not find GeBR configuration file at %s.\n"), path->str);
			return 1;
		}

		usermenus = g_key_file_get_string(file, "general", "usermenus", &error);
		if (!usermenus) {
			fprintf(stderr, _("ERROR: Local menu directory is not defined in GeBR's settings.\n"));
			return 1;
		}

		fprintf(stdout, "%s\n", usermenus);

		g_free(usermenus);
		g_key_file_free(file);
		g_string_free(path, TRUE);
		return 0;
	}

	gtk_init(&argc, &argv);

	gdk_threads_enter();
	gboolean has_config = gebr_config_load();

	gebr_gui_setup_icons();
	gebr_setup_ui();
	gebr_init(has_config);

	gtk_main();
	gdk_threads_leave();

	gebr_geoxml_finalize();

	return 0;
}
