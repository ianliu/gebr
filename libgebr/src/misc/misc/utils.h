/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_MISC_UTILS_H
#define __LIBGEBR_MISC_UTILS_H

#include <glib.h>

GString *
make_unique_filename(const gchar * template);

GString *
make_temp_filename(const gchar * template);

int
home_mode(void);

gboolean
gebr_create_config_dirs(void);

#endif //__LIBGEBR_MISC_UTILS_H
