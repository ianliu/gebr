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

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <libgebr/date.h>
#include <libgebr/gui/gui.h>
#include <libgebr/comm/gebr-comm.h>
#include <stdlib.h>

#include "ui_flow.h"
#include "gebr.h"
#include "gebr-job.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_server.h"
#include "ui_moab.h"

#include "gebr-task.h"

/* Prototypes {{{1 */
static void create_jobs_and_run(GebrCommRunner *runner);

/* Private methods {{{1 */
/*
 * flow_io_run_dialog:
 * @config: A pointer to a gebr_comm_server_run structure, which will hold the parameters entered within the dialog.
 * @server:
 * @mpi_program:
 */
static gboolean
flow_io_run_dialog(GebrCommRunner *config,
		   GebrServer     *server,
		   gboolean        mpi_program)
{
	gboolean ret = TRUE;
	GtkWidget *dialog;
	GtkWidget *box;
	GtkTreeIter iter;

	GtkWidget *cb_account = NULL;
	GtkWidget *entry_queue = NULL;
	GtkWidget *entry_np = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Flow execution parameters"), GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					     GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT,
					     NULL);

	box = GTK_DIALOG(dialog)->vbox; /* This is the 'main box' of the dialog. */

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_box_set_spacing(GTK_BOX(box), 5);

	void plug_widget(const gchar * title, GtkWidget * widget) {
		gchar * markup;
		GtkWidget * frame;
		GtkWidget * align;
		GtkWidget * label;

		align = gtk_alignment_new(0, 0, 1, 1);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 10, 0);
		gtk_container_add(GTK_CONTAINER(align), widget);

		markup = g_markup_printf_escaped("<b>%s</b>", title);
		frame = gtk_frame_new(markup);
		label = gtk_frame_get_label_widget(GTK_FRAME(frame));
		gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
		gtk_container_add(GTK_CONTAINER(frame), align);

		gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

		g_free(markup);
	}

	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		GtkCellRenderer *cell;
		cell = gtk_cell_renderer_text_new();
		cb_account = gtk_combo_box_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_account), cell, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cb_account), cell, "text", 0);
		gtk_combo_box_set_model(GTK_COMBO_BOX(cb_account), GTK_TREE_MODEL(server->accounts_model));
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_account), 0);
		plug_widget(_("Account"), cb_account);
	}
	else {
		/* Common servers. */
		if (config->queue == NULL) {
			/* We should ask for a queue name only if it is originally NULL.
			 * Even an empty string ("") has a meaning: it means the flow is supposed
			 * to run immediately. */
			entry_queue = gtk_entry_new();
			gtk_entry_set_activates_default(GTK_ENTRY(entry_queue), TRUE);
			plug_widget(_("Choose the queue's name"), entry_queue);
		}
	}

	if (mpi_program) {
		/* We should be able to ask for the number of processes (np) to run the mpi program(s). */
		entry_np = gtk_spin_button_new_with_range(1, 99999999999, 1);
		gtk_entry_set_activates_default(GTK_ENTRY(entry_np), TRUE);
		plug_widget(_("Number of processes"), entry_np);
	}

	gtk_widget_show_all(dialog);

	gboolean moab_server_validated = !(server->type == GEBR_COMM_SERVER_TYPE_MOAB);
	gboolean queue_name_validated = !(config->queue == NULL);
	gboolean num_processes_validated = !(mpi_program);

	do {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
			ret = FALSE;
			goto out;
		}

		/* Gathering and validating data from the dialog. */
		if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_account), &iter);
			gtk_tree_model_get(GTK_TREE_MODEL(server->accounts_model), &iter, 0, &(config->account), -1);
			moab_server_validated = TRUE;
		}
		else {
			/* Common servers. */
			if (config->queue == NULL) {
				config->queue = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_queue)));

				if (strlen(config->queue) == 0) {
					gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Empty name"), _("Please type a queue name."));
					/* Return to previous invalid condition. */
					g_free(config->queue);
					config->queue = NULL;
				}
				else {
					gchar *prefixed_queue_name = g_strdup_printf("q%s", config->queue);
					if (server_queue_find(server, prefixed_queue_name, NULL)) {
						gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
									_("Duplicated name"), _("This queue name is already in use. Please give another one."));
						/* Return to previous invalid condition. */
						g_free(config->queue);
						config->queue = NULL;
					}
					else {
						/* Update config->queue with the prefix. */
						g_free(config->queue);
						config->queue = g_strdup(prefixed_queue_name);
						queue_name_validated = TRUE;
					}
					g_free(prefixed_queue_name);
				}
			}
		}

		if (mpi_program) {
			/* Get the number of processes for mpi execution. */
			config->num_processes = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_np)));

			// TODO: Better validation for the number of processes...
			if (strlen(config->num_processes) == 0) {
				gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Empty number"), _("Please enter the number of processes to run this Flow."));
				g_free(config->num_processes);
				config->num_processes = NULL;
			}
			else {
				num_processes_validated = TRUE;
			}
		}
	} while (!(moab_server_validated && queue_name_validated && num_processes_validated));

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
out:
	gtk_widget_destroy(dialog);
	return ret;
}

