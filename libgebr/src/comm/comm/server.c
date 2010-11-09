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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../intl.h"
#include "../../defines.h"

#include "server.h"
#include "listensocket.h"

/*
 * Declarations
 */

static void
gebr_comm_server_log_message(struct gebr_comm_server *server, enum gebr_log_message_type type,
			     const gchar * message, ...);

static void local_run_server_read(GebrCommProcess * process, struct gebr_comm_server *server);
static void local_run_server_finished(GebrCommProcess * process, struct gebr_comm_server *server);

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

static void gebr_comm_server_connected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *server);
static void gebr_comm_server_disconnected_state(struct gebr_comm_server *server);
static void gebr_comm_server_socket_disconnected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *server);
static void gebr_comm_server_socket_read(GebrCommStreamSocket * stream_socket, struct gebr_comm_server
					 *server);
static void
gebr_comm_server_error(GebrCommStreamSocket * stream_socket, enum GebrCommSocketError error,
		       struct gebr_comm_server *server);

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
	server->stream_socket = gebr_comm_stream_socket_new();
	server->protocol = gebr_comm_protocol_new();
	server->address = g_string_new(_address);
	server->port = 0;
	server->password = g_string_new("");
	server->x11_forward_process = NULL;
	server->x11_forward_unix = NULL;
	server->state = SERVER_STATE_DISCONNECTED;
	server->error = SERVER_ERROR_NONE;
	server->ops = ops;
	server->tunnel_pooling_source = 0;
	server->process.use = COMM_SERVER_PROCESS_NONE;

	g_signal_connect(server->stream_socket, "connected",
			 G_CALLBACK(gebr_comm_server_connected), server);
	g_signal_connect(server->stream_socket, "disconnected",
			 G_CALLBACK(gebr_comm_server_socket_disconnected), server);
	g_signal_connect(server->stream_socket, "ready-read",
			 G_CALLBACK(gebr_comm_server_socket_read), server);
	g_signal_connect(server->stream_socket, "error",
			 G_CALLBACK(gebr_comm_server_error), server);

	gebr_comm_server_disconnected_state(server);
	gebr_comm_server_free_for_reuse(server);

	return server;
}

void gebr_comm_server_free(struct gebr_comm_server *server)
{
	gebr_comm_socket_close(GEBR_COMM_SOCKET(server->stream_socket));
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_protocol_free(server->protocol);
	g_string_free(server->address, TRUE);
	g_string_free(server->password, TRUE);
	g_free(server);
}

void gebr_comm_server_connect(struct gebr_comm_server *server)
{
	GString *cmd_line;

	gebr_comm_server_free_for_reuse(server);
	server->error = SERVER_ERROR_NONE;
	server->state = SERVER_STATE_RUN;
	server->tried_existant_pass = FALSE;
	cmd_line = g_string_new(NULL);

	/* initiate the marathon to communicate to server */
	if (gebr_comm_server_is_local(server) == FALSE) {
		GebrCommTerminalProcess *process;

		server->state = SERVER_STATE_RUN;
		gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Launching server at %s."),
					     server->address->str);

		server->process.use = COMM_SERVER_PROCESS_TERMINAL;
		server->process.data.terminal = process = gebr_comm_terminal_process_new();
		g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_run_server_read), server);
		g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_run_server_finished), server);

		g_string_printf(cmd_line, "ssh -x %s \"bash -l -c 'gebrd >&3' 3>&1 >/dev/null\"",
				server->address->str);
		gebr_comm_terminal_process_start(process, cmd_line);
	} else {
		GebrCommProcess *process;

		server->state = SERVER_STATE_RUN;
		gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Launching local server."));

		server->process.use = COMM_SERVER_PROCESS_REGULAR;
		server->process.data.regular = process = gebr_comm_process_new();
		g_signal_connect(process, "ready-read-stdout", G_CALLBACK(local_run_server_read), server);
		g_signal_connect(process, "finished", G_CALLBACK(local_run_server_finished), server);

		g_string_printf(cmd_line, "bash -c \"bash -l -c 'gebrd >&3' 3>&1 >/dev/null\"");
		gebr_comm_process_start(process, cmd_line);
	}

	g_string_free(cmd_line, TRUE);
}

void gebr_comm_server_disconnect(struct gebr_comm_server *server)
{
	gebr_comm_server_disconnected_state(server);
	gebr_comm_server_free_for_reuse(server);
	gebr_comm_stream_socket_disconnect(server->stream_socket);
}

gboolean gebr_comm_server_is_logged(struct gebr_comm_server *server)
{
	return server->protocol->logged;
}

gboolean gebr_comm_server_is_local(struct gebr_comm_server *server)
{
	return strcmp(server->address->str, "127.0.0.1") == 0 ? TRUE : FALSE;
}

