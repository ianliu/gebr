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

#include <libgebr/log.h>
#include <libgebr/geoxml/geoxml.h>

#include "gebr-comm-streamsocket.h"
#include "gebr-comm-protocol.h"
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
	GEBR_COMM_SERVER_LOCATION_LOCAL,
	GEBR_COMM_SERVER_LOCATION_REMOTE
} GebrCommServerLocation;

/**
 */
gchar * gebr_comm_server_get_user(const char * address);

/**
 */
GebrCommServerType gebr_comm_server_get_id(const gchar * name);

/**
 */
struct gebr_comm_server {
	/* the communication channel. */
	GebrCommStreamSocket *stream_socket;
	/* protocol parsing stuff */
	struct gebr_comm_protocol *protocol;
	/* server address/port */
	GString *address;
	guint16 port;
	/* ssh stuff */
	GString *password;
	gint16 tunnel_port;
	gboolean tried_existant_pass;
	GebrCommTerminalProcess *x11_forward_process;
	GebrCommProcess *x11_forward_unix;

	enum gebr_comm_server_state {
		SERVER_STATE_DISCONNECTED,
		SERVER_STATE_RUN,
		SERVER_STATE_OPEN_TUNNEL,
		SERVER_STATE_CONNECT,
		SERVER_STATE_CONNECTED,
	} state;
	enum gebr_comm_server_error {
		SERVER_ERROR_NONE,
		SERVER_ERROR_CONNECT,
		SERVER_ERROR_SERVER,
		SERVER_ERROR_SSH,
	} error;
	const struct gebr_comm_server_ops {
		void (*log_message) (enum gebr_log_message_type type, const gchar * message);
		GString *(*ssh_login) (const gchar * title, const gchar * message);
		gboolean(*ssh_question) (const gchar * title, const gchar * message);
		void (*parse_messages) (struct gebr_comm_server * gebr_comm_server, gpointer user_data);
	} *ops;
	gpointer user_data;

	/* temporary process for operations */
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
 * GebrCommServerRunFlow:
 * Returned by gebr_comm_server_run_config_add_flow 
 */
typedef struct {
	GebrGeoXmlFlow * flow;
	guint run_id;
} GebrCommServerRunFlow;
/**
 * GebrCommServerRunConfig:
 * @flows: use gebr_comm_server_run_config_add_flow to add a flow
 * @parallel: whether this will be executed in a parallel environment
 * @account: account for moab servers
 * @queue: the queue this flow will be appended
 * @num_processes: the number of processes to run in parallel
 *
 * Configurations for running a flow.
 */
typedef struct {
	GList * flows;
	gboolean parallel;
	gchar * account;
	gchar * queue;
	gchar * num_processes;
} GebrCommServerRunConfig;

/**
 */
GebrCommServerRunConfig * gebr_comm_server_run_config_new(void);

/**
 */
void gebr_comm_server_run_config_free(GebrCommServerRunConfig *run_config);

/**
 */
GebrCommServerRunFlow* gebr_comm_server_run_config_add_flow(GebrCommServerRunConfig *config, GebrGeoXmlFlow * flow);

/**
 */
GebrGeoXmlFlow * gebr_comm_server_run_strip_flow(GebrGeoXmlFlow * flow);

/**
 */
struct gebr_comm_server *gebr_comm_server_new(const gchar * _address, const struct gebr_comm_server_ops *ops);

/**
 */
void gebr_comm_server_free(struct gebr_comm_server *gebr_comm_server);

/**
 */
void gebr_comm_server_connect(struct gebr_comm_server *gebr_comm_server);

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

/**
 * Ask _gebr_comm_server_ to run the current _flow_.
 * Returns the run_id for the first flow.
 */
void gebr_comm_server_run_flow(struct gebr_comm_server *gebr_comm_server, GebrCommServerRunConfig * config);

G_END_DECLS
#endif				//__GEBR_COMM_SERVER_H
