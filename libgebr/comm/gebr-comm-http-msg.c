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
#include <stdlib.h>

#include "gebr-comm-http-msg.h"

static void gebr_g_hash_table_fill_with_headers(GHashTable *dict, GString *headers)
{
	gchar **lines = g_strsplit(headers->str, "\n", 0);
	for (int i = 0; lines[i] != NULL; ++i) {
		//TODO: robust
		gchar **split = g_strsplit(lines[i], ":", 2);
		if (split[0] && strlen(split[0])) {
			GString *key = g_string_new(split[0]);
			g_string_ascii_down(key);
			//TODO: robust
			g_hash_table_insert(dict, g_strdup(key->str), g_strdup(split[1]));
			g_string_free(key, TRUE);
			g_strfreev(split);
		}
	}
	g_strfreev(lines);
}
//static void gebr_g_hash_table_fill_with_url_query(GHashTable *dict, GString *query)
//{
	//TODO: parse query of scheme://username:password@domain:port/path?query
//}
static void gebr_comm_http_fieldfy(GebrCommHttpMsg *msg)
{
	const gchar *first_line_feed = strstr(msg->raw->str, "\n");

	/* first line */
	GString *first_line = g_string_new("");
	if (first_line_feed != NULL)
		g_string_append_len(first_line, msg->raw->str, first_line_feed-msg->raw->str);
	else
		g_string_assign(first_line, msg->raw->str);
	gchar **split = g_strsplit(first_line->str, " ", 0);
	g_string_free(first_line, TRUE);
	if (msg->type == GEBR_COMM_HTTP_TYPE_RESPONSE) {
		//TODO: robust
		msg->status_code = atoi(split[1]);
	} else if (msg->type == GEBR_COMM_HTTP_TYPE_REQUEST) {
		//TODO: robust
		g_string_assign(msg->url, split[1]);
	} //else

	/* headers */
	if (first_line_feed) {
		GString *headers = g_string_new(first_line_feed+1);
		gebr_g_hash_table_fill_with_headers(msg->headers, headers);
		g_string_free(headers, TRUE);
	}

	g_strfreev(split);
}
static const gchar *method_enum_to_string [] = {NULL,
	"GET", "PUT", "POST", "DELETE"
};

/* GOBJECT STUFF */
enum {
	PROP_0,
	LAST_PROPERTY
};
//static guint property_member_offset [] = {0,
//};
enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrCommHttpMsg, gebr_comm_http_msg, G_TYPE_OBJECT)
static void gebr_comm_http_msg_init(GebrCommHttpMsg * msg)
{
	msg->status_code = 0;
	msg->url = g_string_new("");
	msg->headers = g_hash_table_new(g_str_hash, g_str_equal);
	msg->params = g_hash_table_new(g_str_hash, g_str_equal);
	msg->content = g_string_new("");
	msg->raw = g_string_new("");
	msg->parsed = FALSE;
	msg->parsed_headers = FALSE;
}
static void gebr_comm_http_msg_finalize(GObject * object)
{
	GebrCommHttpMsg *msg = GEBR_COMM_HTTP_MSG(object);
	g_string_free(msg->url, TRUE);
	g_hash_table_unref(msg->headers);
	g_hash_table_unref(msg->params);
	g_string_free(msg->content, TRUE);
	g_string_free(msg->raw, TRUE);
	G_OBJECT_CLASS(gebr_comm_http_msg_parent_class)->finalize(object);
}
static void
gebr_comm_http_msg_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	//GebrCommHttpMsg *self = (GebrCommHttpMsg *) object;
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void
gebr_comm_http_msg_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	//GebrCommHttpMsg *self = (GebrCommHttpMsg *) object;
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void gebr_comm_http_msg_class_init(GebrCommHttpMsgClass * klass)
{ 
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_comm_http_msg_finalize;
//	GebrCommHttpMsgClass *super_class = GEBR_COMM_HTTP_MSG_GET_CLASS(klass);
	
	/* properties */
	//GParamSpec *pspec;
	gobject_class->set_property = gebr_comm_http_msg_set_property;
	gobject_class->get_property = gebr_comm_http_msg_get_property;
	//pspec = g_param_spec_boxed("client-hostname", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	//g_object_class_install_property(gobject_class, CLIENT_HOSTNAME, pspec);
}

