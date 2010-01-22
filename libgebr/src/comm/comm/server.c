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

#include "server.h"
#include "listensocket.h"

/*
 * Declarations
 */

static void
gebr_comm_server_log_message(struct gebr_comm_server *gebr_comm_server, enum gebr_log_message_type type,
			     const gchar * message, ...);

static void local_run_server_read(GebrCommProcess * process, struct gebr_comm_server *gebr_comm_server);
static void local_run_server_finished(GebrCommProcess * process, struct gebr_comm_server *gebr_comm_server);

static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server,
			   GString * output);
static void gebr_comm_ssh_read(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_ssh_run_server_read(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server);
static void
gebr_comm_ssh_run_server_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server);
static void
gebr_comm_ssh_open_tunnel_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server);

static void gebr_comm_server_connected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_server_disconnected_state(struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_server_socket_disconnected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_server_socket_read(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server);
static void
gebr_comm_server_error(GebrCommStreamSocket * stream_socket, enum GebrCommSocketError error,
		       struct gebr_comm_server *gebr_comm_server);

static void gebr_comm_server_free_x11_forward(struct gebr_comm_server *gebr_comm_server);
static void gebr_comm_server_free_for_reuse(struct gebr_comm_server *gebr_comm_server);

/*
 * Section: Public
 */

struct gebr_comm_server *gebr_comm_server_new(const gchar * _address, const struct gebr_comm_server_ops *ops)
{
	struct gebr_comm_server *gebr_comm_server;

	/* initialize */
	gebr_comm_server = g_malloc(sizeof(struct gebr_comm_server));
	*gebr_comm_server = (struct gebr_comm_server) {
	.stream_socket = gebr_comm_stream_socket_new(),.protocol = gebr_comm_protocol_new(),.address =
		    g_string_new(_address),.port = 0,.password = g_string_new(""),.x11_forward_process =
		    NULL,.x11_forward_channel = NULL,.state = SERVER_STATE_DISCONNECTED,.error =
		    SERVER_ERROR_NONE,.ops = ops,};

	g_signal_connect(gebr_comm_server->stream_socket, "connected",
			 G_CALLBACK(gebr_comm_server_connected), gebr_comm_server);
	g_signal_connect(gebr_comm_server->stream_socket, "disconnected",
			 G_CALLBACK(gebr_comm_server_socket_disconnected), gebr_comm_server);
	g_signal_connect(gebr_comm_server->stream_socket, "ready-read",
			 G_CALLBACK(gebr_comm_server_socket_read), gebr_comm_server);
	g_signal_connect(gebr_comm_server->stream_socket, "error",
			 G_CALLBACK(gebr_comm_server_error), gebr_comm_server);

	gebr_comm_server_disconnected_state(gebr_comm_server);

	return gebr_comm_server;
}

void gebr_comm_server_free(struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_socket_close(GEBR_COMM_SOCKET(gebr_comm_server->stream_socket));
	gebr_comm_server_free_for_reuse(gebr_comm_server);
	gebr_comm_protocol_free(gebr_comm_server->protocol);
	g_string_free(gebr_comm_server->address, TRUE);
	g_string_free(gebr_comm_server->password, TRUE);
	g_free(gebr_comm_server);
}

void gebr_comm_server_connect(struct gebr_comm_server *gebr_comm_server)
{
	GString *cmd_line;

	gebr_comm_server_free_for_reuse(gebr_comm_server);
	gebr_comm_server->error = SERVER_ERROR_NONE;
	gebr_comm_server->state = SERVER_STATE_RUN;
	gebr_comm_server->tried_existant_pass = FALSE;
	cmd_line = g_string_new(NULL);

	/* initiate the marathon to communicate to server */
	if (gebr_comm_server_is_local(gebr_comm_server) == FALSE) {
		GebrCommTerminalProcess *process;

		gebr_comm_server->state = SERVER_STATE_RUN;
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_INFO, _("Launching server at %s"),
					     gebr_comm_server->address->str);

		gebr_comm_server->process.use = COMM_SERVER_PROCESS_TERMINAL;
		gebr_comm_server->process.data.terminal = process = gebr_comm_terminal_process_new();
		g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_run_server_read), gebr_comm_server);
		g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_run_server_finished), gebr_comm_server);

		g_string_printf(cmd_line, "ssh -x %s \"bash -l -c 'gebrd >&3' 3>&1 >/dev/null\"",
				gebr_comm_server->address->str);
		gebr_comm_terminal_process_start(process, cmd_line);
	} else {
		GebrCommProcess *process;

		gebr_comm_server->state = SERVER_STATE_RUN;
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_INFO, _("Launching local server"));

		gebr_comm_server->process.use = COMM_SERVER_PROCESS_REGULAR;
		gebr_comm_server->process.data.regular = process = gebr_comm_process_new();
		g_signal_connect(process, "ready-read-stdout", G_CALLBACK(local_run_server_read), gebr_comm_server);
		g_signal_connect(process, "finished", G_CALLBACK(local_run_server_finished), gebr_comm_server);

