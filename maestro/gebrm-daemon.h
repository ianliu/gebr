/*
 * gebrm-daemon.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
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

#ifndef __GEBRM_DAEMON_H__
#define __GEBRM_DAEMON_H__

#include <glib-object.h>
#include <libgebr/comm/gebr-comm.h>

G_BEGIN_DECLS

#define GEBRM_TYPE_DAEMON            (gebrm_daemon_get_type ())
#define GEBRM_DAEMON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRM_TYPE_DAEMON, GebrmDaemon))
#define GEBRM_DAEMON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBRM_TYPE_DAEMON, GebrmDaemonClass))
#define GEBRM_IS_DAEMON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRM_TYPE_DAEMON))
#define GEBRM_IS_DAEMON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBRM_TYPE_DAEMON))
#define GEBRM_DAEMON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBRM_TYPE_DAEMON, GebrmDaemonClass))

typedef struct _GebrmDaemon GebrmDaemon;
typedef struct _GebrmDaemonPriv GebrmDaemonPriv;
typedef struct _GebrmDaemonClass GebrmDaemonClass;

struct _GebrmDaemon {
	GObject parent;
	GebrmDaemonPriv *priv;
};

struct _GebrmDaemonClass {
	GObjectClass parent_class;

	void (*state_change) (GebrmDaemon *daemon,
			      GebrCommServerState new_state);

	void (*task_define) (GebrmDaemon *daemon,
			     GObject *task);
};

GType gebrm_daemon_get_type(void) G_GNUC_CONST;

GebrmDaemon *gebrm_daemon_new(const gchar *address);

const gchar *gebrm_daemon_get_address(GebrmDaemon *daemon);

/**
 * gebrm_daemon_add_tag:
 *
 * Adds @tag into @daemon's tags list if it does not exists.
 */
void gebrm_daemon_add_tag(GebrmDaemon *daemon,
			  const gchar *tag);

/**
 * gebrm_daemon_update_tag:
 *
 * Update @tag into @daemon's tags list if it does not exists.
 */
gboolean
gebrm_daemon_update_tags(GebrmDaemon *daemon, gchar **tags);

/**
 * gebrm_daemon_has_tag:
 *
 * Returns: %TRUE if @daemon has @tag.
 */
gboolean gebrm_daemon_has_tag(GebrmDaemon *daemon,
			      const gchar *tag);

/**
 * gebrm_daemon_get_tags:
 *
 * Returns: All tags sorted and concatenated with commas. Free with g_free().
 */
gchar *gebrm_daemon_get_tags(GebrmDaemon *daemon);

/**
 * gebrm_daemon_connect:
 *
 * Connects to this @daemon.
 */
void gebrm_daemon_connect(GebrmDaemon *daemon);

void gebrm_daemon_disconnect(GebrmDaemon *daemon);

GebrCommServer *gebrm_daemon_get_server(GebrmDaemon *daemon);

G_END_DECLS

#endif /* __GEBRM_DAEMON_H__ */
