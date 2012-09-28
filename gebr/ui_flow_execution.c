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
#include "ui_flow_execution.h"

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <glib/gi18n.h>
#include "gebr.h"
#include "ui_flow_browse.h"
#include "document.h"
#include "flow.h"
#include "callbacks.h"

#define SLIDER_MAX 8.0
#define SLIDER_100 5.0
#define VALUE_MAX 20.0

struct _GebrUiFlowExecutionPriv {
	GtkWidget *nice_button_high;
	GtkWidget *speed_slider;
	GtkAdjustment *speed_adjustment;
	GtkComboBox *server_combo;
	GtkComboBox *queue_combo;
	GtkToggleButton *parallelism_button;
	GtkToggleButton *save_default_button;
	GtkWidget *window;
	GtkLabel *number_cores_label;
	gdouble exec_speed;
};

G_DEFINE_TYPE(GebrUiFlowExecution, gebr_ui_flow_execution, G_TYPE_OBJECT);

static void
gebr_ui_flow_execution_finalize(GObject *object)
{
	G_OBJECT_CLASS(gebr_ui_flow_execution_parent_class)->finalize(object);
}

static void
gebr_ui_flow_execution_init(GebrUiFlowExecution *ui_flow_execution)
{
	ui_flow_execution->priv = G_TYPE_INSTANCE_GET_PRIVATE(ui_flow_execution,
	                                                      GEBR_TYPE_UI_FLOW_EXECUTION,
	                                                      GebrUiFlowExecutionPriv);
}

static void
gebr_ui_flow_execution_class_init(GebrUiFlowExecutionClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_ui_flow_execution_finalize;

	g_type_class_add_private(klass, sizeof(GebrUiFlowExecutionPriv));
}

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

static void
modify_paths_func(GString *path, gpointer data)
{
	gchar ***paths = data;
	gchar *tmp = gebr_resolve_relative_path(path->str, paths);
	g_string_assign(path, tmp);
	g_free(tmp);
}