#if (!STATIC_MODE)
		g_string_printf(cmd_line, "bash -c \"bash -l -c 'gebrd >&3' 3>&1 >/dev/null\"");
#else
		g_string_printf(cmd_line, "bash -c \"bash -l -c './gebrd >&3' 3>&1 >/dev/null\"");
#endif
		gebr_comm_process_start(process, cmd_line);
	}

	g_string_free(cmd_line, TRUE);
}

void gebr_comm_server_disconnect(struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_server_disconnected_state(gebr_comm_server);
	gebr_comm_stream_socket_disconnect(gebr_comm_server->stream_socket);
}

gboolean gebr_comm_server_is_logged(struct gebr_comm_server *gebr_comm_server)
{
	return gebr_comm_server->protocol->logged;
}

gboolean gebr_comm_server_is_local(struct gebr_comm_server * gebr_comm_server)
{
	return strcmp(gebr_comm_server->address->str, "127.0.0.1") == 0 ? TRUE : FALSE;
}

void gebr_comm_server_kill(struct gebr_comm_server *gebr_comm_server)
{
	GString *cmd_line;
	GebrCommTerminalProcess *process;

	cmd_line = g_string_new(NULL);
	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_INFO, _("Stoping server at %s"),
				     gebr_comm_server->address->str);

	gebr_comm_server->tried_existant_pass = FALSE;
	process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), gebr_comm_server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_finished), gebr_comm_server);

	if (gebr_comm_server_is_local(gebr_comm_server) == FALSE)
		g_string_printf(cmd_line, "ssh -x %s 'killall gebrd'", gebr_comm_server->address->str);
	else
		g_string_printf(cmd_line, "killall gebrd");

	gebr_comm_terminal_process_start(process, cmd_line);
	g_string_free(cmd_line, TRUE);
}

/*
 * Function: gebr_comm_server_forward_x11
 * For the logged _gebr_comm_server_ forward x11 server _port_ to user display
 * Fail if user's display is not set, returning FALSE.
 * If any other x11 redirect was previously made it is unmade
 */
gboolean gebr_comm_server_forward_x11(struct gebr_comm_server *gebr_comm_server, guint16 port)
{
	gchar *display;
	guint16 display_number;
	guint16 redirect_display_port;

	GebrCommSocketAddress listen_address;
	GebrCommSocketAddress forward_address;

	gboolean ret;
	GString *string;

	/* initialization */
	string = g_string_new(NULL);

	/* does we have a display? */
	display = getenv("DISPLAY");
	if (!(ret = !(display == NULL || !strlen(display))))
		goto out;
	sscanf(display, ":%hu.", &display_number);

	/* set redirection port */
	redirect_display_port = (display_number + 6000 > 6010) ? display_number + 6000 : 6010;
	while (!gebr_comm_listen_socket_is_local_port_available(redirect_display_port))
		++redirect_display_port;

	/* free previous forward */
	gebr_comm_server_free_x11_forward(gebr_comm_server);

	/* redirect_display_port tcp port to X11 unix socket */
	listen_address = gebr_comm_socket_address_ipv4_local(redirect_display_port);
	g_string_printf(string, "/tmp/.X11-unix/X%hu", display_number);
	forward_address = gebr_comm_socket_address_unix(string->str);
	gebr_comm_server->x11_forward_channel = gebr_comm_channel_socket_new();
	if (!
	    (ret =
	     gebr_comm_channel_socket_start(gebr_comm_server->x11_forward_channel, &listen_address, &forward_address)))
		goto out;

	/* now ssh from server to redirect_display_port */
	gebr_comm_server->tried_existant_pass = FALSE;
	gebr_comm_server->x11_forward_process = gebr_comm_terminal_process_new();
	g_signal_connect(gebr_comm_server->x11_forward_process, "ready-read",
			 G_CALLBACK(gebr_comm_ssh_read), gebr_comm_server);
	g_string_printf(string, "ssh -x -R %d:127.0.0.1:%d %s 'sleep 999d'",
			port, redirect_display_port, gebr_comm_server->address->str);
	gebr_comm_terminal_process_start(gebr_comm_server->x11_forward_process, string);

	/* log */
	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_INFO, _("Redirecting '%s' graphical output"),
				     gebr_comm_server->address->str);

	/* frees */
 out:	g_string_free(string, TRUE);

	return ret;
}

