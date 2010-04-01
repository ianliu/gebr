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

#include <libgebr/comm/listensocket.h>
#include <libgebr/comm/server.h>
#include <libgebr/log.h>
#include <netdb.h>
#include <libgebr/comm/server.h>

/**
 * \file gebrd.h
 * \brief Global access variable for date interchange 
 */

extern struct gebrd gebrd;

/**
 * \struct gebrd gebr.h
 * \brief Global access variable for date interchange 
 * \see gebrd.h
 */
struct gebrd {
	GebrCommListenSocket *listen_socket;
	GList *clients;
	GList *jobs;
	struct gebr_log *log;
	gchar hostname[256];

	GString *run_filename;
	GHashTable *queues;

	/**
	 * Options passed through command line
	 */
	struct gebrd_options {
		gboolean foreground;
	} options;

	GList * mpi_flavors;

	GMainLoop *main_loop;
	int finished_starting_pipe[2];
};

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

#endif				//__GEBRD_H