GebrCommHttpMsg *gebr_comm_http_msg_new(GebrCommHttpRequestType type, GebrCommHttpRequestMethod method)
{
	GebrCommHttpMsg *msg = GEBR_COMM_HTTP_MSG(g_object_new(GEBR_COMM_HTTP_MSG_TYPE, NULL));
	msg->type = type;
	msg->method = method;
	return msg;
}
void gebr_comm_http_msg_free(GebrCommHttpMsg *msg)
{
	g_object_unref(msg);
}
GebrCommHttpMsg *gebr_comm_http_msg_new_parsing(GebrCommHttpMsg *partial, GString *data)
{
	GebrCommHttpMsg *msg = partial;
	if (msg == NULL) {
		if (g_str_has_prefix(data->str, "HTTP/1.1 "))
			msg = gebr_comm_http_msg_new(GEBR_COMM_HTTP_TYPE_RESPONSE, GEBR_COMM_HTTP_METHOD_UNKNOWN);
		else if (g_str_has_prefix(data->str, "GET "))
			msg = gebr_comm_http_msg_new(GEBR_COMM_HTTP_TYPE_REQUEST, GEBR_COMM_HTTP_METHOD_GET);
		else if (g_str_has_prefix(data->str, "PUT "))
			msg = gebr_comm_http_msg_new(GEBR_COMM_HTTP_TYPE_REQUEST, GEBR_COMM_HTTP_METHOD_PUT);
		else if (g_str_has_prefix(data->str, "POST ")) {
		} else if (g_str_has_prefix(data->str, "DELETE ")) {
		} else
			return NULL;
	}

	/* 
	 * Incremental parser: data may come in several parts
	 */
	while (!msg->parsed && data->len) {
		gsize already_parsed_len = msg->raw->len;

		if (msg->parsed_headers) {
			/* get content */
			const gchar *tmp = g_hash_table_lookup(msg->headers, "content-length");
			if (tmp) {
				gsize content_length = atol(tmp);
				gsize remaining = content_length-msg->content->len;
				if (data->len >= remaining) {
					g_string_append_len(msg->content, data->str, remaining);
					g_string_append_len(msg->raw, data->str, remaining);
					msg->parsed = TRUE;
				} else {
					g_string_append(msg->content, data->str);
					g_string_append(msg->raw, data->str);
				}
			} //else
		} else { 
			GString *full_data = g_string_new(msg->raw->str);
			g_string_append(full_data, data->str);

			/* parse headers */
			gchar *header_end = strstr(full_data->str, "\n\n");
			if (!header_end)
				g_string_append(msg->raw, data->str);
			else {
				msg->parsed_headers = TRUE;
				gint bytes = header_end-(full_data->str+msg->raw->len)+2/*\n\n*/;
				if (bytes >= 0) {
					GString *tmp = g_string_new_len(data->str, bytes);
					g_string_append(msg->raw, tmp->str);
					g_string_free(tmp, TRUE);
				} //else

				gebr_comm_http_fieldfy(msg);
				/* no content? */
				if (!g_hash_table_lookup(msg->headers, "content-length"))
					msg->parsed = TRUE;
			}

			g_string_free(full_data, TRUE);
		}

		g_string_erase(data, 0, msg->raw->len - already_parsed_len);
	}

	return msg;
}

void gebr_comm_http_msg_serialize(GebrCommHttpMsg *msg)
{
	if (msg->type == GEBR_COMM_HTTP_TYPE_REQUEST)
		g_string_printf(msg->raw, "%s %s\n", method_enum_to_string[msg->method], msg->url->str);
	else if (msg->type == GEBR_COMM_HTTP_TYPE_RESPONSE)
		g_string_printf(msg->raw, "HTTP/1.1 %u OK\n", msg->status_code);
	//else 
	
	/* headers */
	if (msg->content->len)
		g_hash_table_insert(msg->headers, g_strdup("content-length"), g_strdup_printf("%u", msg->content->len));
	GHashTableIter iter;
	gchar *key, *value;
	g_hash_table_iter_init(&iter, msg->headers);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
		g_string_append_printf(msg->raw, "%s:%s\n", key, value);

	g_string_append(msg->raw, "\n");

	/* content */
	if (msg->content->len)
		g_string_append(msg->raw, msg->content->str);

	msg->parsed = TRUE;
	msg->parsed_headers = TRUE;
}
GebrCommHttpMsg *gebr_comm_http_msg_new_request(GebrCommHttpRequestMethod method, const gchar *url, GHashTable * headers, const gchar *content)
{
	GebrCommHttpMsg *msg = gebr_comm_http_msg_new(GEBR_COMM_HTTP_TYPE_REQUEST, method);
	g_string_assign(msg->url, url);
	gsize len;
	if (content && (len = strlen(content))) {
		g_string_assign(msg->content, content);
		g_hash_table_insert(msg->headers, g_strdup("content-length"), g_strdup_printf("%u", len));
	}
	if (headers) {
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, headers);
		while (g_hash_table_iter_next(&iter, &key, &value))
			g_hash_table_insert(msg->headers, g_strdup(key), g_strdup(value));
	}

	gebr_comm_http_msg_serialize(msg);

	return msg;
}
GebrCommHttpMsg *gebr_comm_http_msg_new_response(gint status_code, GHashTable * headers, const gchar *content)
{
	GebrCommHttpMsg *msg = gebr_comm_http_msg_new(GEBR_COMM_HTTP_TYPE_RESPONSE, GEBR_COMM_HTTP_METHOD_UNKNOWN);
	msg->status_code = status_code;
	gsize len;
	if (content && (len = strlen(content))) {
		g_string_assign(msg->content, content);
		g_hash_table_insert(msg->headers, g_strdup("content-length"), g_strdup_printf("%u", len));
	}
	if (headers) {
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, headers);
		while (g_hash_table_iter_next(&iter, &key, &value))
			g_hash_table_insert(msg->headers, g_strdup(key), g_strdup(value));
	}

	gebr_comm_http_msg_serialize(msg);

	return msg;
}
