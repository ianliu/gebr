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

#ifndef __GEBRD_H
#define __GEBRD_H

#include <libgebr/comm/gebr-comm-listensocket.h>
#include <libgebr/comm/gebr-comm-server.h>
#include <libgebr/log.h>
#include <libgebr/comm/gebr-comm-server.h>

#include "gebrd-mpi-interface.h"
#include "gebrd-user.h"

G_BEGIN_DECLS

GType gebrd_app_get_type(void);
#define GEBRD_APP_TYPE		(gebrd_app_get_type())
#define GEBRD_APP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRD_APP_TYPE, GebrdApp))
#define GEBRD_APP_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRD_APP_TYPE, GebrdAppClass))
#define GEBRD_IS_APP(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRD_APP_TYPE))
#define GEBRD_IS_APP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRD_APP_TYPE))
#define GEBRD_APP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRD_APP_TYPE, GebrdAppClass))

typedef struct _GebrdApp GebrdApp;
typedef struct _GebrdAppClass GebrdAppClass;

/**
 * \file gebrd.h
 * \brief Global access variable for date interchange 
 */
extern GebrdApp *gebrd;
/**
 * \struct gebrd gebr.h
 * \brief Global access variable for date interchange 
 * \see gebrd.h
 */
struct _GebrdApp {
	GObject parent;

	GebrCommListenSocket *listen_socket;
	GList *clients;
	GList * mpi_flavors;
	GebrdUser *user;
	GString *user_data_filename;

	gchar hostname[256];
	GebrCommSocketAddress socket_address;
	/**
	 * Options passed through command line
	 */
	struct gebrd_options {
		gboolean foreground;
	} options;

	GString *run_filename;
	GString *fs_lock;
	struct gebr_log *log;
	GMainLoop *main_loop;
	int finished_starting_pipe[2];
};
struct _GebrdAppClass {
	GObjectClass parent;
};

GebrdApp *gebrd_app_new(void);

/**
 * Init daemon routines.
 * Called at #main.c
 */
void gebrd_init(void);

/**
 * Called to exit glib main loop and exit program.
 */
void gebrd_quit(void);

/**
 * Log a message in the gebrd log file and to standand output.
 * The message in only shown in the stdout if gebrd is running in interative mode.
 */
void gebrd_message(enum gebr_log_message_type type, const gchar * message, ...);

/**
 * Return a free port to be used for X11 redirection.
 * The server doesn't listen to it; the client is supposed to
 * do a port forward thought SSH
 */
guint8 gebrd_get_x11_redirect_display(void);

/**
 * Check wheter running on a Moab cluster.
 */
GebrCommServerType gebrd_get_server_type(void);

/**
 * Loads gebrd configuration file into gebrd configuration structure.
 * \see gebrd.config
 */
void gebrd_config_load(void);

/**
 */
void gebrd_user_data_save(void);

/**
 * TODO
 */
const GebrdMpiConfig * gebrd_get_mpi_config_by_name(const gchar * name);

G_END_DECLS
#endif				//__GEBRD_H
