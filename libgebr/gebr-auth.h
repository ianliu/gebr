/*
 * gebr-auth.h
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

#ifndef __GEBR_AUTH_H__
#define __GEBR_AUTH_H__

#include <glib.h>

#define GEBR_AUTH_COOKIE_LENGTH 64

G_BEGIN_DECLS

typedef struct _GebrAuth GebrAuth;

/**
 * gebr_auth_new:
 *
 * Creates a new GebrAuth object to handle a list of allowed cookies. See
 * gebr_auth_accepts() method for validating a cookie.
 */
GebrAuth *gebr_auth_new(void);

/**
 * gebr_auth_read_cookie:
 *
 * Reads a cookie from the standard input and add it to the list of authorized
 * cookies.
 */
void gebr_auth_read_cookie(GebrAuth *self);

/**
 * gebr_auth_accepts:
 *
 * Checks if @key is authorized. See gebr_auth_read_cookie() for authorizing
 * cookies.
 */
gboolean gebr_auth_accepts(GebrAuth *self, const gchar *key);

/**
 * gebr_auth_remove_cookie:
 *
 * Removes @key from the authorized cookies.
 */
void gebr_auth_remove_cookie(GebrAuth *self, const gchar *key);

void gebr_auth_free(GebrAuth *self);

G_END_DECLS

#endif /* end of include guard: __GEBR_AUTH_H__ */