static const gchar *
run_flow(GebrUiFlowExecution *ui_flow_execution,
         GebrGeoXmlFlow *flow,
         const gchar *after,
         const gchar *snapshot_id,
         gboolean is_detailed)
{
	gchar *snapshot_title = NULL;
	if (!flow_browse_get_selected(NULL, TRUE))
		return NULL;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	const gchar *parent_rid = is_detailed ? gebr_ui_flow_execution_get_selected_queue(ui_flow_execution->priv->queue_combo, maestro) : "";

	gdouble speed;
	if (!gebr_geoxml_flow_is_single_core(flow, gebr.validator)) {
		gdouble slider_value = is_detailed? gtk_adjustment_get_value(ui_flow_execution->priv->speed_adjustment) : gebr.config.flow_exec_speed;
		gdouble value = gebr_ui_flow_execution_calculate_speed_from_slider_value(slider_value);
		speed = is_detailed ? value : gebr_interface_get_execution_speed();
	} else
		speed = 0.0;

	gchar *submit_date = gebr_iso_date();

	const gchar *flow_id;
	if (snapshot_id && *snapshot_id) {
		flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
		GebrGeoXmlRevision *snapshot = gebr_geoxml_flow_get_revision_by_id(gebr.flow, snapshot_id);
		gebr_geoxml_flow_get_revision_data(snapshot, NULL, NULL, &snapshot_title, NULL);
		gebr_geoxml_document_ref(GEBR_GEOXML_DOCUMENT(snapshot));
	} else {
		flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));
		gebr_geoxml_flow_set_date_last_run(flow, g_strdup(submit_date));
		document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);
	}

	gint niceness;

	if (is_detailed) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_flow_execution->priv->nice_button_high)))
			niceness = 0;
		else
			niceness = 19;
	} else
		niceness =  gebr_interface_get_niceness();


	gchar *speed_str = g_strdup_printf("%lf", speed);
	gchar *nice =  g_strdup_printf("%d", niceness);

	const gchar *hostname = g_get_host_name();

	GebrCommServer *server = gebr_maestro_server_get_server(maestro);

	GebrMaestroServerGroupType type;
	gchar *name, *host = NULL;

	if (is_detailed) {
		gebr_ui_flow_execution_get_current_group(ui_flow_execution->priv->server_combo,
		                                         &type,
		                                         &name,
		                                         maestro);
	} else {
		name = g_strdup(gebr.config.execution_server_name->str);
		type = (GebrMaestroServerGroupType) gebr.config.execution_server_type;
	}


	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		if (is_detailed) {
			gebr_ui_flow_execution_get_server_hostname(ui_flow_execution->priv->server_combo, maestro, &host);
		} else {
			gboolean find_host = FALSE;
			GtkTreeIter iter;
			GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);
			gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
			while (valid && !find_host) {
				gchar *name, *hostname;
				GebrMaestroServerGroupType type;

				gtk_tree_model_get(model, &iter,
				                   MAESTRO_SERVER_TYPE, &type,
				                   MAESTRO_SERVER_NAME, &name,
				                   MAESTRO_SERVER_HOST, &hostname,
				                   -1);

				if (type == gebr.config.execution_server_type &&
						!g_strcmp0(name, gebr.config.execution_server_name->str)) {
					find_host = TRUE;
					host = hostname;
				}
				g_free(name);
				if (!find_host)
					g_free(hostname);
				valid = gtk_tree_model_iter_next(model, &iter);
			}
		}
	}

	const gchar *group_type = gebr_maestro_server_group_enum_to_str(type);
	gebr_geoxml_flow_server_set_group(flow, group_type, name);

	const gchar *run_type;
	if (gebr_geoxml_flow_get_first_mpi_program(flow) == NULL)
		run_type = "normal";
	else
		run_type = "mpi";

	gchar *xml;
	GebrJob *job = gebr_job_new(parent_rid, run_type);

	GebrGeoXmlDocument *clone = gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow));

	gebr_ui_flow_update_mpi_nprocess(GEBR_GEOXML_FLOW(clone), maestro, speed, name, type);

	gchar ***tmp = gebr_geoxml_line_get_paths(gebr.line);
	gebr_flow_modify_paths(GEBR_GEOXML_FLOW(clone), modify_paths_func, FALSE, tmp);
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

	g_free(paths);

	gchar *url = gebr_comm_uri_to_string(uri);
	gebr_comm_uri_free(uri);

	gebr_comm_protocol_socket_send_request(server->socket, GEBR_COMM_HTTP_METHOD_PUT, url, content);

	gebr_job_set_maestro_address(job, gebr_maestro_server_get_address(maestro));
	gebr_job_set_hostname(job, hostname);
	gebr_job_set_exec_speed(job, speed);
	gebr_job_set_submit_date(job, submit_date);
	gebr_job_set_title(job, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_job_set_description(job, gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT(flow)));
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
	gebr_maestro_server_add_temporary_job(maestro, job);
	gebr_job_control_select_job(gebr.job_control, job);
	flow_browse_info_update();

	g_free(name);
	g_free(url);
	g_free(xml);
	g_free(speed_str);
	g_free(nice);

	return gebr_job_get_id(job);
}

