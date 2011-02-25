/*   GeBR Daemon - Process and control execution of flows
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

#ifndef __GEBRD_USER_H
#define __GEBRD_USER_H

#include <libgebr/comm/gebr-comm-listensocket.h>
#include <libgebr/comm/gebr-comm-server.h>
#include <libgebr/log.h>
#include <netdb.h>
#include <libgebr/comm/gebr-comm-server.h>

#include "gebrd-mpi-interface.h"

G_BEGIN_DECLS

GType gebrd_user_get_type(void);
#define GEBRD_USER_TYPE		(gebrd_user_get_type())
#define GEBRD_USER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRD_USER_TYPE, GebrdUser))
#define GEBRD_USER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRD_USER_TYPE, GebrdUserClass))
#define GEBRD_IS_USER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRD_USER_TYPE))
#define GEBRD_IS_USER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRD_USER_TYPE))
#define GEBRD_USER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRD_USER_TYPE, GebrdUserClass))

typedef struct _GebrdUser GebrdUser;
typedef struct _GebrdUserClass GebrdUserClass;

struct _GebrdUser {
	GObject parent;

	GString *fs_nickname;
	GList *jobs;
	GHashTable *queues;
};
struct _GebrdUserClass {
	GObjectClass parent;
};

G_END_DECLS
#endif //__GEBRD_USER_H
