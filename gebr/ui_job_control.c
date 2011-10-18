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
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "ui_job_control.h"
#include "gebr.h"
#include "gebr-task.h"

/*
 * Prototypes
 */

static void job_control_on_cursor_changed(void);

static void on_text_view_populate_popup(GtkTextView * textview, GtkMenu * menu);

static GtkMenu * job_control_popup_menu(GtkWidget * widget, struct ui_job_control *ui_job_control);

static void job_control_queue_by_func(gboolean (* func)(void));

static void on_tree_store_insert_delete(GtkTreeModel *model,
                                        GtkTreePath *path);

/*
 * Public functions.
 */

struct ui_job_control *job_control_setup_ui(void)
{
	struct ui_job_control *ui_job_control;

	GtkWidget *hpanel;
	GtkWidget *vbox;
	GtkWidget *scrolled_window;
	GtkWidget *frame;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *text_view;
	GtkTextIter iter_end;

	/* alloc */
	ui_job_control = g_new(struct ui_job_control, 1);

	hpanel = gtk_hpaned_new();
	ui_job_control->widget = hpanel;

	/*
	 * Left side
	 */
	frame = gtk_frame_new(_("Jobs"));
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	ui_job_control->store = gtk_tree_store_new(JC_N_COLUMN,
	                                           GDK_TYPE_PIXBUF,
	                                           G_TYPE_BOOLEAN,
	                                           G_TYPE_STRING,
						   G_TYPE_STRING,
						   G_TYPE_STRING,
						   G_TYPE_POINTER,
						   G_TYPE_BOOLEAN);
	g_signal_connect(ui_job_control->store, "row-inserted", G_CALLBACK(on_tree_store_insert_delete), NULL);
	g_signal_connect(ui_job_control->store, "row-deleted", G_CALLBACK(on_tree_store_insert_delete), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_job_control->store), JC_SERVER_ADDRESS,
					     GTK_SORT_ASCENDING);
	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(ui_job_control->store), NULL);
	ui_job_control->view = gtk_tree_view_new_with_model(filter);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), JC_VISIBLE);
	g_object_unref(filter);
	
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_job_control->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_job_control->view), FALSE);
	g_signal_connect(GTK_OBJECT(ui_job_control->view), "cursor-changed",
			 G_CALLBACK(job_control_on_cursor_changed), NULL);

	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_job_control->view), col);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", JC_ICON);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", JC_TITLE);

	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_job_control->view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);

	/*
	 * Right side
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), vbox, TRUE, TRUE);

	ui_job_control->label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), ui_job_control->label, FALSE, TRUE, 0);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
	ui_job_control->issues_title_tag = gtk_text_buffer_create_tag(ui_job_control->text_buffer, "issues_title",
								      "invisible", TRUE, NULL);
	gtk_text_buffer_get_end_iter(ui_job_control->text_buffer, &iter_end);
	gtk_text_buffer_create_mark(ui_job_control->text_buffer, "end", &iter_end, FALSE);
	text_view = gtk_text_view_new_with_buffer(ui_job_control->text_buffer);
	g_signal_connect(text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), ui_job_control);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}
	ui_job_control->text_view = text_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_job_control->view),
						  (GebrGuiGtkPopupCallback) job_control_popup_menu, ui_job_control);

	return ui_job_control;
}

gboolean job_control_save(void)
{
	GtkTreeIter iter;
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;
	GtkTreeModel *model;

	gchar *fname;
	FILE *fp;

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *text;
	gchar * title;

	GebrTask *job;

	/* run file chooser */
	chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to save"), GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".txt");

	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Text (*.txt)"));
	gtk_file_filter_add_pattern(filefilter, "*.txt");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	fname = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!fname)
		return TRUE;

	/* save to file */
	fp = fopen(fname, "w");
	if (fp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not write file."));
		g_free(fname);
		return TRUE;
	}

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_get_depth(path) == 1) {
			gtk_tree_path_free(path);
			continue;
		}
		gtk_tree_path_free(path);

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		job_load_details(job);
		
		title = g_strdup_printf("---------- %s ---------\n", job->parent.title->str);
		fputs(title, fp);
		g_free(title);

		gtk_text_buffer_get_start_iter(gebr.ui_job_control->text_buffer, &start_iter);
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &end_iter);
		text = gtk_text_buffer_get_text(gebr.ui_job_control->text_buffer, &start_iter, &end_iter, FALSE);
		text = g_strdup_printf("%s\n\n", text);
		fputs(text, fp);

		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Saved Job information at \"%s\"."), fname);
		
		g_free(text);
	}
	
	fclose(fp);
	g_free(fname);
	return TRUE;
}

