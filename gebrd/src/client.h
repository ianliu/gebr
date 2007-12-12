/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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
#include <comm/gtcpsocket.h>

struct protocol;

struct client {
	/* the communication channel. */
	GTcpSocket *		tcp_socket;
	/* protocol parsing stuff */
	struct protocol *	protocol;
	/* display */
	GString *		display;
	/* magic cookie for xauth */
	GString *		mcookie;
	/* 127.0.0.1 or and remote address got via ssh*/
	GString *		address;
};

void
client_add(GTcpSocket * tcp_socket);

void
client_free(struct client * client);

gboolean
client_is_local(struct client * client);

#endif //__CLIENT_H
