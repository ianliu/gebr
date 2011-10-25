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
#include "gebr-job.h"

/*
 * Prototypes
 */

static void icon_column_data_func(GtkTreeViewColumn *tree_column,
				  GtkCellRenderer *cell,
				  GtkTreeModel *tree_model,
				  GtkTreeIter *iter,
				  gpointer data);

static void title_column_data_func(GtkTreeViewColumn *tree_column,
				   GtkCellRenderer *cell,
				   GtkTreeModel *tree_model,
				   GtkTreeIter *iter,
				   gpointer data);

static void time_column_data_func(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer data);

static void gebr_job_control_load_details(GebrJobControl *jc,
					  GebrJob *job);

static void job_control_on_cursor_changed(GtkTreeSelection *selection,
					  GebrJobControl *jc);

static void on_toggled_more_details(GtkToggleButton *button,
                                    GtkBuilder *builder);

#if 0
static void on_text_view_populate_popup(GtkTextView * textview, GtkMenu * menu);

static GtkMenu * job_control_popup_menu(GtkWidget * widget, struct ui_job_control *ui_job_control);

#endif
static void job_control_queue_by_func(gboolean (* func)(void));

#if 0
static void on_tree_store_insert_delete(GtkTreeModel *model,
                                        GtkTreePath *path);
#endif

/*
 * Public functions.
 */

GebrJobControl *job_control_setup_ui(void)
{
	GebrJobControl *ui_job_control;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkBuilder *builder;

	ui_job_control = g_new(GebrJobControl, 1);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, GEBR_GLADE_DIR"/gebr-job-control.glade", NULL);

	ui_job_control->widget = GTK_WIDGET(gtk_builder_get_object(builder, "top-level-widget"));

	/*
	 * Left side
	 */

	ui_job_control->store = gtk_tree_store_new(JC_N_COLUMN,
	                                           G_TYPE_STRING,  /* JC_SERVER_ADDRESS */
						   G_TYPE_STRING,  /* JC_QUEUE_NAME */
						   G_TYPE_POINTER, /* JC_STRUCT */
						   G_TYPE_BOOLEAN);/* JC_VISIBLE */

	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(ui_job_control->store), NULL);

	GtkTreeView *treeview;
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_jobs"));
	gtk_tree_view_set_model(treeview, filter);

	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), JC_VISIBLE);
	g_object_unref(filter);
	
	ui_job_control->view = GTK_WIDGET(treeview);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_job_control->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_job_control->view), FALSE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_job_control->view)), "changed",
			 G_CALLBACK(job_control_on_cursor_changed), ui_job_control);

	col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "tv_column"));

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tv_icon_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, icon_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tv_title_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, title_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tv_time_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, time_column_data_func, NULL, NULL);

	/*
	 * Right side
	 */

	GtkWidget *text_view;
	GtkTextIter iter_end;

	GtkToggleButton *details = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "more_details"));
	g_signal_connect(details, "toggled", G_CALLBACK(on_toggled_more_details), builder);
	gtk_toggle_button_set_active(details, FALSE);

	/* Text view of output*/
	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(ui_job_control->text_buffer, &iter_end);
	gtk_text_buffer_create_mark(ui_job_control->text_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(builder, "textview_output"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), ui_job_control->text_buffer);

	//g_signal_connect(text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), ui_job_control);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}

	ui_job_control->text_view = text_view;

	/* Text view of command line */
	ui_job_control->cmd_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(ui_job_control->cmd_buffer, &iter_end);
	gtk_text_buffer_create_mark(ui_job_control->cmd_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(builder, "textview_command_line"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), ui_job_control->cmd_buffer);

//	g_signal_connect(text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), ui_job_control);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}

	ui_job_control->cmd_view = text_view;