static gboolean
fill_moab_account_and_queue(GebrCommRunner *runner,
			    GebrServer *server,
			    const gchar *queue_id,
			    gboolean is_mpi)
{
	if (!flow_io_run_dialog(runner, server, is_mpi)) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("No available queue for server \"%s\"."), server->comm->address->str);
		return FALSE;
	}

	runner->queue = g_strdup(queue_id);
	return TRUE;
}

typedef struct {
	GebrServer *server;
	gdouble score;
} ServerScore;

/*
 * @servers: A list of ServerScore's ordered by highest score.
 */
static void
add_flows_to_runner(GebrCommRunner *runner,
		    GList          *servers)
{
	if (gebr.ui_flow_edition->autochoose && gebr_geoxml_flow_is_parallelizable(gebr.flow, gebr.validator)) {
		GList *flows, *list;
		gdouble *weights;
		guint n_servers = g_list_length(servers);
		runner->is_parallelizable = TRUE;
		gdouble *scores = g_new(gdouble, n_servers);
		gdouble acc_scores;
		gint k = 0;

		for (GList *i = servers; i; i = i->next) {
			ServerScore *sc = i->data;
			scores[k] = sc->score;
			acc_scores += sc->score;
			k++;
		}

		weights = gebr_geoxml_flow_calulate_weights(n_servers, scores, acc_scores);
		g_free(scores);

		flows = gebr_geoxml_flow_divide_flows(gebr.flow, gebr.validator, weights, n_servers);
		gint nflows = g_list_length(flows);
		list = servers;

		gint j = 1;
		for (GList *i = flows; i; i = i->next) {
			GebrGeoXmlFlow *frac_flow = i->data;
			GebrCommRunnerFlow *runflow;
			ServerScore *sc = list->data;
			gboolean last = (i->next == NULL);

			runflow = gebr_comm_runner_add_flow(runner, gebr.validator, frac_flow,
							    sc->server->comm, !last, gebr_get_session_id(),
							    list->data);

			gebr_comm_runner_flow_set_frac(runflow, j++, nflows);

			list = list->next;
			if (!list && i->next) {
				list = servers;
				g_warn_if_reached();
			}
		}

		g_free(weights);
		g_list_foreach(flows, (GFunc)gebr_geoxml_document_unref, NULL);
		g_list_free(flows);
	} else {
		ServerScore *sc = servers->data;
		// Use the first server for this flow
		gebr_comm_runner_add_flow(runner, gebr.validator, gebr.flow,
					  sc->server->comm, FALSE,
					  gebr_get_session_id(), servers->data);
	}
}

/*
 * Fills the queue and MPI/MOAB parameters in @runner struct.
 */
static gboolean
fill_runner_struct(GebrCommRunner *runner,
		   GList          *servers,
		   const gchar    *queue_id,
		   gboolean        is_mpi)
{
	GebrServer *server = ((ServerScore *)servers->data)->server;

	add_flows_to_runner(runner, servers);

	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (fill_moab_account_and_queue(runner, server, queue_id, is_mpi))
			return TRUE;
		else
			goto free_and_error;
	} else {
		runner->queue = g_strdup(queue_id? queue_id:"");
		if (is_mpi && !flow_io_run_dialog(runner, server, is_mpi))
			goto free_and_error;
	}

	return TRUE;

free_and_error:
	gebr_comm_runner_free(runner);
	return FALSE;
}

/*
 * Returns: a GList containing ServerScore structs with the score field set to 0.
 */
