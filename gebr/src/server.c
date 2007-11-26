/*   GÍBR - An environment for seismic processing.
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

#include <gtk/gtk.h>

#include <misc/gtcpserver.h>
#include <misc/gprocess.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "client.h"
#include "gebr.h"
#include "cb_job.h"
#include "callbacks.h"

/*
 * Prototypes
 */

void
server_connected(GTcpSocket * tcp_socket, struct server * server);

void
server_disconnected(GTcpSocket * tcp_socket, struct server * server);

void
server_read(GTcpSocket * tcp_socket, struct server * server);

void
server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct server * server);

/*
 * Internal functions
 */

void
ssh_ask_server_port(struct server * server);

void
ssh_run_server_finished(GProcess * process, struct server * server)
{
	/* now ask via ssh its port */
	ssh_ask_server_port(server);
	g_print("ssh_run_server_finished\n");
// 	g_process_free(process);
}

void
ssh_read_stdout(GProcess * process, struct server * server)
{
	GString *	output;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (errno != ERANGE)
		server->port = port;
	g_print("ssh_read_stdout %d %d\n", port, server->port);
	g_string_free(output, TRUE);
}

void
ssh_read_stderr(GProcess * process, struct server * server)
{
	GtkWidget *	dialog;
	GString *	error;

	if (g_process_is_running(process) == TRUE)
		g_process_kill(process);

	error = g_process_read_stderr_string_all(process);
	dialog = gtk_message_dialog_new(GTK_WINDOW(W.mainwin),
		GTK_DIALOG_MODAL,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"Error contacting server host %s via ssh:\n%s",
		server->address->str, error->str);
	gtk_dialog_run(GTK_DIALOG(dialog));

	g_string_free(error, TRUE);
	gtk_widget_destroy(dialog);
}

void
//ssh_read_finished(GProcess * process, gint exit_code, enum GProcessExitStatus exit_status, struct server * server)
ssh_read_finished(GProcess * process, struct server * server)
{
	if (server->port) {
		/* now the port became the tunnel port */
		server->ssh_tunnel = ssh_tunnel_new(2125, server->address->str, server->port);

		GHostAddress *	host;
		host = g_host_address_new();
		g_host_address_set_ipv4_string(host, "127.0.0.1");
		g_tcp_socket_connect(server->tcp_socket, host, server->ssh_tunnel.port);
	} else {
		GString *	status;
		GString *	cmd_line;

		cmd_line = g_string_new(NULL);
		status = g_string_new(NULL);
		g_string_printf(status, "Running server at %s...", server->address->str);
		log_message(ACTION, status->str, TRUE);
		g_string_free(status, TRUE);

		/* run gebrd via ssh for remote hosts */
		{
		   gchar hostname[100];

		   gethostname(hostname, 100);
		   if (!g_ascii_strcasecmp(hostname, server->address->str)) 
		      g_string_printf(cmd_line, "bash -c gebrd&");
		   else
		      g_string_printf(cmd_line, "bash -c \"ssh -f %s bash -c gebrd\"", server->address->str);

		   system(cmd_line->str);
		   ssh_ask_server_port(server);
		   
		   g_string_free(cmd_line, TRUE);
		}
	}

	g_process_free(process);
}

void
ssh_ask_server_port(struct server * server)
{
	if (++server->retries > 3)
		return;

	/* ask via ssh port from server */
	GProcess * process;
	GString * cmd_line;
	process = g_process_new();
	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "ssh %s \"test -e .gebr/run/gebrd.run && cat .gebr/run/gebrd.run\"",
		server->address->str);
g_print("ssh_ask_server_port\n");
	g_signal_connect(process, "ready-read-stdout",
			G_CALLBACK(ssh_read_stdout), server);
	g_signal_connect(process, "ready-read-stderr",
			G_CALLBACK(ssh_read_stderr), server);
	g_signal_connect(process, "finished",
			G_CALLBACK(ssh_read_finished), server);
	g_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

/*
 * Public functions
 */