void
gebr_ui_flow_run(GebrUiFlowExecution *ui_flow_execution,
                 gboolean is_parallel,
                 gboolean is_detailed)
{
	GList *rows;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	const gchar *id = NULL;
	gint nrows;
	GebrUiFlowBrowseType type;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);
	nrows = gtk_tree_selection_count_selected_rows(selection);

	/*Verify if the selection has a program*/
	path = rows->data;
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
	                   FB_STRUCT_TYPE, &type,
	                   -1);

	if (nrows > 1 && type == STRUCT_TYPE_PROGRAM) {
		gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
		rows = gtk_tree_selection_get_selected_rows(selection, &model);
	}

	gint n = g_list_length(rows);

	for (GList *i = rows; i; i = i->next) {
		path = i->data;
		GebrGeoXmlFlow *flow;

		GebrUiFlow *ui_flow;

		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT_TYPE, &type,
		                   FB_STRUCT, &ui_flow,
		                   -1);

		if (type != STRUCT_TYPE_FLOW)
			flow = gebr.flow;
		else
			flow = gebr_ui_flow_get_flow(ui_flow);

		/* Executing snapshots */
		gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook));
		if(n == 1 && current_page == NOTEBOOK_PAGE_FLOW_BROWSE) {
			if (gebr_geoxml_flow_get_revisions_number(flow) > 0) {
				const gchar *filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));
				if (g_list_find_custom(gebr.ui_flow_browse->select_flows, filename, (GCompareFunc)g_strcmp0)) {
					gchar *str = g_strdup_printf("run\b%s\b%s\n",
					                             is_parallel? "parallel" : "single",
					                        		     is_detailed ? "detailed" : "default");
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
			id = run_flow(ui_flow_execution, flow, NULL, NULL, is_detailed);
		else
			id = run_flow(ui_flow_execution, flow, id, NULL, is_detailed);

		if (!id)
			continue;

		gebr_flow_browse_append_job_on_flow(flow, id, gebr.ui_flow_browse);

		gebr_flow_browse_update_jobs_info(flow, gebr.ui_flow_browse, gebr_flow_browse_calculate_n_max(gebr.ui_flow_browse));
	}

	if (n > 1 || gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook)) != NOTEBOOK_PAGE_FLOW_BROWSE)
		gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);
	else
		gebr_flow_browse_select_job_output(id, gebr.ui_flow_browse);

	/* Close dialog of Detailed execution */
	if (is_detailed && gtk_widget_get_visible(ui_flow_execution->priv->window))
		gtk_widget_destroy(ui_flow_execution->priv->window);

}

void
gebr_ui_flow_run_snapshots(GebrUiFlowExecution *ui_flow_execution,
                           GebrGeoXmlFlow *flow,
                           const gchar *snapshots,
                           gboolean is_parallel,
                           gboolean is_detailed)
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

		if (!flow_check_before_execution(GEBR_GEOXML_FLOW(snap_flow), TRUE)) {
			g_free(snapshot_id);
			continue;
		}

		if (is_parallel)
			id = run_flow(ui_flow_execution,
			              GEBR_GEOXML_FLOW(snap_flow),
			              NULL, snapshot_id, is_detailed);
		else
			id = run_flow(ui_flow_execution,
			              GEBR_GEOXML_FLOW(snap_flow),
			              id, snapshot_id, is_detailed);

		gebr_flow_browse_append_job_on_flow(flow, id, gebr.ui_flow_browse);

		g_free(snapshot_id);

		if (!id)
			continue;
	}
	gebr_flow_browse_update_jobs_info(flow, gebr.ui_flow_browse,
	                                  gebr_flow_browse_calculate_n_max(gebr.ui_flow_browse));

	gebr_flow_browse_select_job_output(id, gebr.ui_flow_browse);

	gchar *submit_date = gebr_iso_date();

	gebr_geoxml_flow_set_date_last_run(flow, g_strdup(submit_date));
	document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);
	flow_browse_info_update();

	/* Close dialog of Detailed execution */
	if (is_detailed && gtk_widget_get_visible(ui_flow_execution->priv->window))
		gtk_widget_destroy(ui_flow_execution->priv->window);

	g_strfreev(snaps);
}

gdouble
gebr_ui_flow_execution_calculate_speed_from_slider_value(gdouble x)
{
	if (x > SLIDER_100)
		return (VALUE_MAX - SLIDER_100) / (SLIDER_MAX - SLIDER_100) * (x - SLIDER_100) + SLIDER_100;
	else
		return x;
}

gdouble
gebr_ui_flow_execution_calculate_slider_from_speed (gdouble speed)
{
	if (speed > SLIDER_100)
		return (SLIDER_MAX - SLIDER_100) * (speed - SLIDER_100) / (VALUE_MAX - SLIDER_100) + SLIDER_100;
	else
		return speed;
}

