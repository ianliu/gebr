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

#include <stdarg.h>

#include <libgebr/utils.h>
#include <libgebr/comm/gebr-comm-streamsocket.h>

#include "gebr-comm-protocol-socket.h"
#include "gebr-comm-protocol_p.h"
#include "../marshalers.h"


/* GOBJECT STUFF */
#define GEBR_COMM_PROTOCOL_SOCKET_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_COMM_PROTOCOL_SOCKET_TYPE, GebrCommProtocolSocketPrivate))
struct _GebrCommProtocolSocketPrivate {
	GebrCommHttpMsg *incoming_msg;

	GList *requests_fifo;
	GebrCommStreamSocket *socket;
};
enum {
	PROP_0,
	PROP_STREAM_SOCKET,
	LAST_PROPERTY
};
//static guint property_member_offset [] = {0,
//};
enum {
	CONNECTED,
	DISCONNECTED,
	PROCESS_REQUEST,
	PROCESS_RESPONSE,
	OLD_PARSE_MESSAGES,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrCommProtocolSocket, gebr_comm_protocol_socket, G_TYPE_OBJECT)

static void gebr_comm_protocol_socket_connected(GebrCommStreamSocket *socket, GebrCommProtocolSocket * self)
{
	g_signal_emit(self, object_signals[CONNECTED], 0);
}
static void gebr_comm_protocol_socket_disconnected(GebrCommStreamSocket *socket, GebrCommProtocolSocket * self)
{
	g_signal_emit(self, object_signals[DISCONNECTED], 0);
}
static void gebr_comm_protocol_socket_read(GebrCommStreamSocket *socket, GebrCommProtocolSocket * self)
{
	GString *data = gebr_comm_socket_read_string_all(GEBR_COMM_SOCKET(socket)); 
	gboolean parse_http_msg()
	{
		self->priv->incoming_msg = gebr_comm_http_msg_new_parsing(self->priv->incoming_msg, data);
		if (!self->priv->incoming_msg)
			return FALSE;
		if (!self->priv->incoming_msg->parsed)
			return TRUE; /* more data need... */

		if (self->priv->incoming_msg->type == GEBR_COMM_HTTP_TYPE_REQUEST)
			g_signal_emit(self, object_signals[PROCESS_REQUEST], 0, self->priv->incoming_msg);
		else if (self->priv->incoming_msg->type == GEBR_COMM_HTTP_TYPE_RESPONSE) {
			if (self->priv->requests_fifo != NULL) {
				GebrCommHttpMsg *request = (GebrCommHttpMsg*)self->priv->requests_fifo->data;
				GebrCommHttpMsg *response = self->priv->incoming_msg;
				g_signal_emit(self, object_signals[PROCESS_RESPONSE], 0, request, response);
				gebr_comm_http_msg_response_received(request, response);
				gebr_comm_http_msg_free(request);

				self->priv->requests_fifo = g_list_remove_link(self->priv->requests_fifo, self->priv->requests_fifo);
			}
		}
		gebr_comm_http_msg_free(self->priv->incoming_msg);
		self->priv->incoming_msg = NULL;

		return TRUE;
	}
	gboolean parse_old_msg()
	{
		/* old non-rest protocol */
		//TODO: emit data received with raw string data
		gboolean ret;
		if ((ret = gebr_comm_protocol_receive_data(self->protocol, data)))
			g_signal_emit(self, object_signals[OLD_PARSE_MESSAGES], 0);

		return ret;
	}

	while (data->len) {
		gboolean ret;
		if (self->protocol->message->hash)
			ret = parse_old_msg();
		else if (self->priv->incoming_msg != NULL)
			ret = parse_http_msg();
		else if (g_str_has_prefix(data->str, "HTTP/1.1 ") ||
			 g_str_has_prefix(data->str, "GET ") ||
			 g_str_has_prefix(data->str, "PUT ") ||
			 g_str_has_prefix(data->str, "POST ") ||
			 g_str_has_prefix(data->str, "DELETE "))
			ret = parse_http_msg();
		else
			ret = parse_old_msg();

		if (!ret)
			break;
	}

	g_string_free(data, TRUE);
}
static void gebr_comm_protocol_socket_error(GebrCommProtocolSocket * self, enum GebrCommSocketError error)
{
	//TODO: translate and treat more errors
}
static void gebr_comm_protocol_socket_init(GebrCommProtocolSocket * self)
{
	self->protocol = gebr_comm_protocol_new();
	self->priv = GEBR_COMM_PROTOCOL_SOCKET_GET_PRIVATE(self);
	self->priv->incoming_msg = NULL;
	self->priv->requests_fifo = NULL;
}
static void gebr_comm_protocol_socket_finalize(GObject * object)
{
	GebrCommProtocolSocket *self = GEBR_COMM_PROTOCOL_SOCKET(object);

	gebr_comm_protocol_free(self->protocol);
	gebr_comm_socket_close(GEBR_COMM_SOCKET(self->priv->socket));
	gebr_comm_http_msg_free(self->priv->incoming_msg);
	g_list_foreach(self->priv->requests_fifo, (GFunc)gebr_comm_http_msg_free, NULL);
	g_list_free(self->priv->requests_fifo);

	G_OBJECT_CLASS(gebr_comm_protocol_socket_parent_class)->finalize(object);
}
static void
gebr_comm_protocol_socket_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GebrCommProtocolSocket *self = (GebrCommProtocolSocket *) object;
	switch (property_id) {
	case PROP_STREAM_SOCKET:
		self->priv->socket = g_value_get_pointer(value);
		g_signal_connect(self->priv->socket, "connected",
				 G_CALLBACK(gebr_comm_protocol_socket_connected), self);
		g_signal_connect(self->priv->socket, "disconnected",
				 G_CALLBACK(gebr_comm_protocol_socket_disconnected), self);
		g_signal_connect(self->priv->socket, "ready-read",
				 G_CALLBACK(gebr_comm_protocol_socket_read), self);
		g_signal_connect(self->priv->socket, "error",
				 G_CALLBACK(gebr_comm_protocol_socket_error), self);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void
gebr_comm_protocol_socket_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	GebrCommProtocolSocket *self = (GebrCommProtocolSocket *) object;
	switch (property_id) {
	case PROP_STREAM_SOCKET:
		g_value_set_pointer(value, self->priv->socket);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void gebr_comm_protocol_socket_class_init(GebrCommProtocolSocketClass * klass)
{ 
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_comm_protocol_socket_finalize;
	g_type_class_add_private(klass, sizeof (GebrCommProtocolSocketPrivate));

	/* deprecated */
	gebr_comm_protocol_init();
	
	/* properties */
	GParamSpec *spec;
	gobject_class->set_property = gebr_comm_protocol_socket_set_property;
	gobject_class->get_property = gebr_comm_protocol_socket_get_property;
	spec =	g_param_spec_pointer("stream-socket", "", "",
				     (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)); 
	g_object_class_install_property(gobject_class, PROP_STREAM_SOCKET, spec);
	/* signals */
	object_signals[CONNECTED] = g_signal_new("connected", GEBR_COMM_PROTOCOL_SOCKET_TYPE,
						 (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
						 G_STRUCT_OFFSET(GebrCommProtocolSocketClass, connected),
						 NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[DISCONNECTED] = g_signal_new("disconnected", GEBR_COMM_PROTOCOL_SOCKET_TYPE,
						    (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
						    G_STRUCT_OFFSET(GebrCommProtocolSocketClass, disconnected), NULL,
						    NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[PROCESS_REQUEST] = g_signal_new("process-request", GEBR_COMM_PROTOCOL_SOCKET_TYPE,
							  (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
							  G_STRUCT_OFFSET(GebrCommProtocolSocketClass, process_request), NULL,
							  NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
	object_signals[PROCESS_RESPONSE] = g_signal_new("process-response", GEBR_COMM_PROTOCOL_SOCKET_TYPE,
							  (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
							  G_STRUCT_OFFSET(GebrCommProtocolSocketClass, process_response), NULL,
							  NULL, _gebr_gui_marshal_VOID__POINTER_POINTER,
							  G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
	object_signals[OLD_PARSE_MESSAGES] = g_signal_new("old-parse-messages", GEBR_COMM_PROTOCOL_SOCKET_TYPE,
							  (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
							  G_STRUCT_OFFSET(GebrCommProtocolSocketClass, old_parse_messages), NULL,
							  NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

GebrCommProtocolSocket * gebr_comm_protocol_socket_new(void)
{
	return GEBR_COMM_PROTOCOL_SOCKET(g_object_new(GEBR_COMM_PROTOCOL_SOCKET_TYPE,
						      "stream-socket", gebr_comm_stream_socket_new(), NULL));
}
GebrCommProtocolSocket * gebr_comm_protocol_socket_new_from_socket(GebrCommStreamSocket *socket)
{
	return GEBR_COMM_PROTOCOL_SOCKET(g_object_new(GEBR_COMM_PROTOCOL_SOCKET_TYPE,
						      "stream-socket", socket, NULL));
}
gboolean gebr_comm_protocol_socket_connect(GebrCommProtocolSocket *self, GebrCommSocketAddress * socket_address, gboolean wait)
{
	return gebr_comm_stream_socket_connect(self->priv->socket, socket_address, wait);
}
void gebr_comm_protocol_socket_disconnect(GebrCommProtocolSocket * self)
{
	gebr_comm_stream_socket_disconnect(self->priv->socket);
}

GebrCommHttpMsg *gebr_comm_protocol_socket_send_request(GebrCommProtocolSocket * self, GebrCommHttpRequestMethod method,
							const gchar *url, GebrCommJsonContent *_content)
{
	const gchar *content = _content ? _content->data->str : "";
	GHashTable *headers = g_hash_table_new(g_str_hash, g_str_equal);
	if (content)
	       g_hash_table_insert(headers, g_strdup("content-type"), g_strdup("application/json"));
	GebrCommHttpMsg *msg = gebr_comm_http_msg_new_request(method, url, headers, content);
	g_hash_table_unref(headers);

	gebr_comm_socket_write_string(GEBR_COMM_SOCKET(self->priv->socket), msg->raw);
	self->priv->requests_fifo = g_list_append(self->priv->requests_fifo, msg);
	
	return msg;
}

void gebr_comm_protocol_socket_send_response(GebrCommProtocolSocket * self, int status_code, GebrCommJsonContent *_content)
{
	const gchar *content = _content ? _content->data->str : "";
	GHashTable *headers = g_hash_table_new(g_str_hash, g_str_equal);
	if (content)
	       g_hash_table_insert(headers, g_strdup("content-type"), g_strdup("application/json"));
	GebrCommHttpMsg *msg = gebr_comm_http_msg_new_response(status_code, headers, content);
	g_hash_table_unref(headers);

	gebr_comm_socket_write_string(GEBR_COMM_SOCKET(self->priv->socket), msg->raw);
	gebr_comm_http_msg_free(msg);
}

void gebr_comm_protocol_socket_oldmsg_send(GebrCommProtocolSocket * self, gboolean blocking,
					   struct gebr_comm_message_def gebr_comm_message_def, guint n_params, ...)
{
	va_list ap;
	GString * message;

	va_start(ap, n_params);
	message = gebr_comm_protocol_build_messagev(gebr_comm_message_def, n_params, ap);

	/* does this message need return? */
	self->protocol->waiting_ret_hash = (gebr_comm_message_def.returns == TRUE) ? gebr_comm_message_def.code_hash : 0;
	/* send it */
	if (blocking)
		gebr_comm_socket_write_string_immediately(GEBR_COMM_SOCKET(self->priv->socket), message);
	else 
		gebr_comm_socket_write_string(GEBR_COMM_SOCKET(self->priv->socket), message);

	g_string_free(message, TRUE);
}
GList *gebr_comm_protocol_socket_oldmsg_split(GString * arguments, guint parts)
{
	return gebr_comm_protocol_split_new(arguments, parts);
}
void gebr_comm_protocol_socket_oldmsg_split_free(GList * split)
{
	gebr_comm_protocol_split_free(split);
}