static GList *
get_connected_servers(GtkTreeModel *model)
{
	GtkTreeIter iter;
	GList *servers = NULL;

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gboolean is_auto_choose;
		ServerScore *sc = g_new0(ServerScore, 1);

		gtk_tree_model_get(model, &iter, SERVER_POINTER, &sc->server,
				   SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

		if (is_auto_choose)
			continue;

		if (sc->server->comm->socket->protocol->logged)
			servers = g_list_prepend(servers, sc);
	}

	return servers;
}

/* Heuristics structure and methods {{{1 */
/*
 * Struct to handle 2D points
 */
typedef struct {
	gdouble x;
	gdouble y;
} GebrPoint;

/*
 * Compute the linear regression over a given list of points (GebrPoints)
 */
void
create_linear_model(GList *points, gdouble **parameters)
{
	gint j;
	GList *i;
	gdouble weight[] = {1, 1, 1};
	gdouble sumY=0, sumX=0, sumX2=0, sumXY=0, w=0;
	for (i = points, j = 0; i; i = i->next, j++) {
		GebrPoint *point = i->data;
		sumY += point->y * weight[j];
		sumX += point->x * weight[j];
		sumX2 += (point->x*point->x) * weight[j];
		sumXY += (point->x*point->y) * weight[j];
		w += weight[j];
	}
	// Line Ax + B = 0
	// A = parameters[0]
	// B = parameters[1]
	(*parameters)[0] = (w*sumXY - sumX*sumY)/(w*sumX2 - sumX*sumX);
	(*parameters)[1] = (sumY - (*parameters)[0]*sumX)/w;
}

/*
 * Compute the parameters of a polynomial (2nd order) fit
 */
void
create_2ndDegreePoly_model(GList *points, gdouble parameters[])
{
	gdouble M, MA, MB, MC, **N;
	gint j;
	GList *i;
	N = g_new(gdouble*, 3);
	for (i = points, j = 0; i; i = i->next, j++) {
		GebrPoint *point = i->data;
		N[j] = g_new(gdouble, 4);
		N[j][0] = 1;
		N[j][1] = point->x;
		N[j][2] = N[j][1]*N[j][1];
		N[j][3] = point->y;
	}

	M = (N[1][1] - N[0][1]) * (N[2][1] - N[0][1])*(N[2][1] - N[1][1]);
	MA = N[2][3]*(N[1][1]-N[0][1]) + N[1][3]*(N[0][1]-N[2][1]) + N[0][3]*(N[2][1]-N[1][1]);
	MB = N[2][3]*(N[0][2]-N[1][2]) + N[1][3]*(N[2][2]-N[0][2]) + N[0][3]*(N[1][2]-N[2][2]);
	MC = N[2][3]*(N[0][1]*N[1][2] - N[1][1]*N[0][2]) + N[1][3]*(N[2][1]*N[0][2]-N[0][1]*N[2][2]) + N[0][3]*(N[1][1]*N[2][2]-N[2][1]*N[1][2]);

	// Equation Ax2 + Bx + C = 0
	parameters[0] = MA/M;
	parameters[1] = MB/M;
	parameters[2] = MC/M;

	for (gint j=0; j<3; j++)
		g_free(N[j]);
	g_free(N);
}

/*
 * Predict the future load
 */
static gdouble
predict_current_load(GList *points, gdouble delay)
{
	gdouble prediction;
	gdouble parameters[3];
	//create_linear_model(points, &parameters);
	//prediction = parameters[0]*delay + parameters[1];
	create_2ndDegreePoly_model(points, parameters);
	prediction = parameters[0]*delay*delay + parameters[1]*delay + parameters[2];

	return MAX(0, prediction);
}

/*
 * Compute the effective core, based on the number of cores, 
 * on the flow_exec_speed and on the number of steps
 */ 
static gint
compute_effective_ncores(const gint ncores, const gint scale, const gint nsteps)
{
	gint eff_ncores= (ABS(scale)*ncores)/ 5;
	if (eff_ncores == 0)
		eff_ncores = 1;
	return (eff_ncores < nsteps ? eff_ncores : nsteps);
}

/*
 * Compute the score of a server
 */
