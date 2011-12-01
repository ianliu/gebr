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

#include "../libgebr-gettext.h"
#include <glib/gi18n-lib.h>
#include <libgebr/utils.h>

#include "gebr-comm-server.h"
#include "gebr-comm-listensocket.h"
#include "gebr-comm-protocol.h"

/*
 * Declarations
 */

static void
gebr_comm_server_log_message(struct gebr_comm_server *server, GebrLogMessageType type,
			     const gchar * message, ...);

static void local_run_server_read(GebrCommProcess * process, struct gebr_comm_server *server);
static void local_run_server_finished(GebrCommProcess * process, gint status, struct gebr_comm_server *server);

static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess * process, struct gebr_comm_server *server,
			   GString * output);
static void gebr_comm_ssh_read(GebrCommTerminalProcess * process, struct gebr_comm_server *server);
static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server);
static void gebr_comm_ssh_run_server_read(GebrCommTerminalProcess * process, struct gebr_comm_server *server);
static void
gebr_comm_ssh_run_server_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server);
static gboolean
gebr_comm_ssh_open_tunnel_pool(struct gebr_comm_server *server);

static void gebr_comm_server_disconnected_state(struct gebr_comm_server *server, enum gebr_comm_server_error error,
						const gchar * message, ...);
static void gebr_comm_server_change_state(struct gebr_comm_server *server, GebrCommServerState state);
static void gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket, struct gebr_comm_server *server);
static void gebr_comm_server_socket_disconnected(GebrCommProtocolSocket * socket, struct gebr_comm_server *server);
static void gebr_comm_server_socket_process_request(GebrCommProtocolSocket * socket, GebrCommHttpMsg *request,
						    struct gebr_comm_server *server);
static void gebr_comm_server_socket_process_response(GebrCommProtocolSocket * socket, GebrCommHttpMsg *request,
						     GebrCommHttpMsg *response, struct gebr_comm_server *server);
static void gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket * socket, struct gebr_comm_server *server);

static void gebr_comm_server_free_x11_forward(struct gebr_comm_server *server);
static void gebr_comm_server_free_for_reuse(struct gebr_comm_server *server);

gchar * gebr_comm_server_get_user(const gchar * address)
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

struct gebr_comm_server *gebr_comm_server_new(const gchar * _address, const struct gebr_comm_server_ops *ops)
{
	struct gebr_comm_server *server;

	/* initialize */
	server = g_new(struct gebr_comm_server, 1);
	server->socket = gebr_comm_protocol_socket_new();
	server->address = g_string_new(_address);
	server->port = 0;
	server->password = NULL;
	server->x11_forward_process = NULL;
	server->x11_forward_unix = NULL;
	server->ops = ops;
	server->user_data = NULL;
	server->tunnel_pooling_source = 0;
	server->process.use = COMM_SERVER_PROCESS_NONE;
	server->last_error = g_string_new("");
	server->state = SERVER_STATE_UNKNOWN;
	server->error = SERVER_ERROR_UNKNOWN;
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");

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

void gebr_comm_server_free(struct gebr_comm_server *server)
{
	g_string_free(server->last_error, TRUE);
	gebr_comm_server_free_for_reuse(server);
	g_string_free(server->address, TRUE);
	g_free(server->password);
	g_object_unref (server->socket);
	g_free(server);
}

void gebr_comm_server_connect(struct gebr_comm_server *server,
			      gboolean maestro)
{
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");
	gebr_comm_server_change_state(server, SERVER_STATE_RUN);
	server->tried_existant_pass = FALSE;

	const gchar *binary;

	if (maestro)
		binary = "gebrm";
	else
		binary = "gebrd";

	/* initiate the marathon to communicate to server */
	if (gebr_comm_server_is_local(server) == FALSE) {
		gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Launching server at '%s'."),
					     server->address->str);

		GebrCommTerminalProcess *process;
		server->process.use = COMM_SERVER_PROCESS_TERMINAL;
		server->process.data.terminal = process = gebr_comm_terminal_process_new();
		g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_run_server_read), server);
		g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_run_server_finished), server);

		GString *cmd_line = g_string_new(NULL);
		g_string_printf(cmd_line, "ssh -x %s \"bash -l -c '%s >&3' 3>&1 >/dev/null 2>&1\"",
				server->address->str, binary);
		gebr_comm_terminal_process_start(process, cmd_line);
		g_string_free(cmd_line, TRUE);
	} else {
		gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Launching local server."));

		GebrCommProcess *process;
		server->process.use = COMM_SERVER_PROCESS_REGULAR;
		server->process.data.regular = process = gebr_comm_process_new();
		g_signal_connect(process, "ready-read-stdout", G_CALLBACK(local_run_server_read), server);
		g_signal_connect(process, "finished", G_CALLBACK(local_run_server_finished), server);

		GString *cmd_line = g_string_new(NULL);
		g_string_printf(cmd_line, "bash -c \"bash -l -c '%s >&3' 3>&1 >/dev/null 2>&1\"", binary);
		gebr_comm_process_start(process, cmd_line);
		g_string_free(cmd_line, TRUE);
	}
}

