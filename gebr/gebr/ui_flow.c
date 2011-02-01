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

#include <string.h>

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

#define GEBR_FLOW_UI_RESPONSE_EXECUTE 1

static gboolean flow_io_run_dialog(GebrCommServerRunConfig *config, struct server *server, gboolean mpi_program);
static void flow_io_run(GebrGeoXmlFlowServer * serve, gboolean parallel, gboolean single);

gboolean flow_io_get_selected(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter)
{
	return gtk_tree_selection_get_selected(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_io->treeview)), NULL, iter);
}

void flow_io_select_iter(struct ui_flow_io *ui_flow_io, GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_flow_io->treeview), iter);
}

void flow_io_customized_paths_from_line(GtkFileChooser * chooser)
{
	GError *error;
	GebrGeoXmlSequence *path_sequence;

	if (gebr.line == NULL)
		return;

	error = NULL;
	gebr_geoxml_line_get_path(gebr.line, &path_sequence, 0);
	if (path_sequence != NULL) {
		gtk_file_chooser_set_current_folder(chooser,
						    gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE
										   (path_sequence)));

		do {
			gtk_file_chooser_add_shortcut_folder(chooser,
							     gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE
											    (path_sequence)), &error);
			gebr_geoxml_sequence_next(&path_sequence);
		} while (path_sequence != NULL);
	}
}

void flow_io_set_server(GtkTreeIter * server_iter, const gchar * input, const gchar * output, const gchar * error)
{
	struct server *server;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), server_iter, SERVER_POINTER, &server, -1);

	gebr_geoxml_flow_server_set_address(gebr.flow_server, server->comm->address->str);
	gebr_geoxml_flow_server_io_set_input(gebr.flow_server, input);
	gebr_geoxml_flow_server_io_set_output(gebr.flow_server, output);
	gebr_geoxml_flow_server_io_set_error(gebr.flow_server, error);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

void flow_fast_run(gboolean parallel, gboolean single)
{
	flow_io_run(gebr.flow_server, parallel, single);
}

void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program, gboolean select_last)
{
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		GtkTreeIter iter;
		const gchar *icon;

		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));

		gtk_list_store_insert_before(gebr.ui_flow_edition->fseq_store,
					     &iter, &gebr.ui_flow_edition->output_iter);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
				   FSEQ_TITLE_COLUMN, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)),
				   FSEQ_ICON_COLUMN, icon,
				   FSEQ_GEBR_GEOXML_POINTER, program,
				   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_NONE,
				   FSEQ_EDITABLE, FALSE, FSEQ_SENSITIVE, TRUE, -1);

		if (select_last)
			flow_edition_select_component_iter(&iter);
	}
}

void flow_program_check_sensitiveness (void)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlProgram *first_program;
	GebrGeoXmlProgram *last_program;
	gboolean has_some_error_output = FALSE;
	gboolean has_configured = FALSE;

	gebr_geoxml_flow_get_program(gebr.flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)){
		if (gebr_geoxml_program_get_status (GEBR_GEOXML_PROGRAM(program)) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED){
			if (!has_configured){
				first_program = GEBR_GEOXML_PROGRAM(program);
				has_configured = TRUE;
			}
			if (!has_some_error_output && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))){
				has_some_error_output = TRUE;
			}
			last_program = GEBR_GEOXML_PROGRAM(program);
		}
	}

	if (has_configured){
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
				   FSEQ_EDITABLE, gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program)),
				   FSEQ_SENSITIVE, gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program)), -1);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_EDITABLE, gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(last_program)),
				   FSEQ_SENSITIVE, gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(last_program)), -1);

		if (has_some_error_output)
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
					   FSEQ_EDITABLE, TRUE,
					   FSEQ_SENSITIVE, TRUE, -1);
		else
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
					   FSEQ_EDITABLE, FALSE,
					   FSEQ_SENSITIVE, FALSE, -1);
	}
}

/**
 * \internal
 *
 * @param config A pointer to a gebr_comm_server_run structure, which will hold the parameters entered within the dialog.
 * @param server
 * @param mpi_program
 */
static gboolean flow_io_run_dialog(GebrCommServerRunConfig *config, struct server *server, gboolean mpi_program)
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
							_("Empty number"), _("Please enter the number of processes to run the flow."));
				g_free(config->num_processes);
				config->num_processes = NULL;
			}
			else {
				num_processes_validated = TRUE;
			}
		}
	} while (!(moab_server_validated && queue_name_validated && num_processes_validated));

	gebr_geoxml_flow_io_set_from_server(gebr.flow, gebr.flow_server);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
out:
	gtk_widget_destroy(dialog);
	return ret;
}

/*
 * flow_io_run:
 *
 * Check for current server and if its connected, for the queue selected.
 */