/*
 * Function: gebr_comm_server_run_flow
 * Ask _gebr_comm_server_ to run the current _flow_
 *
 */
void gebr_comm_server_run_flow(struct gebr_comm_server *gebr_comm_server, struct gebr_comm_server_run * config)
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

	gebr_comm_protocol_send_data(gebr_comm_server->protocol, gebr_comm_server->stream_socket,
				     gebr_comm_protocol_defs.run_def, 3, xml,
				     config->account ? config->account : "", config->class ? config->class : "");
	/* frees */
	g_free(xml);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(flow_wnh));
}


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

static void
gebr_comm_server_log_message(struct gebr_comm_server *gebr_comm_server, enum gebr_log_message_type type,
			     const gchar * message, ...)
{
	gchar *string;
	va_list argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	gebr_comm_server->ops->log_message(type, string);

	g_free(string);
}

static void local_run_server_read(GebrCommProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	GString *output;
	gchar *strtol_endptr;
	guint16 port;

	output = gebr_comm_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		gebr_comm_server->port = port;
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "local_run_server_read: %d", port);
	} else {
		gebr_comm_server->error = SERVER_ERROR_SERVER;
		gebr_comm_server_disconnected_state(gebr_comm_server);
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_ERROR, _("Could not launch local server: \n%s"),
					     output->str);
	}

	g_string_free(output, TRUE);
}

static void local_run_server_finished(GebrCommProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	GebrCommSocketAddress socket_address;

	gebr_comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_process_free(process);

	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "local_run_server_finished");
	if (gebr_comm_server->error != SERVER_ERROR_NONE)
		return;

	gebr_comm_server->state = SERVER_STATE_CONNECT;
	socket_address = gebr_comm_socket_address_ipv4_local(gebr_comm_server->port);
	gebr_comm_stream_socket_connect(gebr_comm_server->stream_socket, &socket_address, FALSE);
}

static gboolean
gebr_comm_ssh_parse_output(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server,
			   GString * output)
{
	if (output->len <= 2)
		return TRUE;
	if (output->str[output->len - 2] == ':') {
		GString *string;
		GString *password;

		if (gebr_comm_server->password->len && gebr_comm_server->tried_existant_pass == FALSE) {
			gebr_comm_terminal_process_write_string(process, gebr_comm_server->password);
			gebr_comm_server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		if (gebr_comm_server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Server %s needs SSH login.\n\n%s"),
					gebr_comm_server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for server %s, please try again.\n\n%s"),
					gebr_comm_server->address->str, output->str);

		password = gebr_comm_server->ops->ssh_login(_("SSH login:"), string->str);
		if (password == NULL) {
			gebr_comm_server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(gebr_comm_server);
			g_string_assign(gebr_comm_server->password, "");
			gebr_comm_terminal_process_kill(process);
		} else {
			g_string_printf(gebr_comm_server->password, "%s\n", password->str);
			gebr_comm_terminal_process_write_string(process, gebr_comm_server->password);
			g_string_free(password, TRUE);
		}
		gebr_comm_server->tried_existant_pass = FALSE;

		g_string_free(string, TRUE);
	} else if (output->str[output->len - 2] == '?') {
		GString *answer;

		answer = g_string_new(NULL);
		if (gebr_comm_server->ops->ssh_question(_("SSH question:"), output->str) == FALSE) {
			gebr_comm_server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(gebr_comm_server);
			g_string_assign(answer, "no\n");
		} else
			g_string_assign(answer, "yes\n");
		gebr_comm_terminal_process_write_string(process, answer);

		g_string_free(answer, TRUE);
	} else if (output->str[output->len - 4] == '.') {
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_WARNING, _("Received SSH message: \n%s"),
					     output->str);
	} else if (!strcmp(output->str, "yes\r\n")) {
		goto out;
	} else {
		/* check if it is an ssh error: check if string starts with "ssh:" */
		gchar *str;

		str = strstr(output->str, "ssh:");
		if (str == output->str) {
			gebr_comm_server->error = SERVER_ERROR_SSH;
			gebr_comm_server_disconnected_state(gebr_comm_server);
			gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_ERROR,
						     _("Error contacting server at address %s via ssh: \n%s"),
						     gebr_comm_server->address->str, output->str);
			return TRUE;
		}

		return FALSE;
	}

 out:	return TRUE;
}

