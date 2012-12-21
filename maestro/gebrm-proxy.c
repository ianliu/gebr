/*
 * gebrm-proxy.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Team
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

#include "gebrm-proxy.h"

#include <unistd.h>
#include <stdlib.h>
#include "gebrm-client.h"
#include <libgebr/comm/gebr-comm.h>

struct _GebrmProxy
{
	GMainLoop *loop;
	GebrmClient *client;
	GebrCommServer *maestro;
	GebrCommListenSocket *listener;
	gchar *remote_addr;
	guint remote_port;

	GebrCommPortForward *sftp_forward;
};

static void
gebrm_proxy_quit(GebrmProxy *proxy)
{
	GebrCommServerState state = gebr_comm_server_get_state(proxy->maestro);

	if (state != SERVER_STATE_DISCONNECTED)
		gebr_comm_server_disconnect(proxy->maestro);

	if (proxy->sftp_forward) {
		gebr_comm_port_forward_close(proxy->sftp_forward);
		gebr_comm_port_forward_free(proxy->sftp_forward);
		proxy->sftp_forward = NULL;
	}

	g_main_loop_quit(proxy->loop);
}

static void
gebrm_proxy_server_op_log_message(GebrCommServer *server,
				  GebrLogMessageType type,
				  const gchar *message,
				  gpointer user_data)
{
}

static void
gebrm_proxy_server_op_state_changed(GebrCommServer *comm_server,
				    gpointer user_data)
{
	GebrmProxy *proxy = user_data;
	GebrCommServerState state = gebr_comm_server_get_state(comm_server);
	if (state == SERVER_STATE_DISCONNECTED)
		gebrm_proxy_quit(proxy);
}

static void
gebrm_proxy_server_op_process_request(GebrCommServer *server,
				      GebrCommHttpMsg *request,
				      gpointer user_data)
{
}

static void
gebrm_proxy_server_op_process_response(GebrCommServer *server,
				       GebrCommHttpMsg *request,
				       GebrCommHttpMsg *response,
				       gpointer user_data)
{
}

static void
on_sftp_port_defined(GebrCommPortProvider *self,
		     guint port,
		     GebrmProxy *proxy)
{
	proxy->sftp_forward = gebr_comm_port_provider_get_forward(self);

	gchar *tmp = g_strdup_printf("%d", port);
	GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(proxy->client);
	gebr_comm_protocol_socket_return_message(socket, FALSE,
						 gebr_comm_protocol_defs.sftp_def, 1, tmp);
	g_free(tmp);
}

static void
on_sftp_port_error(GebrCommPortProvider *self,
		   GError *error,
		   GebrmProxy *proxy)
{
	GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(proxy->client);
	gebr_comm_protocol_socket_return_message(socket, FALSE,
						 gebr_comm_protocol_defs.sftp_def, 1, "0");
}

static void
setup_sftp_tunnel(GebrmProxy *proxy, guint remote_port)
{
	GebrCommPortProvider *port_provider =
		gebr_comm_server_create_port_provider(proxy->maestro, GEBR_COMM_PORT_TYPE_SFTP);
	gebr_comm_port_provider_set_sftp_port(port_provider, remote_port);
	g_signal_connect(port_provider, "port-defined", G_CALLBACK(on_sftp_port_defined), proxy);
	g_signal_connect(port_provider, "error", G_CALLBACK(on_sftp_port_error), proxy);
	gebr_comm_port_provider_start(port_provider);
}

static void
gebrm_proxy_server_op_parse_messages(GebrCommServer *comm_server,
				     gpointer user_data)
{
	GList *link;
	struct gebr_comm_message *message;

	gboolean resend_message = TRUE;
	GebrmProxy *proxy = user_data;

	while ((link = g_list_last(comm_server->socket->protocol->messages)) != NULL) {
		message = (struct gebr_comm_message *)link->data;
		if (message->hash == gebr_comm_protocol_defs.ret_def.code_hash) {
			guint ret_hash = message->ret_hash;
			if (ret_hash == gebr_comm_protocol_defs.sftp_def.code_hash) {
				GList *arguments;

				if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 1)) == NULL)
					goto err;

				GString *remote_port_str = g_list_nth_data(arguments, 0);
				guint remote_port = atoi(remote_port_str->str);

				setup_sftp_tunnel(proxy, remote_port);
				resend_message = FALSE;

				gebr_comm_protocol_socket_oldmsg_split_free(arguments);
			}
		}

		if (resend_message) {
			GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(proxy->client);
			gebr_comm_protocol_socket_resend_message(socket, FALSE, message);
		}

		gebr_comm_message_free(message);
		comm_server->socket->protocol->messages = g_list_delete_link(comm_server->socket->protocol->messages, link);
	}

	return;
err:
	gebr_comm_message_free(message);
	gebr_comm_server_disconnect(proxy->maestro);
}

static struct gebr_comm_server_ops proxy_ops = {
	.log_message      = gebrm_proxy_server_op_log_message,
	.state_changed    = gebrm_proxy_server_op_state_changed,
	.process_request  = gebrm_proxy_server_op_process_request,
	.process_response = gebrm_proxy_server_op_process_response,
	.parse_messages   = gebrm_proxy_server_op_parse_messages
};

static void
on_proxy_client_disconnect(GebrCommProtocolSocket *socket,
			   GebrmProxy *proxy)
{
	gebrm_proxy_quit(proxy);
}

static void
on_proxy_client_request(GebrCommProtocolSocket *socket,
			GebrCommHttpMsg *request,
			GebrmProxy *proxy)
{
	g_return_if_fail(proxy->maestro != NULL);

	GebrCommJsonContent *content = gebr_comm_json_content_new(request->content->str);
	gebr_comm_protocol_socket_send_request(proxy->maestro->socket,
					       request->method,
					       request->url->str,
					       content);
	gebr_comm_json_content_free(content);
}

static void
on_proxy_client_parse_messages(GebrCommProtocolSocket *socket,
			       GebrmProxy *proxy)
{
	GList *link;
	struct gebr_comm_message *message;

	GebrmClient *client = proxy->client;

	while ((link = g_list_last(socket->protocol->messages)) != NULL) {
		message = link->data;

		if (message->hash == gebr_comm_protocol_defs.ini_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 7)) == NULL)
				goto err;

			GString *cookie  = g_list_nth_data(arguments, 2);
			GString *gebr_id = g_list_nth_data(arguments, 3);

			gebrm_client_set_id(client, gebr_id->str);
			gebrm_client_set_magic_cookie(client, cookie->str);

			proxy->maestro = gebr_comm_server_new(proxy->remote_addr,
							      gebr_id->str,
							      &proxy_ops);
			proxy->maestro->user_data = proxy;
			gebr_comm_server_set_x11_cookie(proxy->maestro, cookie->str);
			gebr_comm_server_connect(proxy->maestro, TRUE, FALSE);

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else if (message->hash == gebr_comm_protocol_defs.dsp_def.code_hash) {
			GList *arguments;

			if ((arguments = gebr_comm_protocol_socket_oldmsg_split(message->argument, 2)) == NULL)
				goto err;

			GString *port = g_list_nth_data(arguments, 0);
			GString *host = g_list_nth_data(arguments, 1);

			gebr_comm_server_forward_x11(proxy->maestro, host->str, atoi(port->str));

			gebr_comm_protocol_socket_oldmsg_split_free(arguments);
		}
		else {
			if (proxy->maestro)
				gebr_comm_protocol_socket_resend_message(proxy->maestro->socket, FALSE, message);
		}

		gebr_comm_message_free(message);
		socket->protocol->messages = g_list_delete_link(socket->protocol->messages, link);
	}

	return;

err:
	gebr_comm_message_free(message);
	socket->protocol->messages = g_list_delete_link(socket->protocol->messages, link);
	g_object_unref(client);
}


static void
on_proxy_new_connection(GebrCommListenSocket *listener,
			GebrmProxy *proxy)
{
	GebrCommStreamSocket *stream;
	stream = gebr_comm_listen_socket_get_next_pending_connection(listener);
	proxy->client = gebrm_client_new(stream);
	GebrCommProtocolSocket *socket = gebrm_client_get_protocol_socket(proxy->client);

	g_signal_connect(socket, "disconnected",
			 G_CALLBACK(on_proxy_client_disconnect), proxy);
	g_signal_connect(socket, "process-request",
			 G_CALLBACK(on_proxy_client_request), proxy);
	g_signal_connect(socket, "old-parse-messages",
			 G_CALLBACK(on_proxy_client_parse_messages), proxy);
}

GebrmProxy *
gebrm_proxy_new(const gchar *remote_addr,
		guint remote_port)
{
	GebrmProxy *proxy = g_new0(GebrmProxy, 1);
	proxy->loop = g_main_loop_new(NULL, FALSE);
	proxy->client = NULL;
	proxy->maestro = NULL;
	proxy->listener = gebr_comm_listen_socket_new();
	proxy->remote_addr = g_strdup(remote_addr);
	proxy->remote_port = remote_port;
	proxy->sftp_forward = NULL;

	g_signal_connect(proxy->listener, "new-connection",
			 G_CALLBACK(on_proxy_new_connection), proxy);

	return proxy;
}

void
gebrm_proxy_run(GebrmProxy *proxy, int fd)
{
	GebrCommSocketAddress address = gebr_comm_socket_address_ipv4("127.0.0.1", 0);
	gebr_comm_listen_socket_listen(proxy->listener, &address);
	address = gebr_comm_socket_get_address(GEBR_COMM_SOCKET(proxy->listener));
	guint16 port = gebr_comm_socket_address_get_ip_port(&address);

	gchar *port_str = g_strdup_printf("%s%u\n", GEBR_PORT_PREFIX, port);

	ssize_t s = 0;
	do {
		s = write(fd, port_str + s, strlen(port_str) - s);
		if (s == -1)
			exit(-1);
	} while(s != 0);

	g_free(port_str);

	g_main_loop_run(proxy->loop);
}

void
gebrm_proxy_free(GebrmProxy *proxy)
{
	if (GEBRM_IS_CLIENT(proxy->client))
		g_object_unref(proxy->client);

	if (GEBR_COMM_IS_SERVER(proxy->maestro))
		gebr_comm_server_free(proxy->maestro);

	gebr_comm_listen_socket_free(proxy->listener);

	g_main_loop_unref(proxy->loop);
	g_free(proxy->remote_addr);
	g_free(proxy);
}