void gebr_comm_server_disconnect(struct gebr_comm_server *server)
{
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_NONE, "");
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_protocol_socket_disconnect(server->socket);
}

gboolean gebr_comm_server_is_logged(struct gebr_comm_server *server)
{
	return server->socket->protocol->logged;
}

gboolean gebr_comm_server_is_local(struct gebr_comm_server *server)
{
	return strcmp(server->address->str, "127.0.0.1") == 0 ? TRUE : FALSE;
}

void gebr_comm_server_kill(struct gebr_comm_server *server)
{
	GebrCommTerminalProcess *process;

	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Stopping server at '%s'."),
				     server->address->str);

	server->tried_existant_pass = FALSE;
	process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), server);

	GString *cmd_line = g_string_new(NULL);
	if (gebr_comm_server_is_local(server) == FALSE)
		g_string_printf(cmd_line, "ssh -x %s 'killall gebrd'", server->address->str);
	else
		g_string_printf(cmd_line, "killall gebrd");
	gebr_comm_terminal_process_start(process, cmd_line);
	g_string_free(cmd_line, TRUE);
}

gboolean gebr_comm_server_forward_x11(struct gebr_comm_server *server, guint16 port)
{
	gchar *display;
	guint16 display_number;
	guint16 redirect_display_port;

	gboolean ret = TRUE;
	GString *string;

	/* initialization */
	string = g_string_new(NULL);

	/* does we have a display? */
	display = getenv("DISPLAY");
	if (!(ret = !(display == NULL || !strlen(display))))
		goto out;
	GString *display_host = g_string_new_len(display, (strchr(display, ':')-display)/sizeof(gchar));
	if (!display_host->len)
		g_string_assign(display_host, "127.0.0.1");
	GString *tmp = g_string_new(strchr(display, ':'));
	if (sscanf(tmp->str, ":%hu.", &display_number) != 1)
		display_number = 0;
	g_string_free(tmp, TRUE);

	/* free previous forward */
	gebr_comm_server_free_x11_forward(server);

	g_string_printf(string, "/tmp/.X11-unix/X%hu", display_number);
	if (g_file_test(string->str, G_FILE_TEST_EXISTS)) {
		/* set redirection port */
		static gint start_port = 6010;
		while (!gebr_comm_listen_socket_is_local_port_available(start_port))
			++start_port;
		redirect_display_port = start_port;
		++start_port;

		server->x11_forward_unix = gebr_comm_process_new();
		GString *cmdline = g_string_new(NULL);
		g_string_printf(cmdline, "gebr-comm-socketchannel %d %s", redirect_display_port, string->str);
		gebr_comm_process_start(server->x11_forward_unix, cmdline);
		g_string_free(cmdline, TRUE);
	} else
		redirect_display_port = display_number+6000;

	/* now ssh from server to redirect_display_port */
	server->tried_existant_pass = FALSE;
	server->x11_forward_process = gebr_comm_terminal_process_new();
	g_signal_connect(server->x11_forward_process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	g_string_printf(string, "ssh -x -R %d:%s:%d %s -N", port, display_host->str, redirect_display_port, server->address->str);
	gebr_comm_terminal_process_start(server->x11_forward_process, string);

	/* log */
	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Redirecting '%s' graphical output."),
				     server->address->str);

	/* frees */
 out:	g_string_free(string, TRUE);

	return ret;
}

/**
 * \internal
 */
