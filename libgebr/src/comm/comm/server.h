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

#ifndef __COMM_SERVER_H
#define __COMM_SERVER_H

#include <glib.h>

#include <log.h>
#include <geoxml.h>

#include "gstreamsocket.h"
#include "protocol.h"
#include "gterminalprocess.h"
#include "gprocess.h"
#include "gchannelsocket.h"

struct gebr_comm_server {
	/* the communication channel. */
	GStreamSocket *		stream_socket;
	/* protocol parsing stuff */
	struct protocol *	protocol;
	/* server address/port */
	GString *		address;
	guint16			port;
	/* ssh stuff */
	GString *		password;
	gint16			tunnel_port;
	gboolean		tried_existant_pass;
	GTerminalProcess *	x11_forward_process;
	GebrCommChannelSocket *	x11_forward_channel;

	enum comm_server_state {
		SERVER_STATE_DISCONNECTED,
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
		void		(*parse_messages)(struct gebr_comm_server * gebr_comm_server, gpointer user_data);
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

struct gebr_comm_server *
gebr_comm_server_new(const gchar * _address, const struct comm_server_ops * ops);

void
gebr_comm_server_free(struct gebr_comm_server * comm_server);

void
gebr_comm_server_connect(struct gebr_comm_server * comm_server);

void
gebr_comm_server_disconnect(struct gebr_comm_server * comm_server);

gboolean
gebr_comm_server_is_logged(struct gebr_comm_server * comm_server);

gboolean
gebr_comm_server_is_local(struct gebr_comm_server * comm_server);

void
gebr_comm_server_kill(struct gebr_comm_server * comm_server);

gboolean
gebr_comm_server_forward_x11(struct gebr_comm_server * comm_server, guint16 port);

void
gebr_comm_server_run_flow(struct gebr_comm_server * comm_server, GebrGeoXmlFlow * flow);

#endif //__COMM_SERVER_H
