/*   libgebr - GÍBR Library
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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "gtcpserver.h"
#include "gprocess.h"
#include "gterminalprocess.h"
#include "support.h"

/*
 * Internal functions
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
local_ask_port_read(GProcess * process, struct comm_server * comm_server);

static void
local_ask_port_finished(GProcess * process, struct comm_server * comm_server);

static void
local_run_comm_server_finished(GProcess * process, struct comm_server * comm_server)
{
	GString *	cmd_line;

	comm_server_log_message(comm_server, LOG_DEBUG, "local_run_comm_server_finished");

	cmd_line = g_string_new(NULL);
	comm_server->state = SERVER_STATE_ASK_PORT;
	comm_server->tried_existant_pass = FALSE;

	g_process_free(process);
	process = g_process_new();
	g_signal_connect(process, "ready-read-stdout",
		G_CALLBACK(local_ask_port_read), comm_server);
	g_signal_connect(process, "finished",
		G_CALLBACK(local_ask_port_finished), comm_server);

	g_string_printf(cmd_line, "bash -l -c 'test -e ~/.gebr/run/gebrd.run && cat ~/.gebr/run/gebrd.run'");
	g_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

static void
local_ask_port_read(GProcess * process, struct comm_server * comm_server)
{
	GString *	output;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (errno != ERANGE) {
		comm_server->port = port;
		comm_server_log_message(comm_server, LOG_DEBUG, "local_ask_port_read: %d", port, comm_server->port);
	}

	g_string_free(output, TRUE);
}

static void
local_ask_port_finished(GProcess * process, struct comm_server * comm_server)
{
	comm_server_log_message(comm_server, LOG_DEBUG, "local_ask_port_finished");

	if (comm_server->error != SERVER_ERROR_NONE)
		goto out;
	if (comm_server->port) {
		GHostAddress *	host_address;

		host_address = g_host_address_new();
		g_host_address_set_ipv4_string(host_address, "127.0.0.1");
		g_string_assign(comm_server->client_address, "127.0.0.1");

		comm_server->state = SERVER_STATE_CONNECT;
		g_tcp_socket_connect(comm_server->tcp_socket, host_address, comm_server->port, FALSE);

		g_host_address_free(host_address);
	}

out:	g_process_free(process);
}

static gboolean
comm_ssh_parse_output(GTerminalProcess * process, struct comm_server * comm_server, GString * output)
{
	if (output->len <= 2)
		return FALSE;
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
			g_string_printf(string, _("Wrong password for comm_server %s, please try again.\n\n%s"),
				comm_server->address->str, output->str);

		password = comm_server->ops->ssh_login(_("SSH login:"), string->str);
		if (password == NULL) {
			comm_server->error = SERVER_ERROR_SSH;
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
			g_string_assign(answer, "no");
		} else
			g_string_assign(answer, "yes");
		g_terminal_process_write_string(process, answer);

		g_string_free(answer, TRUE);
	} else {
		/* check if it is an ssh error: check if string starts with "ssh:" */
		gchar *	str;

		str = strstr(output->str, "ssh:");
		if (str == output->str) {
			comm_server->error = SERVER_ERROR_SSH;
			comm_server_log_message(comm_server, LOG_ERROR, _("Error contacting comm_server at address %s via ssh: %s"),
				comm_server->address->str, output->str);
			return TRUE;
		}

		return FALSE;
	}

out:	return TRUE;
}

static void
comm_ssh_ask_port_read(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	output;
	gchar **	splits;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_terminal_process_read_string_all(process);
	if (comm_ssh_parse_output(process, comm_server, output) == TRUE)
		goto out;
	if (output->len <= 2) {
		/* comm_server is not running, because run file test lead to empty output
		 * (2 is greater than carriege return)
		 */
		comm_server->port = 0;
		goto out;
	}

	splits = g_strsplit(output->str, " ", 4);
	port = strtol(splits[0], &strtol_endptr, 10);
	if (errno != ERANGE) {
		comm_server->port = port;
		g_string_assign(comm_server->client_address, splits[1]);
		comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_ask_port_read: %d", port, comm_server->port);
	} else {
		comm_server->port = 0;
		comm_server->error = SERVER_ERROR_SSH;
	}

	g_strfreev(splits);
out:	g_string_free(output, TRUE);
}

static void
comm_ssh_open_tunnel_read(GTerminalProcess * process, struct comm_server * comm_server);

static void
comm_ssh_open_tunnel_finished(GTerminalProcess * process, struct comm_server * comm_server);

