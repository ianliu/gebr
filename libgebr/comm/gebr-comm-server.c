/*
 * gebr-comm-server.c
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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <config.h>

#include <libgebr/marshalers.h>
#include <libgebr/gebr-version.h>
#include "../libgebr-gettext.h"
#include <glib/gi18n-lib.h>
#include <libgebr/utils.h>

#include "gebr-comm-server.h"
#include "gebr-comm-listensocket.h"
#include "gebr-comm-protocol.h"
#include "gebr-comm-uri.h"

/*
 * Declarations
 */

typedef enum {
	ISTATE_NONE,
	ISTATE_PASS,
	ISTATE_QUESTION,
} InteractiveState;

struct _GebrCommServerPriv {
	gboolean is_maestro;

	gchar *gebr_id;

	GebrCommPortForward *connection_forward;

	/* Interactive state variables */
	InteractiveState istate;

	gboolean accepts_key;

	GHashTable *qa_cache;

	GList *pending_connections;
};

G_DEFINE_TYPE(GebrCommServer, gebr_comm_server, G_TYPE_OBJECT);

enum {
	SERVER_PASSWORD_REQUEST,
	QUESTION_REQUEST,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void	gebr_comm_server_log_message	(GebrCommServer *server,
						 GebrLogMessageType type,
						 const gchar * message, ...);

static void	gebr_comm_server_disconnected_state(GebrCommServer *server,
						    enum gebr_comm_server_error error,
						    const gchar * message, ...);

static void	gebr_comm_server_change_state	(GebrCommServer *server,
						 GebrCommServerState state);

static void	gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket,
						  GebrCommServer *server);

static void	gebr_comm_server_socket_disconnected(GebrCommProtocolSocket * socket,
						     GebrCommServer *server);

static void	gebr_comm_server_socket_process_request(GebrCommProtocolSocket * socket,
							GebrCommHttpMsg *request,
							GebrCommServer *server);

static void	gebr_comm_server_socket_process_response(GebrCommProtocolSocket *socket,
							 GebrCommHttpMsg *request,
							 GebrCommHttpMsg *response,
							 GebrCommServer *server);

static void	gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket *socket,
							   GebrCommServer *server);

static void	gebr_comm_server_free_x11_forward(GebrCommServer *server);

static void	gebr_comm_server_free_for_reuse	(GebrCommServer *server);


static void
gebr_comm_server_init(GebrCommServer *server)
{
	server->priv = G_TYPE_INSTANCE_GET_PRIVATE(server,
						   GEBR_COMM_TYPE_SERVER,
						   GebrCommServerPriv);

	server->priv->istate = ISTATE_NONE;
	server->priv->pending_connections = NULL;
}

static void
gebr_comm_server_class_init(GebrCommServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[SERVER_PASSWORD_REQUEST] =
		g_signal_new("server-password-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommServerClass, server_password_request),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__BOOLEAN,
			     G_TYPE_NONE, 1,
			     G_TYPE_BOOLEAN);

	signals[QUESTION_REQUEST] =
		g_signal_new("question-request",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommServerClass, question_request),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_STRING, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrCommServerPriv));
}

gchar *
gebr_comm_server_get_user(const gchar *address)
{
	gchar *addr_temp;

	addr_temp = g_strdup(address);

	return (gchar *) strsep(&addr_temp, "@");
}

GebrCommServerType gebr_comm_server_get_id(const gchar * name)
{
	if (strcmp(name, "regular") == 0)
		return GEBR_COMM_SERVER_TYPE_REGULAR;
	else if (strcmp(name, "moab") == 0)
		return GEBR_COMM_SERVER_TYPE_MOAB;
	else
		return GEBR_COMM_SERVER_TYPE_UNKNOWN;
}

const gchar *
gebr_comm_server_get_last_error(GebrCommServer *server)
{
	return server->last_error->str;
}

static void
gebr_comm_server_set_last_errorv(GebrCommServer *server,
				 enum gebr_comm_server_error error,
				 const gchar * message,
				 va_list argp)
{
	if (error != SERVER_ERROR_UNKNOWN) {
		server->error = error;
		gchar *string = g_strdup_vprintf(message, argp);
		g_string_assign(server->last_error, string);
		g_free(string);
	}
}

