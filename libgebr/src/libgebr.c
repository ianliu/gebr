/*   GeBR Library - Common stuff used by GeBR programs
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

#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#ifdef ENABLE_NLS
#	include <libintl.h>
#endif

#include "libgebr.h"
#include "defines.h"

void gebr_libinit(const gchar * gettext_package, const gchar * argv0)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	if (gettext_package != NULL)
		textdomain(gettext_package);

#if (LIBGEBR_STATIC_MODE == 1)
	GString *cwd = g_string_new(NULL);
	gchar *tmp = g_get_current_dir();
	gchar *tmp2 = g_path_get_dirname(argv0);
	g_string_printf(cwd, "%s/%s/", tmp, tmp2);
	g_free(tmp);
	g_free(tmp2);
	if (g_file_test(cwd->str, G_FILE_TEST_IS_DIR))
		chdir(cwd->str);

	GString *path = g_string_new(NULL);
	g_string_printf(path, "%s:%s", cwd->str, getenv("PATH"));
	setenv("PATH", path->str, 1);
	g_string_free(path, TRUE);

	g_string_free(cwd, TRUE);
#endif
}
