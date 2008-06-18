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

#ifndef __LIBGEBR_COMM_SERVER_H
#define __LIBGEBR_COMM_SERVER_H

#include <glib.h>

#include <geoxml.h>
#include <misc/log.h>

#include "gtcpsocket.h"
#include "protocol.h"
#include "gterminalprocess.h"
#include "gprocess.h"

struct comm_server {
	/* the communication channel. */
	GTcpSocket *		tcp_socket;
	/* protocol parsing stuff */
	struct protocol *	protocol;
	/* server address/port */
	GString *		address;
	guint16			port;
	/* ssh stuff */
	GString *		password;
	gint16			tunnel_port;
	gboolean		tried_existant_pass;
	GTerminalProcess *	x11_forward;

	enum comm_server_state {
		SERVER_STATE_RUN,
		SERVER_STATE_OPEN_TUNNEL,
		SERVER_STATE_CONNECT,
		SERVER_STATE_CONNECTED,
	} state;
	enum comm_server_error {
		SERVER_ERROR_NONE,
		SERVER_ERROR_CONNECT,
		SERVER_ERROR_SERVER,
		SERVER_ERROR_SSH,
	} error;
	const struct comm_server_ops {
		void		(*log_message)(enum log_message_type type, const gchar * message);
		GString *	(*ssh_login)(const gchar * title, const gchar * message);
		gboolean	(*ssh_question)(const gchar * title, const gchar * message);
		void		(*parse_messages)(struct comm_server * comm_server, gpointer user_data);
	} * ops;
	gpointer		user_data;

	/* temporary process for operations */
	struct comm_server_process {
		enum comm_server_process_use {
			COMM_SERVER_PROCESS_NONE,
			COMM_SERVER_PROCESS_TERMINAL,
			COMM_SERVER_PROCESS_REGULAR,
		} use;
		union comm_server_process_data {
			GTerminalProcess *	terminal;
			GProcess *		regular;
		} data;
	} process;
};

struct comm_server *
comm_server_new(const gchar * _address, const struct comm_server_ops * ops);

void
comm_server_free(struct comm_server * comm_server);

void
comm_server_connect(struct comm_server * comm_server);

gboolean
comm_server_is_logged(struct comm_server * comm_server);

gboolean
comm_server_is_local(struct comm_server * comm_server);

gboolean
comm_server_forward_x11(struct comm_server * comm_server, guint16 port);

void
comm_server_run_flow(struct comm_server * comm_server, GeoXmlFlow * flow);

#endif //__LIBGEBR_COMM_SERVER_H