void
gebr_comm_server_set_last_error(GebrCommServer *server,
				enum gebr_comm_server_error error,
				const gchar * message,
				...)
{
	va_list argp;
	va_start(argp, message);
	gebr_comm_server_set_last_errorv(server, error, message, argp);
	va_end(argp);
}

GebrCommServer *
gebr_comm_server_new(const gchar * _address,
		     const gchar *gebr_id,
		     const struct gebr_comm_server_ops *ops)
{
	GebrCommServer *server;
	server = g_object_new(GEBR_COMM_TYPE_SERVER, NULL);

	server->address = g_string_new(gebr_apply_pattern_on_address(_address));
	server->priv->gebr_id = g_strdup(gebr_id);
	server->socket = gebr_comm_protocol_socket_new();
	server->port = 0;
	server->use_public_key = FALSE;
	server->password = NULL;
	server->x11_forward_process = NULL;
	server->x11_forward_unix = NULL;
	server->ops = ops;
	server->user_data = NULL;
	server->last_error = g_string_new("");
	server->state = SERVER_STATE_DISCONNECTED;
	server->error = SERVER_ERROR_NONE;
	server->socket->protocol->logged = FALSE;

	server->priv->qa_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	g_signal_connect(server->socket, "connected",
			 G_CALLBACK(gebr_comm_server_socket_connected), server);
	g_signal_connect(server->socket, "disconnected",
			 G_CALLBACK(gebr_comm_server_socket_disconnected), server);
	g_signal_connect(server->socket, "process-request",
			 G_CALLBACK(gebr_comm_server_socket_process_request), server);
	g_signal_connect(server->socket, "process-response",
			 G_CALLBACK(gebr_comm_server_socket_process_response), server);
	g_signal_connect(server->socket, "old-parse-messages",
			 G_CALLBACK(gebr_comm_server_socket_old_parse_messages), server);

	return server;
}

void
gebr_comm_server_free(GebrCommServer *server)
{
	gebr_comm_server_free_x11_forward(server);

	if (server->priv->qa_cache)
		g_hash_table_remove_all(server->priv->qa_cache);

	if (server->priv->connection_forward) {
		gebr_comm_port_forward_close(server->priv->connection_forward);
		gebr_comm_port_forward_free(server->priv->connection_forward);
		server->priv->connection_forward = NULL;
	}

	g_string_free(server->last_error, TRUE);
	g_string_free(server->address, TRUE);
	g_free(server->password);
	g_free(server->memory);
	g_free(server->model_name);
	g_object_unref(server->socket);
	g_object_unref(server);
}

static void
on_comm_port_defined(GebrCommPortProvider *self,
		     guint port,
		     GebrCommServer *server)
{
	// The connection_forward must be reset when gebr_comm_server_connect
	// is called.
	g_warn_if_fail(server->priv->connection_forward == NULL);

	server->priv->connection_forward = gebr_comm_port_provider_get_forward(self);

	GebrCommSocketAddress socket_address;
	socket_address = gebr_comm_socket_address_ipv4_local(port);
	gebr_comm_protocol_socket_connect(server->socket, &socket_address, FALSE);
}

static void
on_comm_ssh_error(GError *error,
		  GebrCommServer *server)
{
	switch (error->code) {
	case GEBR_COMM_PORT_PROVIDER_ERROR_REDIRECT:
		gebr_comm_server_free_for_reuse(server);
		g_string_assign(server->address, error->message);
		gebr_comm_server_connect(server, TRUE);
		break;
	case GEBR_COMM_PORT_PROVIDER_ERROR_SSH:
	case GEBR_COMM_PORT_PROVIDER_ERROR_EMPTY:
		gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH,
		                                    "%s", g_strstrip(error->message));
		g_free(server->password);
		server->password = NULL;
		break;
	case GEBR_COMM_PORT_PROVIDER_ERROR_UNKNOWN_TYPE:
	case GEBR_COMM_PORT_PROVIDER_ERROR_SFTP_NOT_REQUIRED:
	case GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN:
		break;
	}
}

static void
on_comm_port_error(GebrCommPortProvider *self,
                   GError *error,
                   GebrCommServer *server)
{
	on_comm_ssh_error(error, server);
}