static gboolean
change_value(GtkRange *range, GtkScrollType scroll, gdouble value)
{
	GtkAdjustment *adj = gtk_range_get_adjustment(range);
	gdouble min, max;

	min = gtk_adjustment_get_lower(adj);
	max = gtk_adjustment_get_upper(adj);

	gdouble speed = CLAMP (value, min, max);

	gtk_adjustment_set_value(adj, speed);

	return TRUE;
}

const gchar *
gebr_ui_flow_execution_set_text_for_performance(gdouble value)
{
	if (value <= 0.1)
		return _(g_strdup_printf("1 process"));
	else if (value < (SLIDER_100))
		return _(g_strdup_printf("%.0lf%% of nominal capacity", value*20));
	else if (value <= 400)
		return _(g_strdup_printf("%.0lf%% of nominal capacity", value*100 - 400));
	else
		g_return_val_if_reached(NULL);
}

static void
on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
                                           GtkCellRenderer *cell,
                                           GtkTreeModel    *tree_model,
                                           GtkTreeIter     *iter,
                                           gpointer         data)
{
	gchar *name;
	GebrMaestroServerGroupType type;

	gtk_tree_model_get(tree_model, iter,
	                   MAESTRO_SERVER_TYPE, &type,
	                   MAESTRO_SERVER_NAME, &name,
	                   -1);

	GebrDaemonServer *daemon = NULL;
	GebrMaestroServer *maestro;
	gboolean is_connected = TRUE;

	maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
	                                                       gebr.line);

	if (!maestro) {
		if (GTK_IS_CELL_RENDERER_TEXT(cell))
			g_object_set(cell, "text", "", NULL);
		else
			g_object_set(cell, "stock-id", NULL, NULL);
		return;
	}

	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		daemon = gebr_maestro_server_get_daemon(maestro, name);
		if (!daemon)
			return;
		is_connected = gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED;
	}

	if (GTK_IS_CELL_RENDERER_TEXT(cell)) {
		const gchar *txt;

		if (type == MAESTRO_SERVER_TYPE_GROUP) {
			if (name && *name)
				txt = name;
			else
				txt = _("Choose automatically");
		} else  {
			txt = gebr_daemon_server_get_hostname(daemon);
			if (!txt || !*txt)
				txt = gebr_daemon_server_get_address(daemon);
		}
		g_object_set(cell, "text", txt, NULL);
	} else {
		const gchar *stock_id;
		if (type == MAESTRO_SERVER_TYPE_GROUP)
			stock_id = "group";
		else {
			if (is_connected)
				stock_id = GTK_STOCK_CONNECT;
			else
				stock_id = GTK_STOCK_DISCONNECT;
		}

		g_object_set(cell, "stock-id", stock_id, NULL);
	}

	g_object_set(cell, "sensitive", is_connected, NULL);
}

static gboolean
speed_controller_query_tooltip(GtkWidget  *widget,
                               gint        x,
                               gint        y,
                               gboolean    keyboard_mode,
                               GtkTooltip *tooltip,
                               gpointer    user_data)
{
	GtkRange *scale = GTK_RANGE(widget);
	gdouble value = gtk_range_get_value(scale);
	const gchar *text_tooltip;
	text_tooltip = gebr_ui_flow_execution_set_text_for_performance(value);
	gtk_tooltip_set_text (tooltip, text_tooltip);
	return TRUE;
}

static void
on_queue_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer         data)
{
	GebrJob *job;
	gchar *name_queue;

	gtk_tree_model_get(tree_model, iter, 0, &job, -1);

	if (!job)
		name_queue = g_strdup(_("Immediately"));
	else
		name_queue = g_strdup_printf(_("After %s #%s"),
		                             gebr_job_get_title(job),
		                             gebr_job_get_job_counter(job));

	g_object_set(cell, "text", name_queue, NULL);
	g_free(name_queue);
}

