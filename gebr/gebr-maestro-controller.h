/*
 * gebr-maestro-controller.h
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

#ifndef __GEBR_MAESTRO_CONTROLLER_H__
#define __GEBR_MAESTRO_CONTROLLER_H__

#include <gtk/gtk.h>

#include "gebr-maestro-server.h"

G_BEGIN_DECLS

#define GEBR_TYPE_MAESTRO_CONTROLLER            (gebr_maestro_controller_get_type ())
#define GEBR_MAESTRO_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_MAESTRO_CONTROLLER, GebrMaestroController))
#define GEBR_MAESTRO_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_MAESTRO_CONTROLLER, GebrMaestroControllerClass))
#define GEBR_IS_MAESTRO_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_MAESTRO_CONTROLLER))
#define GEBR_IS_MAESTRO_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_MAESTRO_CONTROLLER))
#define GEBR_MAESTRO_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_MAESTRO_CONTROLLER, GebrMaestroControllerClass))


typedef struct _GebrMaestroController GebrMaestroController;
typedef struct _GebrMaestroControllerPriv GebrMaestroControllerPriv;
typedef struct _GebrMaestroControllerClass GebrMaestroControllerClass;

struct _GebrMaestroController {
	GObject parent;

	GebrMaestroControllerPriv *priv;
};

struct _GebrMaestroControllerClass {
	GObjectClass parent_class;

	void (*job_define) (GebrMaestroController *self,
			    GebrMaestroServer *maestro,
			    GebrJob *job);

	void (*maestro_list_changed) (GebrMaestroController *self);

	void (*group_changed) (GebrMaestroController *self,
			       GebrMaestroServer     *maestro);

	void (*maestro_state_changed) (GebrMaestroController *self,
				       GebrMaestroServer     *maestro);
};

enum {
	SERVER_STATUS_ICON = 0,
	SERVER_AUTOCONNECT,
	SERVER_ADDRESS,
	SERVER_POINTER,
	SERVER_TAGS,
	SERVER_CPU,
	SERVER_MEM,
	SERVER_FS,
	SERVER_IS_AUTO_CHOOSE,
	SERVER_N_COLUMN
};
GType gebr_maestro_controller_get_type(void) G_GNUC_CONST;

GebrMaestroController *gebr_maestro_controller_new(void);

GtkDialog *gebr_maestro_controller_create_dialog(GebrMaestroController *self);

GebrMaestroServer * gebr_maestro_controller_get_maestro(GebrMaestroController *self);

void gebr_maestro_controller_connect(GebrMaestroController *self,
				     const gchar *address);

GebrMaestroServer *gebr_maestro_controller_get_maestro_for_address(GebrMaestroController *mc,
								   const gchar *address);

GebrMaestroServer *gebr_maestro_controller_get_maestro_for_line(GebrMaestroController *mc,
								GebrGeoXmlLine *line);

G_END_DECLS

#endif /* __GEBR_MAESTRO_CONTROLLER_H__ */