static gdouble
calculate_server_score(const gchar *load, gint ncores, gdouble cpu_clock, gint scale, gint nsteps)
{
	GList *points = NULL;
	gdouble delay = 1.0;
	GebrPoint point1, point5, point15;

	sscanf(load, "%lf %lf %lf", &(point1.y), &(point5.y), &(point15.y));

	point1.x = -1.0;
	point5.x = -5.0;
	point15.x = -15.0;
	points = g_list_prepend(points, &point1);
	points = g_list_prepend(points, &point5);
	points = g_list_prepend(points, &point15);

	gdouble current_load = predict_current_load(points, delay);
	gint eff_ncores = compute_effective_ncores(ncores, scale, nsteps); 
	gdouble score;
	if (current_load + eff_ncores > ncores)
		score = cpu_clock*eff_ncores/(current_load + eff_ncores - ncores + 1);

	else
		score = cpu_clock*eff_ncores;

	if (current_load + eff_ncores > ncores)
		g_debug ("CASE 01, %lf, cpu_clock: %lf, curren_load:%lf, eff_ncores: %d", score, cpu_clock, current_load, eff_ncores);
	else
		g_debug ("CASE 02, %lf, cpu_clock: %lf, curren_load:%lf, eff_ncores: %d", score, cpu_clock, current_load, eff_ncores);
	g_list_free(points);

	return score;
}

/*
 * Struct to handle the server with the maximum score
 */
typedef struct {
	gint responses;
	gint requests;
	gboolean parallel;
	gboolean single;
	gboolean is_mpi;
	GebrGeoXmlFlow *flow;
	GebrCommRunner *runner;
	GList *servers;
} AsyncRunInfo;

/*
 * Rank the server according to their scores.
 */
static void
on_response_received(GebrCommHttpMsg *request, GebrCommHttpMsg *response, AsyncRunInfo *runinfo)
{
	if (request->method == GEBR_COMM_HTTP_METHOD_GET)
	{
		GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
		GString *value = gebr_comm_json_content_to_gstring(json);
		ServerScore *sc = g_object_get_data(G_OBJECT(request), "current-server");

		gchar *eval_n;
		GebrGeoXmlProgram *loop = gebr_geoxml_flow_get_control_program(runinfo->flow);

		if (loop) {
			gchar *n = gebr_geoxml_program_control_get_n(loop, NULL, NULL);
			gebr_validator_evaluate(gebr.validator, n, GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_n, NULL);
			g_free(n);
			gebr_geoxml_object_unref(loop);
		}
		else
			eval_n = g_strdup("1");
		gdouble score = calculate_server_score(value->str, sc->server->ncores, sc->server->clock_cpu, gebr.config.flow_exec_speed, atoi(eval_n));

		g_printf("[ %s ]\n", sc->server->comm->address->str);
		g_printf("cores = %d\n", sc->server->ncores);
		g_printf("used cores = %d\n", compute_effective_ncores(sc->server->ncores, gebr.config.flow_exec_speed, atoi(eval_n)));
		g_printf("clock = %lf\n", sc->server->clock_cpu);
		g_printf("steps = %s\n", eval_n);
		g_printf("score = %lf\n", score);
		g_printf("\n");
		
		g_free(eval_n);
		sc->score = score;

		runinfo->responses++;
		if (runinfo->responses == runinfo->requests) {
			gint comp_func(ServerScore *a, ServerScore *b) {
				return b->score - a->score;
			}

			runinfo->servers = g_list_sort(runinfo->servers, (GCompareFunc)comp_func);

			if (fill_runner_struct(runinfo->runner, runinfo->servers, NULL, runinfo->is_mpi))
				create_jobs_and_run(runinfo->runner);
			gebr_comm_runner_free(runinfo->runner);
		}
	}
}

/*
 * Request all the connected servers the info on the local file /proc/loadavg
 */
static void
send_sys_load_request(GList *servers,
		      GebrGeoXmlFlow *flow,
		      GebrCommRunner *runner,
		      gboolean is_mpi)
{
	AsyncRunInfo *scores = g_new0(AsyncRunInfo, 1);
	scores->runner = runner;
	scores->servers = servers;
	scores->flow = flow;
	scores->is_mpi = is_mpi;

	for (GList *i = servers; i; i = i->next) {
		GebrCommHttpMsg *request;
		ServerScore *sc = i->data;
		request = gebr_comm_protocol_socket_send_request(sc->server->comm->socket,
								 GEBR_COMM_HTTP_METHOD_GET,
								 "/sys-load", NULL);
		g_object_set_data(G_OBJECT(request), "current-server", sc);
		g_signal_connect(request, "response-received",
				 G_CALLBACK(on_response_received), scores);
		scores->requests++;
	}
}

/*
 * Creates job entries in JobControl tab.
 */
