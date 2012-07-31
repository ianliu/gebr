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
#include <libgebr/utils.h>
#include <glib/gi18n.h>
#include "gebr.h"
#include "ui_flow_browse.h"
#include "document.h"
#include "flow.h"

gchar *
get_line_paths(GebrGeoXmlLine *line)
{
	GebrGeoXmlSequence *seq;
	GString *buf = g_string_new(NULL);

	gebr_geoxml_line_get_path(line, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		gchar *name = gebr_geoxml_line_path_get_name((GebrGeoXmlLinePath*)seq);
		if (g_strcmp0(name, "IMPORT") == 0 || g_strcmp0(name, "HOME") == 0)
			continue;
		g_string_append_c(buf, ',');
		GebrGeoXmlValueSequence *path = GEBR_GEOXML_VALUE_SEQUENCE(seq);
		gchar *value = gebr_geoxml_value_sequence_get(path);

		gchar *rel_value;
		gchar ***pvector = gebr_geoxml_line_get_paths(line);

		rel_value = gebr_resolve_relative_path(value, pvector);

		g_free(value);
		gebr_pairstrfreev(pvector);

		g_string_append(buf, rel_value);
	}

	if (buf->len)
		g_string_erase(buf, 0, 1);

	return g_string_free(buf, FALSE);
}

static gboolean
is_group_connected(GtkTreeModel *model,
		   const gchar *group)
{
	GtkTreeIter iter;
	GebrDaemonServer *daemon;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter, 0, &daemon, -1);
		if (gebr_daemon_server_has_tag(daemon, group))
			if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
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
			return gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED;
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

static void
gebr_ui_flow_update_mpi_nprocess(GebrGeoXmlFlow *flow,
                                 GebrMaestroServer *maestro,
                                 gdouble speed,
                                 const gchar *group_name,
                                 GebrMaestroServerGroupType group_type)
{
	GebrGeoXmlSequence *seq;

	gebr_geoxml_flow_get_program(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlProgram *prog = GEBR_GEOXML_PROGRAM(seq);
		gebr_ui_flow_update_prog_mpi_nprocess(prog, maestro, speed, group_name, group_type);
	}
}

void
gebr_ui_flow_update_prog_mpi_nprocess(GebrGeoXmlProgram *prog,
                                      GebrMaestroServer *maestro,
                                      gdouble speed,
                                      const gchar *group_name,
                                      GebrMaestroServerGroupType group_type)
{
	const gchar *mpi = gebr_geoxml_program_get_mpi(prog);

	if (!*mpi)
		return;

	gint ncores = gebr_maestro_server_get_ncores_for_group(maestro, mpi, group_name, group_type);
	gint nprocs = gebr_calculate_number_of_processors(ncores, speed);
	gebr_geoxml_program_mpi_set_n_process(prog, nprocs);
}