void gebr_comm_server_kill(struct gebr_comm_server *server)
{
	GString *cmd_line;
	GebrCommTerminalProcess *process;

	cmd_line = g_string_new(NULL);
	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Stoping server at %s."),
				     server->address->str);

	server->tried_existant_pass = FALSE;
	process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), server);

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
	g_signal_connect(server->x11_forward_process, "ready-read",
			 G_CALLBACK(gebr_comm_ssh_read), server);
	g_string_printf(string, "ssh -x -R %d:%s:%d %s -N", port, display_host->str, redirect_display_port, server->address->str);
	gebr_comm_terminal_process_start(server->x11_forward_process, string);

	/* log */
	gebr_comm_server_log_message(server, GEBR_LOG_INFO, _("Redirecting '%s' graphical output."),
				     server->address->str);

	/* frees */
 out:	g_string_free(string, TRUE);

	return ret;
}

void gebr_comm_server_run_flow(struct gebr_comm_server *server, GebrCommServerRun * config)
{
	GebrGeoXmlFlow *flow_wnh;	/* wnh: with no help */
	GebrGeoXmlSequence *program;
	gchar *xml;

	/* removes flow's help */
	flow_wnh = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOC(config->flow)));
	gebr_geoxml_document_set_help(GEBR_GEOXML_DOC(flow_wnh), "");
	/* removes programs' help */
	gebr_geoxml_flow_get_program(flow_wnh, &program, 0);
	while (program != NULL) {
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(program), "");

		gebr_geoxml_sequence_next(&program);
	}

	/* get the xml */
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOC(flow_wnh), &xml);

	gebr_comm_protocol_send_data(server->protocol, server->stream_socket,
				     gebr_comm_protocol_defs.run_def, 4, xml,
				     config->account ? config->account : "",
				     config->queue ? config->queue : "",
				     config->num_processes ? config->num_processes : "");
	/* frees */
	g_free(xml);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(flow_wnh));
}

/**
 * \internal
 */
static void
gebr_comm_server_log_message(struct gebr_comm_server *server, enum gebr_log_message_type type,
			     const gchar * message, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	server->ops->log_message(type, string);

	g_free(string);
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
		server->error = SERVER_ERROR_SERVER;
		gebr_comm_server_disconnected_state(server);
		gebr_comm_server_log_message(server, GEBR_LOG_ERROR, _("Could not launch local server: \n%s"),
					     output->str);
	}

	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void local_run_server_finished(GebrCommProcess * process, struct gebr_comm_server *server)
{
	GebrCommSocketAddress socket_address;

	server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_process_free(process);

	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "local_run_server_finished");
	if (server->error != SERVER_ERROR_NONE)
		return;

	server->state = SERVER_STATE_CONNECT;
	socket_address = gebr_comm_socket_address_ipv4_local(server->port);
	gebr_comm_stream_socket_connect(server->stream_socket, &socket_address, FALSE);
}

/**
 * \internal
 * Return TRUE if callee should not proceed.
 */
static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess * process, struct gebr_comm_server *server,
			   GString * output)
{
	if (output->len <= 2)
		return TRUE;
	if (output->str[output->len - 2] == ':') {
		GString *string;
		GString *password;

		if (server->password->len && server->tried_existant_pass == FALSE) {
			gebr_comm_terminal_process_write_string(process, server->password);
			server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		if (server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Server %s needs SSH login.\n\n%s"),
					server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for server %s, please try again.\n\n%s"),
					server->address->str, output->str);

		password = server->ops->ssh_login(_("SSH login:"), string->str);
		if (password == NULL) {
			g_string_assign(server->password, "");

			server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(server);
			gebr_comm_terminal_process_kill(process);
		} else {
			g_string_printf(server->password, "%s\n", password->str);
			gebr_comm_terminal_process_write_string(process, server->password);
			g_string_free(password, TRUE);
		}
		server->tried_existant_pass = FALSE;

		g_string_free(string, TRUE);
	} else if (output->str[output->len - 2] == '?') {
		GString *answer;

		answer = g_string_new(NULL);
		if (server->ops->ssh_question(_("SSH inquiry:"), output->str) == FALSE) {
			g_string_assign(answer, "no\n");

			server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(server);
		} else
			g_string_assign(answer, "yes\n");
		gebr_comm_terminal_process_write_string(process, answer);

		g_string_free(answer, TRUE);
	} else if (output->str[output->len - 4] == '.') {
		gebr_comm_server_log_message(server, GEBR_LOG_WARNING, _("Received SSH message: \n%s"),
					     output->str);
	} else if (!strcmp(output->str, "yes\r\n")) {
		goto out;
	} else {
		/* check if it is an ssh error: check if string starts with "ssh:" */
		gchar *str;

		str = strstr(output->str, "ssh:");
		if (str == output->str) {
			server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(server);
			gebr_comm_server_log_message(server, GEBR_LOG_ERROR,
						     _("Error contacting server at address %s via ssh: \n%s"),
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
	gebr_comm_ssh_parse_output(process, server, output);

	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_read: %s", output->str);

	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
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
		server->error = SERVER_ERROR_SERVER;
		gebr_comm_server_disconnected_state(server);
		gebr_comm_server_log_message(server, GEBR_LOG_ERROR,
					     _("Could not comunicate with server %s: \n%s"),
					     server->address->str, output->str);
	}

out:	g_string_free(output, TRUE);
}

/**
 * \internal
 */
static void
gebr_comm_ssh_run_server_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *server)
{
	GString *cmd_line;

	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_finished");

	server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_terminal_process_free(process);

	if (server->error != SERVER_ERROR_NONE)
		return;

	/* initializations */
	cmd_line = g_string_new(NULL);

	server->tried_existant_pass = FALSE;
	server->process.use = COMM_SERVER_PROCESS_TERMINAL;
	server->process.data.terminal = process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), server);

	server->state = SERVER_STATE_OPEN_TUNNEL;
	static gint tunnel_port = 2125;
	while (!gebr_comm_listen_socket_is_local_port_available(tunnel_port))
		++tunnel_port;
	server->tunnel_port = tunnel_port;
	++tunnel_port;

	g_string_printf(cmd_line, "ssh -x -L %d:127.0.0.1:%d %s 'sleep 300'",
			server->tunnel_port, server->port, server->address->str);
	gebr_comm_terminal_process_start(process, cmd_line);
	server->tunnel_pooling_source = g_timeout_add(200, (GSourceFunc)gebr_comm_ssh_open_tunnel_pool, server);

	/* frees */
	g_string_free(cmd_line, TRUE);
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

	server->state = SERVER_STATE_CONNECT;
	socket_address = gebr_comm_socket_address_ipv4_local(server->tunnel_port);
	gebr_comm_stream_socket_connect(server->stream_socket, &socket_address, FALSE);

out:	server->tunnel_pooling_source = 0;
	return FALSE;
}

