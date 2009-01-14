/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include "server.h"
#include "glistensocket.h"
#include "support.h"

/*
 * Declarations
 */

static void
comm_server_log_message(struct comm_server * comm_server, enum log_message_type type, const gchar * message, ...);

static void
local_run_server_read(GProcess * process, struct comm_server * comm_server);
static void
local_run_server_finished(GProcess * process, struct comm_server * comm_server);

static gboolean
comm_ssh_parse_output(GTerminalProcess * process, struct comm_server * comm_server, GString * output);
static void
comm_ssh_read(GTerminalProcess * process, struct comm_server * comm_server);
static void
comm_ssh_run_server_read(GTerminalProcess * process, struct comm_server * comm_server);
static void
comm_ssh_run_server_finished(GTerminalProcess * process, struct comm_server * comm_server);
static void
comm_ssh_open_tunnel_finished(GTerminalProcess * process, struct comm_server * comm_server);

static void
comm_server_connected(GStreamSocket * stream_socket, struct comm_server * comm_server);
static void
comm_server_disconnected(GStreamSocket * stream_socket, struct comm_server * comm_server);
static void
comm_server_read(GStreamSocket * stream_socket, struct comm_server * comm_server);
static void
comm_server_error(GStreamSocket * stream_socket, enum GSocketError error, struct comm_server * comm_server);

static void
comm_server_free_x11_process(struct comm_server * comm_server);
static void
comm_server_free_for_reuse(struct comm_server * comm_server);

/*
 * Section: Public
 */

struct comm_server *
comm_server_new(const gchar * _address, const struct comm_server_ops * ops)
{
	struct comm_server *	comm_server;

	/* initialize */
	comm_server = g_malloc(sizeof(struct comm_server));
	*comm_server = (struct comm_server) {
		.stream_socket = g_stream_socket_new(),
		.protocol = protocol_new(),
		.address = g_string_new(_address),
		.port = 0,
		.password = g_string_new(""),
		.x11_forward = NULL,
		.state = SERVER_STATE_DISCONNECTED,
		.error = SERVER_ERROR_NONE,
		.ops = ops,
	};

	g_signal_connect(comm_server->stream_socket, "connected",
		G_CALLBACK(comm_server_connected), comm_server);
	g_signal_connect(comm_server->stream_socket, "disconnected",
		G_CALLBACK(comm_server_disconnected), comm_server);
	g_signal_connect(comm_server->stream_socket, "ready-read",
		G_CALLBACK(comm_server_read), comm_server);
	g_signal_connect(comm_server->stream_socket, "error",
		G_CALLBACK(comm_server_error), comm_server);

	return comm_server;
}

void
comm_server_free(struct comm_server * comm_server)
{
	g_socket_close(G_SOCKET(comm_server->stream_socket));
	protocol_free(comm_server->protocol);
	g_string_free(comm_server->address, TRUE);
	g_string_free(comm_server->password, TRUE);
	comm_server_free_for_reuse(comm_server);
	g_free(comm_server);
}

void
comm_server_connect(struct comm_server * comm_server)
{
	GString *	cmd_line;

	comm_server_free_for_reuse(comm_server);
	comm_server->error = SERVER_ERROR_NONE;
	comm_server->state = SERVER_STATE_RUN;
	comm_server->tried_existant_pass = FALSE;
	cmd_line = g_string_new(NULL);

	/* initiate the marathon to communicate to server */
	if (comm_server_is_local(comm_server) == FALSE) {
		GTerminalProcess *	process;

		comm_server->state = SERVER_STATE_RUN;
		comm_server_log_message(comm_server, LOG_INFO, _("Launching server at %s"), comm_server->address->str);

		comm_server->process.use = COMM_SERVER_PROCESS_TERMINAL;
		comm_server->process.data.terminal = process = g_terminal_process_new();
		g_signal_connect(process, "ready-read",
			G_CALLBACK(comm_ssh_run_server_read), comm_server);
		g_signal_connect(process, "finished",
			G_CALLBACK(comm_ssh_run_server_finished), comm_server);

		g_string_printf(cmd_line, "ssh -x %s \"bash -l -c gebrd\"",
			comm_server->address->str);
		g_terminal_process_start(process, cmd_line);
	} else {
		GProcess *		process;

		comm_server->state = SERVER_STATE_RUN;
		comm_server_log_message(comm_server, LOG_INFO, _("Launching local server"));

		comm_server->process.use = COMM_SERVER_PROCESS_REGULAR;
		comm_server->process.data.regular = process = g_process_new();
		g_signal_connect(process, "ready-read-stdout",
			G_CALLBACK(local_run_server_read), comm_server);
		g_signal_connect(process, "finished",
			G_CALLBACK(local_run_server_finished), comm_server);

#if (!LIBGEBR_STATIC_MODE)
		g_string_printf(cmd_line, "bash -l -c 'gebrd'");
#else
		g_string_printf(cmd_line, "bash -l -c './gebrd'");
#endif
		g_process_start(process, cmd_line);
	}

	g_string_free(cmd_line, TRUE);
}

