/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>

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
on_text_view_populate_popup(GtkTextView * textview, GtkMenu * menu); 

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

	GtkWidget *			hpanel;
	GtkWidget *			vbox;
	GtkWidget *			scrolled_win;
	GtkWidget *			frame;

	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	GtkWidget *			text_view;

	/* alloc */
	ui_job_control = g_malloc(sizeof(struct ui_job_control));

	hpanel = gtk_hpaned_new();
	ui_job_control->widget = hpanel;

	/*
	 * Left side
	 */
	frame = gtk_frame_new("Jobs");
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_win);

	ui_job_control->store = gtk_list_store_new(JC_N_COLUMN,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER);

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

	gtk_container_add(GTK_CONTAINER(scrolled_win), ui_job_control->view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), 180, 30);

	/*
	 * Right side
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), vbox, TRUE, TRUE);

	ui_job_control->label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), ui_job_control->label, FALSE, TRUE, 0);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(ui_job_control->text_buffer);
	g_signal_connect(text_view, "populate-popup", (GCallback)on_text_view_populate_popup, NULL);
	g_object_set(G_OBJECT(text_view),
		"editable", FALSE,
		"cursor-visible", FALSE,
		NULL);
	{
		PangoFontDescription* font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "courier 10 pitch");
		pango_font_description_set_style(font, PANGO_STYLE_NORMAL);

		gtk_widget_modify_font(text_view, font);
		pango_font_description_free(font);
	}
	ui_job_control->text_view = text_view;
	gtk_container_add(GTK_CONTAINER(scrolled_win), text_view);

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
 * Function: job_control_save
 * *Fill me in!*
 */
void
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
		gebr_message(LOG_ERROR, TRUE, TRUE, _("Could not write file"));
		goto out;
	}
	gtk_text_buffer_get_start_iter(gebr.ui_job_control->text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &end_iter);
	text = gtk_text_buffer_get_text(gebr.ui_job_control->text_buffer, &start_iter, &end_iter, FALSE);
	fputs(text, fp);
	fclose(fp);

	gebr_message(LOG_INFO, TRUE, TRUE, _("Saved job information at '%s'"), path);

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
		gebr_message(LOG_WARNING, TRUE, FALSE, _("Job is not running"));
		return;
	}
	if (comm_server_is_logged(job->server->comm) == FALSE) {
		gebr_message(LOG_WARNING, TRUE, FALSE, _("You are not connected to job's server"));
		return;
	}
	if (confirm_action_dialog(_("Terminate job"), _("Are you sure you want to terminate job '%s'?"), job->title->str) == FALSE)
		return;

	gebr_message(LOG_INFO, TRUE, FALSE, _("Asking server to terminate job"));
	if (comm_server_is_local(job->server->comm) == FALSE) {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking server '%s' to terminate job '%s'"),
			job->server->comm->address->str, job->title->str);
	} else {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking local server to terminate job '%s'"),
			job->title->str);
	}

	protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
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

	if (confirm_action_dialog(_("Clear job "), _("Are you sure you want to clear job '%s'?"), job->title->str) == FALSE)
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

	if (confirm_action_dialog(_("Clear all jobs"), _("Are you sure you want to clear all jobs from all servers?")) == FALSE)
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
		gebr_message(LOG_WARNING, TRUE, FALSE, _("Job is not running"));
		return;
	}
	if (comm_server_is_logged(job->server->comm) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("You are not connected to job's server"));
		return;
	}
	if (confirm_action_dialog(_("Kill job"), _("Are you sure you want to kill job '%s'?"), job->title->str) == FALSE)
		return;

	gebr_message(LOG_INFO, TRUE, FALSE, _("Asking server to kill job"));
	if (comm_server_is_local(job->server->comm) == FALSE) {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking server '%s' to kill job '%s'"),
			job->server->comm->address->str, job->title->str);
	} else {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking local server to kill job '%s'"),
			job->title->str);
	}

	protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
		protocol_defs.kil_def, 1, job->jid->str);
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

static void
wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GtkTextView * text_view)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	g_object_set(G_OBJECT(text_view), "wrap-mode",
		gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
}

static void
on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu)
{
	GtkWidget *	menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", (GCallback)wordwrap_toggled, text_view);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);
}