struct server *
server_new(const gchar * address)
{
	struct server *	server;

	server = g_malloc(sizeof(struct server));
	*server = (struct server) {
		.tcp_socket = g_tcp_socket_new(),
		.protocol = protocol_new(),
		.address = g_string_new(address),
		.port = 0,
		.retries = 0,
	};
g_print("server_new\n");

	g_signal_connect(server->tcp_socket, "connected",
			G_CALLBACK(server_connected), server);
	g_signal_connect(server->tcp_socket, "disconnected",
			G_CALLBACK(server_disconnected), server);
	g_signal_connect(server->tcp_socket, "ready-read",
			G_CALLBACK(server_read), server);
	g_signal_connect(server->tcp_socket, "error",
			G_CALLBACK(server_error), server);

	ssh_ask_server_port(server);

	return server;
}

void
server_free(struct server * server)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* delete all jobs at server */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.job_store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get (GTK_TREE_MODEL(W.job_store), &iter,
				JC_STRUCT, &job,
				-1);
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(W.job_store), &iter);

		if (job->server == server)
			job_delete(job);
	}

	if (server->port)
		ssh_tunnel_free(server->ssh_tunnel);
	g_string_free(server->address, TRUE);
	g_socket_close(G_SOCKET(server->tcp_socket));
	protocol_free(server->protocol);
	g_free(server);
}

void
server_connected(GTcpSocket * tcp_socket, struct server * server)
{
	g_print("server_connected\n");

	gchar		hostname[100];
	gchar *		display;
	GString *	mcookie_cmd;
	FILE *		output_fp;
	gchar		line[1024];
	gchar **	splits;

	/* initialization */
	mcookie_cmd = g_string_new(NULL);

	/* hostname and display */
	gethostname(hostname, 100);
	display = getenv("DISPLAY");

	/* TODO: port to GProcess using blocking calls */ 
	/* get this X session magic cookie */
	g_string_printf(mcookie_cmd, "xauth list %s", display);
	output_fp = popen(mcookie_cmd->str, "r");
	fread(line, 1, 1024, output_fp); 
	/* split output and get only the magic cookie */
	splits = g_strsplit_set(line, " \n", 6);
	pclose(output_fp);

	protocol_send_data(server->protocol, server->tcp_socket,
		protocol_defs.ini_def, 3, hostname, display, splits[4]);

	g_string_free(mcookie_cmd, TRUE);
	g_strfreev(splits);
}

void
server_disconnected(GTcpSocket * tcp_socket, struct server * server)
{
	g_print("server_disconnected\n");
	server->port = 0;
}

void
server_read(GTcpSocket * tcp_socket, struct server * server)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));g_print("server_read %s\n", data->str);
	protocol_receive_data(server->protocol, data);
	client_parse_server_messages(server);

	g_string_free(data, TRUE);
}

void
server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct server * server)
{
	g_print("server_error = %d\n", error);
	server_free(server);
}

void
server_run_flow(struct server * server)
{
	GeoXmlFlow *		flow_wnh; /* wnh: with no help */
	GeoXmlProgram *		program;
	GeoXmlSequence *	sequence;
	gchar *			xml;

	/* TODO: check logged instead of connected */
	if (g_socket_get_state(G_SOCKET(server->tcp_socket)) != G_SOCKET_STATE_CONNECTED) {
	   GtkWidget *	dialog;
	   
	   dialog = gtk_message_dialog_new(GTK_WINDOW(W.mainwin),
					   GTK_DIALOG_MODAL,
					   GTK_MESSAGE_ERROR,
					   GTK_BUTTONS_CLOSE,
					   "You are not connected to this server");
	   gtk_widget_show_all(dialog);
	   gtk_dialog_run(GTK_DIALOG(dialog));
	   
	   gtk_widget_destroy(dialog);
	   return;
	}

	/* removes flow's help */
	flow_wnh = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOC(flow)));
	geoxml_document_set_help(GEOXML_DOC(flow_wnh), "");
	/* removes programs' help */
	geoxml_flow_get_program(flow_wnh, &program, 0);
	sequence = GEOXML_SEQUENCE(program);
	while (sequence != NULL) {
		program = GEOXML_PROGRAM(sequence);
		geoxml_program_set_help(program, "");
		geoxml_sequence_next(&sequence);
	}

	/* get the xml */
	geoxml_document_to_string(GEOXML_DOC(flow_wnh), &xml);

	/* finally... */
	protocol_send_data(server->protocol, server->tcp_socket, protocol_defs.run_def, 1, xml);

	g_free(xml);
	geoxml_document_free(GEOXML_DOC(flow_wnh));
}

void
server_list_flows(struct server * server)
{
}