/**
 * \internal
 */
static void gebr_comm_server_connected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *server)
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
		GString *cmd_line;
		gchar mcookie_str[33];

		cmd_line = g_string_new(NULL);

		if (display_number != NULL) {
			FILE *output_fp;

			/* get this X session magic cookie */
			g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display_number);
			gint try = 0;
			while (try++ < 5) {
				output_fp = popen(cmd_line->str, "r");
				if (fscanf(output_fp, "%32s", mcookie_str) != 1) {
					g_warning("%s:%d: Error fetching authorization code for display %s",
						  __FILE__, __LINE__, display);
					usleep(100*1000);
				}

				pclose(output_fp);
			}
			if (try == 5)
				strcpy(mcookie_str, "");
		} else
			strcpy(mcookie_str, "");

		/* send INI */
		gebr_comm_protocol_send_data(server->protocol, server->stream_socket,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "remote",
					     mcookie_str);

		g_string_free(cmd_line, TRUE);
	} else {
		/* send INI */
		gebr_comm_protocol_send_data(server->protocol, server->stream_socket,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "local",
					     display);
	}
}

/**
 * \internal
 */
static void gebr_comm_server_disconnected_state(struct gebr_comm_server *server)
{
	/* take care not to free the process here cause this function
	 * maybe be used by Process's read callback */
	server->port = 0;
	server->protocol->logged = FALSE;
	server->state = SERVER_STATE_DISCONNECTED;
}

/**
 * \internal
 */
static void gebr_comm_server_socket_disconnected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *server)
{
	gebr_comm_server_disconnected_state(server);

	gebr_comm_server_log_message(server, GEBR_LOG_WARNING, "Server '%s' disconnected",
				     server->address->str);
}

/**
 * \internal
 */
static void gebr_comm_server_socket_read(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *server)
{
	GString *data;
	gchar *data_stripped;

	data = gebr_comm_socket_read_string_all(GEBR_COMM_SOCKET(stream_socket));
	gebr_comm_protocol_receive_data(server->protocol, data);
	server->ops->parse_messages(server, server->user_data);

	/* we don't want a giant output */
	data_stripped = g_strndup(data->str, 50);
	gebr_comm_server_log_message(server, GEBR_LOG_DEBUG, "Read from server '%s': %s",
				     server->address->str, data_stripped);

	g_string_free(data, TRUE);
	g_free(data_stripped);
}

/**
 * \internal
 */
static void
gebr_comm_server_error(GebrCommStreamSocket * stream_socket, enum GebrCommSocketError error,
		       struct gebr_comm_server *server)
{
	server->error = SERVER_ERROR_CONNECT;
	gebr_comm_server_disconnected_state(server);
	gebr_comm_server_log_message(server, GEBR_LOG_ERROR, _("Connection error '%d' on server '%s'."), error,
				     server->address->str);
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
	gebr_comm_protocol_reset(server->protocol);
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