gboolean job_control_close(void)
{
	GtkTreeIter iter;
	GebrTask *job;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *rows;
	GList *rowrefs = NULL;
	gboolean retval = TRUE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);
	
	gtk_tree_model_get_iter(model, &iter, rows->data);
	gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);

	if (!rows->next) {
		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear Job \"%s\"?"),
		                                    job->parent.title->str))
			goto free_rows;
	} else {
		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear the selected Jobs?")))
			goto free_rows;
	}

	retval = FALSE;
	for (GList *i = rows; i; i = i->next) {
		GtkTreeRowReference *rowref = gtk_tree_row_reference_new(model, i->data);
		rowrefs = g_list_prepend(rowrefs, rowref);
	}

	for (GList *i = rowrefs; i; i = i->next) {
		GtkTreePath *path = gtk_tree_row_reference_get_path(i->data);
		if (gtk_tree_path_get_depth(path) == 1) {
			gtk_tree_path_free(path);
			continue;
		}

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			continue;
		}
		gtk_tree_path_free(path);

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		job_close(job, FALSE, TRUE);
	}

	job_control_on_cursor_changed();

	g_list_foreach(rowrefs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free(rowrefs);

free_rows:
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
	return retval;
}

void job_control_clear(gboolean force)
{
	if (!force && !gebr_gui_confirm_action_dialog(_("Clear all Jobs"),
						      _("Are you sure you want to clear all Jobs from all servers?")))
		return;

	gboolean job_control_clear_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		GebrTask *job;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_STRUCT, &job, JC_IS_JOB,
				   &is_job, -1);
		if (!is_job)
			return FALSE;
		job_close(job, force, FALSE);

		return FALSE;
	}
	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)job_control_clear_foreach, NULL); 
}

gboolean job_control_stop(void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	GebrTask *job;
	
	gboolean asked = FALSE;
	gint selected_rows = 0;
	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view));
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_get_depth(path) == 1) {
			gtk_tree_path_free(path);
			continue;
		}
		gtk_tree_path_free(path);

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		if (gebr_comm_server_is_logged(job->server->comm) == FALSE) {
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("You are not connected to Job's server."));
			continue;
		}
		
		if (selected_rows == 1)
		{
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel Job \"%s\"?"),
							   job->parent.title->str) == FALSE)
				return TRUE;
		}
		else if (!asked)
		{
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel the selected Jobs?")) == FALSE)
				return TRUE;
			asked = TRUE;
		}
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to cancel Job."));
		if (gebr_comm_server_is_local(job->server->comm) == FALSE) 
			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server \"%s\" to cancel Job \"%s\"."),
				     job->server->comm->address->str, job->parent.title->str);
		else 
			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking local server to cancel Job \"%s\"."),
				     job->parent.title->str);
			
		gebr_comm_protocol_socket_oldmsg_send(job->server->comm->socket, FALSE,
						      gebr_comm_protocol_defs.kil_def, 1,
						      job->parent.jid->str);
	}

	return TRUE;
}

