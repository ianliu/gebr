/*   GeBR Daemon - Process and control execution of flows
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

#include <locale.h>
/* TODO: Check for libintl on configure */
#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <glib.h>

#include "gebrd.h"
#include "support.h"

int
main(int argc, char ** argv)
{
	static GOptionEntry	entries[] = {
		{"foreground", 'f', 0, G_OPTION_ARG_NONE, &gebrd.options.foreground,
			"Run server in foreground mode, not as a daemon", ""},
		{NULL}
	};
	GError *		error = NULL;
	GOptionContext *	context;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	context = g_option_context_new("GeBR daemon");
	g_option_context_set_summary(context,
		""
	);
	g_option_context_set_description(context,
		""
	);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_ignore_unknown_options(context, FALSE);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE || argv == NULL) {
		fprintf(stderr, _("%s: syntax error\n"), argv[0]);
		fprintf(stderr, _("Try %s --­­help\n"), argv[0]);
		return -1;
	}

	gebrd_init();

	return 0;
}