/*
 * Function: gebr_comm_ssh_read
 * Simple ssh messages parser, like login questions and warnings
 */
static void gebr_comm_ssh_read(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	GString *output;

	output = gebr_comm_terminal_process_read_string_all(process);
	gebr_comm_ssh_parse_output(process, gebr_comm_server, output);

	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "gebr_comm_ssh_read: %s", output->str);

	g_string_free(output, TRUE);
}

static void gebr_comm_ssh_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_terminal_process_free(process);
}

static void gebr_comm_ssh_run_server_read(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	gchar *strtol_endptr;
	guint16 port;
	GString *output;

	output = gebr_comm_terminal_process_read_string_all(process);
	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_read: %s",
				     output->str);
	if (gebr_comm_ssh_parse_output(process, gebr_comm_server, output) == TRUE)
		goto out;

	port = strtol(output->str, &strtol_endptr, 10);
	if (port) {
		gebr_comm_server->port = port;
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_read: %d",
					     port);
	} else {
		gebr_comm_server->error = SERVER_ERROR_SERVER;
		gebr_comm_server_disconnected_state(gebr_comm_server);
		gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_ERROR,
					     _("Could not comunicate with server %s: \n%s"),
					     gebr_comm_server->address->str, output->str);
	}

 out:	g_string_free(output, TRUE);
}

static void
gebr_comm_ssh_run_server_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	GString *cmd_line;

	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "gebr_comm_ssh_run_server_finished");

	gebr_comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_terminal_process_free(process);

	if (gebr_comm_server->error != SERVER_ERROR_NONE)
		return;

	/* initializations */
	cmd_line = g_string_new(NULL);

	gebr_comm_server->tried_existant_pass = FALSE;
	gebr_comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_server->process.data.terminal = process = gebr_comm_terminal_process_new();
	g_signal_connect(process, "ready-read", G_CALLBACK(gebr_comm_ssh_read), gebr_comm_server);
	g_signal_connect(process, "finished", G_CALLBACK(gebr_comm_ssh_open_tunnel_finished), gebr_comm_server);

	gebr_comm_server->state = SERVER_STATE_OPEN_TUNNEL;
	gebr_comm_server->tunnel_port = 2125;
	while (!gebr_comm_listen_socket_is_local_port_available(gebr_comm_server->tunnel_port))
		++gebr_comm_server->tunnel_port;

	g_string_printf(cmd_line, "ssh -x -f -L %d:127.0.0.1:%d %s 'sleep 300'",
			gebr_comm_server->tunnel_port, gebr_comm_server->port, gebr_comm_server->address->str);
	gebr_comm_terminal_process_start(process, cmd_line);

	/* frees */
	g_string_free(cmd_line, TRUE);
}

static void
gebr_comm_ssh_open_tunnel_finished(GebrCommTerminalProcess * process, struct gebr_comm_server *gebr_comm_server)
{
	GebrCommSocketAddress socket_address;

	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "gebr_comm_ssh_open_tunnel_finished");

	if (gebr_comm_server->error != SERVER_ERROR_NONE)
		goto out;

	gebr_comm_server->state = SERVER_STATE_CONNECT;
	socket_address = gebr_comm_socket_address_ipv4_local(gebr_comm_server->tunnel_port);
	gebr_comm_stream_socket_connect(gebr_comm_server->stream_socket, &socket_address, FALSE);

 out:	gebr_comm_server->process.use = COMM_SERVER_PROCESS_NONE;
	gebr_comm_terminal_process_free(process);
}

