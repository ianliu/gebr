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

	gchar *group, *group_type;
	gebr_flow_edition_get_current_group(&group, &group_type);

	g_debug("must implement gebr_flow_edition_get_current_group");
	group = g_strdup("Foo Group. Must Change."); 
	group_type = g_strdup("1");

	gchar *submit_date = gebr_iso_date();
	gebr_geoxml_flow_set_date_last_run(gebr.flow, g_strdup(submit_date));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), FALSE, FALSE);

	gchar *xml;
	GebrJob *job = gebr_job_new(parent_rid);

	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(gebr.flow), &xml);
	GebrCommJsonContent *content = gebr_comm_json_content_new_from_string(xml);
	gchar *url = g_strdup_printf("/run?address=%s;parent_rid=%s;speed=%s;nice=%s;group=%s;type=%s;host=%s;temp_id=%s",
				     gebr_flow_edition_get_selected_server(gebr.ui_flow_edition),
				     parent_rid, speed_str, nice, group, group_type, hostname, gebr_job_get_id(job));

	GList *m = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(m->data);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, content);

	g_debug("Resta tratar informacoes do group_type e do grupo para inserir no gebr-job");
	gebr_job_set_hostname(job, hostname);
	gebr_job_set_exec_speed(job, speed);
	gebr_job_set_submit_date(job, submit_date);
	gebr_job_set_title(job, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.flow)));
	gebr_job_set_nice(job, nice);
	gebr_job_set_server_group(job, group);
	gebr_job_set_server_group_type(job, group_type);
	
	gebr_job_control_add(gebr.job_control, job);
	gebr_job_control_select_job(gebr.job_control, job);
	gebr_maestro_server_add_temporary_job(m->data, job);

	gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);
	
	g_free(url);
	g_free(xml);
	g_free(speed_str);
	g_free(nice);
	g_free(group);
	g_free(group_type);
}
