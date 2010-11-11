/*   GeBR - An environment for seismic processing.
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

#include <unistd.h>
#include <string.h>

#include <glib.h>

#include <libgebr/intl.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/streamsocket.h>
#include <libgebr/comm/protocol.h>

#include "server.h"
#include "gebrclient.h"
#include "client.h"

/*
 * Internal functions
 */

static void server_log_message(enum gebr_log_message_type type, const gchar * message)
{
	gebr_client_message(type, message);
}

static GString *server_ssh_login(const gchar * title, const gchar * message)
{
	GString *password;

	password = g_string_new(getpass(message));

	return password;
}

static gboolean server_ssh_question(const gchar * title, const gchar * message)
{
	gchar answer[10];

	while (1) {
		gebr_client_message(GEBR_LOG_INFO, _("%s: %s"), title, message);
		fgets(answer, 10, stdin);
		if (!strcmp(answer, "yes\n"))
			return TRUE;
		else if (!strcmp(answer, "no\n"))
			return FALSE;
	}

	return FALSE;
}

static void server_disconnected(GebrCommStreamSocket * stream_socket, struct server *server)
{
//      TODO:

}

/*
 * Public functions
 */
/*
 * Function: server_new
 * Create a new server from _address_
 * Setup gebr_comm_server with its function pointer operators
 */
struct server *server_new(const gchar * address)
{
	static const struct gebr_comm_server_ops ops = {
		.log_message = server_log_message,
		.ssh_login = server_ssh_login,
		.ssh_question = server_ssh_question,
		.parse_messages = (typeof(ops.parse_messages)) client_parse_server_messages
	};
	struct server *server;

	server = g_new(struct server, 1);
	server->comm = gebr_comm_server_new(address, &ops);
	server->comm->user_data = server;

	g_signal_connect(server->comm->stream_socket, "disconnected", G_CALLBACK(server_disconnected), server);

	gebr_comm_server_connect(server->comm);

	return server;
}

void server_free(struct server *server)
{
	gebr_comm_server_free(server->comm);
	g_free(server);
}

gboolean server_run_flow(struct server *server, const gchar * flow_path)
{
	GebrGeoXmlDocument *document;
	GebrGeoXmlFlow *flow;
	int ret;

	ret = gebr_geoxml_document_load(&document, flow_path, TRUE, NULL);
	if (ret) {
		gebr_client_message(GEBR_LOG_ERROR, _("Could not load flow: %s"),
				    gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		return FALSE;
	}
	if (gebr_geoxml_document_get_type(document) != GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
		gebr_client_message(GEBR_LOG_ERROR, _("The document is not a flow."));
		return FALSE;
	}

	flow = GEBR_GEOXML_FLOW(document);
	//FIXME
	//gebr_comm_server_run_flow(server->comm, flow);

	return TRUE;
}
