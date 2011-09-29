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
#include <libgebr/gui/gebr-gui-file-entry.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-icons.h>

#include "ui_flow.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_server.h"
#include "ui_moab.h"

#include "job.h"

/* Private methods {{{1 */
/*
 * flow_io_run_dialog:
 * @config: A pointer to a gebr_comm_server_run structure, which will hold the parameters entered within the dialog.
 * @server:
 * @mpi_program:
 */
static gboolean
flow_io_run_dialog(GebrCommServerRunConfig *config,
		   GebrServer              *server,
		   gboolean                 mpi_program)
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

static void
flow_io_run_on_server(GebrGeoXmlFlow *flow,
		      GebrServer     *server,
		      const gchar    *queue_id,
		      gboolean        parallel,
		      gboolean        single)
{

	/* SERVER: check connection */
	if (!gebr_comm_server_is_logged(server->comm)) {
		if (gebr_comm_server_is_local(server->comm))
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to the local server."), server->comm->address->str);
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to server '%s'."), server->comm->address->str);
		return;
	}

	/* initialization */
	GebrCommServerRunConfig *config = gebr_comm_server_run_config_new();
	config->queue = NULL;
	config->parallel = parallel;
	config->execution_speed = g_strdup_printf("%d", gebr_interface_get_execution_speed());
	
	/* SET config->queue */
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gboolean multiple = !single && gtk_tree_selection_count_selected_rows(selection) > 1;
	gboolean mpi_program = (gebr_geoxml_flow_get_first_mpi_program(gebr.flow) != NULL);
	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		config->queue = g_strdup(queue_id);
		if (!flow_io_run_dialog(config, server, mpi_program))
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("No available queue for server \"%s\"."), server->comm->address->str);
	} else {
		/* Common servers. */
		gboolean is_immediately = queue_id == NULL || gtk_combo_box_get_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox)) == 0;

		/* ENSURE THAT AFTER IF/ELSE CHAINS config->queue WAS INITIALIZED!
		 * REMEMBER flow_io_run_dialog ALWAYS SET config->queue IF NULL AND TRUE RETURNED
		 * FIXME: SIMPLIFY CONDITIONS */
		if (multiple && !parallel) {
			GString *new_internal_queue_name = g_string_new(NULL);
			const gchar *internal_queue_name = queue_id? queue_id:"j";

			/* If the first flow is marked to run Immediately or it has a Job Queue selected,
			 * we must create a new queue name. The name is composed by the first and the last
			 * flow title in the selection.
			 */
			if (is_immediately || internal_queue_name[0] == 'j') {
				GList *selected = gebr_gui_gtk_tree_view_get_selected_iters(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
				GebrGeoXmlFlow *first;
				GebrGeoXmlFlow *last;
				gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store),
						   (GtkTreeIter*)(g_list_first(selected)->data), FB_XMLPOINTER, &first, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store),
						   (GtkTreeIter*)(g_list_last(selected)->data), FB_XMLPOINTER, &last, -1);
				g_list_foreach(selected, (GFunc) gtk_tree_iter_free, NULL);
				g_list_free(selected);

				/* compose it */
				GString *aux = g_string_new(NULL);
				g_string_printf(aux, _("After '%s'...'%s'"),
						gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(first)),
						gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(last)));
				g_string_prepend_c(aux, 'q');
				g_string_assign(new_internal_queue_name, aux->str);
				/* Finds a unique name for the queue. */
				for (gint queue_num = 1; server_queue_find(server, new_internal_queue_name->str, NULL);)
					g_string_printf(new_internal_queue_name, "%s %d", aux->str, queue_num++);
				g_string_free(aux, TRUE);

				/* In this case, we have renamed the `internal_queue_name' to 'new_internal_queue_name', so we must
				 * send the request to the server. */
				gebr_comm_protocol_socket_oldmsg_send(server->comm->socket, FALSE,
								      gebr_comm_protocol_defs.rnq_def, 2,
								      internal_queue_name, new_internal_queue_name->str);
			} else
				g_string_assign(new_internal_queue_name, internal_queue_name);

			/* frees */
			config->queue = g_string_free(new_internal_queue_name, FALSE);
		} else if (!parallel && !is_immediately) {
			const gchar *internal_queue_name = queue_id;
			
			/* Other queue option is selected: after a running job (flow) or
			 * on a pre-existent queue.
			 */
			if (internal_queue_name[0] == 'j') {
				/* Prefix 'j' indicates a single running job. So, the user is placing a new
				 * job behind this single one, which denotes proper enqueuing. In this case,
				 * it is necessary to give a name to the new queue. */

				/* config->queue being set here! */
				if (!flow_io_run_dialog(config, server, mpi_program)) {
					goto err;
				}
				gebr_comm_protocol_socket_oldmsg_send(server->comm->socket, FALSE,
								      gebr_comm_protocol_defs.rnq_def, 2, internal_queue_name, config->queue);
			} else {
				config->queue = g_strdup(internal_queue_name);
				if (mpi_program && !flow_io_run_dialog(config, server, mpi_program))
					goto err;
			}
		} else {
			/* If the active combobox entry is the first one (index 0), then
			 * "Immediately" (a queue name starting with j) is selected as queue option. */
			config->queue = g_strdup("j");
			if (mpi_program && !flow_io_run_dialog(config, server, mpi_program))
			    goto err;
		}
	}

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	flow_run(server, config, !multiple);
	return;