static void
gebr_comm_server_log_message(struct gebr_comm_server *server, GebrLogMessageType type,
			     const gchar * message, ...)
{
	va_list argp;
	va_start(argp, message);
	gchar *string = g_strdup_vprintf(message, argp);
	server->ops->log_message(server, type, string, server->user_data);
	g_free(string);
	va_end(argp);
}

/**
 * \internal
 */
static void local_run_server_read(GebrCommProcess * process, struct gebr_comm_server *server)
{
	GString *output;
	gchar *strtol_endptr;
	guint16 port;

	output = gebr_comm_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		server->port = port;
		gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "local_run_server_read: %d", port);
	} else {
		gebr_comm_server_disconnected_state(server, SERVER_ERROR_SERVER,
						    _("Could not launch local server: \n%s"), output->str);
		gebr_comm_server_log_message(server, GEBR_LOG_ERROR, server->last_error->str);
	}

	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void local_run_server_finished(GebrCommProcess * process, gint status, struct gebr_comm_server *server)
{
	GebrCommSocketAddress socket_address;

	server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_process_free(process);

	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "local_run_server_finished");
	if (server->error != SERVER_ERROR_NONE)
		return;

	gebr_comm_server_change_state(server, SERVER_STATE_CONNECT);
	socket_address = gebr_comm_socket_address_ipv4_local(server->port);
	gebr_comm_protocol_socket_connect(server->socket, &socket_address, FALSE);
}

static void
write_pass_in_process(GebrCommTerminalProcess *process,
		      const gchar *pass)
{
	gsize len = strlen(pass);
	GString p = {(gchar*)pass, len, len};
	gebr_comm_terminal_process_write_string(process, &p);
	p.str = "\n";
	p.len = p.allocated_len = 1;
	gebr_comm_terminal_process_write_string(process, &p);
}

/**
 * Return TRUE if callee should not proceed.
 */
static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess *process,
			   struct gebr_comm_server *server,
			   GString * output)
{
	if (output->len <= 2)
		return TRUE;
	if (output->str[output->len - 2] == ':') {
		GString *string;
		GString *password;

		if (server->password && *server->password
		    && !server->tried_existant_pass) {
			write_pass_in_process(process, server->password);
			server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		if (server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Server '%s' needs SSH login.\n\n%s"),
					server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for server '%s', please try again.\n\n%s"),
					server->address->str, output->str);

		password = server->ops->ssh_login(server, _("SSH login:"), string->str,
						  server->user_data);
		if (password == NULL) {
			g_free(server->password);
			server->password = NULL;

			gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("No password provided."));
			gebr_comm_terminal_process_kill(process);
		} else {
			gebr_comm_server_set_password(server, password->str);
			write_pass_in_process(process, password->str);
			g_string_free(password, TRUE);
		}
		server->tried_existant_pass = FALSE;

		g_string_free(string, TRUE);
	} else if (output->str[output->len - 2] == '?') {
		GString *answer = g_string_new(NULL);
		if (server->ops->ssh_question(server, _("SSH host key question:"),
					      output->str, server->user_data) == FALSE) {
			g_string_assign(answer, "no\n");
			gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("SSH host key rejected."));
		} else
			g_string_assign(answer, "yes\n");
		gebr_comm_terminal_process_write_string(process, answer);
		g_string_free(answer, TRUE);
	} else if (g_str_has_prefix(output->str, "@@@")) {
		gebr_g_string_replace(output, "\r\n", "\n");
		g_string_erase(output, strlen(output->str)-1, 1); //last \n

		gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, _("SSH error: %s"), output->str);
		gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Received SSH error for server '%s': %s"),
					     server->address->str, output->str);
	} else if (output->str[output->len - 4] == '.') {
		gebr_g_string_replace(output, "\r\n", "\n");
		g_string_erase(output, strlen(output->str)-1, 1); //last \n

		gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Received SSH message for server '%s': %s"),
					     server->address->str, output->str);
	} else if (!strcmp(output->str, "yes\r\n")) {
		goto out;
	} else {
		/* check for known error prefixes */
		if (g_str_has_prefix(output->str, "ssh:") || g_str_has_prefix(output->str, "channel ")) {
			gebr_g_string_replace(output, "\r\n", "\n");
			g_string_erase(output, strlen(output->str)-1, 1); //last \n

			gebr_comm_server_disconnected_state(server, SERVER_ERROR_SSH, 
							    _("SSH reported the following error: \n%s"), output->str);
			gebr_comm_server_log_message(server, GEBR_LOG_ERROR, _("SSH reported the following error for server '%s': %s"),
						     server->address->str, output->str);
			return TRUE;
		}

		return FALSE;
	}