void
gebr_ui_flow_execution_get_server_hostname(GtkComboBox *combo,
                                           GebrMaestroServer *maestro,
                                           gchar **host)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
	                   MAESTRO_SERVER_HOST, host,
	                   -1);
}

void
gebr_ui_flow_execution_get_current_group(GtkComboBox *combo,
                                         GebrMaestroServerGroupType *type,
                                         gchar **name,
                                         GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
	                   MAESTRO_SERVER_TYPE, type,
	                   MAESTRO_SERVER_NAME, name,
	                   -1);
}

const gchar *
gebr_ui_flow_execution_get_selected_queue(GtkComboBox *combo,
                                          GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return "";
	else {
		GebrJob *job;
		gtk_tree_model_get(model, &iter, 0, &job, -1);
		return job ? gebr_job_get_id(job) : "";
	}
}

void
gebr_ui_flow_execution_save_default(GebrUiFlowExecution *ui_flow_execution)
{
	/* Set server*/
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(ui_flow_execution->priv->server_combo, &iter)) {
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);

		gchar *name;
		GebrMaestroServerGroupType type;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_SERVER_TYPE, &type,
		                   MAESTRO_SERVER_NAME, &name,
		                   -1);

		gebr.config.execution_server_name = g_string_new(name);
		gebr.config.execution_server_type = type;
		g_free(name);
	}

	/* Set speed*/
	if (gtk_widget_get_sensitive(GTK_WIDGET(ui_flow_execution->priv->speed_slider)))
		gebr.config.flow_exec_speed = gebr_ui_flow_execution_calculate_speed_from_slider_value(gtk_adjustment_get_value(ui_flow_execution->priv->speed_adjustment));

	/* Set niceness*/
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_flow_execution->priv->nice_button_high));
	gebr.config.niceness = active ? 0 : 19;
}

void
on_execution_details_destroy (GtkWidget *widget,
                              gpointer   user_data)
{
	gtk_widget_destroy(widget);
}

void
on_cancel_button_clicked(GtkButton *button,
                         GtkWidget *window)
{
	gtk_widget_destroy(window);
}

static void
on_help_button_clicked(GtkButton *button,
                       gpointer pointer)
{
	gebr_gui_help_button_clicked("flows_browser_detailed_execution", NULL);
}

static void
on_run_button_clicked(GtkButton *button,
                      GebrUiFlowExecution *ui_flow_execution)
{
	if (gtk_toggle_button_get_active(ui_flow_execution->priv->save_default_button)) {
		gebr_ui_flow_execution_save_default(ui_flow_execution);
		gebr.config.save_preferences = TRUE;
	} else {
		gebr.config.save_preferences = FALSE;
	}

	gebr_ui_flow_run(ui_flow_execution, gtk_toggle_button_get_active(ui_flow_execution->priv->parallelism_button), TRUE);
}

static void 
on_show_scale(GtkWidget * scale,
              GebrUiFlowExecution *ui_flow_execution)
{
	gtk_range_set_value(GTK_RANGE(scale),
	                    gebr_ui_flow_execution_calculate_slider_from_speed(ui_flow_execution->priv->exec_speed));
}

static void
restore_execution_servers(GebrUiFlowExecution *ui_flow_execution, GebrMaestroServer *maestro)
{
	GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);
	GtkComboBox *combo = ui_flow_execution->priv->server_combo;
	GtkTreeIter iter;
	gchar *name = gebr.config.execution_server_name->str;
	gint type = gebr.config.execution_server_type;

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid)
	{
		GebrMaestroServerGroupType ttype;
		gchar *tname;

		gtk_tree_model_get(model, &iter,
		                   MAESTRO_SERVER_TYPE, &ttype,
		                   MAESTRO_SERVER_NAME, &tname,
		                   -1);
		if (g_strcmp0(tname, name) == 0 && ttype == type) {
			g_free(tname);
			break;
		}
		g_free(tname);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	if (valid)
		gtk_combo_box_set_active_iter(combo, &iter);
	else if (gtk_tree_model_get_iter_first(model, &iter))
		gtk_tree_model_get_iter_first(model, &iter);
}


