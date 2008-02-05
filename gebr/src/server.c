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

#include <comm/gtcpserver.h>
#include <comm/gprocess.h>
#include <comm/gterminalprocess.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "gebr.h"
#include "defines.h"
#include "support.h"
#include "client.h"
#include "job.h"
#include "callbacks.h"

/*
 * Internal functions
 */

static void
local_ask_port_read(GProcess * process, struct server * server);

static void
local_ask_port_finished(GProcess * process, struct server * server);

static void
local_run_server_finished(GProcess * process, struct server * server)
{
	GString *	cmd_line;

	gebr_message(DEBUG, FALSE, TRUE, "local_run_server_finished");

	cmd_line = g_string_new(NULL);
	server->state = SERVER_STATE_ASK_PORT;
	server->tried_existant_pass = FALSE;

	g_process_free(process);
	process = g_process_new();
	g_signal_connect(process, "ready-read-stdout",
		G_CALLBACK(local_ask_port_read), server);
	g_signal_connect(process, "finished",
		G_CALLBACK(local_ask_port_finished), server);

	g_string_printf(cmd_line, "bash -l -c 'test -e ~/.gebr/run/gebrd.run && cat ~/.gebr/run/gebrd.run'");
	g_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

static void
local_ask_port_read(GProcess * process, struct server * server)
{
	GString *	output;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_process_read_stdout_string_all(process);
	port = strtol(output->str, &strtol_endptr, 10);
	if (errno != ERANGE) {
		server->port = port;
		gebr_message(DEBUG, FALSE, TRUE, "local_ask_port_read: %d", port, server->port);
	}

	g_string_free(output, TRUE);
}

static void
local_ask_port_finished(GProcess * process, struct server * server)
{
	gebr_message(DEBUG, FALSE, TRUE, "local_ask_port_finished");

	if (server->error != SERVER_ERROR_NONE)
		goto out;
	if (server->port) {
		GHostAddress *	host_address;

		host_address = g_host_address_new();
		g_host_address_set_ipv4_string(host_address, "127.0.0.1");
		g_string_assign(server->client_address, "127.0.0.1");

		server->state = SERVER_STATE_CONNECT;
		g_tcp_socket_connect(server->tcp_socket, host_address, server->port, FALSE);

		g_host_address_free(host_address);
	}

out:	g_process_free(process);
}

static gboolean
ssh_parse_output(GTerminalProcess * process, struct server * server, GString * output)
{
	if (output->len <= 2)
		return FALSE;
	if (output->str[output->len-2] == ':') {
		GtkWidget *	dialog;
		GtkWidget *	label;
		GtkWidget *	entry;

		GString *	string;

		if (server->password->len && server->tried_existant_pass == FALSE) {
			g_terminal_process_write_string(process, server->password);
			server->tried_existant_pass = TRUE;
			goto out;
		}

		string = g_string_new(NULL);
		dialog = gtk_dialog_new_with_buttons(_("SSH login:"),
						GTK_WINDOW(gebr.window),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);

		if (server->tried_existant_pass == FALSE)
			g_string_printf(string, _("Server %s needs SSH login.\n\n%s"),
				server->address->str, output->str);
		else
			g_string_printf(string, _("Wrong password for server %s, please try again.\n\n%s"),
				server->address->str, output->str);

		label = gtk_label_new(string->str);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

		entry = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
		gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

		gtk_widget_show_all(dialog);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
			server->error = SERVER_ERROR_SSH;
			g_terminal_process_kill(process);
			gtk_widget_destroy(dialog);
			goto out;
		}

		g_string_printf(server->password, "%s\n", gtk_entry_get_text(GTK_ENTRY(entry)));
		g_terminal_process_write_string(process, server->password);
		server->tried_existant_pass = FALSE;

		g_string_free(string, TRUE);
		gtk_widget_destroy(dialog);
	} else if (output->str[output->len-2] == '?') {
		GtkWidget *	dialog;
		GString *	answer;

		answer = g_string_new(NULL);
		dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
			GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			output->str);
		gtk_window_set_title(GTK_WINDOW(dialog), _("SSH question:"));

		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
			g_string_assign(answer, "yes\n");
		else {
			server->error = SERVER_ERROR_SSH;
			g_string_assign(answer, "no\n");
		}
		g_terminal_process_write_string(process, answer);

		g_string_free(answer, TRUE);
		gtk_widget_destroy(dialog);
	} else {
		/* check if it is an ssh error: check if string starts with "ssh:" */
		gchar *	str;

		str = strstr(output->str, "ssh:");
		if (str == output->str) {
			server->error = SERVER_ERROR_SSH;
			gebr_message(ERROR, TRUE, TRUE, _("Error contacting server at address %s via ssh: %s"),
				server->address->str, output->str);
			return TRUE;
		}

		return FALSE;
	}