static void
create_jobs_and_run(GebrCommRunner *runner)
{
	gboolean first = TRUE;
	GHashTable *map = g_hash_table_new(g_str_hash, g_str_equal);

	for (GList *i = runner->flows; i; i = i->next) {
		GString **values;
		GebrCommRunnerFlow *runflow = i->data;

		values = g_hash_table_lookup(map, runflow->run_id);
		if (!values) {
			values = g_new(GString *, 2);
			values[0] = g_string_new(runflow->server->address->str);
			values[1] = g_string_new(gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(runflow->flow)));
			g_hash_table_insert(map, runflow->run_id, values);
		} else
			g_string_append_printf(values[0], ",%s", runflow->server->address->str);

	}

	void func(gpointer key, gpointer value, gpointer data) {
		gchar *runid = key;
		GString **values = value;
		GebrJob *job = gebr_job_new_with_id(runid, runner->queue, values[0]->str);
		gebr_job_set_title(job, values[1]->str);
		gebr_job_set_hostname(job, g_get_host_name());
		gebr_job_set_server_group(job, runner->server_group_name);
		gebr_job_set_model(job, gebr_job_control_get_model(gebr.job_control));
		gebr_job_control_add(gebr.job_control, job);
		g_string_free((GString *)values[0], TRUE);
		g_string_free((GString *)values[1], TRUE);
		g_free(values);

		if (first) {
			gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);
			gebr_job_control_select_job(gebr.job_control, job);
			first = FALSE;
		}
	}

	g_hash_table_foreach(map, func, NULL);
	g_hash_table_unref(map);

	gebr_comm_runner_run(runner);
}

/*
 * Gets the selected queue. Returns %TRUE if no queue was selected, %FALSE
 * otherwise.
 */
static gboolean
get_selected_queue(gchar **queue, GebrServer *server)
{
	GtkTreeIter queue_iter;
	gboolean has_error = FALSE;

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &queue_iter)) {
		if (server->type == GEBR_COMM_SERVER_TYPE_MOAB ||
		    !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(server->queues_model), &queue_iter)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("No available queue for server \"%s\"."), server->comm->address->str);
			has_error = TRUE;
		}
	}

	if (!has_error)
		gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &queue_iter,
				   SERVER_QUEUE_ID, queue, -1);

	return !has_error;
}

/* Public methods {{{1 */
void
gebr_ui_flow_run(void)
{
	GtkTreeIter iter;
	gboolean has_mpi;
	GebrGeoXmlFlow *flow;
	GList *servers = NULL;
	GebrCommRunner *runner;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox));

	if (!flow_browse_get_selected(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
			   FB_XMLPOINTER, &flow, -1);
	gebr_geoxml_flow_set_date_last_run(flow, gebr_iso_date());
	document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);
	has_mpi = (gebr_geoxml_flow_get_first_mpi_program(flow) != NULL);

	gboolean is_fs;
	runner = gebr_comm_runner_new();
	runner->execution_speed = g_strdup_printf("%d", gebr_interface_get_execution_speed());
	runner->niceness = g_strdup_printf("%d", gebr_interface_get_niceness());
	runner->server_group_name = g_strdup(gebr_geoxml_line_get_group(gebr.line, &is_fs));

	if (gebr.ui_flow_edition->autochoose) {
		send_sys_load_request(get_connected_servers(model), flow, runner, has_mpi);
		return;
	} else {
		GtkTreeIter server_iter;
		GebrServer *server;
		gchar *queue_id;

		if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &server_iter)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No server selected."));
			goto free_and_error;
		}

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->servers_sort), &server_iter,
				   SERVER_POINTER, &server, -1);

		if (!get_selected_queue(&queue_id, server)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No queue selected."));
			goto free_and_error;
		}

		ServerScore sc = {server, 1};
		servers = g_list_prepend(NULL, &sc);
		if (!fill_runner_struct(runner, servers, queue_id, has_mpi))
			goto free_and_error;

		create_jobs_and_run(runner);
	}

free_and_error:
	gebr_comm_runner_free(runner);
	g_list_free(servers);
}

GebrQueueTypes
gebr_get_queue_type(const gchar *queue_id)
{
	if (g_strcmp0(queue_id, "") == 0)
		return IMMEDIATELY_QUEUE;

//	if (queue_id[0] == 'j')
//		return AUTOMATIC_QUEUE;
//
//	if (queue_id[0] == 'q')
//		return USER_QUEUE;

	g_return_val_if_reached(IMMEDIATELY_QUEUE);
}