static void
execution_details_restore_default_values(GebrUiFlowExecution *ui_flow_execution,
                                         GtkBuilder *builder,
                                         gboolean slider_sensitiviness)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "after_another_button")), TRUE);

	/* Speed */
	gdouble speed_value;
	GtkAdjustment *flow_exec_adjustment = ui_flow_execution->priv->speed_adjustment;

	if (!slider_sensitiviness) {
		GtkLabel *dispersion_label = GTK_LABEL(gtk_builder_get_object(builder, "dispersion_label"));
		gtk_label_set_markup(dispersion_label, _("This flow execution cannot be split."));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "dispersion_enclosure_box")),
					 FALSE);
		gtk_widget_set_sensitive(ui_flow_execution->priv->speed_slider, FALSE);
		speed_value = 0.0;
	} else {
		speed_value = gebr.config.flow_exec_speed;
		speed_value = gebr_ui_flow_execution_calculate_slider_from_speed(gebr.config.flow_exec_speed);
	}
	ui_flow_execution->priv->exec_speed = speed_value;
	gtk_adjustment_set_value(flow_exec_adjustment, speed_value);
	gtk_adjustment_value_changed(flow_exec_adjustment);

	/* Priority */
	if (gebr.config.niceness == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_flow_execution->priv->nice_button_high), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "low_priority_button")), TRUE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	/* Server */
	restore_execution_servers(ui_flow_execution, maestro);

	/* Queue */
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(ui_flow_execution->priv->queue_combo);
	if (gtk_tree_model_get_iter_first(model, &iter))
		gtk_combo_box_set_active_iter(ui_flow_execution->priv->queue_combo, &iter);

	/* Preferences*/
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_flow_execution->priv->save_default_button), gebr.config.save_preferences);
}

static void
speed_slider_setup_ui(GebrUiFlowExecution *ui_flow_execution,
                      gdouble speed)
{
	GtkAdjustment *flow_exec_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, SLIDER_MAX, 0.1, 1, 0.1));

	GtkWidget *scale = gtk_hscale_new(flow_exec_adjustment);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_scale_set_digits(GTK_SCALE(scale), 1);
	g_object_set(scale, "has-tooltip",TRUE, NULL);

	g_signal_connect(scale, "change-value", G_CALLBACK(change_value), NULL);
	g_signal_connect(scale, "query-tooltip", G_CALLBACK(speed_controller_query_tooltip), NULL);
	g_signal_connect(scale, "map", G_CALLBACK(on_show_scale), ui_flow_execution);

	ui_flow_execution->priv->speed_slider = GTK_WIDGET(scale);
	ui_flow_execution->priv->speed_adjustment = flow_exec_adjustment;
}

void
update_speed_slider(GtkScale *scale, gint ncores)
{
	gtk_scale_clear_marks(scale);

	gchar *half_cores = g_markup_printf_escaped("<span size='x-small'>%d</span>", ncores/2);
	gchar *total_cores = g_markup_printf_escaped("<span size='x-small' font_weight='bold'>%d</span>", ncores);
	gchar *over_cores = g_markup_printf_escaped("<span size='x-small'>%d</span>", 4*ncores);

	gdouble med = SLIDER_100 / 2.0;
	gtk_scale_add_mark(GTK_SCALE(scale), 0, GTK_POS_LEFT, "<span size='x-small'>1</span>");
	gtk_scale_add_mark(GTK_SCALE(scale), (med/2), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), med, GTK_POS_LEFT, half_cores);
	gtk_scale_add_mark(GTK_SCALE(scale), ((med+SLIDER_100)/2), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), SLIDER_100, GTK_POS_LEFT, total_cores);
	gtk_scale_add_mark(GTK_SCALE(scale), (SLIDER_100+1), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), (SLIDER_MAX-1), GTK_POS_LEFT, "");
	gtk_scale_add_mark(GTK_SCALE(scale), SLIDER_MAX, GTK_POS_LEFT, over_cores);

	g_free(half_cores);
	g_free(total_cores);
	g_free(over_cores);
}