out:	return TRUE;
}

static void
ssh_ask_port_read(GTerminalProcess * process, struct server * server)
{
	GString *	output;
	gchar **	splits;
	gchar *		strtol_endptr;
	guint16		port;

	output = g_terminal_process_read_string_all(process);
	if (ssh_parse_output(process, server, output) == TRUE)
		goto out;
	if (output->len <= 2) {
		/* server is not running, because run file test lead to empty output
		 * (2 is greater than carriege return)
		 */
		server->port = 0;
		goto out;
	}

	splits = g_strsplit(output->str, " ", 4);
	port = strtol(splits[0], &strtol_endptr, 10);
	if (errno != ERANGE) {
		server->port = port;
		g_string_assign(server->client_address, splits[1]);
		gebr_message(DEBUG, FALSE, TRUE, "ssh_ask_port_read: %d", port, server->port);
	} else {
		server->port = 0;
		server->error = SERVER_ERROR_SSH;
	}

	g_strfreev(splits);
out:	g_string_free(output, TRUE);
}

static void
ssh_open_tunnel_read(GTerminalProcess * process, struct server * server);

static void
ssh_open_tunnel_finished(GTerminalProcess * process, struct server * server);

static void
ssh_ask_port_finished(GTerminalProcess * process, struct server * server)
{
	GString *	cmd_line;

	gebr_message(DEBUG, FALSE, TRUE, "ssh_ask_port_finished");
	cmd_line = g_string_new(NULL);

	if (server->error != SERVER_ERROR_NONE)
		goto out;

	server->tried_existant_pass = FALSE;
	if (server->port) {
		GTerminalProcess *	tunnel_process;

		tunnel_process = g_terminal_process_new();
		g_signal_connect(tunnel_process, "ready-read",
			G_CALLBACK(ssh_open_tunnel_read), server);
		g_signal_connect(tunnel_process, "finished",
			G_CALLBACK(ssh_open_tunnel_finished), server);

		server->state = SERVER_STATE_OPEN_TUNNEL;
		server->tunnel_port = 2125;
		while (!g_tcp_server_is_local_port_available(server->tunnel_port))
			++server->tunnel_port;

		g_string_printf(cmd_line, "ssh -f -L %d:127.0.0.1:%d %s 'sleep 300'",
			server->tunnel_port, server->port, server->address->str);
		g_terminal_process_start(tunnel_process, cmd_line);
	}

out:	g_string_free(cmd_line, TRUE);
	g_terminal_process_free(process);
}

static void
ssh_run_server_read(GTerminalProcess * process, struct server * server)
{
	GString *	output;

	gebr_message(DEBUG, FALSE, TRUE, "ssh_run_server_read");

	/* ssh only output; gebrd doesn't output nothing */
	output = g_terminal_process_read_string_all(process);
	ssh_parse_output(process, server, output);

	g_string_free(output, TRUE);
}

