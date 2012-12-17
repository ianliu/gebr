/*
 * gebrm-proxy.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Team
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

#ifndef __GEBRM_PROXY_H__
#define __GEBRM_PROXY_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrmProxy GebrmProxy;

GebrmProxy *gebrm_proxy_new(const gchar *remote_addr,
			    guint remote_port);

void gebrm_proxy_run(GebrmProxy *proxy, gint fd);

void gebrm_proxy_free(GebrmProxy *proxy);

G_END_DECLS

#endif /* end of include guard: __GEBRM_PROXY_H__ */

