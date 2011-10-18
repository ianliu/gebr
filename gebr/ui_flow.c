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
#include <libgebr/date.h>
#include <libgebr/gui/gui.h>
#include <libgebr/comm/gebr-comm.h>
#include <stdlib.h>

#include "ui_flow.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_server.h"
#include "ui_moab.h"

#include "job.h"

/* Prototypes {{{1 */
static void create_jobs_and_run(GebrCommRunner *runner, gboolean is_divided);

static gboolean is_divisible(gboolean single);

static void add_selected_flows_to_runner(GebrCommRunner *runner, GList *servers);

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
	GtkSizeGroup *group;

	GtkWidget *cb_account = NULL;
	GtkWidget *entry_queue = NULL;
	GtkWidget *entry_np = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Flow execution parameters"), GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					     GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT,
					     NULL);

	box = GTK_DIALOG(dialog)->vbox; /* This is the 'main box' of the dialog. */
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

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

/*
 * QueueTypes:
 * @AUTOMATIC_QUEUE: A queue created when a flow is executed
 * @USER_QUEUE: A queue created when the user chooses a name or when multiple
 * flows are executed.
 * @IMMEDIATELY_QUEUE: The immediately queue.
 */
typedef enum {
	AUTOMATIC_QUEUE,
	USER_QUEUE,
	IMMEDIATELY_QUEUE,
} QueueTypes;

static QueueTypes
get_queue_type(const gchar *queue_id)
{
	if (g_strcmp0(queue_id, "j") == 0)
		return IMMEDIATELY_QUEUE;

	if (queue_id[0] == 'j')
		return AUTOMATIC_QUEUE;

	if (queue_id[0] == 'q')
		return USER_QUEUE;

	g_return_val_if_reached(IMMEDIATELY_QUEUE);
}

/*
 * Creates the queue name for multiple flows execution.
 * The queue name consists of the title of the first flow
 * concatenated with the title of the last flow.
 */
static gchar *
create_multiple_execution_queue(GebrCommRunner *runner,
				GebrServer *server)
{
	GebrCommRunnerFlow *runflow;
	GebrGeoXmlFlow *first, *last;
	GString *new_queue = g_string_new(NULL);
	gchar *tfirst, *tlast;

	runflow = g_list_first(runner->flows)->data;
	first = runflow->flow;

	runflow = g_list_last(runner->flows)->data;
	last = runflow->flow;

	tfirst = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(first));
	tlast = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(last));
	g_string_printf(new_queue, _("After \"%s\"...\"%s\""), tfirst, tlast);
	g_string_prepend_c(new_queue, 'q');
	g_free(tfirst);
	g_free(tlast);

	/* Finds a unique name for the queue. */
	gint queue_num = 1;
	gchar *tmpname = g_strdup(new_queue->str);
	while (server_queue_find(server, tmpname, NULL)) {
		g_free(tmpname);
		tmpname = g_strdup_printf("%s %d", new_queue->str, queue_num);
		queue_num++;
	}
	g_string_assign(new_queue, tmpname);

	return g_string_free(new_queue, FALSE);
}

/*
 * Fills the queue and MPI/MOAB parameters in @runner struct.
 */
