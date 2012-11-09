/*
 * gebr-comm-utils.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

G_BEGIN_DECLS

/**
 * gebr_comm_get_display:
 *
 * All arguments are return values.
 *
 * This method checks the value of $DISPLAY and sets the arguments
 * appropriately.
 *
 * Examples:
 *    DISPLAY=:0.0          -> x11_file = /tmp/.X11-unix/X0, port = 0,    host = localhost
 *    DISPLAY=:1.0          -> x11_file = /tmp/.X11-unix/X1, port = 0,    host = localhost
 *    DISPLAY=localhost:1.0 -> x11_file = NULL,              port = 6001, host = localhost
 *
 * You must free the string parameters if they are non-%NULL.
 */
void gebr_comm_get_display(gchar **x11_file, guint *port, gchar **host);

gboolean gebr_comm_is_address_equal(const gchar *addr1, const gchar *addr2);

G_END_DECLS

#endif /* end of include guard: __GEBR_COMM_UTILS_H__ */

