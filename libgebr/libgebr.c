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

#include <glib.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "libgebr-gettext.h"
#include <glib/gi18n-lib.h>

#include "libgebr.h"

void gebr_libinit(const gchar * gettext_package)
{
	g_return_if_fail (gettext_package != NULL);

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	if (strcmp (gettext_package, "libgebr") != 0) {
		bindtextdomain ("libgebr", PACKAGE_LOCALE_DIR);
		bind_textdomain_codeset ("libgebr", "UTF-8");
	}
	bindtextdomain (gettext_package, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (gettext_package, "UTF-8");
	textdomain (gettext_package);
}
