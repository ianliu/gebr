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

#ifndef __CLIENT_H
#define __CLIENT_H

#include <glib.h>
#include <libgebr/comm/streamsocket.h>

struct gebr_comm_protocol;

struct client {
	GebrCommStreamSocket *stream_socket;
	struct gebr_comm_protocol *protocol;

	gboolean is_local;
	/* x11 redirected display, if server is remote. if local this is the true display */
	GString *display;
};

void client_add(GebrCommStreamSocket * stream_socket);

void client_free(struct client *client);

#endif				//__CLIENT_H