static gint
get_number_of_cores(GtkWidget *servers_combo)
{
	gebr_maestro_controller_get_servers_model(gebr.maestro_controller);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	gint cores = 0;
	GtkTreeIter iter;
	gchar *name;
	GebrMaestroServerGroupType type;
	GtkTreeModel *servers_model = gtk_combo_box_get_model(GTK_COMBO_BOX(servers_combo));

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(servers_combo), &iter)) {
		gtk_tree_model_get(servers_model, &iter,
		                   MAESTRO_SERVER_NAME, &name,
		                   MAESTRO_SERVER_TYPE, &type,
		                   -1);

		cores = gebr_maestro_server_get_ncores_for_group(maestro, NULL,
		                                                 name, type);
	}
	return cores;
}

void
on_servers_combo_changed(GtkComboBox *widget,
                         GebrUiFlowExecution *ui_flow_execution)
{
	gint ncores = get_number_of_cores(GTK_WIDGET(ui_flow_execution->priv->server_combo));
	gchar *number_cores_markup = g_markup_printf_escaped(_("<small>The nominal capacity of this group of nodes is %d cores.</small>"),
	                                                     ncores);
	gtk_label_set_markup(ui_flow_execution->priv->number_cores_label, number_cores_markup);
	update_speed_slider(GTK_SCALE(ui_flow_execution->priv->speed_slider), ncores);
}

static GtkWidget *
create_detailed_execution_servers_combo(GebrUiFlowExecution *ui_flow_execution,
                                        GebrMaestroServer *maestro)
{
	/*Server combo*/
	GtkTreeModel *servers_model = gebr_maestro_server_get_groups_model(maestro);
	GtkWidget *servers_combo = gtk_combo_box_new_with_model(servers_model);
	ui_flow_execution->priv->server_combo = GTK_COMBO_BOX(servers_combo);
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();

	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(servers_combo), renderer, FALSE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(servers_combo), renderer,
	                                   on_server_disconnected_set_row_insensitive, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(servers_combo), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(servers_combo), renderer,
	                                   on_server_disconnected_set_row_insensitive, NULL, NULL);
	g_signal_connect(servers_combo, "changed", G_CALLBACK(on_servers_combo_changed), ui_flow_execution);
	return servers_combo;
}

static GtkWidget *
create_detailed_execution_queue_combo(GebrUiFlowExecution *ui_flow_execution,
                                       GebrMaestroServer *maestro)
{
	GtkTreeModel *queue_model_orig = gebr_maestro_server_get_queues_model(maestro);
	GtkTreeModel *queue_model = gebr_maestro_server_copy_queues_model(queue_model_orig);
	GtkWidget *queue_combo = gtk_combo_box_new_with_model(queue_model);
	ui_flow_execution->priv->queue_combo = GTK_COMBO_BOX(queue_combo);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(queue_combo), renderer, TRUE);

	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(queue_combo), renderer,
	                                   on_queue_set_text, NULL, NULL);
	return queue_combo;
}


static void
set_single_execution_labels(GtkBuilder *builder){
	gchar *order_text = g_markup_printf_escaped(
			_("Execute this flow <i>immediately</i> or <i>after another job</i>?"));
	gchar *server_text = g_markup_printf_escaped(
			_("In which set of nodes should this flow be executed?"));
	gchar *dispersion_text = g_markup_printf_escaped(
			_("The execution of this flow can be split to speed it up.\n"
					"Up to how many tasks should it be split?"));
	gchar *priority_text = g_markup_printf_escaped(
			_("How should the execution of this flow compete with\n"
			  "other processes?"));

	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "order_label")),
	                     order_text);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "servers_label")),
	                     server_text);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "dispersion_label")),
	                     dispersion_text);
	gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "priority_label")),
	                     priority_text);

	g_free(order_text);
	g_free(server_text);
	g_free(dispersion_text);
	g_free(priority_text);
}

