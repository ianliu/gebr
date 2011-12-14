/*   libgebr - GeBR Library
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

#ifndef __GEBR_COMM_SERVER_H
#define __GEBR_COMM_SERVER_H

#include <glib.h>

#include <gebr-validator.h>
#include <libgebr/log.h>
#include <libgebr/geoxml/geoxml.h>

#include "gebr-comm-protocol-socket.h"
#include "gebr-comm-terminalprocess.h"
#include "gebr-comm-process.h"
#include "gebr-comm-channelsocket.h"

G_BEGIN_DECLS

/**
 */
typedef enum {
	GEBR_COMM_SERVER_TYPE_MOAB,
	GEBR_COMM_SERVER_TYPE_REGULAR,
	GEBR_COMM_SERVER_TYPE_UNKNOWN
} GebrCommServerType;

/**
 */
typedef enum {
	GEBR_COMM_SERVER_LOCATION_UNKNOWN,
	GEBR_COMM_SERVER_LOCATION_LOCAL,
	GEBR_COMM_SERVER_LOCATION_REMOTE
} GebrCommServerLocation;

/**
 */
gchar * gebr_comm_server_get_user(const char * address);

/**
 */
GebrCommServerType gebr_comm_server_get_id(const gchar * name);

typedef enum {
	SERVER_STATE_UNKNOWN,
	SERVER_STATE_DISCONNECTED,
	SERVER_STATE_RUN,
	SERVER_STATE_OPEN_TUNNEL,
	SERVER_STATE_CONNECT,
} GebrCommServerState;

typedef struct gebr_comm_server GebrCommServer;

struct gebr_comm_server {
	GebrCommProtocolSocket *socket;

	/* server address/port */
	GString *address;
	guint16 port;
	gboolean ac;

	/* ssh stuff */
	gchar *password;
	gboolean tried_existant_pass;
	gint16 tunnel_port;
	GebrCommTerminalProcess *x11_forward_process;
	GebrCommProcess *x11_forward_unix;

	gint ncores;
	gdouble clock_cpu;

	GebrCommServerState state;

	enum gebr_comm_server_error {
		SERVER_ERROR_UNKNOWN,
		SERVER_ERROR_NONE,
		SERVER_ERROR_CONNECT,
		SERVER_ERROR_SERVER,
		SERVER_ERROR_SSH,
	} error;

	GString *last_error;

	/* virtual methods */
	const struct gebr_comm_server_ops {
		void     (*log_message)      (GebrCommServer *server,
					      GebrLogMessageType type,
					      const gchar *message,
					      gpointer user_data);

		void     (*state_changed)    (GebrCommServer *server,
					      gpointer user_data);

		GString *(*ssh_login)        (GebrCommServer *server,
					      const gchar *title,
					      const gchar *message,
					      gpointer user_data);

		gboolean (*ssh_question)     (GebrCommServer *server,
					      const gchar *title,
					      const gchar *message,
					      gpointer user_data);

		void     (*process_request)  (GebrCommServer *server,
					      GebrCommHttpMsg *request,
					      gpointer user_data);

		void     (*process_response) (GebrCommServer *server,
					      GebrCommHttpMsg *request,
					      GebrCommHttpMsg *response,
					      gpointer user_data);

		void     (*parse_messages)   (GebrCommServer *server,
					      gpointer user_data);
	} *ops;
	gpointer user_data;

	/* temporary process */
	struct gebr_comm_server_process {
		enum gebr_comm_server_process_use {
			COMM_SERVER_PROCESS_NONE,
			COMM_SERVER_PROCESS_TERMINAL,
			COMM_SERVER_PROCESS_REGULAR,
		} use;
		union gebr_comm_server_process_data {
			GebrCommTerminalProcess *terminal;
			GebrCommProcess *regular;
		} data;
	} process;
	guint tunnel_pooling_source;
};

/**
 */
struct gebr_comm_server *gebr_comm_server_new(const gchar * _address, const struct gebr_comm_server_ops *ops);

/**
 */
const gchar *gebr_comm_server_get_last_error(struct gebr_comm_server *server);
/**
 */
void gebr_comm_server_free(struct gebr_comm_server *gebr_comm_server);

/**
 */
void gebr_comm_server_connect(struct gebr_comm_server *server,
			      gboolean maestro);

/**
 */
void gebr_comm_server_disconnect(struct gebr_comm_server *gebr_comm_server);

/**
 */
gboolean gebr_comm_server_is_logged(struct gebr_comm_server *gebr_comm_server);

/**
 */
gboolean gebr_comm_server_is_local(struct gebr_comm_server *gebr_comm_server);

/**
 */
void gebr_comm_server_kill(struct gebr_comm_server *gebr_comm_server);

/**
 * For the logged _gebr_comm_server_ forward x11 server _port_ to user display
 * Fail if user's display is not set, returning FALSE.
 * If any other x11 redirect was previously made it is unmade
 */
gboolean gebr_comm_server_forward_x11(struct gebr_comm_server *gebr_comm_server, guint16 port);

const gchar *gebr_comm_server_state_to_string(GebrCommServerState state);

GebrCommServerState gebr_comm_server_state_from_string(const gchar *string);

GebrCommServerState gebr_comm_server_get_state(GebrCommServer *server);

void gebr_comm_server_set_password(GebrCommServer *server,
				   const gchar *pass);

G_END_DECLS

#endif				//__GEBR_COMM_SERVER_H
