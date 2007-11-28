/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef _GEBR_SERVER_H_
#define _GEBR_SERVER_H_

#include <misc/gtcpsocket.h>
#include <misc/protocol.h>
#include <misc/ssh.h>

struct server {
	/* the communication channel. */
	GTcpSocket *		tcp_socket;
	/* protocol parsing stuff */
	struct protocol *	protocol;
	/* address */
	GString *		address;
	guint16			port;
	/* ssh tunneling */
	struct ssh_tunnel	ssh_tunnel;

	guint			retries;
};

struct server *
server_new(const gchar * address);

void
server_free(struct server * server);

void
server_run_flow(struct server * server);

void
server_list_flows(struct server * server);

#endif //_GEBR_SERVER_H_