static gboolean
fill_runner_struct(GebrCommRunner *runner,
		   GList          *servers,
		   const gchar    *queue_id,
		   gboolean        parallel,
		   gboolean        single)
{
	GebrCommRunnerFlow *runflow;
	gboolean multiple, is_mpi;

	if (single) {
		if (gebr.ui_flow_edition->autochoose && gebr_geoxml_flow_is_parallelizable(gebr.flow, gebr.validator)) {
			GList *flows, *list;
			gdouble *weights;
			guint n_servers = g_list_length(servers);
			runner->is_parallelizable = TRUE;

			weights = gebr_geoxml_flow_calulate_weights(n_servers);
			flows = gebr_geoxml_flow_divide_flows(gebr.flow, gebr.validator, weights, n_servers);
			list = servers;

			gint j = 1;
			for (GList *i = flows; i; i = i->next) {
				GebrGeoXmlFlow *frac_flow = i->data;
				GebrCommRunnerFlow *runflow;
				runflow = gebr_comm_runner_add_flow(runner, gebr.validator, frac_flow,
								    ((GebrServer*)list->data)->comm, TRUE,
								    list->data);

				gchar *frac = g_strdup_printf("%d:%d", j++, n_servers);
				gebr_comm_runner_flow_set_frac(runflow, frac);
				g_free(frac);

				list = list->next;
				if (!list) {
					list = servers;
					g_warn_if_reached();
				}
			}

			g_free(weights);
			g_list_free(flows);
		} else
			// Use the first server for this flow
			gebr_comm_runner_add_flow(runner, gebr.validator, gebr.flow,
						  ((GebrServer*)servers->data)->comm, FALSE,
						  servers->data);
	} else
		// FIXME This method accesses the GUI!
		add_selected_flows_to_runner(runner, servers);

	GebrServer *server = servers->data;
	runflow = runner->flows->data;
	multiple = !single && runner->flows->next != NULL;
	is_mpi = gebr_geoxml_flow_get_first_mpi_program(runflow->flow) != NULL;

	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (fill_moab_account_and_queue(runner, server, queue_id, is_mpi))
			return TRUE;
		else
			goto free_and_error;
	} else {
		gboolean is_immediately = queue_id == NULL || get_queue_type(queue_id) == IMMEDIATELY_QUEUE;

		// Multiple flows in sequence
		if (multiple && !parallel) {
			const gchar *internal_queue_name = queue_id? queue_id:"j";
			if (is_immediately || get_queue_type(internal_queue_name) == AUTOMATIC_QUEUE) {
				runner->queue = create_multiple_execution_queue(runner, server);
				gebr_comm_protocol_socket_oldmsg_send(server->comm->socket, FALSE,
								      gebr_comm_protocol_defs.rnq_def, 2,
								      internal_queue_name, runner->queue);
			} else
				runner->queue = g_strdup(internal_queue_name);

		// Append single flow in queue
		} else if (!parallel && !is_immediately) {
			if (get_queue_type(queue_id) == AUTOMATIC_QUEUE) {
				if (!flow_io_run_dialog(runner, server, is_mpi))
					goto free_and_error;

				gebr_comm_protocol_socket_oldmsg_send(server->comm->socket, FALSE,
								      gebr_comm_protocol_defs.rnq_def, 2,
								      queue_id, runner->queue);
			} else {
				runner->queue = g_strdup(queue_id);
				if (is_mpi && !flow_io_run_dialog(runner, server, is_mpi))
					goto free_and_error;
			}

		// Single & Multiple parallel execution
		} else {
			/* If the active combobox entry is the first one (index 0), then
			 * "Immediately" (a queue name starting with j) is selected as queue option. */
			runner->queue = g_strdup("j");
			if (is_mpi && !flow_io_run_dialog(runner, server, is_mpi))
			    goto free_and_error;
		}
	}

	return TRUE;

free_and_error:
	gebr_comm_runner_free(runner);
	return FALSE;
}

typedef struct {
	GebrServer *server;
	gdouble score;
} ServerScore;

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

	return MAX(0,prediction);
}

/*
 * Compute the effective core, based on the number of cores, 
 * on the flow_exec_speed and on the number of steps
 */ 
static gint
compute_effective_cores(const gint ncores, const gint scale, const gint nsteps) 
{
	gint eff_ncores= (scale*ncores)/ 5;
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
	gint eff_ncores = compute_effective_cores(ncores, scale, nsteps); 
	gdouble score;
	if (current_load*ncores > ncores)
		score = cpu_clock*ncores/(current_load + eff_ncores - ncores + 1);
	else
		score = cpu_clock*ncores;

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
	GList *flows;
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

		GebrGeoXmlProgram *loop = gebr_geoxml_flow_get_control_program(runinfo->flows->data);
		gchar *eval_n;
		if(loop){
			gchar *n = gebr_geoxml_program_control_get_n(loop, NULL, NULL);
			gebr_validator_evaluate(gebr.validator, n, GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_n, NULL);
		}
		else
			eval_n = "1";
		gdouble score = calculate_server_score(value->str, sc->server->ncores, sc->server->clock_cpu, gebr.config.flow_exec_speed, atoi(eval_n));
		
		g_free(n);
		g_free(eval_n);
		gebr_geoxml_object_unref(loop);

		sc->score = score;
		g_debug("Server: %s, Score: %lf, Load: %s", sc->server->comm->address->str, score, value->str);

		runinfo->responses++;
		if (runinfo->responses == runinfo->requests) {
			gint comp_func(ServerScore *a, ServerScore *b) {
				return b->score - a->score;
			}
			runinfo->servers = g_list_sort(runinfo->servers, (GCompareFunc)comp_func);
			gdouble acc_scores = 0;

			for (GList *i = runinfo->servers; i; i = i->next) {
				ServerScore *sc = i->data;
				i->data = sc->server;
				acc_scores += sc->score;
				g_debug("Server %s, score %lf", sc->server->comm->address->str, sc->score);
				g_free(sc);
			}

			if (fill_runner_struct(runinfo->runner, runinfo->servers, NULL, runinfo->parallel, runinfo->single))
				create_jobs_and_run(runinfo->runner, is_divisible(runinfo->single));
			gebr_comm_runner_free(runinfo->runner);
		}
	}
}

