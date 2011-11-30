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

/* Private methods {{{1 */
/*
 * Gets the selected queue. Returns %TRUE if no queue was selected, %FALSE
 * otherwise.
 */
static gchar *
get_selected_queue(void)
{
//	GtkTreeIter queue_iter;
//	gchar *queue;
//
//	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &queue_iter))
//		return g_strdup("");
//	else {
//		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox));
//		gtk_tree_model_get(model, &queue_iter, SERVER_QUEUE_ID, &queue, -1);
//	}

	return g_strdup("");
}

static const gchar *
get_selected_server(void)
{
	GtkTreeIter iter;
	GtkComboBox *combo = GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(combo);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return "";

	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, &iter, 0, &daemon, -1);
	return gebr_daemon_server_get_address(daemon);
}

/* Public methods {{{1 */
void
gebr_ui_flow_run(void)
{
	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	gchar *parent_rid = get_selected_queue();
	gchar *speed = g_strdup_printf("%d", gebr_interface_get_execution_speed());
	gchar *nice = g_strdup_printf("%d", gebr_interface_get_niceness());

	gchar *group;
	gebr_geoxml_line_get_group(gebr.line, NULL, &group);

	gebr_geoxml_flow_set_date_last_run(gebr.flow, gebr_iso_date());
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), FALSE, FALSE);

	gchar *xml;
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(gebr.flow), &xml);
	GebrCommJsonContent *content = gebr_comm_json_content_new_from_string(xml);
	gchar *url = g_strdup_printf("/run?address=%s;parent_rid=%s;speed=%s;nice=%s;group=%s;host=%s",
				     get_selected_server(), parent_rid, speed, nice, group,
				     g_get_host_name());

	GebrCommServer *server = gebr_maestro_server_get_server(gebr.ui_server_list->maestro);
	gebr_comm_protocol_socket_send_request(server->socket,
					       GEBR_COMM_HTTP_METHOD_PUT, url, content);
	g_free(url);
	g_free(xml);

	gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);

	g_free(parent_rid);
	g_free(speed);
	g_free(nice);
	g_free(group);
}
