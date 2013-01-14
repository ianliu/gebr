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
#include <libgebr/comm/gebr-comm-protocol-socket.h>
#include <libgebr/comm/gebr-comm-server.h>

G_BEGIN_DECLS

struct gebr_comm_protocol;

struct client {
	GebrCommProtocolSocket *socket;
	GebrCommServerLocation server_location;
	/* x11 redirected display, if server is remote. if local this is the true display */
	GString *display;
	guint16 display_port;
	gchar *gebr_cookie;
};

void client_add(GebrCommProtocolSocket *client);

void client_free(struct client *client);

void client_disconnected(GebrCommProtocolSocket * socket,
                         struct client *client);

GList * parse_comma_separated_string(gchar *str);

G_END_DECLS
#endif				//__CLIENT_H
