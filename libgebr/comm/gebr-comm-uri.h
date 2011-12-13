/*
 * gebr-comm-uri.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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

#ifndef __GEBR_COMM_URI_H__
#define __GEBR_COMM_URI_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrCommUri GebrCommUri;

GebrCommUri *gebr_comm_uri_new();

void gebr_comm_uri_set_prefix(GebrCommUri *uri, const gchar *prefix);

const gchar *gebr_comm_uri_get_prefix(GebrCommUri *uri);

void gebr_comm_uri_add_param(GebrCommUri *uri, const gchar *key, const gchar *value);

gchar *gebr_comm_uri_to_string(GebrCommUri *uri);


void gebr_comm_uri_parse(GebrCommUri *uri, const gchar *url);

const gchar *gebr_comm_uri_get_param(GebrCommUri *uri, const gchar *param);

void gebr_comm_uri_free(GebrCommUri *uri);

G_END_DECLS

#endif /* __GEBR_COMM_URI_H__ */