void
comm_server_disconnect(struct comm_server * comm_server)
{
	comm_server->state = SERVER_STATE_DISCONNECTED;
	g_stream_socket_disconnect(comm_server->stream_socket);
}

gboolean
comm_server_is_logged(struct comm_server * comm_server)
{
	return comm_server->protocol->logged;
}

gboolean
comm_server_is_local(struct comm_server * comm_server)
{
	return strcmp(comm_server->address->str, "127.0.0.1") == 0
		? TRUE : FALSE;
}

/*
 * Function: comm_server_forward_x11
 * For the logged _comm_server_ forward x11 server _port_ to user display
 * Fail if user's display is not set, returning FALSE.
 * If any other x11 redirect was previously made it is unmade
 */
gboolean
comm_server_forward_x11(struct comm_server * comm_server, guint16 port)
{
	gchar *		display;
	guint16		display_port;
	GString *	cmd_line;

	display = getenv("DISPLAY");
	if (display == NULL || !strlen(display))
		return FALSE;

	/* initialization */
	cmd_line = g_string_new(NULL);
	sscanf(display, ":%hu.", &display_port);
	display_port += 6000;

	comm_server_free_x11_process(comm_server);

	comm_server_log_message(comm_server, LOG_INFO, _("Redirecting '%s' graphical output"),
		comm_server->address->str);

	comm_server->tried_existant_pass = FALSE;
	comm_server->x11_forward = g_terminal_process_new();
	g_signal_connect(comm_server->x11_forward, "ready-read",
		G_CALLBACK(comm_ssh_read), comm_server);

	g_string_printf(cmd_line, "ssh -x -R %d:127.0.0.1:%d %s 'sleep 999d'",
		port, display_port, comm_server->address->str);
	g_terminal_process_start(comm_server->x11_forward, cmd_line);

	/* frees */
	g_string_free(cmd_line, TRUE);

	return TRUE;
}

/*
 * Function: comm_server_run_flow
 * Ask _comm_server_ to run the current _flow_
 *
 */
void
comm_server_run_flow(struct comm_server * comm_server, GeoXmlFlow * flow)
{
	GeoXmlFlow *		flow_wnh; /* wnh: with no help */
	GeoXmlSequence *	program;
	gchar *			xml;

	/* removes flow's help */
	flow_wnh = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOC(flow)));
	geoxml_document_set_help(GEOXML_DOC(flow_wnh), "");
	/* removes programs' help */
	geoxml_flow_get_program(flow_wnh, &program, 0);
	while (program != NULL) {
		geoxml_program_set_help(GEOXML_PROGRAM(program), "");

		geoxml_sequence_next(&program);
	}

	/* get the xml */
	geoxml_document_to_string(GEOXML_DOC(flow_wnh), &xml);

	/* finally... */
	protocol_send_data(comm_server->protocol, comm_server->stream_socket, protocol_defs.run_def, 1, xml);

	/* frees */
	g_free(xml);
	geoxml_document_free(GEOXML_DOC(flow_wnh));
}

/*
 * Section: Private
 */

static void
comm_server_log_message(struct comm_server * comm_server, enum log_message_type type, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	comm_server->ops->log_message(type, string);

	g_free(string);
}

static void
local_run_server_read(GProcess * process, struct comm_server * comm_server)
{
	GString *	output;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		comm_server->port = port;
		comm_server_log_message(comm_server, LOG_DEBUG, "local_run_server_read: %d", port);
	} else {
		comm_server->error = SERVER_ERROR_SERVER;
		comm_server->state = SERVER_STATE_DISCONNECTED;
		comm_server_log_message(comm_server, LOG_ERROR, _("Could not launch local server: \n%s"),
			output->str);
	}

	g_string_free(output, TRUE);
}

static void
local_run_server_finished(GProcess * process, struct comm_server * comm_server)
{
	GSocketAddress *	socket_address;

	comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	g_process_free(process);

	comm_server_log_message(comm_server, LOG_DEBUG, "local_run_server_finished");
	if (comm_server->error != SERVER_ERROR_NONE)
		return;

	comm_server->state = SERVER_STATE_CONNECT;
	socket_address = g_socket_address_new("127.0.0.1", G_SOCKET_ADDRESS_TYPE_IPV4);
	g_stream_socket_connect(comm_server->stream_socket, socket_address, comm_server->port, FALSE);

	g_socket_address_free(socket_address);
}

