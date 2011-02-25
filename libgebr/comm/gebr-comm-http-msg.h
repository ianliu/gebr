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

#ifndef __GEBR_COMM_HTTP_MSG_H
#define __GEBR_COMM_HTTP_MSG_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

GType gebr_comm_http_msg_get_type(void);
#define GEBR_COMM_HTTP_MSG_TYPE		(gebr_comm_http_msg_get_type())
#define GEBR_COMM_HTTP_MSG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_HTTP_MSG_TYPE, GebrCommHttpMsg))
#define GEBR_COMM_HTTP_MSG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_HTTP_MSG_TYPE, GebrCommHttpMsgClass))
#define GEBR_COMM_IS_HTTP_MSG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_HTTP_MSG_TYPE))
#define GEBR_COMM_IS_HTTP_MSG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_HTTP_MSG_TYPE))
#define GEBR_COMM_HTTP_MSG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_HTTP_MSG_TYPE, GebrCommHttpMsgClass))

typedef struct _GebrCommHttpMsg GebrCommHttpMsg;
typedef struct _GebrCommHttpMsgClass GebrCommHttpMsgClass;

typedef enum {
	GEBR_COMM_HTTP_TYPE_UNKNOWN = 0,
	GEBR_COMM_HTTP_TYPE_REQUEST,
	GEBR_COMM_HTTP_TYPE_RESPONSE,
} GebrCommHttpRequestType;
typedef enum {
	GEBR_COMM_HTTP_METHOD_UNKNOWN = 0,
	GEBR_COMM_HTTP_METHOD_GET,
	GEBR_COMM_HTTP_METHOD_PUT,
	GEBR_COMM_HTTP_METHOD_POST,
	GEBR_COMM_HTTP_METHOD_DELETE,
} GebrCommHttpRequestMethod;
struct _GebrCommHttpMsg {
	GObject parent;
	GebrCommHttpRequestType type;
	GebrCommHttpRequestMethod method;
	/* first line */
	guint status_code;
	GString *url;
	/* headers */
	GHashTable *headers;
	/* after headers and \n\n */
	GHashTable *params;
	GString *content;

	GString *raw;
	gboolean parsed;
	gboolean parsed_headers;
};
struct _GebrCommHttpMsgClass {
	GObjectClass parent;

	void (*response_received)(GebrCommHttpMsg *request, GebrCommHttpMsg *response);
};

GebrCommHttpMsg *gebr_comm_http_msg_new(GebrCommHttpRequestType type, GebrCommHttpRequestMethod method);
GebrCommHttpMsg *gebr_comm_http_msg_new_parsing(GebrCommHttpMsg *partial, GString *data);
GebrCommHttpMsg *gebr_comm_http_msg_new_request(GebrCommHttpRequestMethod method, const gchar *url, GHashTable * headers, const gchar *content);
GebrCommHttpMsg *gebr_comm_http_msg_new_response(gint status_code, GHashTable * headers, const gchar *content);
void gebr_comm_http_msg_free(GebrCommHttpMsg *msg);

void gebr_comm_http_msg_response_received(GebrCommHttpMsg *request, GebrCommHttpMsg *response);

G_END_DECLS

#endif //__GEBR_COMM_HTTP_MSG_H