out:	return TRUE;
}

/**
 * \internal
 * Simple ssh messages parser, like login questions and warnings
 */
static void gebr_comm_ssh_read(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
	GString *output;

	output = gebr_comm_terminal_process_read_string_all(process);
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_read: %s", output->str);

	if (gebr_comm_ssh_parse_output(process, server, output) == TRUE)
		goto out;

out:	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_finished");
	server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_terminal_process_free(process);
}

/**
 * \internal
 */
static void gebr_comm_ssh_run_server_read(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
	gchar *strtol_endptr;
	guint16 port;
	GString *output;

	output = gebr_comm_terminal_process_read_string_all(process);
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_read: %s",
				     output->str);
	if (gebr_comm_ssh_parse_output(process, server, output) == TRUE)
		goto out;

	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		server->port = port;
		gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_read: %d",
					     port);
	} else {
		gebr_comm_server_disconnected_state(server, SERVER_ERROR_SERVER, _("Could not run server."));
		gebr_comm_server_log_message(server, GEBR_LOG_ERROR, _("Could not run server '%s'."), server->address->str);
	}

out:	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void
gebr_comm_ssh_run_server_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_finished");

	server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_terminal_process_free(process);

	if (server->error != SERVER_ERROR_NONE)
		return;

	server->tried_existant_pass = FALSE;
	server->process.use = COMM_SERVER_PROCESS_TERMINAL;
	server->process.data.terminal = process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), server);

	gebr_comm_server_change_state(server, SERVER_STATE_OPEN_TUNNEL);
	static gint tunnel_port = 2125;
	while (!gebr_comm_listen_socket_is_local_port_available(tunnel_port))
		++tunnel_port;
	server->tunnel_port = tunnel_port;
	++tunnel_port;

	GString *cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "ssh -x -L %d:127.0.0.1:%d %s 'sleep 300'",
			server->tunnel_port, server->port, server->address->str);
	gebr_comm_terminal_process_start(process, cmd_line);
	g_string_free(cmd_line, TRUE);
	
	server->tunnel_pooling_source = g_timeout_add(200, (GSourceFunc)gebr_comm_ssh_open_tunnel_pool, server);
}

/**
 * \internal
 */
static gboolean
gebr_comm_ssh_open_tunnel_pool(struct gebr_comm_server *server)
{
	GebrCommSocketAddress socket_address;

	/* wait for ssh listen tunnel port */
	if (gebr_comm_listen_socket_is_local_port_available(server->tunnel_port))
		return TRUE;

	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_open_tunnel_pool");

	if (server->error != SERVER_ERROR_NONE)
		goto out;

	gebr_comm_server_change_state(server, SERVER_STATE_CONNECT);
	socket_address = gebr_comm_socket_address_ipv4_local(server->tunnel_port);
	gebr_comm_protocol_socket_connect(server->socket, &socket_address, FALSE);

out:	server->tunnel_pooling_source = 0;
	return FALSE;
}

/**
 * \internal
 */
static void gebr_comm_server_disconnected_state(struct gebr_comm_server *server, enum gebr_comm_server_error error,
						const gchar * message, ...)
{
	if (error != SERVER_ERROR_UNKNOWN) {
		server->error = error;
		va_list argp;
		va_start(argp, message);
		gchar *string = g_strdup_vprintf(message, argp);
		g_string_assign(server->last_error, string);
		g_free(string);
		va_end(argp);
	}

	/* take care not to free the process here cause this function
	 * maybe be used by Process's read callback */
	server->port = 0;
	server->socket->protocol->logged = FALSE;
	gebr_comm_server_change_state(server, SERVER_STATE_DISCONNECTED);
}

static void gebr_comm_server_change_state(struct gebr_comm_server *server, GebrCommServerState state)
{
	if (server->state != SERVER_STATE_UNKNOWN) {
		server->state = state;
		server->ops->state_changed(server, server->user_data);
	} else
		server->state = state;
}

/**
 * \internal
 */