static gboolean
comm_ssh_parse_output(GTerminalProcess * process, struct comm_server * comm_server, GString * output)
{
	if (output->len <= 2)
		return TRUE;
	if (output->str[output->len-2] == ':') {
		GString *	string;
		GString *	password;

		if (comm_server->password->len && comm_server->tried_existant_pass == FALSE) {
			g_terminal_process_write_string(process, comm_server->password);
			comm_server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		if (comm_server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Server %s needs SSH login.\n\n%s"),
				comm_server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for server %s, please try again.\n\n%s"),
				comm_server->address->str, output->str);

		password = comm_server->ops->ssh_login(_("SSH login:"), string->str);
		if (password == NULL) {
			comm_server->error = SERVER_ERROR_SSH;
			comm_server->state = SERVER_STATE_DISCONNECTED;
			g_string_assign(comm_server->password, "");
			g_terminal_process_kill(process);
		} else {
			g_string_printf(comm_server->password, "%s\n", password->str);
			g_terminal_process_write_string(process, comm_server->password);
			g_string_free(password, TRUE);
		}
		comm_server->tried_existant_pass = FALSE;

		g_string_free(string, TRUE);
	} else if (output->str[output->len-2] == '?') {
		GString *	answer;

		answer = g_string_new(NULL);
		if (comm_server->ops->ssh_question(_("SSH question:"), output->str) == FALSE) {
			comm_server->error = SERVER_ERROR_SSH;
			comm_server->state = SERVER_STATE_DISCONNECTED;
			g_string_assign(answer, "no\n");
		} else
			g_string_assign(answer, "yes\n");
		g_terminal_process_write_string(process, answer);

		g_string_free(answer, TRUE);
	} else if (output->str[output->len-4] == '.') {
		comm_server_log_message(comm_server, LOG_WARNING, _("Received SSH message: \n%s"),
			output->str);
	} else if (!strcmp(output->str, "yes\r\n")) {
		goto out;
	} else {
		/* check if it is an ssh error: check if string starts with "ssh:" */
		gchar *	str;

		str = strstr(output->str, "ssh:");
		if (str == output->str) {
			comm_server->error = SERVER_ERROR_SSH;
			comm_server->state = SERVER_STATE_DISCONNECTED;
			comm_server_log_message(comm_server, LOG_ERROR,
				_("Error contacting server at address %s via ssh: \n%s"),
				comm_server->address->str, output->str);
			return TRUE;
		}

		return FALSE;
	}

out:	return TRUE;
}

/*
 * Function: comm_ssh_read
 * Simple ssh messages parser, like login questions and warnings
 */
static void
comm_ssh_read(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	output;

	output = g_terminal_process_read_string_all(process);
	comm_ssh_parse_output(process, comm_server, output);

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_read: %s", output->str);

	g_string_free(output, TRUE);
}

static void
comm_ssh_run_server_read(GTerminalProcess * process, struct comm_server * comm_server)
{
	gchar *		strtol_endptr;
	guint16		port;
	GString *	output;

	output = g_terminal_process_read_string_all(process);
	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_run_server_read: %s", output->str);
	if (comm_ssh_parse_output(process, comm_server, output) == TRUE)
		goto out;

	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		comm_server->port = port;
		comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_run_server_read: %d", port);
	} else {
		comm_server->error = SERVER_ERROR_SERVER;
		comm_server->state = SERVER_STATE_DISCONNECTED;
		comm_server_log_message(comm_server, LOG_ERROR, _("Could not comunicate with server %s: \n%s"),
			comm_server->address->str, output->str);
	}

out:	g_string_free(output, TRUE);
}

static void
comm_ssh_run_server_finished(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	cmd_line;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_run_server_finished");

	comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	g_terminal_process_free(process);

	if (comm_server->error != SERVER_ERROR_NONE)
		return;

	/* initializations */
	cmd_line = g_string_new(NULL);

	comm_server->tried_existant_pass = FALSE;
	comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	comm_server->process.data.terminal = process = g_terminal_process_new();
	g_signal_connect(process, "ready-read",
		G_CALLBACK(comm_ssh_read), comm_server);
	g_signal_connect(process, "finished",
		G_CALLBACK(comm_ssh_open_tunnel_finished), comm_server);

	comm_server->state = SERVER_STATE_OPEN_TUNNEL;
	comm_server->tunnel_port = 2125;
	while (!g_listen_socket_is_local_port_available(comm_server->tunnel_port))
		++comm_server->tunnel_port;

	g_string_printf(cmd_line, "ssh -x -f -L %d:127.0.0.1:%d %s 'sleep 300'",
		comm_server->tunnel_port, comm_server->port, comm_server->address->str);
	g_terminal_process_start(process, cmd_line);

	/* frees */
	g_string_free(cmd_line, TRUE);
}

