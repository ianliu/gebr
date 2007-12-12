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

#ifndef __FAKE_CLIENT_H
#define __FAKE_CLIENT_H

#include <glib.h>
#include <comm/gtcpsocket.h>

extern struct fake_client	fake_client;

struct fake_client {
	GMainLoop *	main_loop;
};

struct fake_client_info {
	/* a pointer to the message code */
	gchar *		last_message_sent;
	/* some messages sent by the client return value */
	gboolean	waiting_return;

};

void
fake_client_connect_to_server(void);

void
fake_client_connected(GTcpSocket * tcp_socket);

void
fake_client_disconnected(GTcpSocket * tcp_socket);

void
fake_client_read(GTcpSocket * tcp_socket);

void
fake_client_write(GTcpSocket * tcp_socket);

void
fake_client_error(GTcpSocket * tcp_socket, enum GSocketError error);

#endif //__FAKE_CLIENT_H
