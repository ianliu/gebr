/*
 * gebr-comm-server.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_COMM_SERVER_H__
#define __GEBR_COMM_SERVER_H__

#include <glib.h>

#include <gebr-validator.h>
#include <libgebr/log.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/utils.h>
#include <libgebr/comm/gebr-comm-port-provider.h>
#include <libgebr/comm/gebr-comm-ssh.h>
#include <libgebr/comm/gebr-comm-utils.h>

#include "gebr-comm-protocol-socket.h"
#include "gebr-comm-terminalprocess.h"
#include "gebr-comm-process.h"
#include "gebr-comm-channelsocket.h"

G_BEGIN_DECLS


#define GEBR_COMM_TYPE_SERVER            (gebr_comm_server_get_type ())
#define GEBR_COMM_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_TYPE_SERVER, GebrCommServer))
#define GEBR_COMM_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_COMM_TYPE_SERVER, GebrCommServerClass))
#define GEBR_COMM_IS_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_TYPE_SERVER))
#define GEBR_COMM_IS_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_COMM_TYPE_SERVER))
#define GEBR_COMM_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_COMM_TYPE_SERVER, GebrCommServerClass))

#define GEBR_PORT_PREFIX "gebr-port="
#define GEBR_ADDR_PREFIX "gebr-addr="


typedef enum {
	GEBR_COMM_SERVER_TYPE_MOAB,
	GEBR_COMM_SERVER_TYPE_REGULAR,
	GEBR_COMM_SERVER_TYPE_UNKNOWN
} GebrCommServerType;

typedef enum {
	GEBR_COMM_SERVER_LOCATION_UNKNOWN,
	GEBR_COMM_SERVER_LOCATION_LOCAL,
	GEBR_COMM_SERVER_LOCATION_REMOTE
} GebrCommServerLocation;

gchar *gebr_comm_server_get_user(const char *address);

GebrCommServerType gebr_comm_server_get_id(const gchar * name);

// If you change these states, don't forget to update
// the gebr_comm_server_state_to_string() method.
typedef enum {
	SERVER_STATE_DISCONNECTED,
	SERVER_STATE_RUN,
	SERVER_STATE_REDIRECT,
	SERVER_STATE_CONNECT,
	SERVER_STATE_LOGGED,
} GebrCommServerState;

typedef struct _GebrCommServer GebrCommServer;
typedef struct _GebrCommServerPriv GebrCommServerPriv;
typedef struct _GebrCommServerClass GebrCommServerClass;

struct _GebrCommServer {
	GObject parent;
	GebrCommProtocolSocket *socket;

	GebrCommServerPriv *priv;

	/* server address/port */
	GString *address;
	guint16 port;
	gboolean ac;

	/* ssh stuff */
	gboolean use_public_key;
	gchar *password;
	gint16 tunnel_port;
	GebrCommTerminalProcess *x11_forward_process;
	GebrCommProcess *x11_forward_unix;

	gint ncores;
	gdouble clock_cpu;
	gchar *memory;
	gchar *model_name;

	GebrCommServerState state;

	enum gebr_comm_server_error {
		SERVER_ERROR_UNKNOWN,
		SERVER_ERROR_NONE,
		SERVER_ERROR_CONNECT,
		SERVER_ERROR_SERVER,
		SERVER_ERROR_SSH,
		SERVER_ERROR_PROTOCOL_VERSION,
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

	GebrCommTerminalProcess *process;
};

struct _GebrCommServerClass {
	GObjectClass parent_class;

	void (*server_password_request) (GebrCommServer *server,
					 const gchar *title,
				         const gchar *message);

	void (*question_request) (GebrCommServer *server,
				  const gchar *title,
				  const gchar *message);
};

GType gebr_comm_server_get_type(void) G_GNUC_CONST;

GebrCommServer *gebr_comm_server_new(const gchar *_address,
				     const gchar *gebr_id,
				     const struct gebr_comm_server_ops *ops);

const gchar *gebr_comm_server_get_last_error(GebrCommServer *server);

