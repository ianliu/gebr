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
#include "gebr.h"
#include "ui_flow_browse.h"
#include "document.h"

void
gebr_ui_flow_run(void)
{
	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	const gchar *parent_rid = gebr_flow_edition_get_selected_queue(gebr.ui_flow_edition);
	gint speed = gebr_interface_get_execution_speed();
	gchar *speed_str = g_strdup_printf("%d", speed);
	gchar *nice = g_strdup_printf("%d", gebr_interface_get_niceness());
	const gchar *hostname = g_get_host_name();

	GebrMaestroServerGroupType type;
	gchar *name;
	gebr_flow_edition_get_current_group(gebr.ui_flow_edition, &type, &name);
	const gchar *group_type = gebr_maestro_server_group_enum_to_str(type);
	gebr_geoxml_flow_server_set_group(gebr.flow, group_type, name);

	gchar *submit_date = gebr_iso_date();

	gebr_geoxml_flow_set_date_last_run(gebr.flow, g_strdup(submit_date));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), FALSE, FALSE);

	gchar *xml;
	GebrJob *job = gebr_job_new(parent_rid);

	GebrGeoXmlDocument *clone = gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(gebr.flow));

	gebr_geoxml_document_merge_dicts(gebr.validator,
	                                 clone,
	                                 GEBR_GEOXML_DOCUMENT(gebr.line),
	                                 GEBR_GEOXML_DOCUMENT(gebr.project),
	                                 NULL);

	gebr_geoxml_document_to_string(clone, &xml);
	GebrCommJsonContent *content = gebr_comm_json_content_new_from_string(xml);
	gebr_geoxml_document_unref(clone);

	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/run");
	gebr_comm_uri_add_param(uri, "parent_id", parent_rid);
	gebr_comm_uri_add_param(uri, "speed", speed_str);
	gebr_comm_uri_add_param(uri, "nice", nice);
	gebr_comm_uri_add_param(uri, "name", name);
	gebr_comm_uri_add_param(uri, "group_type", group_type);
	gebr_comm_uri_add_param(uri, "host", hostname);
	gebr_comm_uri_add_param(uri, "temp_id", gebr_job_get_id(job));
	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	/*
	gchar *url = g_strdup_printf("/run?parent_rid=%s;speed=%s;nice=%s;name=%s;type=%s;host=%s;temp_id=%s",
				     parent_rid,
				     speed_str,
				     nice,
				     name,
				     group_type,
				     hostname,
				     gebr_job_get_id(job));
				     */

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

	gboolean has_servers = FALSE;
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		if(gebr_daemon_server_get_state(daemon) == SERVER_STATE_CONNECT) {
			has_servers = TRUE;
			break;
		}
	}
	if (!has_servers) {
		GtkWidget *dialog  = gtk_message_dialog_new_with_markup(GTK_WINDOW(gebr.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                                                        "<span size='large' weight='bold'>There are no connected servers on maestro %s.</span>",
		                                                        gebr_maestro_server_get_display_address(maestro));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	gebr_comm_protocol_socket_send_request(server->socket, GEBR_COMM_HTTP_METHOD_PUT, url, content);

	gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
	gebr_job_set_hostname(job, hostname);
	gebr_job_set_exec_speed(job, speed);
	gebr_job_set_submit_date(job, submit_date);
	gebr_job_set_title(job, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.flow)));
	gebr_job_set_nice(job, nice);
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
}