static void
on_comm_ssh_password(GebrCommSsh *ssh,
		     gboolean retry,
		     GebrCommServer *server)
{
	// FIXME: This is a workaround to a problem faced when using this
	// method for other SSH connections other than the server connection
	// itself. See gebr_comm_server_forward_remote_port() for an example.

	if (retry) {
		g_free(server->password);
		server->password = NULL;
	}

	if (server->password) {
		gebr_comm_ssh_set_password(ssh, server->password);
		return;
	}

	server->priv->istate = ISTATE_PASS;

	server->priv->pending_connections = g_list_append(server->priv->pending_connections, ssh);

	g_signal_emit(server, signals[SERVER_PASSWORD_REQUEST], 0, retry);
}

static void
on_comm_port_password(GebrCommPortProvider *self,
		      GebrCommSsh *ssh,
		      gboolean retry,
		      GebrCommServer *server)
{
	on_comm_ssh_password(ssh, retry, server);
}

static void
on_comm_ssh_question(GebrCommSsh *ssh,
		     const gchar *question,
		     GebrCommServer *server)
{
	server->priv->istate = ISTATE_QUESTION;

	gchar *title = g_strdup(_("Please, answer the question"));
	gchar *description = g_strdup(question);

	g_signal_emit(server, signals[QUESTION_REQUEST], 0,
	              title,
	              description);

	server->priv->pending_connections = g_list_append(server->priv->pending_connections, ssh);

	g_free(title);
	g_free(description);
}

static void
on_comm_port_question(GebrCommPortProvider *self,
		      GebrCommSsh *ssh,
		      const gchar *question,
		      GebrCommServer *server)
{
	on_comm_ssh_question(ssh, question, server);
}

static void
on_comm_port_accepts_key(GebrCommPortProvider *self,
                         gboolean accepts_key,
                         GebrCommServer *server)
{
	server->priv->accepts_key = accepts_key;
}

void gebr_comm_server_connect(GebrCommServer *server,
			      gboolean maestro)
{
	GebrCommPortType port_type;

	if (server->state != SERVER_STATE_DISCONNECTED)
		return;

	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("%p: Launching machine at '%s'."),
				     server->socket, server->address->str);

	gebr_comm_server_change_state(server, SERVER_STATE_RUN);

	server->priv->is_maestro = maestro;

	if (maestro)
		port_type = GEBR_COMM_PORT_TYPE_MAESTRO;
	else
		port_type = GEBR_COMM_PORT_TYPE_DAEMON;

	GebrCommPortProvider *port_provider =
		gebr_comm_port_provider_new(port_type, server->address->str);

	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_comm_port_defined), server);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_comm_port_error), server);
	g_signal_connect(port_provider, "repass-password", G_CALLBACK(on_comm_port_password), server);
	g_signal_connect(port_provider, "question", G_CALLBACK(on_comm_port_question), server);
	g_signal_connect(port_provider, "accepts-key", G_CALLBACK(on_comm_port_accepts_key), server);
	gebr_comm_port_provider_start(port_provider);
}

void gebr_comm_server_disconnect(GebrCommServer *server)
{
	gebr_comm_protocol_socket_disconnect(server->socket);
}

gboolean
gebr_comm_server_is_logged(GebrCommServer *server)
{
	return server->socket->protocol->logged;
}

void
gebr_comm_server_set_logged(GebrCommServer *server)
{
	server->socket->protocol->logged = TRUE;
	gebr_comm_server_change_state(server, SERVER_STATE_LOGGED);
}

gboolean gebr_comm_server_is_local(GebrCommServer *server)
{
	return strcmp(server->address->str, "127.0.0.1") == 0 ? TRUE : strcmp(server->address->str, "localhost") == 0 ? TRUE : FALSE;
}