static void flow_io_run(GebrGeoXmlFlowServer * flow_server, gboolean parallel, gboolean single)
{
	GtkTreeIter iter;
	const gchar *address;
	struct server *server;
	GebrCommServerRunConfig *config;
	gboolean mpi_program;
	GtkTreeSelection *selection;
	gboolean multiple;

	if (!flow_browse_get_selected(NULL, FALSE)) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No flow selected."));
		return;
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	multiple = !single && gtk_tree_selection_count_selected_rows(selection) > 1;

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &iter)) {
		//just happen if the current server was removed
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No server selected."));
		return;
	}

	/* initialization */
	config = gebr_comm_server_run_config_new();
	config->parallel = parallel;

	/* find iter */
	address = gebr_geoxml_flow_server_get_address(flow_server);
	if (!server_find_address(address, &iter)) {
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, "Server should be present on list!");
		goto err;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter, SERVER_POINTER, &server, -1);
	
	/* check connection */
	if (!gebr_comm_server_is_logged(server->comm)) {
		if (gebr_comm_server_is_local(server->comm))
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to the local server."), server->comm->address->str);
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
				     _("You are not connected to server '%s'."), server->comm->address->str);
		goto err;
	}
	
	mpi_program = (gebr_geoxml_flow_get_first_mpi_program(gebr.flow) != NULL);

	/* SET config->queue */
	if (server->type == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, 1, &config->queue, -1);
			if (!flow_io_run_dialog(config, server, mpi_program))
				goto err;
		}
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("No available queue for server '%s'."), server->comm->address->str);

	} else {
		/* Common servers. */
		gboolean is_immediately = gtk_combo_box_get_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox)) == 0;

		if (multiple && !parallel) {
			GtkTreeIter queue_iter;
			gchar *internal_queue_name;
			struct job *job;
			GString *new_internal_queue_name = g_string_new(NULL);

			if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &iter))
				gtk_tree_model_get_iter_first(GTK_TREE_MODEL(server->queues_model), &iter);

			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter,
					   1, &internal_queue_name, 2, &job, -1);

			/* If the first flow is marked to run Immediately or it has a Job Queue selected,
			 * we must create a new queue name. The name is composed by the first and the last
			 * flow title in the selection.
			 */
			if (is_immediately || internal_queue_name[0] == 'j') {
				GString *queue_name = g_string_new(NULL);

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
				g_string_assign(queue_name, new_internal_queue_name->str+1);
				g_string_free(aux, TRUE);

				if (is_immediately || (internal_queue_name[0] == 'j' && job->status != JOB_STATUS_RUNNING)) {
					gtk_list_store_append(server->queues_model, &queue_iter);
					iter = queue_iter;
				}
				gtk_list_store_set(server->queues_model, &iter, 0, queue_name->str, 1, new_internal_queue_name->str, -1);

				g_string_free(queue_name, TRUE);
			} else
				g_string_assign(new_internal_queue_name, internal_queue_name);

			/* In this case, we have renamed the `internal_queue_name' to 'new_internal_queue_name', so we must
			 * send the request to the server. */
			if (!is_immediately && internal_queue_name[0] == 'j')
				gebr_comm_protocol_send_data(server->comm->protocol, server->comm->stream_socket,
							     gebr_comm_protocol_defs.rnq_def, 2, internal_queue_name, new_internal_queue_name->str);

			/* frees */
			config->queue = g_string_free(new_internal_queue_name, FALSE);
			g_free(internal_queue_name);
		} else if (parallel || is_immediately) {
			/* If the active combobox entry is the first one (index 0), then
			 * "Immediately" (a queue name starting with j) is selected as queue option. */
			config->queue = g_strdup("j");

			if (mpi_program && !flow_io_run_dialog(config, server, mpi_program))
			    goto err;
		} else {
			/* Other queue option is selected: after a running job (flow) or
			 * on a pre-existent queue. */
			gchar *internal_queue_name = NULL;
			struct job *job = NULL;

			/* Get queue name from the queues combobox. */
			if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), &iter))
				gtk_tree_model_get_iter_first(GTK_TREE_MODEL(server->queues_model), &iter);

			gtk_tree_model_get(GTK_TREE_MODEL(server->queues_model), &iter, 1, &internal_queue_name, 2, &job, -1);

			if (internal_queue_name && internal_queue_name[0] == 'j') {
				/* Prefix 'j' indicates a single running job. So, the user is placing a new
				 * job behind this single one, which denotes proper enqueuing. In this case,
				 * it is necessary to give a name to the new queue. */

				if (!flow_io_run_dialog(config, server, mpi_program)) {
					g_free(internal_queue_name);
					goto err;
				}

				/* A race condition can happen if the single running job finishes before
				 * assigning a name to the queue. We try to prevent this race condition here. */
				/* Set the entry in the queues model accordingly. */
				if (job->status == JOB_STATUS_RUNNING) {
					/* The single job is still running; so, its entry in the combobox is still valid. */
					gtk_list_store_set(server->queues_model, &iter, 1, config->queue, -1);
				} else {
					GtkTreeIter queue_iter;
					gtk_list_store_append(server->queues_model, &queue_iter);
					gtk_list_store_set(server->queues_model, &queue_iter, 1, config->queue, -1);
				}

				gebr_comm_protocol_send_data(server->comm->protocol, server->comm->stream_socket,
							     gebr_comm_protocol_defs.rnq_def, 2, internal_queue_name, config->queue);

				g_free(internal_queue_name);
			} 
			else {
				if (internal_queue_name) {
					config->queue = g_strdup(internal_queue_name);
					if (mpi_program) {
						if (!flow_io_run_dialog(config, server, mpi_program)) {
							goto err;
						}
					}
				}
			}
		}
	}

	gebr_geoxml_flow_io_set_from_server(gebr.flow, flow_server);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	flow_run(server, config, single);
	return;

err:
	gebr_comm_server_run_config_free(config);
}