gboolean job_control_get_selected(GtkTreeIter * iter, enum JobControlSelectionType check_type)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_job_control->view), iter)) {
		switch (check_type) {
		case JobControlDontWarnUnselection:
			break;
		case JobControlJobQueueSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Nothing selected."));
			break;
		case JobControlJobSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Job selected."));
			break;
		case JobControlQueueSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No queue selected."));
			break;
		default:
			break;
		}
		return FALSE;
	}

	gboolean is_job;
	GtkTreePath *path;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view));
	path = gtk_tree_model_get_path(model, iter);
	is_job = gtk_tree_path_get_depth(path) == 2 ? TRUE : FALSE;
	gtk_tree_path_free(path);
	if (check_type == JobControlJobSelection && !is_job) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a Job."));
		return FALSE;
	}
	if (check_type == JobControlQueueSelection && is_job) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a queue."));
		return FALSE;
	}

	return TRUE;
}

void job_control_selected(void)
{
	job_control_on_cursor_changed();
}

/*
 * \internal
 */
static void job_control_on_cursor_changed(void)
{
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_close"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_stop"), FALSE);

	gboolean has_finished = FALSE;
	gboolean has_execution = FALSE;
	gboolean has_job = FALSE;

	void get_state(GebrTask *job)
	{
		has_job = TRUE;
		has_execution = has_execution || job_is_running(job);
		has_finished = has_finished || job_has_finished(job);
	}

	GtkTreeIter iter;
	GtkTreeModelFilter *filter;
	filter = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view)));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {
		GebrTask *job;
		gboolean is_job;

		GtkTreeIter model_iter;
		gtk_tree_model_filter_convert_iter_to_child_iter(filter, &model_iter, &iter);

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &model_iter, JC_STRUCT, &job, JC_IS_JOB, &is_job, -1);

		if (is_job) {
			get_state(job);
			job_load_details(job);
		} else {
			GtkTreeIter iter_queue = model_iter;
			GtkTreeIter iter_child;
			gboolean has_children = gtk_tree_model_iter_children (GTK_TREE_MODEL(gebr.ui_job_control->store), &iter_child, &iter_queue);
			if (has_children) {
				GtkTreeIter filter_iter;
				gtk_tree_model_filter_convert_child_iter_to_iter(filter, &filter_iter, &iter_child);
				gtk_tree_selection_unselect_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)), &filter_iter);

				GtkTreeIter iter_child_first = iter_child;
				GtkTreeIter iter_child_last;
				while (has_children) {
					iter_child_last = iter_child;
					has_children = gtk_tree_model_iter_next (GTK_TREE_MODEL(gebr.ui_job_control->store), &iter_child);
				}

				GtkTreePath *path_first = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter_child_first);
				GtkTreePath *path_last = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter_child_last);
				GtkTreePath *fpath_first = gtk_tree_model_filter_convert_child_path_to_path(filter, path_first);
				GtkTreePath *fpath_last= gtk_tree_model_filter_convert_child_path_to_path(filter, path_last);
				gtk_tree_selection_select_range (gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)),
				                                 fpath_first, fpath_last);

				GtkTreeIter iter;
				gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {
					gboolean is_job;
					GtkTreeIter model_iter;
					gtk_tree_model_filter_convert_iter_to_child_iter(filter, &model_iter, &iter);

					gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &model_iter,
					                   JC_STRUCT, &job,
					                   JC_IS_JOB, &is_job,
					                   -1);
					if (is_job)
						get_state(job);
				}

				if (has_job) {
					gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), has_finished);
					gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), has_execution);
					gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_close"), has_finished);
					gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_stop"), has_execution);
					gtk_tree_selection_unselect_range (gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)), fpath_first, fpath_last);
					//gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)), &iter);
				}
				gtk_tree_path_free(path_first);
				gtk_tree_path_free(path_last);
				gtk_tree_path_free(fpath_first);
				gtk_tree_path_free(fpath_last);
			} else 
				job_load_details(NULL);
		}
	}
}

/**
 * \internal
 */
static void wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GtkTextView * text_view)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	g_object_set(G_OBJECT(text_view), "wrap-mode",
		     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
}