void gebr_comm_server_kill(GebrCommServer *server)
{
	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Stopping machine at '%s'."),
				     server->address->str);

	const gchar *bin;
	if (gebr_comm_server_is_maestro(server))
		bin = "gebrm";
	else
		bin = "gebrd";

	gchar *kill = g_strdup_printf("fuser -sk -15 $(cat $HOME/.gebr/%s/$HOSTNAME/lock)/tcp", bin);
	GString *cmd_line = g_string_new(NULL);

	if (gebr_comm_server_is_local(server))
		g_string_printf(cmd_line, "bash -c '%s'", kill);
	else {
		gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key();
		g_string_printf(cmd_line, "%s -x %s '%s'", ssh_cmd, server->address->str, kill);
		g_free(ssh_cmd);
	}

	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_comm_ssh_password), server);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_comm_ssh_question), server);
	gebr_comm_ssh_set_command(ssh, cmd_line->str);
	gebr_comm_ssh_run(ssh);

	g_string_free(cmd_line, TRUE);
	g_free(kill);
}

static void
on_x11_port_defined(GebrCommPortProvider *self,
		    guint port,
		    GebrCommServer *server)
{
	gchar *tmp = g_strdup_printf("%d", port);
	gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
					      gebr_comm_protocol_defs.dsp_def, 2,
					      tmp,
					      gebr_comm_port_provider_get_display_host(self));
	g_free(tmp);
}

static void
on_x11_port_error(GebrCommPortProvider *self,
		  GError *error,
		  GebrCommServer *server)
{
	g_critical("Error when forwarding x11: %s", error->message);
}

static guint
get_gebr_display_port(GebrCommServer *server,
                      gchar **display_host)
{
	gchar *x11_file, *host;
	guint display_port;

	gebr_comm_get_display(&x11_file, &display_port, &host);

	if (display_host)
		*display_host = g_strdup(host);

	if (x11_file) {
		display_port = gebr_comm_get_available_port(6012);
		server->x11_forward_unix = gebr_comm_process_new();
		GString *cmdline = g_string_new(NULL);
		g_string_printf(cmdline, "gebr-comm-socketchannel %d %s", display_port, x11_file);
		gebr_comm_process_start(server->x11_forward_unix, cmdline);
		g_string_free(cmdline, TRUE);
	} else if (display_port == 0) {
		g_warn_if_reached();
	}

	g_free(host);
	g_free(x11_file);

	return display_port;
}

void
gebr_comm_server_forward_x11(GebrCommServer *server)
{
	gchar *host;
	guint display_port;

	if (server->priv->is_maestro)
		display_port = get_gebr_display_port(server, &host);

	GebrCommPortProvider *port_provider =
		gebr_comm_port_provider_new(GEBR_COMM_PORT_TYPE_X11, server->address->str);
	gebr_comm_port_provider_set_display(port_provider, display_port, host);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_x11_port_defined), server);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_x11_port_error), server);
	g_signal_connect(port_provider, "repass-password", G_CALLBACK(on_comm_port_password), server);
	g_signal_connect(port_provider, "question", G_CALLBACK(on_comm_port_question), server);
	gebr_comm_port_provider_start(port_provider);

	g_free(host);
}

/**
 * \internal
 */
static void
gebr_comm_server_log_message(GebrCommServer *server, GebrLogMessageType type,
			     const gchar * message, ...)
{
	va_list argp;
	va_start(argp, message);
	gchar *string = g_strdup_vprintf(message, argp);
	gchar *msg = g_strdup_printf("%s: %s", server->address->str, string);
	server->ops->log_message(server, type, msg, server->user_data);
	g_free(string);
	g_free(msg);
	va_end(argp);
}

/**
 * \internal
 */
static void gebr_comm_server_disconnected_state(GebrCommServer *server,
						enum gebr_comm_server_error error,
						const gchar * message, ...)
{
	va_list argp;
	va_start(argp, message);
	gebr_comm_server_set_last_errorv(server, error, message, argp);
	va_end(argp);

	/* take care not to free the process here cause this function
	 * maybe be used by Process's read callback */
	server->port = 0;
	server->socket->protocol->logged = FALSE;
	gebr_comm_server_change_state(server, SERVER_STATE_DISCONNECTED);
}

static void gebr_comm_server_change_state(GebrCommServer *server, GebrCommServerState state)
{
	g_debug("State of machine %s changed from %s to %s",
		server->address->str,
		gebr_comm_server_state_to_string(server->state),
		gebr_comm_server_state_to_string(state));

	if (state == SERVER_STATE_DISCONNECTED)
		g_queue_clear(server->socket->protocol->waiting_ret_hashs);
	server->state = state;
	server->ops->state_changed(server, server->user_data);
}