err:
	gebr_comm_server_run_config_free(config);
}

static GList *
get_connected_servers(GtkTreeModel *model)
{
	GtkTreeIter iter;
	GList *servers = NULL;
	GebrServer *server;

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gboolean is_auto_choose;
		gtk_tree_model_get(model, &iter, SERVER_POINTER, &server, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);
		if (is_auto_choose)
			continue;
		if (server->comm->socket->protocol->logged)
			servers = g_list_prepend(servers, server);
	}

	return servers;
}

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
create_linear_model(GList *points, GebrPoint *parameters)
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
	// A = parameters->y
	// B = parameters->x
	parameters->y = (w*sumXY - sumX*sumY)/(w*sumX2 - sumX*sumX);
	parameters->x = (sumY - parameters->y*sumX)/w;
}

/*
 * Predict the future load
 */
static gdouble
predict_current_load(GList *points, gdouble delay)
{
	GebrPoint parameters;
	create_linear_model(points, &parameters);

	return (parameters.x + parameters.y*delay);
}

/*
 * Compute the score of a server
 */
static gdouble
calculate_server_score(const gchar *load, gint ncores)
{
	GList *points = NULL;
	gdouble delay = 1.0;
	GebrPoint point1, point5, point15;
	
	sscanf(load, "%lf %lf %lf", &(point1.y), &(point5.y), &(point15.y));
	
	point1.y /= ncores; 
	point5.y /= ncores;
       	point15.y /= ncores;	
	point1.x = -1.0; 
	point5.x = -5.0; 
	point15.x = -15.0;
	points = g_list_prepend(points, &point1);
	points = g_list_prepend(points, &point5);
	points = g_list_prepend(points, &point15);
	
	gdouble current_load = predict_current_load(points, delay);
	
	g_list_free(points);

	return current_load;
}

/*
 * Struct to handle the server with the maximum score
 */
typedef struct {
	gdouble min_score;
	GebrServer *the_one;
	gint responses;
	gint requests;
	GebrGeoXmlFlow *flow;
	gboolean parallel;
	gboolean single;
} ServerScores;

/*
 * Rank the server according to the scores
 */
static void
on_response_received(GebrCommHttpMsg *request, GebrCommHttpMsg *response, ServerScores *scores)
{
	if (request->method == GEBR_COMM_HTTP_METHOD_GET)
	{
		GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
		GString *value = gebr_comm_json_content_to_gstring(json);
		GebrServer *server = g_object_get_data(G_OBJECT(request), "current-server");
		gdouble score = calculate_server_score(value->str, server->ncores);
		g_debug("Server: %s, Score: %lf, Load: %s", server->comm->address->str, score*4, value->str);

		if (score < scores->min_score) {
			scores->min_score = score;
			scores->the_one = server;
		}
		scores->responses++;
		if (scores->responses == scores->requests) {
			flow_io_run_on_server(scores->flow, scores->the_one, NULL, scores->parallel, scores->single);
			g_debug("THE ONE: %s, Max Score: %lf", scores->the_one->comm->address->str, scores->min_score*4);
		}
	}
}

/*
 * Request all the connected servers the info on the local file /proc/loadavg
 */
static void
send_sys_load_request(GList *servers, GebrGeoXmlFlow *flow, gboolean parallel, gboolean single)
{
	ServerScores *scores = g_new0(ServerScores, 1);
	scores->flow = flow;
	scores->parallel = parallel;
	scores->single = single;
	scores->min_score = G_MAXDOUBLE;

	for (GList *i = servers; i; i = i->next) {
		GebrCommHttpMsg *request;
		GebrServer *server = i->data;
		request = gebr_comm_protocol_socket_send_request(server->comm->socket,
								 GEBR_COMM_HTTP_METHOD_GET,
								 "/sys-load", NULL);
		g_object_set_data(G_OBJECT(request), "current-server", i->data);
		g_signal_connect(request, "response-received",
				 G_CALLBACK(on_response_received), scores);
		scores->requests++;
	}
}

/* Public methods {{{1 */
void
gebr_ui_flow_run(gboolean parallel, gboolean single)
{
	if (!flow_browse_get_selected(NULL, FALSE)) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected."));
		return;
	}

	if (gebr.ui_flow_edition->autochoose) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox));
		GList *servers = get_connected_servers(model);
		send_sys_load_request(servers, gebr.flow, parallel, single);
		g_list_free(servers);
	} else {
		/* SERVER on combox: get selected */
		GtkTreeIter server_iter;
		if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &server_iter)) {
			//just happen if the current server was removed
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No server selected."));
			return;
		}

		GebrServer *server;

		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_project_line->servers_sort), &server_iter,
				    SERVER_POINTER, &server, -1);

		GtkTreeIter queue_iter;
		gboolean has_error = FALSE;

		if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &queue_iter)) {
			if (server->type == GEBR_COMM_SERVER_TYPE_MOAB ||
			    !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(server->queues_model), &queue_iter)) {
				gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("No available queue for server \"%s\"."), server->comm->address->str);
				has_error = TRUE;
			}
		}

		if (!has_error) {
			gchar *queue_id;
			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &queue_iter, 
					   SERVER_QUEUE_ID, &queue_id, -1);

			flow_io_run_on_server(gebr.flow, server, queue_id, parallel, single);
			g_free(queue_id);
		}
	}
}
