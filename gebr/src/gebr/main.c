/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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
/* TODO: Check for libintl on configure */
#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <gtk/gtk.h>

#include "gebr.h"
#include "../defines.h"
#include "support.h"
#include "interface.h"

int
main(int argc, char ** argv, char ** env)
{
	gboolean		show_version;
	GOptionEntry		entries[] = {
		{"version", 0, 0, G_OPTION_ARG_NONE, &show_version,
			"Show GeBR daemon version", NULL},
		{NULL}
	};
	GError *		error = NULL;
	GOptionContext *	context;

	context = g_option_context_new(_("GeBR"));
	g_option_context_set_summary(context,
		_("a seismic processing environment")
	);
	g_option_context_set_description(context,
		_("")
	);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE || argv == NULL) {
		fprintf(stderr, _("%s: syntax error\n"), argv[0]);
		fprintf(stderr, _("Try %s --­­help\n"), argv[0]);
		return -1;
	}

	if (show_version == TRUE) {
		fprintf(stdout, _("%s\n"), GEBR_VERSION);
		return 0;
	}

	gtk_init(&argc, &argv);

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif
	/* FIXME: necessary for representing fractional numbers only with comma */
	setlocale(LC_NUMERIC, "C");
	assembly_interface();
	gebr_init(argc, argv);

	gtk_main();

	return 0;
}