/**
 * \internal
 */
static void autoscroll_toggled(GtkCheckMenuItem * check_menu_item)
{
	gebr.config.job_log_auto_scroll = gtk_check_menu_item_get_active(check_menu_item);
}

/**
 * \internal
 */
static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(wordwrap_toggled), text_view);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);

	menu_item = gtk_check_menu_item_new_with_label(_("Auto-scroll"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(autoscroll_toggled), NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_auto_scroll);
}


/*
 * Queue Actions
 */
void job_control_queue_stop(void)
{
	job_control_queue_by_func(job_control_stop);
}

void job_control_queue_save(void)
{
	job_control_queue_by_func(job_control_save);
}

void job_control_queue_close(void)
{
	job_control_queue_by_func(job_control_close);
}


/**
 * \internal
 * Build popup menu
 */
static GtkMenu *job_control_popup_menu(GtkWidget * widget, struct ui_job_control *ui_job_control)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkTreeIter iter;
	GtkTreeIter iter_child;
	gint iter_depth = 0;

	gint selected_rows = 0;

	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)));

	menu = gtk_menu_new();
	GtkTreeModelFilter *filter;
	filter = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view)));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {
		GtkTreeIter model_iter;
		gtk_tree_model_filter_convert_iter_to_child_iter(filter, &model_iter, &iter);

		iter_depth = gtk_tree_store_iter_depth(gebr.ui_job_control->store, &model_iter);

		if (iter_depth == 0 && gtk_tree_model_iter_children (GTK_TREE_MODEL(gebr.ui_job_control->store), &iter_child, &model_iter))
		{
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(
			                		  gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_save")));
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(
			                		  gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_close")));
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(
			                		  gtk_action_group_get_action(gebr.action_group_job_control, "job_control_queue_stop")));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			goto out;
		}

		if (iter_depth > 0)
		{
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save")));
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close")));
			gtk_container_add(GTK_CONTAINER(menu),
			                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop")));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			goto out;
		}
	}

out:
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_job_control->view);

	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_job_control->view);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void job_control_queue_by_func(gboolean (* func)(void))
{
	GList *rows;
	GList *unselect = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	/* By the end of this 'for' loop, the selected jobs have this characteristics:
	 *  - It is selected, or
	 *  - Its parent is selected
	 */

	if (!rows)
		return;

	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path = i->data;
		if (gtk_tree_path_get_depth(path) == 1) {
			int n, first;
			first = gtk_tree_path_get_indices(path)[0];
			gtk_tree_model_get_iter(model, &iter, path);
			n = gtk_tree_model_iter_n_children(model, &iter);
			for (int j = 0; j < n; j++) {
				GtkTreePath *newpath;
				newpath = gtk_tree_path_new_from_indices(first, j, -1);
				gtk_tree_selection_select_path(selection, newpath);
				gtk_tree_path_free(newpath);
			}
			unselect = g_list_prepend(unselect, path);
		} else {
			GtkTreePath *up = gtk_tree_path_copy(path);
			gtk_tree_path_up(up);
			if (!gtk_tree_selection_path_is_selected(selection, up))
				gtk_tree_selection_select_path(selection, path);
			gtk_tree_path_free(up);
		}
	}

	for (GList *i = unselect; i; i = i->next)
		gtk_tree_selection_unselect_path(selection, i->data);

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
	g_list_free(unselect);

	func();
}

static void
on_tree_store_insert_delete(GtkTreeModel *model,
                            GtkTreePath *path)
{
	GtkTreeIter iter;
	GtkTreePath *copy = gtk_tree_path_copy(path);

	if (gtk_tree_path_get_depth(copy) == 2)
		gtk_tree_path_up(copy);

	if (!gtk_tree_model_get_iter(model, &iter, copy))
		goto free_copy;

	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
	                   JC_VISIBLE, gtk_tree_model_iter_has_child(model, &iter),
	                   -1);

free_copy:
	gtk_tree_path_free(copy);
}
