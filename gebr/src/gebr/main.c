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
#include <locale.h>
#ifdef ENABLE_NLS
#	include <libintl.h>
#endif

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/gui/icons.h>

#include "gebr.h"
#include "../defines.h"
#include "interface.h"

int
main(int argc, char ** argv, char ** env)
{
	gboolean		show_version = FALSE;
        gboolean                overwrite = FALSE;
        gchar **                menus = NULL;
	GOptionEntry		entries[] = {
		{"version", 'V', 0, G_OPTION_ARG_NONE, &show_version, _("Show GeBR version"), NULL},
                {"install-menu", 'I', 0, G_OPTION_ARG_FILENAME_ARRAY, &menus, _("Install menus into private menu directory"), NULL},
                {"force-overwrite", 'f', 0, G_OPTION_ARG_NONE, &overwrite, _("Force overwriting when installing menus"), NULL},
		{NULL}
	};
	GError *		error = NULL;
	GOptionContext *	context;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	context = g_option_context_new(_(" - GeBR, a seismic processing environment"));
	g_option_context_set_summary(context,
		_("GeBR is a free-software which provides an environment to seismic\n"
			"data processing, designed to easily assemble and run processing\n"
			"flows, as well as to manage jobs.")
	);
	g_option_context_set_description(context,
		_("GeBR Project - http://www.gebrproject.com/"));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		fprintf(stderr, _("%s: syntax error\n"), argv[0]);
		fprintf(stderr, _("Try %s --help\n"), argv[0]);
		return -1;
	}
	if (show_version == TRUE) {
		fprintf(stdout, "%s\n", GEBR_VERSION);
		return 0;
	}
	if (menus != NULL)
		return gebr_install_private_menus(menus, overwrite);

	gtk_init(&argc, &argv);
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	gui_setup_theme();
	gui_setup_icons();
	gebr_setup_ui();
	gebr_init();

	gtk_main();

	return 0;
}