//	vbox = gtk_vbox_new(FALSE, 0);
//	gtk_paned_pack2(GTK_PANED(hpanel), vbox, TRUE, TRUE);
//
//	ui_job_control->label = gtk_label_new("");
//	gtk_box_pack_start(GTK_BOX(vbox), ui_job_control->label, FALSE, TRUE, 0);
//
//	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
//	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
//				       GTK_POLICY_AUTOMATIC);
//	gtk_box_pack_end(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
//
//	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
//	ui_job_control->issues_title_tag = gtk_text_buffer_create_tag(ui_job_control->text_buffer, "issues_title",
//								      "invisible", TRUE, NULL);
//	gtk_text_buffer_get_end_iter(ui_job_control->text_buffer, &iter_end);
//	gtk_text_buffer_create_mark(ui_job_control->text_buffer, "end", &iter_end, FALSE);
//	text_view = gtk_text_view_new_with_buffer(ui_job_control->text_buffer);
//	//g_signal_connect(text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), ui_job_control);
//	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
//	{
//		PangoFontDescription *font;
//
//		font = pango_font_description_new();
//		pango_font_description_set_family(font, "monospace");
//		gtk_widget_modify_font(text_view, font);
//
//		pango_font_description_free(font);
//	}
//	ui_job_control->text_view = text_view;
//	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	
	//gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_job_control->view),
	//					  (GebrGuiGtkPopupCallback) job_control_popup_menu, ui_job_control);

	return ui_job_control;
}

#if 0
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

	GebrJob *job;

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
		
		title = g_strdup_printf("---------- %s ---------\n", gebr_job_get_title(job));
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
#endif

gboolean job_control_stop(void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	GebrJob *job;
	
	gboolean asked = FALSE;
	gint selected_rows = 0;
	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view));
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_job_control->view) {

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		
		if (selected_rows == 1)
		{
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel Job \"%s\"?"),
							   gebr_job_get_title(job)) == FALSE)
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
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server(s) \"%s\" to cancel Job \"%s\"."),
			     gebr_job_get_servers(job), gebr_job_get_title(job));

		gebr_job_kill(job);
	}

	return TRUE;
}

gboolean job_control_close(void)
{
	GtkTreeIter iter;
	GebrJob *job;
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
		                                    _("Are you sure you want to clear Job \"%s\"?"), gebr_job_get_title(job)))
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

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			continue;
		}
		gtk_tree_path_free(path);

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		gebr_job_close(job);
	}

	job_control_on_cursor_changed(selection, gebr.ui_job_control);

	g_list_foreach(rowrefs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free(rowrefs);

free_rows:
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
	return retval;
}

#if 0
void job_control_clear(gboolean force)
{
	if (!force && !gebr_gui_confirm_action_dialog(_("Clear all Jobs"),
						      _("Are you sure you want to clear all Jobs from all servers?")))
		return;

	gboolean job_control_clear_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		GebrJob *job;
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
#endif

static void
job_control_on_cursor_changed(GtkTreeSelection *selection,
			      GebrJobControl *jc)
{
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);

	if(!rows)
		return;

	if (!rows->next) {
		GebrJob *job;
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(jc->store), &iter, (GtkTreePath*)rows->data)) {
			gtk_tree_model_get(GTK_TREE_MODEL(jc->store), &iter, JC_STRUCT, &job, -1);
			if (job)
				gebr_job_control_load_details(jc, job);
		} else
			g_warn_if_reached();
	} else
		g_debug("MULTIPLE SELECTION! SURFIN BIRD");

	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);
}

#if 0
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
void job_control_queue_save(void)
{
	job_control_queue_by_func(job_control_save);
}

#endif

void job_control_queue_stop(void)
{
	job_control_stop();
}

void job_control_queue_close(void)
{
	job_control_close();
}

#if 0
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
#endif

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

#if 0
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

#endif

static void
on_toggled_more_details(GtkToggleButton *button,
                        GtkBuilder *builder)
{
	GtkVBox *details = GTK_VBOX(gtk_builder_get_object(builder, "details_widget"));

	if (!gtk_toggle_button_get_active(button)) {
		gtk_widget_set_visible(GTK_WIDGET(details), FALSE);
		return;
	}
	gtk_widget_set_visible(GTK_WIDGET(details), TRUE);
}