static void
comm_ssh_ask_port_finished(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	cmd_line;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_ask_port_finished");
	cmd_line = g_string_new(NULL);

	if (comm_server->error != SERVER_ERROR_NONE)
		goto out;

	comm_server->tried_existant_pass = FALSE;
	if (comm_server->port) {
		GTerminalProcess *	tunnel_process;

		tunnel_process = g_terminal_process_new();
		g_signal_connect(tunnel_process, "ready-read",
			G_CALLBACK(comm_ssh_open_tunnel_read), comm_server);
		g_signal_connect(tunnel_process, "finished",
			G_CALLBACK(comm_ssh_open_tunnel_finished), comm_server);

		comm_server->state = SERVER_STATE_OPEN_TUNNEL;
		comm_server->tunnel_port = 2125;
		while (!g_tcp_server_is_local_port_available(comm_server->tunnel_port))
			++comm_server->tunnel_port;

		g_string_printf(cmd_line, "ssh -f -L %d:127.0.0.1:%d %s 'sleep 300'",
			comm_server->tunnel_port, comm_server->port, comm_server->address->str);
		g_terminal_process_start(tunnel_process, cmd_line);
	}

out:	g_string_free(cmd_line, TRUE);
	g_terminal_process_free(process);
}

static void
comm_ssh_run_comm_server_read(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	output;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_run_comm_server_read");

	/* ssh only output; gebrd doesn't output nothing */
	output = g_terminal_process_read_string_all(process);
	comm_ssh_parse_output(process, comm_server, output);

	g_string_free(output, TRUE);
}

static void
comm_ssh_run_comm_server_finished(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	cmd_line;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_run_comm_server_finished");

	g_terminal_process_free(process);
	if (comm_server->error != SERVER_ERROR_NONE)
		return;

	cmd_line = g_string_new(NULL);
	comm_server->state = SERVER_STATE_ASK_PORT;
	comm_server->tried_existant_pass = FALSE;

	process = g_terminal_process_new();
	g_signal_connect(process, "ready-read",
		G_CALLBACK(comm_ssh_ask_port_read), comm_server);
	g_signal_connect(process, "finished",
		G_CALLBACK(comm_ssh_ask_port_finished), comm_server);

	g_string_printf(cmd_line,
		"ssh %s \"LOCK=~/.gebr/run/gebrd-`echo $SSH_CLIENT | awk '{print $1}'`.run;"
		"test -e $LOCK && echo `cat $LOCK` $SSH_CLIENT\"", comm_server->address->str);
	g_terminal_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

static void
comm_ssh_open_tunnel_read(GTerminalProcess * process, struct comm_server * comm_server)
{
	GString *	output;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_open_tunnel_read");

	/* in fact there is no output because ssh was ran with -f option, only ssh login stuff */
	output = g_terminal_process_read_string_all(process);
	comm_ssh_parse_output(process, comm_server, output);

	g_string_free(output, TRUE);
}

static void
comm_ssh_open_tunnel_finished(GTerminalProcess * process, struct comm_server * comm_server)
{
	GHostAddress *		host_address;

	comm_server_log_message(comm_server, LOG_DEBUG, "comm_ssh_open_tunnel_finished");

	if (comm_server->error != SERVER_ERROR_NONE)
		goto out;

	/* connection is made to a local tunnel */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	comm_server->state = SERVER_STATE_CONNECT;
	g_tcp_socket_connect(comm_server->tcp_socket, host_address, comm_server->tunnel_port, FALSE);

	g_host_address_free(host_address);
out:	g_terminal_process_free(process);
}

static void
comm_server_connected(GTcpSocket * tcp_socket, struct comm_server * comm_server)
{
	gchar		hostname[100];
	gchar *		display;

	GString *	cmd_line;
	FILE *		output_fp;
	gchar		line[1024];

	GString *	mcookie;
	gchar **	splits;

	/* initialization */
	mcookie = g_string_new(NULL);
	cmd_line = g_string_new(NULL);

	/* hostname and display */
	gethostname(hostname, 100);
	display = getenv("DISPLAY");

	/* get this X session magic cookie */
	g_string_printf(cmd_line, "xauth list %s | awk '{print $3}'", display);
	output_fp = popen(cmd_line->str, "r");
	fread(line, 1, 1024, output_fp);
	/* split output and get only the magic cookie */
	splits = g_strsplit_set(line, " \n", 1);
	g_string_assign(mcookie, splits[0]);

	/* send INI */
	protocol_send_data(comm_server->protocol, comm_server->tcp_socket,
		protocol_defs.ini_def, 5, PROTOCOL_VERSION, hostname, comm_server->client_address->str, display, mcookie->str);

	/* frees */
	g_strfreev(splits);
	pclose(output_fp);
	g_string_free(mcookie, TRUE);
	g_string_free(cmd_line, TRUE);
}

static void
comm_server_disconnected(GTcpSocket * tcp_socket, struct comm_server * comm_server)
{
	comm_server->port = 0;
	comm_server->protocol->logged = FALSE;

	comm_server_log_message(comm_server, LOG_WARNING, "Server '%s' disconnected",
		     comm_server->address->str);
}

static void
comm_server_read(GTcpSocket * tcp_socket, struct comm_server * comm_server)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));
	protocol_receive_data(comm_server->protocol, data);
	comm_server->ops->parse_messages(comm_server, comm_server->user_data);

	comm_server_log_message(comm_server, LOG_DEBUG, "Read from comm_server '%s': %s",
		comm_server->address->str, data->str);

	g_string_free(data, TRUE);
}