static void gebr_comm_server_connected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server)
{
	gchar hostname[256];
	gchar *display;

	/* initialization */
	gethostname(hostname, 255);
	display = getenv("DISPLAY");
	if (display == NULL)
		display = "";

	if (gebr_comm_server_is_local(gebr_comm_server) == FALSE) {
		GString *cmd_line;
		gchar mcookie_str[33];

		/* initialization */
		cmd_line = g_string_new(NULL);

		if (strlen(display)) {
			FILE *output_fp;

			/* get this X session magic cookie */
			g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display);
			output_fp = popen(cmd_line->str, "r");
			fscanf(output_fp, "%32s", mcookie_str);

			pclose(output_fp);
		} else
			strcpy(mcookie_str, "");

		/* send INI */
		gebr_comm_protocol_send_data(gebr_comm_server->protocol, gebr_comm_server->stream_socket,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "remote",
					     mcookie_str);

		/* frees */
		g_string_free(cmd_line, TRUE);
	} else {
		/* send INI */
		gebr_comm_protocol_send_data(gebr_comm_server->protocol, gebr_comm_server->stream_socket,
					     gebr_comm_protocol_defs.ini_def, 4, PROTOCOL_VERSION, hostname, "local",
					     display);
	}
}

static void gebr_comm_server_disconnected_state(struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_server->port = 0;
	gebr_comm_server->protocol->logged = FALSE;
	gebr_comm_server->state = SERVER_STATE_DISCONNECTED;
	gebr_comm_server_free_for_reuse(gebr_comm_server);
}

static void gebr_comm_server_socket_disconnected(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_server_disconnected_state(gebr_comm_server);

	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_WARNING, "Server '%s' disconnected",
				     gebr_comm_server->address->str);
}

static void gebr_comm_server_socket_read(GebrCommStreamSocket * stream_socket, struct gebr_comm_server *gebr_comm_server)
{
	GString *data;
	gchar *data_stripped;

	data = gebr_comm_socket_read_string_all(GEBR_COMM_SOCKET(stream_socket));
	gebr_comm_protocol_receive_data(gebr_comm_server->protocol, data);
	gebr_comm_server->ops->parse_messages(gebr_comm_server, gebr_comm_server->user_data);

	/* we don't want a giant output */
	data_stripped = g_strndup(data->str, 50);
	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_DEBUG, "Read from server '%s': %s",
				     gebr_comm_server->address->str, data_stripped);

	g_string_free(data, TRUE);
	g_free(data_stripped);
}

static void
gebr_comm_server_error(GebrCommStreamSocket * stream_socket, enum GebrCommSocketError error,
		       struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_server->error = SERVER_ERROR_CONNECT;
	gebr_comm_server_disconnected_state(gebr_comm_server);
	if (error == G_SOCKET_ERROR_UNKNOWN)
		puts("FIXME: handle G_SOCKET_ERROR_UNKNOWN");
	gebr_comm_server_log_message(gebr_comm_server, GEBR_LOG_ERROR, _("Connection error '%d' on server '%s'"), error,
				     gebr_comm_server->address->str);
}

/*
 * Function: gebr_comm_server_free_x11_forward
 * Free (if necessary) gebr_comm_server->x11_forward_process for reuse
 */
static void gebr_comm_server_free_x11_forward(struct gebr_comm_server *gebr_comm_server)
{
	if (gebr_comm_server->x11_forward_process != NULL) {
		gebr_comm_terminal_process_kill(gebr_comm_server->x11_forward_process);
		gebr_comm_terminal_process_free(gebr_comm_server->x11_forward_process);
		gebr_comm_server->x11_forward_process = NULL;
	}
	if (gebr_comm_server->x11_forward_channel != NULL) {
		gebr_comm_socket_close(GEBR_COMM_SOCKET(gebr_comm_server->x11_forward_channel));
		gebr_comm_server->x11_forward_channel = NULL;
	}
}

static void gebr_comm_server_free_for_reuse(struct gebr_comm_server *gebr_comm_server)
{
	gebr_comm_protocol_reset(gebr_comm_server->protocol);
	gebr_comm_server_free_x11_forward(gebr_comm_server);
	switch (gebr_comm_server->process.use) {
	case COMM_SERVER_PROCESS_NONE:
		break;
	case COMM_SERVER_PROCESS_TERMINAL:
		gebr_comm_terminal_process_free(gebr_comm_server->process.data.terminal);
		break;
	case COMM_SERVER_PROCESS_REGULAR:
		gebr_comm_process_free(gebr_comm_server->process.data.regular);
		break;
	}
	gebr_comm_server->process.use = COMM_SERVER_PROCESS_NONE;
}