static void
comm_ssh_open_tunnel_finished(GTerminalProcess * process, struct comm_server * comm_server)
{
	GSocketAddress *		socket_address;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_open_tunnel_finished");

	if (comm_server->error != SERVER_ERROR_NONE)
		goto out;

	comm_server->state = SERVER_STATE_CONNECT;
	socket_address = g_socket_address_new("127.0.0.1", G_SOCKET_ADDRESS_TYPE_IPV4);
	g_stream_socket_connect(comm_server->stream_socket, socket_address, comm_server->tunnel_port, FALSE);

	g_socket_address_free(socket_address);
out:	comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	g_terminal_process_free(process);
}

static void
comm_server_connected(GStreamSocket * stream_socket, struct comm_server * comm_server)
{
	gchar		hostname[256];
	gchar *		display;

	/* initialization */
	gethostname(hostname, 255);
	display = getenv("DISPLAY");
	if (display == NULL)
		display = "";

	if (comm_server_is_local(comm_server) == FALSE) {
		GString *	cmd_line;
		gchar		mcookie_str[33];

		/* initialization */
		cmd_line = g_string_new(NULL);

		if (strlen(display)) {
			FILE * output_fp;

			/* get this X session magic cookie */
			g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display);
			output_fp = popen(cmd_line->str, "r");
			fscanf(output_fp, "%32s", mcookie_str);

			pclose(output_fp);
		} else
			strcpy(mcookie_str, "");

		/* send INI */
		protocol_send_data(comm_server->protocol, comm_server->stream_socket,
			protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "remote", mcookie_str);

		/* frees */
		g_string_free(cmd_line, TRUE);
	} else {
		/* send INI */
		protocol_send_data(comm_server->protocol, comm_server->stream_socket,
			protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "local", display);
	}
}

static void
comm_server_disconnected(GStreamSocket * stream_socket, struct comm_server * comm_server)
{
	comm_server->port = 0;
	comm_server->protocol->logged = FALSE;
	comm_server->state = SERVER_STATE_DISCONNECTED;

	comm_server_log_message(comm_server, LOG_WARNING, "Server '%s' disconnected",
		     comm_server->address->str);

	comm_server_free_for_reuse(comm_server);
}

static void
comm_server_read(GStreamSocket * stream_socket, struct comm_server * comm_server)
{
	GString *	data;
	gchar *		data_stripped;

	data = g_socket_read_string_all(G_SOCKET(stream_socket));
	protocol_receive_data(comm_server->protocol, data);
	comm_server->ops->parse_messages(comm_server, comm_server->user_data);

	/* we don't want a giant output */
	data_stripped = g_strndup(data->str, 500);
	comm_server_log_message(comm_server, LOG_DEBUG, "Read from server '%s': %s",
		comm_server->address->str, data_stripped);

	g_string_free(data, TRUE);
	g_free(data_stripped);
}

static void
comm_server_error(GStreamSocket * stream_socket, enum GSocketError error, struct comm_server * comm_server)
{
	comm_server->error = SERVER_ERROR_CONNECT;
	comm_server->state = SERVER_STATE_DISCONNECTED;
	if (error == G_SOCKET_ERROR_UNKNOWN)
		puts("unk");
	comm_server_log_message(comm_server, LOG_ERROR, _("Connection error '%d' on server '%s'"), error, comm_server->address->str);
}

/*
 * Function: comm_server_free_x11_process
 * Free (if necessary) comm_server->x11_forward for reuse
 */
static void
comm_server_free_x11_process(struct comm_server * comm_server)
{
	if (comm_server->x11_forward != NULL) {
		g_terminal_process_kill(comm_server->x11_forward);
		g_terminal_process_free(comm_server->x11_forward);
		comm_server->x11_forward = NULL;
	}
}

static void
comm_server_free_for_reuse(struct comm_server * comm_server)
{
	comm_server_free_x11_process(comm_server);
	switch (comm_server->process.use) {
	case COMM_SERVER_PROCESS_NONE:
		break;
	case COMM_SERVER_PROCESS_TERMINAL:
		g_terminal_process_free(comm_server->process.data.terminal);
		break;
	case COMM_SERVER_PROCESS_REGULAR:
		g_process_free(comm_server->process.data.regular);
		break;
	}
	comm_server->process.use = COMM_SERVER_PROCESS_NONE;
}
