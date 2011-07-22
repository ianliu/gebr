/*   GeBR Library - libgebr by GeBR programs
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <glib/gstdio.h>
#include <unistd.h>

#include "defines.h"

#ifdef MAXPATHLEN
# undef MAXPATHLEN
#endif
#define MAXPATHLEN 1024

static gchar dtd_dir[MAXPATHLEN] = { 0, };

void
gebr_dirs_set_dtd_dir(const gchar *path)
{
	g_return_if_fail(path != NULL);

	if (g_strlcpy(dtd_dir, path, MAXPATHLEN) >= MAXPATHLEN
	    || !g_access(dtd_dir, R_OK))
		dtd_dir[0] = '\0';
}

const gchar *
gebr_dirs_get_dtd_dir(void)
{
	if (!dtd_dir[0])
		return GEBR_GEOXML_DTD_DIR;
	return dtd_dir;
}