static void
ssh_run_server_finished(GTerminalProcess * process, struct server * server)
{
	GString *	cmd_line;

	gebr_message(DEBUG, FALSE, TRUE, "ssh_run_server_finished");

	g_terminal_process_free(process);
	if (server->error != SERVER_ERROR_NONE)
		return;

	cmd_line = g_string_new(NULL);
	server->state = SERVER_STATE_ASK_PORT;
	server->tried_existant_pass = FALSE;

	process = g_terminal_process_new();
	g_signal_connect(process, "ready-read",
		G_CALLBACK(ssh_ask_port_read), server);
	g_signal_connect(process, "finished",
		G_CALLBACK(ssh_ask_port_finished), server);

	g_string_printf(cmd_line,
		"ssh %s \"LOCK=~/.gebr/run/gebrd-`echo $SSH_CLIENT | awk '{print $1}'`.run;"
		"test -e $LOCK && echo `cat $LOCK` $SSH_CLIENT\"", server->address->str);
	g_terminal_process_start(process, cmd_line);

	g_string_free(cmd_line, TRUE);
}

static void
ssh_open_tunnel_read(GTerminalProcess * process, struct server * server)
{
	GString *	output;

	gebr_message(DEBUG, FALSE, TRUE, "ssh_open_tunnel_read");

	/* in fact there is no output because ssh was ran with -f option, only ssh login stuff */
	output = g_terminal_process_read_string_all(process);
	ssh_parse_output(process, server, output);

	g_string_free(output, TRUE);
}

static void
ssh_open_tunnel_finished(GTerminalProcess * process, struct server * server)
{
	GHostAddress *		host_address;

	gebr_message(DEBUG, FALSE, TRUE, "ssh_open_tunnel_finished");

	if (server->error != SERVER_ERROR_NONE)
		goto out;

	/* connection is made to a local tunnel */
	host_address = g_host_address_new();
	g_host_address_set_ipv4_string(host_address, "127.0.0.1");

	server->state = SERVER_STATE_CONNECT;
	g_tcp_socket_connect(server->tcp_socket, host_address, server->tunnel_port, FALSE);

	g_host_address_free(host_address);
out:	g_terminal_process_free(process);
}

static void
server_connected(GTcpSocket * tcp_socket, struct server * server)
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
	protocol_send_data(server->protocol, server->tcp_socket,
		protocol_defs.ini_def, 5, PROTOCOL_VERSION, hostname, server->client_address->str, display, mcookie->str);

	/* frees */
	g_strfreev(splits);
	pclose(output_fp);
	g_string_free(mcookie, TRUE);
	g_string_free(cmd_line, TRUE);
}

static void
server_disconnected(GTcpSocket * tcp_socket, struct server * server)
{
	server->port = 0;
	server->protocol->logged = FALSE;
	server_list_updated_status(server);

	gebr_message(WARNING, TRUE, TRUE, "Server '%s' disconnected",
		     server->address->str);
}

static void
server_read(GTcpSocket * tcp_socket, struct server * server)
{
	GString *	data;

	data = g_socket_read_string_all(G_SOCKET(tcp_socket));
	protocol_receive_data(server->protocol, data);
	client_parse_server_messages(server);

	gebr_message(DEBUG, FALSE, TRUE, "Read from server '%s': %s",
		server->address->str, data->str);

	g_string_free(data, TRUE);
}

static void
server_error(GTcpSocket * tcp_socket, enum GSocketError error, struct server * server)
{
	server->error = SERVER_ERROR_CONNECT;
	if (error == G_SOCKET_ERROR_UNKNOWN)
		puts("unk");
	gebr_message(ERROR, FALSE, TRUE, _("Connection error '%s' on server '%s'"), error, server->address->str);
}


/*
 * Public functions
 */

struct server *
server_new(const gchar * _address)
{
	GtkTreeIter	iter;

	struct server *	server;

	/* initialize */
	server = g_malloc(sizeof(struct server));
	gtk_list_store_append(gebr.ui_server_list->common.store, &iter);
	/* fill struct */
	*server = (struct server) {
		.tcp_socket = g_tcp_socket_new(),
		.protocol = protocol_new(),
		.address = g_string_new(_address),
		.port = 0,
		.client_address = g_string_new(""),
		.password = g_string_new(""),
		.iter = iter
	};
	/* fill iter */
	gtk_list_store_set(gebr.ui_server_list->common.store, &iter,
			SERVER_STATUS_ICON, gebr.pixmaps.stock_disconnect,
			SERVER_ADDRESS, server_is_local(server) == TRUE ? _("Local server") : (gchar*)_address,
			SERVER_POINTER, server,
			-1);

	g_signal_connect(server->tcp_socket, "connected",
		G_CALLBACK(server_connected), server);
	g_signal_connect(server->tcp_socket, "disconnected",
		G_CALLBACK(server_disconnected), server);
	g_signal_connect(server->tcp_socket, "ready-read",
		G_CALLBACK(server_read), server);
	g_signal_connect(server->tcp_socket, "error",
		G_CALLBACK(server_error), server);

	server_connect(server);

	return server;
}