static void gebr_comm_server_socket_connected(GebrCommProtocolSocket * socket, struct gebr_comm_server *server)
{
	gchar hostname[256];
	gchar *display;
	gchar *display_number = NULL;

	/* initialization */
	gethostname(hostname, 255);
	display = getenv("DISPLAY");
	if (display == NULL)
		display = "";
	else
		display_number = strchr(display, ':');

	if (gebr_comm_server_is_local(server) == FALSE) {
		gchar mcookie_str[33];

		if (display_number != NULL) {
			/* get this X session magic cookie */
			GString *cmd_line = g_string_new(NULL);
			g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display_number);
			/* WORKAROUND: if xauth is already executing it will lock
			 * the auth file and it will fail to retrieve the m-cookie.
			 * So, as a workaround, we try to get the m-cookie many times.
			 */
			for (gint try = 0;;) {
				FILE *output_fp = popen(cmd_line->str, "r");
				if (fscanf(output_fp, "%32s", mcookie_str) != 1) {
					g_warning("%s:%d: Error fetching authorization code for display %s",
						  __FILE__, __LINE__, display);
					usleep(100*1000);
				} else
					break;
				pclose(output_fp);

				if (++try == 5) {
					strcpy(mcookie_str, "");
					break;
				}
			}
			g_string_free(cmd_line, TRUE);
		} else
			strcpy(mcookie_str, "");

		/* send INI */
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "remote",
					     mcookie_str);
	} else {
		/* send INI */
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "local",
					     display);
	}
}

/**
 * \internal
 */
static void gebr_comm_server_socket_disconnected(GebrCommProtocolSocket * socket, struct gebr_comm_server *server)
{
	gebr_comm_server_disconnected_state(server, SERVER_ERROR_UNKNOWN, "");
	gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Server '%s' disconnected"), 
				     server->address->str);
}

/**
 * \internal
 */
static void gebr_comm_server_socket_process_request(GebrCommProtocolSocket * socket, GebrCommHttpMsg *request,
						    struct gebr_comm_server *server)
{
	server->ops->process_request(server, request, server->user_data);
}

/**
 * \internal
 */
static void gebr_comm_server_socket_process_response(GebrCommProtocolSocket * socket, GebrCommHttpMsg *request,
						     GebrCommHttpMsg *response, struct gebr_comm_server *server)
{
	server->ops->process_response(server, request, response, server->user_data);
}

/**
 * \internal
 */
static void gebr_comm_server_socket_old_parse_messages(GebrCommProtocolSocket * socket, struct gebr_comm_server *server)
{
	server->ops->parse_messages(server, server->user_data);
}

/**
 * \internal
 * Free (if necessary) server->x11_forward_process for reuse
 */
static void gebr_comm_server_free_x11_forward(struct gebr_comm_server *server)
{
	if (server->tunnel_pooling_source) {
		g_source_remove(server->tunnel_pooling_source);
		server->tunnel_pooling_source = 0;
	}

	if (server->x11_forward_process != NULL) {
		gebr_comm_terminal_process_kill(server->x11_forward_process);
		gebr_comm_terminal_process_free(server->x11_forward_process);
		server->x11_forward_process = NULL;
	}
	if (server->x11_forward_unix != NULL) {
		gebr_comm_process_free(server->x11_forward_unix);
		server->x11_forward_unix = NULL;
	}
}

/**
 * \internal
 */
static void gebr_comm_server_free_for_reuse(struct gebr_comm_server *server)
{
	gebr_comm_protocol_reset(server->socket->protocol);
	gebr_comm_server_free_x11_forward(server);
	switch (server->process.use) {
	case COMM_SERVER_PROCESS_NONE:
		break;
	case COMM_SERVER_PROCESS_TERMINAL:
		gebr_comm_terminal_process_free(server->process.data.terminal);
		server->process.data.terminal = NULL;
		break;
	case COMM_SERVER_PROCESS_REGULAR:
		gebr_comm_process_free(server->process.data.regular);
		server->process.data.regular = NULL;
		break;
	}
	server->process.use = COMM_SERVER_PROCESS_NONE;
}

static const gchar *state_hash[] = {
	"unknown",
	"disconnected",
	"run",
	"open_tunnel",
	"connect",
	"connected",
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

	return SERVER_STATE_UNKNOWN;
}

GebrCommServerState
gebr_comm_server_get_state(GebrCommServer *server)
{
	return server->state;
}

void
gebr_comm_server_set_password(GebrCommServer *server, const gchar *pass)
{
	server->password = g_strdup(pass);
}
