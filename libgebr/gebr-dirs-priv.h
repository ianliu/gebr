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

#ifndef __GEBR_DIRS_PRIV_H__
#define __GEBR_DIRS_PRIV_H__

#include <glib.h>

/**
 * gebr_dirs_set_dtd_dir:
 * @path: The new DTD search path.
 *
 * Sets the directory for searching for DTDs. The maximum length for @path is
 * 1024 bytes. If @path does not exists, then it is set to the default path
 * ${datadir}/libgebr/geoxml/data.
 */
void gebr_dirs_set_dtd_dir(const gchar *path);

/**
 * gebr_dirs_get_dtd_dir:
 *
 * Returns: The last DTD search path set with gebr_dirs_set_dtd_dir() or the
 * default value if it was never set.
 */
const gchar *gebr_dirs_get_dtd_dir(void);

#endif /* __GEBR_DIRS_PRIV_H__ */
