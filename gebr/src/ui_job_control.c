/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team (http://sourceforge.net)
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

#include "ui_job_control.h"
#include "gebr.h"
#include "support.h"
#include "job.h"

/*
 * File: ui_job_control.c
 * Responsible for UI for job management.
 */

/*
 * Prototypes
 */

static void
job_control_clicked(void);

static void
job_control_save(void);

static void
job_control_cancel(void);

static void
job_control_close(void);

static void
job_control_clear(void);

static void
job_control_stop(void);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: job_control_setup_ui
 * Assembly the job control page.
 *
 * Return:
 * The structure containing relevant data.
 *
 */
struct ui_job_control *
job_control_setup_ui(void)
{
	struct ui_job_control *		ui_job_control;
	GtkWidget *			page;
	GtkWidget *			vbox;

	GtkWidget *			toolbar;
	GtkWidget *			toolitem;
	GtkWidget *			button;

	GtkWidget *			hpanel;
	GtkWidget *			scrolledwin;
	GtkWidget *			frame;

	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	GtkWidget *			text_view;

	/* alloc */
	ui_job_control = g_malloc(sizeof(struct ui_job_control));

	/* Create flow edit page */
	page = gtk_vbox_new(FALSE, 0);
	ui_job_control->widget = page;

	/* Vbox to hold toolbar and main content */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page), vbox);

	/* Toolbar */
	toolbar = gtk_toolbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	/* FIXME ! */
	/* g_object_set_property(G_OBJECT(toolbar), "shadow-type", GTK_SHADOW_NONE); */

	/* Save */
	toolitem = GTK_WIDGET(gtk_tool_item_new());
	gtk_container_add(GTK_CONTAINER(toolbar), toolitem);
	button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(toolitem), button);
	set_tooltip(button, _("Save job information in a file"));

	g_signal_connect(GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_control_save), NULL);

	/* Cancel button = END */
	toolitem = GTK_WIDGET(gtk_tool_item_new());
	gtk_container_add(GTK_CONTAINER(toolbar), toolitem);
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(toolitem), button);
	set_tooltip(button, _("Ask server to terminate the job"));

	g_signal_connect(GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_control_cancel), NULL);

	/* Close button */
	toolitem =(GtkWidget*)gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(toolbar), toolitem);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(toolitem), button);
	set_tooltip(button, _("Clear current job log"));

	g_signal_connect(GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_control_close), NULL);

	/* Clear button */
	toolitem =(GtkWidget*) gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(toolbar), toolitem);

	button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(toolitem), button);
	set_tooltip(button, _("Clear all job logs"));

	g_signal_connect(GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_control_clear), NULL);

	/* Stop button = KILL */
	toolitem =(GtkWidget*) gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(toolbar), toolitem);

	button = gtk_button_new_from_stock(GTK_STOCK_STOP);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(toolitem), button);
	set_tooltip(button, _("Ask server to kill the job"));

	g_signal_connect(GTK_BUTTON(button), "clicked",
			GTK_SIGNAL_FUNC(job_control_stop), NULL);

	hpanel = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), hpanel, TRUE, TRUE, 0);

	/*
	 * Left side
	 */
	frame = gtk_frame_new("Jobs");
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolledwin);

	ui_job_control->store = gtk_list_store_new(JC_N_COLUMN,
					GDK_TYPE_PIXBUF,	/* Icon		*/
					G_TYPE_STRING,		/* Title	*/
					G_TYPE_POINTER);	/* struct job	*/

	ui_job_control->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_job_control->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_job_control->view), FALSE);

	g_signal_connect(GTK_OBJECT(ui_job_control->view), "cursor-changed",
		GTK_SIGNAL_FUNC(job_control_clicked), NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_job_control->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", JC_ICON);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_job_control->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", JC_TITLE);

	gtk_container_add(GTK_CONTAINER(scrolledwin), ui_job_control->view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolledwin), 180, 30);

	/*
	 * Right side
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), vbox, TRUE, TRUE);

	ui_job_control->label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), ui_job_control->label, FALSE, TRUE, 0);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(vbox), scrolledwin, TRUE, TRUE, 0);

	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(ui_job_control->text_buffer);
	g_object_set(G_OBJECT(text_view),
		"editable", FALSE,
		"cursor-visible", FALSE,
		NULL);
	ui_job_control->text_view = text_view;
	gtk_container_add(GTK_CONTAINER(scrolledwin), text_view);

	return ui_job_control;
}

/*
 * Function; job_control_clear_or_select_first
 * *Fill me in!*
 */