void
server_free(struct server * server)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* delete all jobs at server */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);

		if (job->server == server)
			job_delete(job);
	}

	g_socket_close(G_SOCKET(server->tcp_socket));
	protocol_free(server->protocol);
	g_string_free(server->address, TRUE);
	g_string_free(server->password, TRUE);
	g_string_free(server->client_address, TRUE);
	g_free(server);
}

void
server_connect(struct server * server)
{
	GString *	cmd_line;

	/* initiate the marathon to communicate to server */
	server->error = SERVER_ERROR_NONE;
	server->state = SERVER_STATE_RUN;
	server->tried_existant_pass = FALSE;

	cmd_line = g_string_new(NULL);

	if (server_is_local(server) == FALSE) {
		GTerminalProcess *	process;

		server->state = SERVER_STATE_RUN;
		gebr_message(INFO, TRUE, TRUE, _("Launching server at %s"), server->address->str);

		process = g_terminal_process_new();
		g_signal_connect(process, "ready-read",
			G_CALLBACK(ssh_run_server_read), server);
		g_signal_connect(process, "finished",
			G_CALLBACK(ssh_run_server_finished), server);

		g_string_printf(cmd_line, "ssh -x %s 'bash -l -c gebrd'", server->address->str);
		g_terminal_process_start(process, cmd_line);
	} else {
		GProcess *	process;

		server->state = SERVER_STATE_RUN;
		gebr_message(INFO, TRUE, TRUE, _("Launching local server"), server->address->str);

// 		process = g_process_new();
// // 		g_signal_connect(process, "finished",
// 			G_CALLBACK(local_run_server_finished), server);

#if (!GEBR_STATIC_MODE)
// 		g_string_printf(cmd_line, "bash -l -c 'gebrd'");
#else
// 		g_string_printf(cmd_line, "bash -l -c './gebrd'");
#endif
// 		g_process_start(process, cmd_line);
		system("bash -l -c 'gebrd'");

		GString *	cmd_line;

		gebr_message(DEBUG, FALSE, TRUE, "local_run_server_finished");

		server->state = SERVER_STATE_ASK_PORT;
		server->tried_existant_pass = FALSE;

		g_process_free(process);
		process = g_process_new();
		g_signal_connect(process, "ready-read-stdout",
				 G_CALLBACK(local_ask_port_read), server);
		g_signal_connect(process, "finished",
				 G_CALLBACK(local_ask_port_finished), server);

		g_string_printf(cmd_line, "bash -l -c 'test -e ~/.gebr/run/gebrd.run && cat ~/.gebr/run/gebrd.run'");
		g_process_start(process, cmd_line);
	}

	g_string_free(cmd_line, TRUE);
}

gboolean
server_is_logged(struct server * server)
{
	return server->protocol->logged;
}

gboolean
server_is_local(struct server * server)
{
	return g_ascii_strcasecmp(server->address->str, "127.0.0.1") == 0
		? TRUE : FALSE;
}

/*
 * Function: server_run_flow
 * Ask _server_ to run the current flow (at gebr.flow)
 *
 */
void
server_run_flow(struct server * server)
{
	GeoXmlFlow *		flow_wnh; /* wnh: with no help */
	GeoXmlSequence *	program;
	gchar *			xml;

	/* removes flow's help */
	flow_wnh = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOC(gebr.flow)));
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
	protocol_send_data(server->protocol, server->tcp_socket, protocol_defs.run_def, 1, xml);

	/* frees */
	g_free(xml);
	geoxml_document_free(GEOXML_DOC(flow_wnh));
}
