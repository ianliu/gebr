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

#ifndef __GEBR_COMM_PROTOCOL_SOCKET_H
#define __GEBR_COMM_PROTOCOL_SOCKET_H

#include <glib.h>
#include <glib-object.h>

#include <libgebr/comm/gebr-comm-socketaddress.h>
#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/comm/gebr-comm-http-msg.h>
#include <libgebr/comm/gebr-comm-json-content.h>

G_BEGIN_DECLS

GType gebr_comm_protocol_socket_get_type(void);
#define GEBR_COMM_PROTOCOL_SOCKET_TYPE		(gebr_comm_protocol_socket_get_type())
#define GEBR_COMM_PROTOCOL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_PROTOCOL_SOCKET_TYPE, GebrCommProtocolSocket))
#define GEBR_COMM_PROTOCOL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_PROTOCOL_SOCKET_TYPE, GebrCommProtocolSocketClass))
#define GEBR_COMM_IS_PROTOCOL_SOCKET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_PROTOCOL_SOCKET_TYPE))
#define GEBR_COMM_IS_PROTOCOL_SOCKET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_PROTOCOL_SOCKET_TYPE))
#define GEBR_COMM_PROTOCOL_SOCKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_PROTOCOL_SOCKET_TYPE, GebrCommProtocolSocketClass))

typedef struct _GebrCommProtocolSocket GebrCommProtocolSocket;
typedef struct _GebrCommProtocolSocketPrivate GebrCommProtocolSocketPrivate;
typedef struct _GebrCommProtocolSocketClass GebrCommProtocolSocketClass;

struct _GebrCommProtocolSocket {
	GObject parent;

	GebrCommProtocolSocketPrivate *priv;
	
	/* deprecated */
	struct gebr_comm_protocol *protocol;
};

struct _GebrCommProtocolSocketClass {
	GObjectClass parent;

	/* signals */
	void (*connected)(GebrCommProtocolSocket *socket);
	void (*disconnected)(GebrCommProtocolSocket *socket);
	void (*process_request)(GebrCommProtocolSocket *socket, GebrCommHttpMsg * request);
	void (*process_response)(GebrCommProtocolSocket *socket, GebrCommHttpMsg * request, GebrCommHttpMsg * response);
	void (*old_parse_messages)(GebrCommProtocolSocket *socket);
};

GebrCommProtocolSocket * gebr_comm_protocol_socket_new(void);
GebrCommProtocolSocket * gebr_comm_protocol_socket_new_from_socket(GebrCommStreamSocket *socket);

gboolean gebr_comm_protocol_socket_connect(GebrCommProtocolSocket *self, GebrCommSocketAddress * socket_address, gboolean wait);
void gebr_comm_protocol_socket_disconnect(GebrCommProtocolSocket * self);

GebrCommHttpMsg *gebr_comm_protocol_socket_send_request(GebrCommProtocolSocket * self, GebrCommHttpRequestMethod method,
							const gchar *url, GebrCommJsonContent *content);
void gebr_comm_protocol_socket_send_response(GebrCommProtocolSocket * self, int status_code, GebrCommJsonContent *content);

void gebr_comm_protocol_socket_oldmsg_send(GebrCommProtocolSocket * self, gboolean blocking,
					   struct gebr_comm_message_def gebr_comm_message_def, guint n_params, ...);
GList *gebr_comm_protocol_socket_oldmsg_split(GString * arguments, guint parts);
void gebr_comm_protocol_socket_oldmsg_split_free(GList * split);

G_END_DECLS
#endif				//__GEBR_COMM_PROTOCOL_SOCKET_H