static const gchar *
run_flow(GebrGeoXmlFlow *flow,
	 const gchar *after,
	 const gchar *snapshot_id)
{
	gchar *snapshot_title = NULL;
	if (!flow_browse_get_selected(NULL, TRUE))
		return NULL;

	const gchar *parent_rid = gebr_flow_edition_get_selected_queue(gebr.ui_flow_edition);

	gdouble speed;
	if (!gebr_geoxml_flow_is_single_core(flow, gebr.validator))
		speed = gebr_interface_get_execution_speed();
	else
		speed = 0.0;

	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
	if (snapshot_id && *snapshot_id) {
		GebrGeoXmlRevision *snapshot = gebr_geoxml_flow_get_revision_by_id(gebr.flow, snapshot_id);
		gebr_geoxml_flow_get_revision_data(snapshot, NULL, NULL, &snapshot_title, NULL);
		gebr_geoxml_document_ref(GEBR_GEOXML_DOCUMENT(snapshot));
	}

	gchar *speed_str = g_strdup_printf("%lf", speed);
	gchar *nice = g_strdup_printf("%d", gebr_interface_get_niceness());

	const gchar *hostname = g_get_host_name();

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

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

	const gchar *run_type;
	if (gebr_geoxml_flow_get_first_mpi_program(flow) == NULL)
		run_type = "normal";
	else
		run_type = "mpi";

	gchar *xml;
	GebrJob *job = gebr_job_new(parent_rid, run_type);

	GebrGeoXmlDocument *clone = gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow));

	gebr_ui_flow_update_mpi_nprocess(GEBR_GEOXML_FLOW(clone), maestro, speed, name, type);

	void func(GString *path, gpointer data)
	{
		gchar ***paths = data;
		gchar *tmp = gebr_resolve_relative_path(path->str, paths);
		g_string_assign(path, tmp);
		g_free(tmp);
	}
	gchar ***tmp = gebr_geoxml_line_get_paths(gebr.line);
	gebr_flow_modify_paths(GEBR_GEOXML_FLOW(clone), func, FALSE, tmp);
	gebr_pairstrfreev(tmp);

	gebr_geoxml_document_merge_dicts(gebr.validator,
	                                 clone,
	                                 GEBR_GEOXML_DOCUMENT(gebr.line),
	                                 GEBR_GEOXML_DOCUMENT(gebr.project),
	                                 NULL);

	gebr_geoxml_document_to_string(clone, &xml);
	GebrCommJsonContent *content = gebr_comm_json_content_new_from_string(xml);
	gebr_geoxml_document_unref(clone);

	if (!has_connected_server(maestro, type, name)) {
		gchar *new_name;
		gchar *msg = NULL;
		switch (type) {
		case MAESTRO_SERVER_TYPE_GROUP:
			if (!strlen(name))
				new_name = g_strdup_printf("Maestro %s", gebr_maestro_server_get_address(maestro));
			else
				new_name = g_strdup(name);

			msg = g_markup_printf_escaped(_("<span size='large' weight='bold'>Execution error</span>\n\n"
							"There are no connected nodes on group <b>%s</b>."), new_name);
			g_free(new_name);
			break;
		case MAESTRO_SERVER_TYPE_DAEMON:
			msg = g_markup_printf_escaped(_("<span size='large' weight='bold'>Execution error</span>\n\n"
							"The selected node (<b>%s</b>) is not connected."), name);
			break;
		default:
			msg = g_strdup("");
			break;
		}

		GtkWidget *dialog  = gtk_message_dialog_new_with_markup(GTK_WINDOW(gebr.window),
									GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		                                                        NULL);

		gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);

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

	gebr_comm_uri_add_param(uri, "flow_id", flow_id);
	gebr_comm_uri_add_param(uri, "speed", speed_str);
	gebr_comm_uri_add_param(uri, "nice", nice);
	gebr_comm_uri_add_param(uri, "name", name);

	if (host)
		gebr_comm_uri_add_param(uri, "server-hostname", host);

	gebr_comm_uri_add_param(uri, "group_type", group_type);
	gebr_comm_uri_add_param(uri, "host", hostname);
	gebr_comm_uri_add_param(uri, "temp_id", gebr_job_get_id(job));

	gchar *paths = get_line_paths(gebr.line);
	gebr_comm_uri_add_param(uri, "paths", paths);
	gebr_comm_uri_add_param(uri, "snapshot_title", snapshot_title ? snapshot_title : "");
	gebr_comm_uri_add_param(uri, "snapshot_id", snapshot_id ? snapshot_id : "");

	//g_debug("------SENDING----------- ....On '%s', line '%d', snapshot_title:'%s', snapshot_id:'%s', flow_id:'%s'",
		//__FILE__, __LINE__, snapshot_title, snapshot_id, flow_id);

	g_free(paths);

	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(server->socket, GEBR_COMM_HTTP_METHOD_PUT, url, content);

	gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
	gebr_job_set_hostname(job, hostname);
	gebr_job_set_exec_speed(job, speed);
	gebr_job_set_submit_date(job, submit_date);
	gebr_job_set_title(job, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_job_set_nice(job, nice);
	gebr_job_set_flow_id(job, flow_id);
	gebr_job_set_snapshot_title(job, snapshot_title ? snapshot_title : "");
	gebr_job_set_snapshot_id(job, snapshot_id ? snapshot_id : "");

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

		/* Executing snapshots */
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook));
		if(i->next == NULL && current_page == NOTEBOOK_PAGE_FLOW_BROWSE) {
			if (gebr_geoxml_flow_get_revisions_number(flow) > 0) {
				const gchar *filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));
				if (g_list_find_custom(gebr.ui_flow_browse->select_flows, filename, (GCompareFunc)g_strcmp0)) {
					gchar *str = g_strdup_printf("run\b%s\n", is_parallel? "parallel" : "single");
					GString *action = g_string_new(str);

					if (gebr_comm_process_write_stdin_string(gebr.ui_flow_browse->graph_process, action) == 0)
						g_debug("Fail to run!");

					g_free(str);
					g_string_free(action, TRUE);
					return;
				}
			}
		}

		if (is_parallel)
			id = run_flow(flow, NULL, NULL);
		else
			id = run_flow(flow, id, NULL);

		if (!id)
			return;
	}
}

void
gebr_ui_flow_run_snapshots(GebrGeoXmlFlow *flow,
                           const gchar *snapshots,
                           gboolean is_parallel)
{
	GebrGeoXmlRevision *rev;
	gchar **snaps = g_strsplit(snapshots, ",", -1);
	const gchar *id = NULL;

	for (gint i = 0; snaps[i]; i++) {
		GebrGeoXmlDocument *snap_flow;
		gchar *xml;
		gchar *comment = NULL;
		gchar *snapshot_id = NULL;

		if (!g_strcmp0(snaps[i], "head")) {
			snap_flow = GEBR_GEOXML_DOCUMENT(flow);
		} else {
			rev = gebr_geoxml_flow_get_revision_by_id(flow, snaps[i]);
			gebr_geoxml_flow_get_revision_data(rev, &xml, NULL, &comment, &snapshot_id);

			if (gebr_geoxml_document_load_buffer(&snap_flow, xml) != GEBR_GEOXML_RETV_SUCCESS) {
				g_warn_if_reached();
				return;
			}

			const gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(snap_flow));

			gebr_geoxml_document_set_title(GEBR_GEOXML_DOCUMENT(snap_flow), flow_title);
			gebr_geoxml_document_set_description(GEBR_GEOXML_DOCUMENT(snap_flow), comment);

			g_free(xml);
			g_free(comment);
		}


		if (is_parallel)
			id = run_flow(GEBR_GEOXML_FLOW(snap_flow),
				      NULL, snapshot_id);
		else
			id = run_flow(GEBR_GEOXML_FLOW(snap_flow),
				      id, snapshot_id);

		g_free(snapshot_id);

		if (!id)
			return;
	}
	g_strfreev(snaps);
}