void gebr_comm_server_set_last_error(GebrCommServer *server,
				     enum gebr_comm_server_error error,
				     const gchar * message,
				     ...);

void gebr_comm_server_free(GebrCommServer *gebr_comm_server);

/**
 * gebr_comm_server_set_x11_cookie:
 */
void gebr_comm_server_set_x11_cookie(GebrCommServer *server,
				     const gchar *cookie);

void gebr_comm_server_connect(GebrCommServer *server,
			      gboolean maestro,
			      gboolean force_init);

void gebr_comm_server_disconnect(GebrCommServer *gebr_comm_server);

gboolean gebr_comm_server_is_logged(GebrCommServer *gebr_comm_server);

void gebr_comm_server_set_logged(GebrCommServer *server);

gboolean gebr_comm_server_is_local(GebrCommServer *gebr_comm_server);

void gebr_comm_server_kill(GebrCommServer *gebr_comm_server);

/**
 * gebr_comm_server_forward_x11:
 *
 * Forward @remote_display to the local machine.
 */
void gebr_comm_server_forward_x11(GebrCommServer *gebr_comm_server,
				  const gchar *display_host,
				  guint display_port);

guint gebr_comm_server_get_display_port(GebrCommServer *server,
					gchar **display_host);

const gchar *gebr_comm_server_state_to_string(GebrCommServerState state);

GebrCommServerState gebr_comm_server_state_from_string(const gchar *string);

GebrCommServerState gebr_comm_server_get_state(GebrCommServer *server);

/**
 * gebr_comm_server_set_password:
 * @server:
 * @pass:
 *
 * Writes @pass into @server.
 *
 * If @server is in interactive mode (see
 * gebr_comm_server_set_interactive()) then @pass is written into @server only
 * if the signal #GebrCommServer::password-request has been emitted.
 *
 * Otherwise, if @server is in noninteractive mode, the password is cached for
 * the next connection, see gebr_comm_server_connect().
 */
void gebr_comm_server_set_password(GebrCommServer *server,
				   const gchar *pass);

/**
 * gebr_comm_server_invalid_password:
 *
 * Set password error for all pending connections,
 * and clean list after that.
 */
void gebr_comm_server_invalid_password(GebrCommServer *server);

/**
 * gebr_comm_server_answer_question:
 *
 * Writes a response into @server.
 *
 * This method is similar to gebr_comm_server_set_password(), but it can only
 * be called if a #GebrCommServer::question_request signal was emitted. The
 * @response is the answer for a Yes or No question.
 */
void gebr_comm_server_answer_question(GebrCommServer *server,
				      gboolean response);

/**
 * gebr_comm_server_get_accepts_key:
 *
 * Return if the server accepts public key
 */
gboolean gebr_comm_server_get_accepts_key(GebrCommServer *server);

/**
 * gebr_comm_server_append_key:
 *
 * Append key in the @server
 */
void gebr_comm_server_append_key(GebrCommServer *server,
				 void *finished_callback,
				 gpointer user_data);
/**
 * gebr_comm_server_set_use_public_key:
 */
void gebr_comm_server_set_use_public_key(GebrCommServer *server, gboolean use_key);

/**
 * gebr_comm_server_get_use_pubblic_key:
 */
gboolean gebr_comm_server_get_use_public_key(GebrCommServer *server);

/**
 * gebr_comm_server_is_maestro:
 */
gboolean gebr_comm_server_is_maestro(GebrCommServer *server);

void gebr_comm_server_maestro_connect_on_daemons(GebrCommServer *server);

GebrCommPortProvider *gebr_comm_server_create_port_provider(GebrCommServer *server,
							    GebrCommPortType type);

void gebr_comm_server_set_check_host(GebrCommServer *server,
                                     gboolean check_host);

void gebr_comm_server_set_cookie(GebrCommServer *server, const gchar *gebr_cookie);

G_END_DECLS

#endif /* __GEBR_COMM_SERVER_H__ */
