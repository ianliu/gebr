/*
 * gebrd-user.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
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

#ifndef __GEBRD_USER_H__
#define __GEBRD_USER_H__

#include <libgebr/comm/gebr-comm-listensocket.h>
#include <libgebr/comm/gebr-comm-server.h>
#include <libgebr/log.h>
#include <netdb.h>
#include <libgebr/comm/gebr-comm-server.h>
#include "gebrd-client.h"

#include "gebrd-mpi-interface.h"

G_BEGIN_DECLS

#define GEBRD_USER_TYPE            (gebrd_user_get_type())
#define GEBRD_USER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRD_USER_TYPE, GebrdUser))
#define GEBRD_USER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEBRD_USER_TYPE, GebrdUserClass))
#define GEBRD_IS_USER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRD_USER_TYPE))
#define GEBRD_IS_USER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRD_USER_TYPE))
#define GEBRD_USER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRD_USER_TYPE, GebrdUserClass))

typedef struct _GebrdUser GebrdUser;
typedef struct _GebrdUserPriv GebrdUserPriv;
typedef struct _GebrdUserClass GebrdUserClass;

struct _GebrdUser {
	GObject parent;

	GebrdUserPriv *priv;

	GString *fs_nickname;
	GList *jobs;
	GHashTable *queues;
};

struct _GebrdUserClass {
	GObjectClass parent;
};

GType gebrd_user_get_type(void) G_GNUC_CONST;

const gchar *gebrd_user_get_daemon_id(GebrdUser *self);

void gebrd_user_set_daemon_id(GebrdUser *self, const gchar *id);

gboolean gebrd_user_has_connection(GebrdUser *user);

/**
 * gebrd_user_set_connection:
 *
 * This method sets the only connection for this Daemon. It is an error to call
 * it more than once with a non-%NULL @connection.
 */
void gebrd_user_set_connection(GebrdUser *user, struct client *connection);

G_END_DECLS

#endif /* __GEBRD_USER_H__ */