/*
 * get this X session magic cookie
 */
gchar *
get_xauth_cookie(const gchar *display_number)
{
	if (!display_number)
		return g_strdup("");

	gchar *mcookie_str = g_new(gchar, 33);
	GString *cmd_line = g_string_new(NULL);

	g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display_number);

	g_debug("GET XATUH COOKIE WITH COMMAND: %s", cmd_line->str);

	/* WORKAROUND: if xauth is already executing it will lock
	 * the auth file and it will fail to retrieve the m-cookie.
	 * So, as a workaround, we try to get the m-cookie many times.
	 */
	gint i;
	for (i = 0; i < 5; i++) {
		FILE *output_fp = popen(cmd_line->str, "r");
		if (fscanf(output_fp, "%32s", mcookie_str) != 1)
			usleep(100*1000);
		else {
			pclose(output_fp);
			break;
		}
		pclose(output_fp);
	}

	if (i == 5)
		strcpy(mcookie_str, "");

	g_debug("===== COOKIE ARE %s", mcookie_str);

	g_string_free(cmd_line, TRUE);

	return mcookie_str;
}

static void
gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket,
				  GebrCommServer *server)
{
	const gchar *display;
	gchar *display_number = NULL;
	const gchar *hostname = g_get_host_name();


	display = getenv("DISPLAY");
	if (!display)
		display = "";
	else
		display_number = strchr(display, ':');

	gebr_comm_server_change_state(server, SERVER_STATE_CONNECT);

	if (server->priv->is_maestro) {
		gchar *mcookie_str = get_xauth_cookie(display_number);
		GTimeVal gebr_time;
		g_get_current_time(&gebr_time);
		gchar *gebr_time_iso = g_time_val_to_iso8601(&gebr_time);
		               gchar *maestro_location = g_find_program_in_path("gebrm");
		               gchar *daemon_location = g_find_program_in_path("gebrd");

			       gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
								     gebr_comm_protocol_defs.ini_def, 7,
								     g_get_host_name(),
								     gebr_version(),
								     mcookie_str,
								     server->priv->gebr_id,
								     gebr_time_iso,
								     maestro_location ? "1" : "0",
								     daemon_location ? "1" : "0");

			       g_free(maestro_location);
			       g_free(daemon_location);

		g_free(mcookie_str);
		g_free(gebr_time_iso);
	} else {
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
						      gebr_comm_protocol_defs.ini_def, 2,
						      gebr_comm_protocol_get_version(),
						      hostname);
	}

}

static void
gebr_comm_server_socket_disconnected(GebrCommProtocolSocket *socket,
				     GebrCommServer *server)
{
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Machine '%s' disconnected"), 
				     server->address->str);
}

static void
gebr_comm_server_socket_process_request(GebrCommProtocolSocket *socket,
					GebrCommHttpMsg *request,
					GebrCommServer *server)
{
	server->ops->process_request(server, request, server->user_data);
}

static void
gebr_comm_server_socket_process_response(GebrCommProtocolSocket *socket,
					 GebrCommHttpMsg *request,
					 GebrCommHttpMsg *response,
					 GebrCommServer *server)
{
	server->ops->process_response(server, request, response, server->user_data);
}

static void
gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket *socket,
					   GebrCommServer *server)
{
	server->ops->parse_messages(server, server->user_data);
}

/**
 * \internal
 * Free (if necessary) server->x11_forward_process for reuse
 */
static void gebr_comm_server_free_x11_forward(GebrCommServer *server)
{
	if (server->x11_forward_unix != NULL) {
		gebr_comm_process_free(server->x11_forward_unix);
		server->x11_forward_unix = NULL;
	}
}

/**
 * \internal
 */
static void gebr_comm_server_free_for_reuse(GebrCommServer *server)
{
	server->port = 0;
	server->socket->protocol->logged = FALSE;
	gebr_comm_server_change_state(server, SERVER_STATE_DISCONNECTED);
	gebr_comm_protocol_reset(server->socket->protocol);
	gebr_comm_server_free_x11_forward(server);

	if (server->priv->qa_cache)
		g_hash_table_remove_all(server->priv->qa_cache);

	if (server->priv->connection_forward) {
		gebr_comm_port_forward_close(server->priv->connection_forward);
		gebr_comm_port_forward_free(server->priv->connection_forward);
		server->priv->connection_forward = NULL;
	}
}