void
job_control_clear_or_select_first(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;

	/* select the first job */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter) == TRUE) {
		gtk_tree_selection_select_iter(selection, &iter);
		job_control_clicked();
	} else {
		gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), "");
		gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, "", 0);
	}
}



/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: job_control_clicked
 * *Fill me in!*
 */
static void
job_control_clicked(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	struct job *		job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);

	job_fill_info(job);
}

/*
 * Function: job_control_save
 * *Fill me in!*
 */
static void
job_control_save(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;

	gchar *			path;
	FILE *			fp;

	GtkTextIter		start_iter;
	GtkTextIter		end_iter;
	gchar *			text;

	struct job *	        job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Text (*.txt)"));
	gtk_file_filter_add_pattern(filefilter, "*.txt");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out2;
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));

	/* save to file */
	fp = fopen(path, "w");
	if (fp == NULL) {
		gebr_message(ERROR, TRUE, TRUE, _("Could not write file"));
		goto out;
	}
	gtk_text_buffer_get_start_iter(gebr.ui_job_control->text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &end_iter);
	text = gtk_text_buffer_get_text(gebr.ui_job_control->text_buffer, &start_iter, &end_iter, FALSE);
	fputs(text, fp);
	fclose(fp);

	gebr_message(INFO, TRUE, TRUE, _("Saved job information at '%s'"), path);

	g_free(text);
out:	g_free(path);
out2:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: job_control_cancel
 * *Fill me in!*
 */
void
job_control_cancel(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	struct job *	        job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);

	if (job->status != JOB_STATUS_RUNNING) {
		gebr_message(WARNING, TRUE, FALSE, _("Job is not running"));
		return;
	}
	if (confirm_action_dialog(_("Are you sure you want to terminate job '%s'?"), job->title->str) == FALSE)
		return;

	gebr_message(INFO, TRUE, FALSE, _("Asking server to terminate job"));
	gebr_message(INFO, FALSE, TRUE, _("Asking server '%s' to terminate job '%s'"), job->server->address, job->title->str);

	protocol_send_data(job->server->protocol, job->server->tcp_socket,
		protocol_defs.end_def, 1, job->jid->str);
}

/*
 * Function: job_control_close
 * *Fill me in!*
 */
void
job_control_close(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	struct job *		job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);

	if (confirm_action_dialog(_("Are you sure you want to clear job '%s'?"), job->title->str) == FALSE)
		return;

	job_close(job);
	job_control_clear_or_select_first();
}

/*
 * Function: job_control_clear
 * *Fill me in!*
 */
void
job_control_clear(void)
{
	GtkTreeIter		iter;
	gboolean		valid;

	if (confirm_action_dialog(_("Are you sure you want to clear all jobs from all servers?")) == FALSE)
		return;

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		/* go to next before the possible deletion */
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);

		job_close(job);
	}
	job_control_clear_or_select_first();
}

/*
 * Function: job_control_stop
 * *Fill me in!*
 */
void
job_control_stop(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	struct job *	        job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
			JC_STRUCT, &job,
			-1);

	if (job->status != JOB_STATUS_RUNNING) {
		gebr_message(WARNING, TRUE, FALSE, _("Job is not running"));
		return;
	}
	if (confirm_action_dialog(_("Are you sure you want to kill job '%s'?"), job->title->str) == FALSE)
		return;

	gebr_message(INFO, TRUE, FALSE, _("Asking server to kill job"));
	gebr_message(INFO, FALSE, TRUE, _("Asking server '%s' to kill job '%s'"), job->server->address->str, job->title->str);

	protocol_send_data(job->server->protocol, job->server->tcp_socket,
		protocol_defs.kil_def, 1, job->jid->str);
}
