/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */
#include "ui_flow.h"

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/date.h>
#include <glib/gi18n.h>
#include "gebr.h"
#include "ui_flow_browse.h"
#include "document.h"

static gboolean
is_group_connected(GtkTreeModel *model,
		   const gchar *group)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		if (gebr_daemon_server_has_tag(daemon, group))
			if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_CONNECT)
				return TRUE;
	}

	return FALSE;
}

static gboolean
is_address_connected(GtkTreeModel *model,
		     const gchar *address)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		const gchar *addr = gebr_daemon_server_get_address(daemon);
		if (g_strcmp0(addr, address) == 0)
			return gebr_daemon_server_get_state(daemon) == SERVER_STATE_CONNECT;
	}
	return FALSE;
}

static gboolean
has_connected_server(GebrMaestroServer *maestro,
		     GebrMaestroServerGroupType type,
		     const gchar *name)
{
	gboolean result = FALSE;
	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);

	switch (type)
	{
	case MAESTRO_SERVER_TYPE_GROUP:
		result = is_group_connected(model, name);
		break;
	case MAESTRO_SERVER_TYPE_DAEMON:
		result = is_address_connected(model, name);
		break;
	}

	g_object_unref(model);

	return result;
}

static const gchar *
run_flow(GebrGeoXmlFlow *flow,
	 const gchar *after)
{
	if (!flow_browse_get_selected(NULL, TRUE))
		return NULL;

	const gchar *parent_rid = gebr_flow_edition_get_selected_queue(gebr.ui_flow_edition);
	gint speed = gebr_interface_get_execution_speed();
	gchar *speed_str = g_strdup_printf("%d", speed);
	gchar *nice = g_strdup_printf("%d", gebr_interface_get_niceness());
	const gchar *hostname = g_get_host_name();

	GebrMaestroServerGroupType type;
	gchar *name, *host = NULL;
	gebr_flow_edition_get_current_group(gebr.ui_flow_edition, &type, &name);

	if (type == MAESTRO_SERVER_TYPE_DAEMON)
		gebr_flow_edition_get_server_hostname(gebr.ui_flow_edition, &host);

	const gchar *group_type = gebr_maestro_server_group_enum_to_str(type);
	gebr_geoxml_flow_server_set_group(flow, group_type, name);

	gchar *submit_date = gebr_iso_date();

	gebr_geoxml_flow_set_date_last_run(flow, g_strdup(submit_date));
	document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);

	gchar *xml;
	GebrJob *job = gebr_job_new(parent_rid);

	GebrGeoXmlDocument *clone = gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow));

	gebr_geoxml_document_merge_dicts(gebr.validator,
	                                 clone,
	                                 GEBR_GEOXML_DOCUMENT(gebr.line),
	                                 GEBR_GEOXML_DOCUMENT(gebr.project),
	                                 NULL);

	gebr_geoxml_document_to_string(clone, &xml);
	GebrCommJsonContent *content = gebr_comm_json_content_new_from_string(xml);
	gebr_geoxml_document_unref(clone);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

	if (!has_connected_server(maestro, type, name)) {
		gchar *msg;
		switch (type) { case MAESTRO_SERVER_TYPE_GROUP:
			msg = g_strdup_printf(_("There are no connected servers on group %s."), name);
			break;
		case MAESTRO_SERVER_TYPE_DAEMON:
			msg = g_strdup_printf(_("The selected server (%s) is not connected."), name);
			break;
		}

		GtkWidget *dialog  = gtk_message_dialog_new_with_markup(GTK_WINDOW(gebr.window),
									GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                                                        "<span size='large' weight='bold'>%s</span>", msg);
		g_free(msg);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/run");
	gebr_comm_uri_add_param(uri, "gid", gebr_get_session_id());

	if (after)
		gebr_comm_uri_add_param(uri, "temp_parent", after);
	else
		gebr_comm_uri_add_param(uri, "parent_id", parent_rid);

	gebr_comm_uri_add_param(uri, "speed", speed_str);
	gebr_comm_uri_add_param(uri, "nice", nice);
	gebr_comm_uri_add_param(uri, "name", name);
	if (host)
		gebr_comm_uri_add_param(uri, "server-hostname", host);
	gebr_comm_uri_add_param(uri, "group_type", group_type);
	gebr_comm_uri_add_param(uri, "host", hostname);
	gebr_comm_uri_add_param(uri, "temp_id", gebr_job_get_id(job));
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(server->socket, GEBR_COMM_HTTP_METHOD_PUT, url, content);

	gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
	gebr_job_set_hostname(job, hostname);
	gebr_job_set_exec_speed(job, speed);
	gebr_job_set_submit_date(job, submit_date);
	gebr_job_set_title(job, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_job_set_nice(job, nice);
	if (host)
		gebr_job_set_server_group(job, host);
	else
		gebr_job_set_server_group(job, name);
	gebr_job_set_server_group_type(job, group_type);
	
	gebr_job_control_add(gebr.job_control, job);
	gebr_job_control_select_job(gebr.job_control, job);
	gebr_maestro_server_add_temporary_job(maestro, job);

	gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);
	
	g_free(name);
	g_free(url);
	g_free(xml);
	g_free(speed_str);
	g_free(nice);

	return gebr_job_get_id(job);
}

void
gebr_ui_flow_run(gboolean is_parallel)
{
	GList *rows;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	const gchar *id = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;
		GebrGeoXmlFlow *flow;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, FB_XMLPOINTER, &flow, -1);

		g_debug("Running after %s", id);

		if (is_parallel)
			id = run_flow(flow, NULL);
		else
			id = run_flow(flow, id);

		if (!id)
			return;
	}
}