void
gebr_jc_get_queue_group_iter(GtkTreeStore *store,
			     const gchar  *queue,
			     const gchar  *group,
			     GtkTreeIter  *iter)
{
	GtkTreeIter it;
	GtkTreeModel *model = GTK_TREE_MODEL(store);
	gboolean valid = gtk_tree_model_get_iter_first(model, &it);

	gboolean are_queues_equal(const gchar *queue1, const gchar *queue2)
	{
		if (g_strcmp0(queue1, queue2) == 0)
			return TRUE;
		if (gebr_get_queue_type(queue1) == AUTOMATIC_QUEUE
		    && gebr_get_queue_type(queue2) == AUTOMATIC_QUEUE)
			return TRUE;

		return FALSE;
	}

	while (valid) {
		gchar *g, *q;
		gtk_tree_model_get(model, &it,
				   JC_SERVER_ADDRESS, &g, /* GEBR_JC_GROUP */
				   JC_QUEUE_NAME, &q, /* GEBR_JC_QUEUE */
				   -1);

		g_debug("------- comparing with %s Versus %s", q, queue);

		if (g_strcmp0(group, g) == 0 && are_queues_equal(queue, q)) {
			*iter = it;
			g_free(g);
			g_free(q);
			return;
		}

		g_free(g);
		g_free(q);
		valid = gtk_tree_model_iter_next(model, &it);
	}

	g_debug("Created a father %s!", queue);

	gtk_tree_store_append(store, iter, NULL);
	gtk_tree_store_set(store, iter,
			   JC_SERVER_ADDRESS, group,
			   JC_QUEUE_NAME, queue,
			   JC_VISIBLE, TRUE,
			   -1);
}

static void
icon_column_data_func(GtkTreeViewColumn *tree_column,
		      GtkCellRenderer *cell,
		      GtkTreeModel *tree_model,
		      GtkTreeIter *iter,
		      gpointer data)
{
	GebrJob *job;
	const gchar *stock_id;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);

	switch (gebr_job_get_status(job))
	{
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FAILED:
		stock_id = GTK_STOCK_CANCEL;
		break;
	case JOB_STATUS_FINISHED:
		stock_id = GTK_STOCK_APPLY;
		break;
	case JOB_STATUS_INITIAL:
		stock_id = GTK_STOCK_NETWORK;
		break;
	case JOB_STATUS_QUEUED:
		stock_id = "chronometer";
		break;
	case JOB_STATUS_RUNNING:
		stock_id = GTK_STOCK_EXECUTE;
		break;
	case JOB_STATUS_ISSUED:
	case JOB_STATUS_REQUEUED:
		break;
	}

	g_object_set(cell, "stock-id", stock_id, NULL);
}

static void
title_column_data_func(GtkTreeViewColumn *tree_column,
		       GtkCellRenderer *cell,
		       GtkTreeModel *tree_model,
		       GtkTreeIter *iter,
		       gpointer data)
{
	GebrJob *job;
	gchar *servers, *queue;

	gtk_tree_model_get(tree_model, iter,
			   JC_STRUCT, &job,
			   JC_SERVER_ADDRESS, &servers,
			   JC_QUEUE_NAME, &queue,
			   -1);

	g_object_set(cell, "text", gebr_job_get_title(job), NULL);

	g_free(queue);
	g_free(servers);
}

static void time_column_data_func(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer data)
{
	GebrJob *job;

	gtk_tree_model_get(tree_model, iter,
	                   JC_STRUCT, &job,
	                   -1);

	g_object_set(cell, "text", "moments ago", NULL);

	return;
}

void
gebr_job_control_select_job(GebrJobControl *jc, const gchar *rid)
{
	GebrJob *job = gebr_job_find(rid);

	g_debug("SELECT JOB: %p", job);

	if (job) {
		GtkTreeModel *filter = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->view));
		GtkTreeIter iter = gebr_job_get_iter(job), filter_iter;

		if (!gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter), &filter_iter, &iter))
			return;

		gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(jc->view), &filter_iter);
	}
}

static void
add_job_issues(GebrJobControl *jc, const gchar *issues)
{
	g_object_set(jc->issues_title_tag, "invisible", FALSE, NULL);

	GtkTextMark * mark = gtk_text_buffer_get_mark(jc->text_buffer, "last-issue");
	if (mark != NULL) {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(jc->text_buffer, &iter, mark);
		gtk_text_buffer_insert_with_tags(jc->text_buffer, &iter, issues, strlen(issues),
						 jc->issues_title_tag, NULL);

		gtk_text_buffer_delete_mark(jc->text_buffer, mark);
		gtk_text_buffer_create_mark(jc->text_buffer, "last-issue", &iter, TRUE);
	} else
		g_warning("Can't find mark \"issue\"");
}

