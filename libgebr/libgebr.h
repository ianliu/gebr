/*   libgebr - GeBR Library
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

#ifndef __LIBGEBR_H
#define __LIBGEBR_H

#include <locale.h>

#include "defines.h"
#include "date.h"
#include "log.h"
#include "utils.h"
#include "validate.h"

G_BEGIN_DECLS

/**
 * Call this before gtk/glib initialization and before any internationalized string.
 * If static mode is enabled, then change the current working directory to the binary directory,
 * and prepend it to PATH.
 */
void gebr_libinit(const gchar * gettext_package, const gchar * argv0);

G_END_DECLS
#endif				// __LIBGEBR_H