static void
set_single_execution_dialog(GtkBuilder *builder)
{
	set_single_execution_labels(builder);
	GtkWidget *parallelism_box = GTK_WIDGET(gtk_builder_get_object(builder, "parallelism_box"));
	gtk_widget_hide(GTK_WIDGET(parallelism_box));
}

GebrUiFlowExecution *
gebr_ui_flow_execution_details_setup_ui(gboolean slider_sensitiviness,
                                        gboolean multiple)
{
	GtkBuilder *builder = gtk_builder_new();

	GebrUiFlowExecution *ui_flow_execution = g_object_new(GEBR_TYPE_UI_FLOW_EXECUTION, NULL);

	g_return_val_if_fail(gtk_builder_add_from_file(builder,
	                                               GEBR_GLADE_DIR "/gebr-execution-details.glade",
	                                               NULL), NULL);

	GtkWidget *main_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "execution_dialog"));
	ui_flow_execution->priv->window = main_dialog;

	GtkWidget *servers_box = GTK_WIDGET(gtk_builder_get_object(builder, "servers_box"));
	GtkWidget *order_box = GTK_WIDGET(gtk_builder_get_object(builder, "order_box"));
	GtkWidget *dispersion_box = GTK_WIDGET(gtk_builder_get_object(builder, "dispersion_box"));

	GtkWidget *run_button = GTK_WIDGET(gtk_builder_get_object(builder, "run_button"));
	GtkWidget *cancel_button = GTK_WIDGET(gtk_builder_get_object(builder, "cancel_button"));
	GtkWidget *help_button = GTK_WIDGET(gtk_builder_get_object(builder, "help_button"));

	ui_flow_execution->priv->save_default_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "save_default_button"));
	ui_flow_execution->priv->nice_button_high = GTK_WIDGET(gtk_builder_get_object(builder, "high_priority_button")); 
	ui_flow_execution->priv->parallelism_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "simultaneous_button"));
	ui_flow_execution->priv->number_cores_label = GTK_LABEL(gtk_builder_get_object(builder, "number_cores_label")); 

	gtk_window_set_title(GTK_WINDOW(main_dialog), _("Run"));

	speed_slider_setup_ui(ui_flow_execution, gebr.config.flow_exec_speed);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	GtkWidget *servers_combo = create_detailed_execution_servers_combo(ui_flow_execution, maestro);
	GtkWidget *queue_combo = create_detailed_execution_queue_combo(ui_flow_execution, maestro);

	gtk_box_pack_start(GTK_BOX(servers_box), servers_combo, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(order_box), queue_combo, FALSE, FALSE, 5);

	GtkSizeGroup *single_size_group = GTK_SIZE_GROUP(gtk_builder_get_object(builder, "single_size_group"));
	gtk_size_group_add_widget(single_size_group, GTK_WIDGET(servers_combo));
	gtk_size_group_add_widget(single_size_group, GTK_WIDGET(queue_combo));

	gtk_container_add(GTK_CONTAINER(dispersion_box), ui_flow_execution->priv->speed_slider);


	gtk_window_set_transient_for(GTK_WINDOW(main_dialog), GTK_WINDOW(gebr.window));

	gtk_window_set_position(GTK_WINDOW(main_dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(main_dialog), TRUE);
	gtk_widget_show_all(main_dialog);

	if(!multiple)
		set_single_execution_dialog(builder);

	execution_details_restore_default_values(ui_flow_execution, builder, slider_sensitiviness);

	g_signal_connect(main_dialog, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
	g_signal_connect(GTK_BUTTON(run_button), "clicked", G_CALLBACK(on_run_button_clicked), ui_flow_execution);
	g_signal_connect(GTK_BUTTON(cancel_button), "clicked", G_CALLBACK(on_cancel_button_clicked), main_dialog);
	g_signal_connect(GTK_BUTTON(help_button), "clicked", G_CALLBACK(on_help_button_clicked), NULL);

	return ui_flow_execution;
}