static const gchar *state_hash[] = {
	"disconnected",
	"run",
	"connect",
	"logged",
	NULL
};

const gchar *
gebr_comm_server_state_to_string(GebrCommServerState state)
{
	return state_hash[state];
}

GebrCommServerState
gebr_comm_server_state_from_string(const gchar *string)
{
	for (int i = 0; state_hash[i]; i++)
		if (g_strcmp0(state_hash[i], string) == 0)
			return (GebrCommServerState) i;

	return SERVER_STATE_DISCONNECTED;
}

GebrCommServerState
gebr_comm_server_get_state(GebrCommServer *server)
{
	return server->state;
}

void
gebr_comm_server_invalid_password(GebrCommServer *server)
{
	for (GList *i = server->priv->pending_connections; i; i = i->next)
		gebr_comm_ssh_password_error(i->data, FALSE);

	g_list_free(server->priv->pending_connections);
	server->priv->pending_connections = NULL;
}

void
gebr_comm_server_set_password(GebrCommServer *server, const gchar *pass)
{
	server->password = g_strdup(pass);

	for (GList *i = server->priv->pending_connections; i; i = i->next)
		gebr_comm_ssh_set_password(i->data, server->password);

	g_list_free(server->priv->pending_connections);
	server->priv->pending_connections = NULL;
}

void
gebr_comm_server_answer_question(GebrCommServer *server,
				 gboolean response)
{
	for (GList *i = server->priv->pending_connections; i; i = i->next)
		gebr_comm_ssh_answer_question(i->data, response);

	g_list_free(server->priv->pending_connections);
	server->priv->pending_connections = NULL;
}

static gchar *
get_append_key_command(GebrCommServer *server)
{
	gchar *path = gebr_key_filename(TRUE);
	gchar *public_key;

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_free(path);
		return FALSE;
	}

	// FIXME: please handle GError of the g_file_get_contents
	g_file_get_contents(path, &public_key, NULL, NULL);
	public_key[strlen(public_key) - 1] = '\0'; // Erase new line

	gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key();
	GString *cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s '%s' -o StrictHostKeyChecking=no "
			"'umask 077; test -d $HOME/.ssh || mkdir $HOME/.ssh ; echo \"%s (%s)\" >> $HOME/.ssh/authorized_keys'",
			ssh_cmd, server->address->str, public_key, gebr_comm_server_is_maestro(server) ? "gebr" : "gebrm");

	g_debug("Cmd line:'%s'", cmd_line->str);

	g_free(path);
	g_free(ssh_cmd);
	g_free(public_key);

	return g_string_free(cmd_line, FALSE);
}

void
gebr_comm_server_append_key(GebrCommServer *server,
			    void *finished_callback,
			    gpointer user_data)
{
	gchar *command;
	command = get_append_key_command(server);

	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_comm_ssh_password), server);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_comm_ssh_question), server);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_comm_ssh_error), server);
	g_signal_connect(ssh, "ssh-finished", G_CALLBACK(finished_callback), user_data);

	gebr_comm_ssh_set_command(ssh, command);
	gebr_comm_ssh_run(ssh);

	g_free(command);
}

gboolean
gebr_comm_server_get_accepts_key(GebrCommServer *server)
{
	return server->priv->accepts_key;
}

void
gebr_comm_server_set_use_public_key(GebrCommServer *server,
                                    gboolean use_key)
{
	server->use_public_key = use_key;
}

gboolean
gebr_comm_server_get_use_public_key(GebrCommServer *server)
{
	return server->use_public_key;
}

gboolean
gebr_comm_server_is_maestro(GebrCommServer *server)
{
	return server->priv->is_maestro;
}

GebrCommPortProvider *
gebr_comm_server_create_port_provider(GebrCommServer *server,
				      GebrCommPortType type)
{
	GebrCommPortProvider *port_provider =
		gebr_comm_port_provider_new(type, server->address->str);
	g_signal_connect(port_provider, "repass-password", G_CALLBACK(on_comm_port_password), server);
	g_signal_connect(port_provider, "question", G_CALLBACK(on_comm_port_question), server);
	return port_provider;
}