static void
comm_server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct comm_server * comm_server)
{
	comm_server->error = SERVER_ERROR_CONNECT;
	if (error == G_SOCKET_ERROR_UNKNOWN)
		puts("unk");
	comm_server_log_message(comm_server, LOG_ERROR, _("Connection error '%s' on comm_server '%s'"), error, comm_server->address->str);
}


/*
 * Public functions
 */

struct comm_server *
comm_server_new(const gchar * _address, const struct comm_server_ops * ops)
{
	struct comm_server *	comm_server;

	/* initialize */
	comm_server = g_malloc(sizeof(struct comm_server));
	*comm_server = (struct comm_server) {
		.tcp_socket = g_tcp_socket_new(),
		.protocol = protocol_new(),
		.address = g_string_new(_address),
		.port = 0,
		.client_address = g_string_new(""),
		.ops = ops,
		.password = g_string_new(""),
	};

	g_signal_connect(comm_server->tcp_socket, "connected",
		G_CALLBACK(comm_server_connected), comm_server);
	g_signal_connect(comm_server->tcp_socket, "disconnected",
		G_CALLBACK(comm_server_disconnected), comm_server);
	g_signal_connect(comm_server->tcp_socket, "ready-read",
		G_CALLBACK(comm_server_read), comm_server);
	g_signal_connect(comm_server->tcp_socket, "error",
		G_CALLBACK(comm_server_error), comm_server);

	return comm_server;
}

void
comm_server_free(struct comm_server * comm_server)
{
	g_socket_close(G_SOCKET(comm_server->tcp_socket));
	protocol_free(comm_server->protocol);
	g_string_free(comm_server->address, TRUE);
	g_string_free(comm_server->password, TRUE);
	g_string_free(comm_server->client_address, TRUE);
	g_free(comm_server);
}

void
comm_server_connect(struct comm_server * comm_server)
{
	GString *	cmd_line;

	/* initiate the marathon to communicate to comm_server */
	comm_server->error = SERVER_ERROR_NONE;
	comm_server->state = SERVER_STATE_RUN;
	comm_server->tried_existant_pass = FALSE;

	cmd_line = g_string_new(NULL);

	if (comm_server_is_local(comm_server) == FALSE) {
		GTerminalProcess *	process;

		comm_server->state = SERVER_STATE_RUN;
		comm_server_log_message(comm_server, LOG_INFO, _("Launching server at %s"), comm_server->address->str);

		process = g_terminal_process_new();
		g_signal_connect(process, "ready-read",
			G_CALLBACK(comm_ssh_run_comm_server_read), comm_server);
		g_signal_connect(process, "finished",
			G_CALLBACK(comm_ssh_run_comm_server_finished), comm_server);

		g_string_printf(cmd_line, "ssh -x %s 'bash -l -c gebrd'", comm_server->address->str);
		g_terminal_process_start(process, cmd_line);
	} else {
		GProcess *	process;

		comm_server->state = SERVER_STATE_RUN;
		comm_server_log_message(comm_server, LOG_INFO, _("Launching local server"), comm_server->address->str);

// 		process = g_process_new();
// 		g_signal_connect(process, "finished",
// 			G_CALLBACK(local_run_comm_server_finished), comm_server);

#if (!LIBGEBR_STATIC_MODE)
// 		g_string_printf(cmd_line, "bash -l -c 'gebrd'");
#else
// 		g_string_printf(cmd_line, "bash -l -c './gebrd'");
#endif
// 		g_process_start(process, cmd_line);
		system("bash -l -c 'gebrd'");

		comm_server_log_message(comm_server, LOG_DEBUG, "local_run_comm_server_finished");

		comm_server->state = SERVER_STATE_ASK_PORT;
		comm_server->tried_existant_pass = FALSE;

//		g_process_free(process);
		process = g_process_new();
		g_signal_connect(process, "ready-read-stdout",
				 G_CALLBACK(local_ask_port_read), comm_server);
		g_signal_connect(process, "finished",
				 G_CALLBACK(local_ask_port_finished), comm_server);

		g_string_printf(cmd_line, "bash -l -c 'test -e ~/.gebr/run/gebrd.run && cat ~/.gebr/run/gebrd.run'");
		g_process_start(process, cmd_line);
	}

	g_string_free(cmd_line, TRUE);
}

gboolean
comm_server_is_logged(struct comm_server * comm_server)
{
	return comm_server->protocol->logged;
}

gboolean
comm_server_is_local(struct comm_server * comm_server)
{
	return g_ascii_strcasecmp(comm_server->address->str, "127.0.0.1") == 0
		? TRUE : FALSE;
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
	protocol_send_data(comm_server->protocol, comm_server->tcp_socket, protocol_defs.run_def, 1, xml);

	/* frees */
	g_free(xml);
	geoxml_document_free(GEOXML_DOC(flow_wnh));
}