/*
 * Request all the connected servers the info on the local file /proc/loadavg
 */
static void
send_sys_load_request(GList *servers, GList *flows, GebrCommRunner *runner, gboolean parallel, gboolean single)
{
	AsyncRunInfo *scores = g_new0(AsyncRunInfo, 1);
	scores->parallel = parallel;
	scores->single = single;
	scores->runner = runner;
	scores->servers = servers;
	scores->flows = flows;

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

static gboolean
is_divisible(gboolean single)
{
	g_debug("Is single? %s. Is Auto-choose? %s.", single?"yes":"no", gebr.ui_flow_edition->autochoose?"yes":"no");

	return single && gebr.ui_flow_edition->autochoose;
}

/* Job Creation methods {{{1 */
static void
create_job_entry(GebrCommRunner *runner, GebrServer *server, GebrGeoXmlFlow *flow, guint runid, gboolean select)
{
	gchar *title;
	GebrJob *job;
	GString *queue_string;

	title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow));
	queue_string = g_string_new(runner->queue);
	job = job_new_from_flow(server, title, queue_string);

	g_string_free(queue_string, TRUE);
	g_string_printf(job->parent.run_id, "%u", runid);
	g_debug("Job entry created with RID = %s!", job->parent.run_id->str);

	if (select) {
		job_set_active(job);
		gebr.config.current_notebook = 3;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
	}

	if (gebr_comm_server_is_local(server->comm) == FALSE)
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server '%s' to run Flow '%s'."), server->comm->address->str, title);
	else
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking local server to run Flow '%s'."), title);
	g_free(title);
}

/*
 * Creates job entries in JobControl tab. Also swaps the data structs from
 * GebrCommRunner::servers list from GebrServer to GebrCommServer, so GebrComm
 * can handle it (because GebrComm can't see GebrServer struct).
 */
static void
create_jobs_and_run(GebrCommRunner *runner, gboolean is_divided)
{
	gboolean first = TRUE;

	if (is_divided) {
		GebrCommRunnerFlow *runflow = runner->flows->data;
		create_job_entry(runner, NULL, runflow->flow, runflow->run_id, first);
	} else {
		for (GList *i = runner->flows; i; i = i->next) {
			GebrCommRunnerFlow *runflow = i->data;
			create_job_entry(runner, (GebrServer*)runflow->user_data, runflow->flow, runflow->run_id, first);
			first = FALSE;
		}
	}

	gebr_comm_runner_run(runner, gebr_get_session_id());
}

static void
add_selected_flows_to_runner(GebrCommRunner *runner, GList *servers)
{
	GebrGeoXmlFlow *flow;
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GebrServer *server = servers->data;

	treeview = GTK_TREE_VIEW(gebr.ui_flow_browse->view);
	model = gtk_tree_view_get_model(treeview);

	gebr_gui_gtk_tree_view_foreach_selected(&iter, treeview) {
		gtk_tree_model_get(model, &iter, FB_XMLPOINTER, &flow, -1);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gebr_comm_runner_add_flow(runner, gebr.validator, flow, server->comm, FALSE, server);
	}
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
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
gebr_ui_flow_run(gboolean parallel, gboolean single)
{
	GList *flows = NULL, *servers;
	GebrCommRunner *runner;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox));

	if (!flow_browse_get_selected(NULL, FALSE)) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected."));
		return;
	}

	GtkTreeIter iter;
	gebr_gui_gtk_tree_view_foreach_selected(&iter, GTK_TREE_VIEW(gebr.ui_flow_browse->view)) {
		GebrGeoXmlFlow *flow = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
				   FB_XMLPOINTER, &flow, -1);
		gebr_geoxml_flow_set_date_last_run(flow, gebr_iso_date());
		document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);
		flows = g_list_prepend(flows, flow);
	}

	runner = gebr_comm_runner_new();
	runner->parallel = parallel;
	runner->execution_speed = g_strdup_printf("%d", gebr_interface_get_execution_speed());

	if (gebr.ui_flow_edition->autochoose) {
		send_sys_load_request(get_connected_servers(model), flows,
				      runner, parallel, single);
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

		servers = g_list_prepend(NULL, server);
		if (!fill_runner_struct(runner, servers, queue_id, parallel, single))
			goto free_and_error;

		create_jobs_and_run(runner, FALSE);
	}

free_and_error:
	gebr_comm_runner_free(runner);
	g_list_free(servers);
}
