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
#include <libgebr/log.h>
#include <netdb.h>

extern struct gebrd gebrd;

struct gebrd {
	GebrCommListenSocket *listen_socket;
	GList *clients;
	GList *jobs;
	struct gebr_log *log;
	gchar hostname[256];

	GString *run_filename;

	struct gebrd_options {
		gboolean foreground;
	} options;

	GMainLoop *main_loop;
	int finished_starting_pipe[2];
};

void gebrd_init(void);

void gebrd_quit(void);

void gebrd_message(enum gebr_log_message_type type, const gchar * message, ...);

guint8 gebrd_get_x11_redirect_display(void);

#endif				//__GEBRD_H