static void
gebr_job_control_load_details(GebrJobControl *jc,
			      GebrJob *job)
{
	g_return_if_fail(job != NULL);


	enum JobStatus status = gebr_job_get_status(job);
	GString *queue_info = g_string_new(NULL); 

	const gchar *start_date = gebr_job_get_start_date(job);
	const gchar *finish_date = gebr_job_get_finish_date(job);


//	if (status == JOB_STATUS_INITIAL) {
//		g_string_append_printf(info, _("Waiting for more details from the server..."));
//		gtk_label_set_text(GTK_LABEL(jc->label), info->str);
//		goto out;
//	} else {
//		GString *label;
//		const gchar *queue = gebr_job_get_queue(job);
//
//		if (gebr_get_queue_type(queue) == AUTOMATIC_QUEUE)
//			g_string_assign(queue_info, _("without queue"));
//		else
//			g_string_printf(queue_info, _("on %s"), queue);
//
//		g_string_append_printf(info, _("Job submitted to '%s' ('%s') by %s"),
//				       gebr_job_get_servers(job), queue_info->str, g_get_host_name()); // FIXME: Hostname
//		label = g_string_new(info->str);
//
//		if (start_date && strlen(start_date)) {
//			g_string_append_printf(label, "\n%s", gebr_localized_date(start_date));
//			if (finish_date && strlen(finish_date))
//				g_string_append_printf(label, " - %s", gebr_localized_date(finish_date));
//			else if(status == JOB_STATUS_FAILED)
//				g_string_append(label, _(" (Failed)"));
//			else
//				g_string_append(label, _(" (running)"));
//		}
//		g_string_free(queue_info, TRUE);
//		gtk_label_set_text(GTK_LABEL(jc->label), label->str);
//	}
//
//	g_string_append(info, "\n");
//
//	/* moab job id */
//	//if (job->server->type == GEBR_COMM_SERVER_TYPE_MOAB && job->parent.moab_jid->len)
//	//	g_string_append_printf(info, "\n%s\n%s\n", _("Moab Job ID:"), job->parent.moab_jid->str);
//
//	gtk_text_buffer_insert_at_cursor(jc->text_buffer, info->str, info->len);
//
//	/* issues title with tag and mark, for receiving issues */
//	g_object_set(jc->issues_title_tag, "invisible", TRUE, NULL);
//	gtk_text_buffer_get_end_iter(jc->text_buffer, &end_iter);
//	g_string_assign(info, _("Issues:\n"));
//	gtk_text_buffer_insert_with_tags(jc->text_buffer, &end_iter, info->str, info->len,
//					 jc->issues_title_tag, NULL);
//	g_string_assign(info, "");
//	gtk_text_buffer_get_end_iter(jc->text_buffer, &end_iter);
//	gtk_text_buffer_create_mark(jc->text_buffer, "last-issue", &end_iter, FALSE);
//
//	if (status == JOB_STATUS_QUEUED)
//		goto out;
//
//	/* issues */
//	gchar *issues = gebr_job_get_issues(job);
//	if (issues && strlen(issues) > 0) {
//		add_job_issues(jc, issues);
//		g_free(issues);
//	}

	GString *info = g_string_new("");
	GString *info_cmd = g_string_new("");
	GtkTextIter end_iter;
	GtkTextIter end_iter_cmd;

	gtk_text_buffer_set_text(jc->text_buffer, "", 0);
	gtk_text_buffer_set_text(jc->cmd_buffer, "", 0);

	/* command-line */
	gchar *cmdline = gebr_job_get_command_line(job);
	g_string_append_printf(info_cmd, "\n%s\n%s\n", _("Command line:"), cmdline);
	g_free(cmdline);

	/* start date (may have failed, never started) */
	if (start_date && strlen(start_date))
		g_string_append_printf(info, "\n%s %s\n", _("Start date:"),
				       gebr_localized_date(start_date));

	/* output */
	g_string_append(info, gebr_job_get_output(job));

	/* finish date */
	if (finish_date && strlen(finish_date))
		g_string_append_printf(info, "\n%s %s",
				       status == JOB_STATUS_FINISHED ? _("Finish date:") : _("Cancel date:"),
				       gebr_localized_date(finish_date));

out:
	gtk_text_buffer_get_end_iter(jc->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->text_buffer, &end_iter, info->str, info->len);

	gtk_text_buffer_get_end_iter(jc->cmd_buffer, &end_iter_cmd);
	gtk_text_buffer_insert(jc->cmd_buffer, &end_iter_cmd, info_cmd->str, info_cmd->len);

	/* frees */
	g_string_free(info, TRUE);
	g_string_free(info_cmd, TRUE);
}
