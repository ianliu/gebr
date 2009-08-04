/*   GeBR - An environment for seismic processing.
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

#ifndef __GEBR_CLIENT_H
#define __GEBR_CLIENT_H

#include <glib.h>

#include <libgebr/log.h>

#include "server.h"

extern struct gebr_client gebr_client;

struct gebr_client {
	struct server *	server;

	struct gebr_client_options {
		
	} options;

	GMainLoop *	main_loop;
};

gboolean
gebr_client_init(const gchar * server_address);

void
gebr_client_quit(void);

void
gebr_client_message(enum log_message_type type, const gchar * message, ...);

#endif //__GEBR_CLIENT_H
