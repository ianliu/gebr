/*
 * gebr-comm-utils.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_COMM_UTILS_H__
#define __GEBR_COMM_UTILS_H__

#include <glib.h>

/**
 * gebr_get_xauth_cookie:
 *
 * Get this X session magic cookie as a string.
 */
gchar *gebr_get_xauth_cookie(const gchar *display_number);

/**
 * gebr_convert_xauth_cookie_to_binary:
 *
 * Converts the xauth cookie from a string representing hexdecimal digits to
 * binary data.
 */
GByteArray *gebr_convert_xauth_cookie_to_binary(const gchar *xauth_str);

/**
 * gebr_get_display:
 *
 * Returns: the display number from $DISPLAY environment variable.
 */
gchar *gebr_get_display(void);

#endif /* end of include guard: __GEBR_COMM_UTILS_H__ */
