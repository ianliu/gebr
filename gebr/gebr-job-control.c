/*
 * gebr-job-control.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>
#include <libgebr/gui/gebr-gui-pie.h>

#include "gebr-job-control.h"
#include "gebr.h"
#include "gebr-job.h"

typedef struct {
	GebrJob *job;
	guint sig_output;
	guint sig_status;
	guint sig_issued;
	guint sig_cmd_line;
	guint sig_button;
} LastSelection;

typedef struct {
	gdouble r, g, b;
} Color;

struct _GebrJobControlPriv {
	GtkBuilder *builder;
	GtkComboBox *flow_combo;
	GtkComboBox *server_combo;
	GtkComboBox *status_combo;
	GtkListStore *server_filter;
	GtkListStore *status_model;
	GtkListStore *store;
	GtkTextBuffer *text_buffer;
	GtkTextMark *text_mark;
	GtkListStore *flow_filter;
	GList *cmd_views;
	GtkWidget *filter_info_bar;
	GtkWidget *label;
	GtkWidget *text_view;
	GtkWidget *output_window;
	GtkWidget *view;
	GtkWidget *widget;
	GtkWidget *info_button;
	LastSelection last_selection;
	guint timeout_source_id;

	gboolean time_control[TIME_N_TYPES];

	gboolean use_filter_status;
	gboolean use_filter_servers;
	gboolean use_filter_flow;

	gboolean restore_mode;
	gboolean automatic_filter;
	GebrCommJobStatus user_status_filter_id;
	GebrMaestroServerGroupType user_server_type_filter;
	gchar *user_server_name_filter;
	gchar *user_server_filter_id;
	gchar *user_flow_filter_id;

	struct {
		gchar **servers;
		gdouble *percentages;
	} servers_info;
};

enum {
	JC_STRUCT,
	JC_TEXT_CONTROL,
	JC_IS_CONTROL,
	JC_CONTROL_TYPE,
	JC_N_COLUMN
};

enum {
	ST_ICON,
	ST_TEXT,
	ST_STATUS,
	ST_N_COLUMN
};

#define BLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_block_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), \
					job_control_on_cursor_changed, jc);

#define UNBLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_unblock_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), \
					  job_control_on_cursor_changed, jc);

/* Prototypes {{{1 */
static gboolean update_tree_view(gpointer data);

static gchar * compute_relative_time(GebrJob *job,
                                     TimesType *type,
                                     gdouble *delta,
                                     GebrJobControl *jc);

static gboolean jobs_visible_func(GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  GebrJobControl *jc);

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

static void gebr_job_control_load_details(GebrJobControl *jc,
					  GebrJob *job);

static void job_control_on_cursor_changed(GtkTreeSelection *selection,
					  GebrJobControl *jc);

static void on_toggled_more_details(GtkToggleButton *button,
                                    GtkBuilder *builder);

static void gebr_jc_update_status_and_time(GebrJobControl *jc,
                                           GebrJob 	  *job,
                                           GebrCommJobStatus status);

static void on_maestro_flow_filter_changed(GtkComboBox *combo,
                                           GebrJobControl *jc);

static void on_maestro_server_filter_changed(GtkComboBox *combo,
                                             GebrJobControl *jc);

static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu, GebrJobControl *jc);


static GtkMenu *job_control_popup_menu(GtkWidget * widget, GebrJobControl *job_control);

static void job_control_fill_servers_info(GebrJobControl *jc);

static void job_control_disconnect_signals(GebrJobControl *jc);

static void gebr_job_control_info_set_visible(GebrJobControl *jc,
					      gboolean visible,
					      const gchar *txt);

static void gebr_job_control_include_cmd_line(GebrJobControl *jc,
                                              GebrJob *job);

gboolean static update_user_defined_filters(GebrJobControl *jc);

void gebr_job_control_free_user_defined_filter(GebrJobControl *jc);
/* Private methods {{{1 */
static void
gebr_jc_get_jobs_state(GebrJobControl *jc,
		       GList *jobs,
		       gboolean *can_close,
		       gboolean *can_kill)
{
	*can_close = FALSE;
	*can_kill = FALSE;
	GtkTreeIter iter;
	GebrJob *job;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));

	for (GList *i = jobs; i; i = i->next) {
		gboolean control;
		if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data)) {
			gtk_tree_model_get(model, &iter,
			                   JC_STRUCT, &job,
			                   JC_IS_CONTROL, &control,
			                   -1);

			if (control)
				continue;

			if (!(*can_close) && gebr_job_can_close(job))
				*can_close = TRUE;

			if (!(*can_kill) && gebr_job_can_kill(job))
				*can_kill = TRUE;

			if (*can_close && *can_kill)
				break;
		}
	}
}

static void
update_control_buttons(GebrJobControl *jc,
		       gboolean can_close,
		       gboolean can_kill,
		       gboolean can_save)
{
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), can_close);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), can_kill);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save"), can_save);
}

static void
on_select_non_single_job(GebrJobControl *jc,
			 GList *rows)
{
	gboolean can_close, can_kill, can_save;

	if (!rows)
		can_close = can_kill = can_save = FALSE;
	else {
		can_save = TRUE;
		gebr_jc_get_jobs_state(jc, rows, &can_close, &can_kill);
	}

	job_control_disconnect_signals(jc);
	update_control_buttons(jc, can_close, can_kill, can_save);
	jc->priv->last_selection.job = NULL;

	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);

	gchar *msg;
	gint real_n = gtk_tree_model_iter_n_children(model, NULL);
	gint virt_n = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view)), NULL);

	gint control_n = 0;
	GtkTreeIter iter;
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		gboolean control;
		gtk_tree_model_get(model, &iter,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			control_n++;

		valid = gtk_tree_model_iter_next(model, &iter);
	}
	real_n -= control_n;

	if (!rows) {
		if (real_n == 0)
			msg = g_strdup(_("No jobs! Try out one of our demos (go to Help->Samples)."));
		else if (virt_n < (real_n))
			msg = g_strdup_printf(_("There are filtered jobs. %d out of %d will not appear in the list."),
					      real_n - virt_n, real_n);
		else
			msg = g_strdup(_("Select a job in the list."));
	} else
		msg = g_strdup(_("Multiple jobs selected."));

	gebr_job_control_info_set_visible(jc, FALSE, msg);
	g_free(msg);
}

static gboolean
get_flow_iter(GebrJobControl *jc,
              const gchar *flow_id,
              GtkTreeIter *iter)
{
	GtkTreeIter i;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->flow_filter);
	gchar *id;

	gboolean valid = gtk_tree_model_get_iter_first(model, &i);

	if (valid)
		valid = gtk_tree_model_iter_next(model, &i);

	while (valid) {
		gtk_tree_model_get(model, &i,
		                   1, &id,
		                   -1);

		if (g_strcmp0(id, flow_id) == 0) {
			if (iter)
				*iter = i;
			g_free(id);
			return TRUE;
		}
		g_free(id);
		valid = gtk_tree_model_iter_next(model, &i);
	}

	return FALSE;
}

static gboolean
get_server_group_iter(GebrJobControl *jc, const gchar *group, const gchar *type_str, GtkTreeIter *iter)
{
	GtkTreeIter i;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->server_filter);
	gchar *name;
	GebrMaestroServerGroupType type, type_job;
	gboolean valid = gtk_tree_model_get_iter_first(model, &i);

	type_job = gebr_maestro_server_group_str_to_enum(type_str);

	if (valid)
		valid = gtk_tree_model_iter_next(model, &i);

	while (valid) {
		gtk_tree_model_get(model, &i,
		                   1, &type,
		                   2, &name,
		                   -1);

		if (type == type_job && g_strcmp0(name, group) == 0) {
			if (iter)
				*iter = i;
			g_free(name);
			return TRUE;
		}
		g_free(name);
		valid = gtk_tree_model_iter_next(model, &i);
	}

	return FALSE;
}

static gboolean
jobs_visible_for_flow(GtkTreeModel *model,
			 GtkTreeIter *iter,
			 GebrJobControl *jc)
{
	GtkTreeIter active;
	gchar *flow_id;
	gchar *flow_name;
	gboolean visible = FALSE;

	if (!gtk_combo_box_get_active_iter (jc->priv->flow_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->flow_filter), &active,
			   0, &flow_name,
	                   1, &flow_id, -1);

	gchar *tmp = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(jc->priv->flow_filter),
							 &active);
	gint index = atoi(tmp);
	g_free(tmp);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));
	labels = labels->next;

	if (index == 0) { // Any
		jc->priv->use_filter_flow = FALSE;
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Flow:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}

	if (!jc->priv->use_filter_flow) {
		gchar *text = g_markup_printf_escaped("<span size='x-small'>Flow: %s</span>", flow_name);
		GtkWidget *label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), text);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(box->data), label, FALSE, FALSE, 0);
		jc->priv->use_filter_flow = TRUE;
		g_free(text);
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Flow:")) {
				gchar *new_text = g_markup_printf_escaped("<span size='x-small'>Flow: %s</span>", flow_name);
				gtk_label_set_markup(i->data, new_text);
				g_free(new_text);
			}
		}
	}

	g_list_free(box);
	g_list_free(labels);
	g_free(flow_name);

	GebrJob *job;
	const gchar *id;
	gboolean control;

	gtk_tree_model_get(model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (control)
		return TRUE;

	if (!job)
		return FALSE;

	if (!flow_id)
		return TRUE;

	id = gebr_job_get_flow_id(job);

	if (!g_strcmp0(flow_id, id))
		visible = TRUE;

	g_free(flow_id);

	return visible;
}

static gboolean
jobs_visible_for_servers(GtkTreeModel *model,
                         GtkTreeIter *iter,
                         GebrJobControl *jc)
{
	GtkTreeIter active;

	gchar *display;
	GebrMaestroServerGroupType type;
	gchar *name;

	if (!gtk_combo_box_get_active_iter(jc->priv->server_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->server_filter), &active,
	                   0, &display,
	                   1, &type,
			   2, &name,
			   -1);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));
	labels = labels->next;

	if (!name) {
		jc->priv->use_filter_servers = FALSE;
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Group/Node:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}


	if (!jc->priv->use_filter_servers) {
		gchar *text = g_markup_printf_escaped("<span size='x-small'>Group/Node: %s</span>", display);
		GtkWidget *label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), text);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_box_pack_start(GTK_BOX(box->data), label, FALSE, FALSE, 0);
		jc->priv->use_filter_servers = TRUE;
		g_free(text);
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Group/")) {
				gchar *new_text = g_markup_printf_escaped("<span size='x-small'>Group/Node: %s</span>", display);
				gtk_label_set_markup(i->data, new_text);
				g_free(new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);

	GebrJob *job;
	gboolean control;

	gtk_tree_model_get(model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (!job) {
		g_free(name);
		g_free(display);
		if (control)
			return TRUE;
		return FALSE;
	}
	const gchar *type_str = gebr_job_get_server_group_type(job);
	const gchar *tname = gebr_job_get_server_group(job);
	GebrMaestroServerGroupType ttype = gebr_maestro_server_group_str_to_enum(type_str);

	gboolean has_server = FALSE;

	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		gint n;
		gchar **servers = gebr_job_get_servers(job, &n);

		for (int i = 0; i < n; i++)
			if (g_strcmp0(servers[i], name) == 0)
				has_server = TRUE;

		g_strfreev(servers);
		g_free(name);
		g_free(display);
		return has_server;
	}

	has_server = (type == ttype && g_strcmp0(tname, name) == 0);

	g_free(name);
	g_free(display);
	return has_server;
}

static gboolean
jobs_visible_for_status(GtkTreeModel *model,
                        GtkTreeIter *iter,
                        GebrJobControl *jc)
{
	GtkTreeIter active;
	GebrCommJobStatus combo_status;
	gchar *combo_text;

	if (!gtk_combo_box_get_active_iter (jc->priv->status_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->status_model), &active,
	                   ST_STATUS, &combo_status,
	                   ST_TEXT, &combo_text, -1);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));
	labels = labels->next;

	if (combo_status == -1) {
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Status:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		jc->priv->use_filter_status = FALSE;
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}

	if (!jc->priv->use_filter_status) {
		gchar *text = g_markup_printf_escaped("<span size='x-small'>Status: %s</span>", combo_text);
		GtkWidget *label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), text);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_box_pack_start(GTK_BOX(box->data), label, FALSE, FALSE, 0);
		jc->priv->use_filter_status = TRUE;
		g_free(text);
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Status:")) {
				gchar *new_text = g_markup_printf_escaped("<span size='x-small'>Status: %s</span>", combo_text);
				gtk_label_set_markup(i->data, new_text);
				g_free(new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);
	g_free(combo_text);

	GebrJob *job;
	gboolean control;

	gtk_tree_model_get(model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (control)
		return TRUE;

	if (!job)
		return FALSE;

	GebrCommJobStatus status = gebr_job_get_status(job);
	gboolean visible = FALSE;

	if (status == combo_status)
		visible = TRUE;
	else if (status == JOB_STATUS_FAILED && combo_status == JOB_STATUS_CANCELED)
		visible = TRUE;

	return visible;
}

static gboolean
jobs_control_visible(GtkTreeModel *model,
                     GtkTreeIter *iter,
                     GebrJobControl *jc)
{
	TimesType control_type;

	gtk_tree_model_get(model, iter,
	                   JC_CONTROL_TYPE, &control_type,
	                   -1);

	gboolean has_job = FALSE;
	GtkTreeIter new_iter;
	gboolean valid = gtk_tree_model_get_iter_first(model, &new_iter);
	while (valid && !has_job) {
		TimesType type;
		GebrJob *job;
		gboolean is_control;

		gtk_tree_model_get(model, &new_iter,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &is_control,
		                   -1);

		if (!is_control) {
			if (!job)
				return has_job;

			gchar *rel_time = compute_relative_time(job, &type, NULL, jc);

			if (control_type == type && jobs_visible_func(model, &new_iter, jc))
				has_job = TRUE;

			g_free(rel_time);
		}
		valid = gtk_tree_model_iter_next(model, &new_iter);
	}

	return has_job;
}

static gboolean
jobs_visible_func(GtkTreeModel *model,
		  GtkTreeIter *iter,
		  GebrJobControl *jc)
{
	gboolean control;
	gboolean visible = TRUE;

	gtk_tree_model_get(model, iter,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (control)
		return jobs_control_visible(model, iter, jc);

	if (!jobs_visible_for_status(model, iter, jc))
		visible = FALSE;

	if (!jobs_visible_for_servers(model, iter, jc))
		visible = FALSE;

	if (!jobs_visible_for_flow(model, iter, jc))
		visible = FALSE;

	return visible;
}

static void
on_cb_changed(GtkComboBox *combo,
	      GebrJobControl *jc)
{
	GtkTreeModel *sort = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
	GtkTreeModel *filter = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));

	if (jc->priv->use_filter_status == FALSE &&
	    jc->priv->use_filter_flow == FALSE &&
	    jc->priv->use_filter_servers == FALSE) {
		gtk_widget_hide(jc->priv->filter_info_bar);
	} else {
		gtk_widget_show_all(jc->priv->filter_info_bar);
		if (!jc->priv->automatic_filter && !jc->priv->restore_mode) {
			update_user_defined_filters(jc);
		}
	}

	on_select_non_single_job(jc, NULL);
	update_tree_view(jc);
}

static void
fill_issues_properties(GebrJobControl *jc,
                       const gchar *issues,
                       GebrJob *job)
{
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_label"));
	GtkImage *image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_image"));

	gtk_widget_show_all(GTK_WIDGET(info));
	GtkButton *show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	gtk_widget_hide(GTK_WIDGET(show_issues));

	gtk_info_bar_set_message_type(info, GTK_MESSAGE_INFO);
	gtk_label_set_markup(label, issues);
	gtk_label_set_line_wrap(label, TRUE);
	gtk_label_set_line_wrap_mode(label, PANGO_WRAP_WORD);
	gtk_image_set_from_stock(image, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
}

static void
on_dismiss_clicked(GtkButton *dismiss, GebrJobControl *jc)
{
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	gtk_widget_hide(GTK_WIDGET(info));

	GtkButton *show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	gtk_widget_show(GTK_WIDGET(show_issues));
}

static void
on_show_issues_clicked(GtkButton *show_issues, GebrJobControl *jc)
{
	GebrJob *job = gebr_job_control_get_selected_job(jc);
	if (!job)
		return;
	fill_issues_properties(jc, gebr_job_get_issues(job), job);
}

static void
on_job_wait_button(GtkButton *button,
                   GebrJobControl *jc)
{
	GebrJob *job, *parent;

	job = gebr_job_control_get_selected_job(jc);
	if (!job)
		return;
	parent = gebr_job_control_find(jc, gebr_job_get_queue(job));
	gebr_job_control_select_job(jc, parent);
}

static void
on_job_output(GebrJob *job,
	      gint frac,
	      const gchar *output,
	      GebrJobControl *jc)
{
	GtkTextIter end;

	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &end);
	gtk_text_buffer_insert(jc->priv->text_buffer, &end, output, strlen(output));
	if (gebr.config.job_log_auto_scroll)
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(jc->priv->text_view), jc->priv->text_mark, 0, FALSE, 0, 0);
}

static void
on_job_status(GebrJob *job,
	      GebrCommJobStatus old_status,
	      GebrCommJobStatus new_status,
	      const gchar *parameter,
	      GebrJobControl *jc)
{
	gebr_jc_update_status_and_time(jc, job, new_status);

	if (old_status == JOB_STATUS_QUEUED && new_status == JOB_STATUS_RUNNING)
		job_control_fill_servers_info(jc);

	GtkTreeIter *iter = gebr_job_get_iter(job);
	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(jc->priv->store), iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(jc->priv->store), path, iter);
}

static void
on_job_issued(GebrJob *job,
	      const gchar *issues,
	      GebrJobControl *jc)
{
	fill_issues_properties(jc, issues, job);
}

static void
on_job_cmd_line(GebrJob *job,
                gint frac,
                const gchar *cmd,
                GebrJobControl *jc)
{
	gebr_job_control_include_cmd_line(jc, job);
}

static void
gebr_job_control_info_set_visible(GebrJobControl *jc,
				  gboolean visible,
				  const gchar *txt)
{
	GtkWidget *job_info = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "job_info_widget"));
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	GtkLabel *empty_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "empty_job_selection_label"));

	if (!visible)
		gtk_widget_hide(GTK_WIDGET(info));
	gtk_widget_set_visible(job_info, visible);
	gtk_widget_set_visible(GTK_WIDGET(empty_label), !visible);

	if (!visible)
		gtk_label_set_text(empty_label, txt);
}

static gboolean
on_pie_tooltip(GebrGuiPie *pie,
               gint x, gint y,
               gboolean keyboard,
               GtkTooltip *tooltip,
               GebrJobControl *jc)
{
	gint i = gebr_gui_pie_get_hovered(pie);

	if (i == -1)
		return FALSE;

	const gchar *server;

	GebrJob *job = gebr_job_control_get_selected_job(jc);
	if (!job)
		return FALSE;

	if (!g_strcmp0(jc->priv->servers_info.servers[i], "127.0.0.1"))
		server = gebr_job_get_maestro_address(job);
	else
		server = jc->priv->servers_info.servers[i];
	gfloat value = round((jc->priv->servers_info.percentages[i])*1000)/10;
	gchar *t = g_strdup_printf(_("%s\n%.1lf%% of total"), server, value );
	gtk_tooltip_set_text(tooltip, t);
	g_free(t);

	return TRUE;
}

static gboolean
paint_square(GtkWidget *widget, GdkEventExpose *event, Color *color)
{
	cairo_t *ctx = gdk_cairo_create(GDK_DRAWABLE(widget->window));
	cairo_set_source_rgb(ctx, color->r, color->g, color->b);
	cairo_rectangle(ctx, 0, 0, widget->allocation.width, widget->allocation.height);
	cairo_fill(ctx);
	cairo_destroy(ctx);

	return TRUE;
}

static void
job_control_fill_servers_info(GebrJobControl *jc)
{
	GebrJob *job = gebr_job_control_get_selected_job(jc);
	if (!job)
		return;

	GString *resources = g_string_new(NULL);
	GString *bold_resources = g_string_new("");
	GtkLabel *res_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "resources_text"));
	GtkLabel *bold_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "label6"));
	GtkLabel *header_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "header_label"));
	GtkLabel *snapshot_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "snapshot_label"));
	GtkImage *snapshot_image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "snapshot_image"));
	const gchar *nprocs;
	const gchar *niceness;
	gint n_servers, i;
	gchar **servers;
	gint total_procs;

	if (!job)
		return;

	const gchar *snapshot_title = gebr_job_get_snapshot_title(job);

	if (snapshot_title && *snapshot_title) {
		gchar *snapshot_markup = g_strdup_printf(_("<span size=\"x-large\">%s</span>"),
							 snapshot_title);
		gtk_label_set_markup(snapshot_label, snapshot_markup);
		gtk_label_set_ellipsize(snapshot_label, PANGO_ELLIPSIZE_END);
		g_free(snapshot_markup);
		gtk_widget_hide(GTK_WIDGET(header_label));
		gtk_widget_show(GTK_WIDGET(snapshot_label));
		gtk_widget_show(GTK_WIDGET(snapshot_image));
	} else {
		gtk_widget_show(GTK_WIDGET(header_label));
		gtk_widget_hide(GTK_WIDGET(snapshot_label));
		gtk_widget_hide(GTK_WIDGET(snapshot_image));
	}
	servers = gebr_job_get_servers(job, &n_servers);
	total_procs = gebr_job_get_total_procs(job);
	gebr_job_get_resources(job, &nprocs, &niceness);

	const gchar *maddr = gebr_job_get_maestro_address(job);
	if ((!nprocs || !niceness) || (!*nprocs || !*niceness)) {
		g_string_printf(bold_resources, _("This flow is queued and \nhas not been executed."));
	} else {
		const gchar *type_str = gebr_job_get_server_group_type(job);
		const gchar *groups = gebr_job_get_server_group(job);
		GebrMaestroServerGroupType type = gebr_maestro_server_group_str_to_enum(type_str);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);
		g_string_printf(bold_resources, _("<b>Process distribution among working nodes</b>"));
		gchar *markup;

		if (type == MAESTRO_SERVER_TYPE_GROUP)
			if (g_strcmp0(groups, "") == 0)
				groups = g_strdup_printf(_("%s"), gebr_maestro_server_get_display_address(maestro));

		markup = g_markup_printf_escaped(_("Job submitted by <b>%s</b> to Maestro <b>%s</b>.\n"
						   "Executed on total <b>%d</b> processes on %s <b>%s</b>,\n"
						   "%sdistributed on <b>%d</b> node(s).\n"),
						  gebr_job_get_hostname(job), gebr_job_get_maestro_address(job),
						  total_procs, type == MAESTRO_SERVER_TYPE_DAEMON? _("node") : _("group"), groups,
						  g_strcmp0(niceness, "0")? _("using the nodes idle time, ") : "",
						  n_servers);
		g_string_append(resources, markup);

		const gchar *mpi_owner_tst = gebr_job_get_mpi_owner(job);
		const gchar *mpi_flavor = gebr_job_get_mpi_flavor(job);
		if (mpi_owner_tst && *mpi_owner_tst) {
			gchar *mpi_message = g_markup_printf_escaped(_("This MPI job (<b>%s</b>) was dispatched by <b>%s</b>.\n"), mpi_flavor, mpi_owner_tst);
			g_string_append(resources, mpi_message);
			g_free(mpi_message);
		}

		g_free(markup);
	}
	gtk_label_set_markup (res_label, resources->str);
	gtk_label_set_markup (bold_label, bold_resources->str);
	g_string_free(bold_resources, TRUE);
	g_string_free(resources, TRUE);

	guint *values = g_new(guint, n_servers);
	GtkWidget *piechart = gebr_gui_pie_new(values, 0);

	jc->priv->servers_info.servers = servers;

	GtkBox *pie_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "pie_box"));
	GtkBox *servers_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "servers_box"));

	gtk_container_foreach(GTK_CONTAINER(servers_box), (GtkCallback)gtk_widget_destroy, NULL);
	gtk_container_foreach(GTK_CONTAINER(pie_box), (GtkCallback)gtk_widget_destroy, NULL);

	if (!servers)
		return;

	if (jc->priv->servers_info.percentages)
		g_free(jc->priv->servers_info.percentages);
	jc->priv->servers_info.percentages = g_new(gdouble, n_servers);

	g_signal_connect(piechart, "query-tooltip", G_CALLBACK(on_pie_tooltip), jc);

	gtk_widget_set_has_tooltip(piechart, TRUE);
	gtk_widget_set_size_request(piechart, 150, 150);

	gtk_box_pack_start(pie_box, piechart, TRUE, FALSE, 0);
	gtk_widget_show_all(piechart);

	gint len;
	gdouble *percs = gebr_job_get_percentages(job, &len);
	jc->priv->servers_info.percentages = percs;
	double acc = 0;
	for (i = 0; i < len; i++)
		acc += percs[i];

	for (i = 0; i < len; i++) {
		values[i] = (guint)((percs[i]/acc)*1000);
		jc->priv->servers_info.percentages[i] /= acc;
	}

	gebr_gui_pie_set_data(GEBR_GUI_PIE(piechart), values, n_servers);
	g_free(values);

	for (i = 0; servers[i]; i++) {
		const gchar *server;
		if (!g_strcmp0(servers[i], "127.0.0.1"))
			 server = maddr; 
		else
			server = servers[i];

		Color *color = g_new(Color, 1);
		GtkWidget *hbox = gtk_hbox_new(FALSE, 5);
		GtkWidget *label = gtk_label_new(server);
		GtkWidget *square = gtk_drawing_area_new();

		gtk_widget_set_size_request(square, 15, 10);
		gebr_gui_pie_get_color(GEBR_GUI_PIE(piechart), i, &color->r, &color->g, &color->b);
		g_signal_connect(square, "expose-event", G_CALLBACK(paint_square), color);
		g_object_weak_ref(G_OBJECT(label), (GWeakNotify)g_free, color);

		gtk_box_pack_start(GTK_BOX(hbox), square, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

		gtk_box_pack_start(servers_box, hbox, TRUE, FALSE, 0);
		gtk_widget_show_all(hbox);
	}
}

static void
job_control_disconnect_signals(GebrJobControl *jc)
{
	if (jc->priv->last_selection.job) {
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_output);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_status);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_issued);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_cmd_line);
	}
}

static void
job_control_on_cursor_changed(GtkTreeSelection *selection,
			      GebrJobControl *jc)
{
	GtkTreeModel *model;
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows) {
		on_select_non_single_job(jc, rows);
		return;
	}

	GebrJob *job = NULL;
	gboolean control = FALSE;

	GtkTreeIter iter;
	if (g_list_length(rows) == 1) {
		if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)rows->data))
			gtk_tree_model_get(model, &iter,
			                   JC_STRUCT, &job,
			                   JC_IS_CONTROL, &control,
			                   -1);
		else
			g_warn_if_reached();
	}

	gboolean has_job = (job != NULL);

	if (!has_job) {
		on_select_non_single_job(jc, rows);
		return;
	}

	gebr_job_control_info_set_visible(jc, TRUE, NULL);
	GebrJob *old_job = jc->priv->last_selection.job;

	if (has_job) {
		job_control_disconnect_signals(jc);

		jc->priv->last_selection.job = job;
		jc->priv->last_selection.sig_output =
				g_signal_connect(job, "output", G_CALLBACK(on_job_output), jc);
		jc->priv->last_selection.sig_status =
				g_signal_connect(job, "status-change", G_CALLBACK(on_job_status), jc);
		jc->priv->last_selection.sig_issued =
				g_signal_connect(job, "issued", G_CALLBACK(on_job_issued), jc);
		jc->priv->last_selection.sig_cmd_line =
				g_signal_connect(job, "cmd-line-received", G_CALLBACK(on_job_cmd_line), jc);

		gebr_job_control_load_details(jc, job);
	}

	if (job == old_job)
		return;

	GtkButton *button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));

	gtk_widget_hide(GTK_WIDGET(info));

	if (has_job && gebr_job_has_issues(job)) {
		GtkImage *image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "show_issues_img"));
		gtk_image_set_from_stock(image, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DND);
		gtk_widget_show(GTK_WIDGET(button));
	} else
		gtk_widget_hide(GTK_WIDGET(button));


	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);
}

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

static const gchar *
job_control_get_icon_for_job(GebrJob *job)
{
	const gchar *stock_id = NULL;

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
	default:
		g_warn_if_reached();
		break;
	}
	return stock_id;
}

static void
icon_column_data_func(GtkTreeViewColumn *tree_column,
		      GtkCellRenderer *cell,
		      GtkTreeModel *tree_model,
		      GtkTreeIter *iter,
		      gpointer data)
{
	gboolean control;
	GebrJob *job;
	const gchar *stock_id;

	gtk_tree_model_get(tree_model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if(!control) {
		stock_id = job_control_get_icon_for_job(job);
		g_object_set(cell, "sensitive", TRUE, "visible", TRUE, NULL);
	} else {
		stock_id = NULL;
		g_object_set(cell, "sensitive", FALSE, "visible", FALSE, NULL);
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
	gboolean control;
	GebrJob *job;
	gchar *title;

	gtk_tree_model_get(tree_model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   JC_TEXT_CONTROL, &title,
	                   -1);

	if (!control) {
		title = g_strdup_printf("%s <span>#%s</span>",
		                        gebr_job_get_title(job),
		                        gebr_job_get_job_counter(job));

		g_object_set(cell, "sensitive", TRUE, NULL);
	}
	else {
		g_object_set(cell, "sensitive", FALSE, NULL);
	}

	g_object_set(cell, "markup", title, NULL);
	g_free(title);
}

static void
snap_icon_column_data_func(GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell,
                           GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           gpointer data)
{
	GebrJob *job;
	gboolean control;

	gtk_tree_model_get(tree_model, iter,
	                   JC_STRUCT, &job,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (control) {
		g_object_set(cell, "stock-id", NULL, NULL);
		g_object_set(cell, "sensitive", FALSE, NULL);
		return;
	}

	const gchar *snapshot_title = gebr_job_get_snapshot_title(job);
	if (snapshot_title && *snapshot_title)
		g_object_set(cell, "stock-id", "photos", NULL);
	else
		g_object_set(cell, "stock-id", NULL, NULL);
	g_object_set(cell, "sensitive", TRUE, NULL);
}

static gchar *
compute_relative_time(GebrJob *job,
                      TimesType *type,
                      gdouble *delta,
                      GebrJobControl *jc)
{
	const gchar *start_time_str;
	GTimeVal start_time, curr_time;

	start_time_str = gebr_job_get_last_run_date(job);
	if (!start_time_str)
		return NULL;

	g_time_val_from_iso8601(start_time_str, &start_time);

	const gchar *maddr = gebr_job_get_maestro_address(job);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);

	g_get_current_time(&curr_time);
	gint clocks_diff = gebr_maestro_server_get_clocks_diff(maestro);
	curr_time.tv_sec -= clocks_diff;

	return gebr_calculate_relative_time(&start_time, &curr_time, type, delta);
}

static void
compute_subheader_label(GebrJob *job,
			GebrCommJobStatus status,
			gchar **subheader,
			gchar **start_detail)
{
	const gchar *start_date = gebr_job_get_start_date(job);
	if (!start_date || !*start_date)
		start_date = gebr_job_get_last_run_date(job);
	const gchar *finish_date = gebr_job_get_finish_date(job);
	GString *start = g_string_new(NULL);
	GString *finish = g_string_new(NULL);

	/* start date (may have failed, never started) */
	if (start_date && strlen(start_date))
		g_string_append_printf(start, _("Started on %s"),
				       gebr_localized_date(start_date));

	/* finish date */
	if (finish_date && strlen(finish_date)) {
		const gchar *tmp = gebr_localized_date(finish_date);
		if (status == JOB_STATUS_FINISHED)
			g_string_append_printf(finish, _("Finished on %s"), tmp);
		else if (status == JOB_STATUS_CANCELED)
			g_string_append_printf(finish, _("Canceled on %s"), tmp);
		else if (status == JOB_STATUS_FAILED)
			g_string_append_printf(finish, _("Failed on %s"), tmp);
	}

	if (status == JOB_STATUS_FINISHED) {
		g_string_append_c(finish, '\n');
		g_string_append_printf(finish, _("Elapsed time: %s"),
				       gebr_job_get_elapsed_time(job));
		*subheader = g_string_free(finish, FALSE);
		*start_detail = g_string_free(start, FALSE);
		return;
	}

	if (status == JOB_STATUS_RUNNING) {
		g_string_append_c(start, '\n');
		g_string_append_printf(start, _("Elapsed time: %s"),
				       gebr_job_get_running_time(job, start_date));
		*subheader = g_string_free(start, FALSE);
		*start_detail = NULL;
		g_string_free(finish, TRUE);
		return;
	}

	if (status == JOB_STATUS_CANCELED) {
		g_string_append_c(finish, '\n');
		g_string_append_printf(finish, _("Elapsed time: %s"),
		                       gebr_job_get_elapsed_time(job));
		*subheader = g_string_free(finish, FALSE);
		*start_detail = g_string_free(start, FALSE);
		return;
	}

	if (status == JOB_STATUS_FAILED) {
		g_string_append_c(finish, '\n');
		g_string_append_printf(finish, _("Elapsed time: %s"),
		                       gebr_job_get_elapsed_time(job));
		*subheader = g_string_free(finish, FALSE);
		*start_detail = g_string_free(start, FALSE);
		return;
	}

	if (status == JOB_STATUS_QUEUED) {
		*subheader = _("Waiting for job");
		*start_detail = NULL;
		g_string_free(start, TRUE);
		g_string_free(finish, TRUE);
		return;
	}

	if (status == JOB_STATUS_INITIAL) {
		*subheader = _("Waiting for nodes");
		*start_detail = NULL;
		g_string_free(start, TRUE);
		g_string_free(finish, TRUE);
		return;
	}

	*subheader = NULL;
	*start_detail = NULL;
	g_string_free(start, TRUE);
	g_string_free(finish, TRUE);
	return;
}

static void
gebr_jc_update_status_and_time(GebrJobControl *jc,
                               GebrJob 	      *job,
                               GebrCommJobStatus status)
{
	gchar *subheader_str = NULL, *start_detail_str = NULL;
	GtkImage *img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "status_image"));
	GtkLabel *subheader = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_label"));
	GtkButton *queued_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	GtkLabel *details_start_date = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "detail_start_date"));
	gtk_widget_hide(GTK_WIDGET(queued_button));

	compute_subheader_label(job, status, &subheader_str, &start_detail_str);

	if (subheader_str) {
		gtk_label_set_markup(subheader, subheader_str);
		gtk_widget_show(GTK_WIDGET(subheader));
	} else
		gtk_widget_hide(GTK_WIDGET(subheader));

	if (start_detail_str)
		gtk_label_set_text(details_start_date, start_detail_str);
	else
		gtk_label_set_text(details_start_date, "");

	gtk_widget_show(GTK_WIDGET(details_start_date));

	if (status == JOB_STATUS_FINISHED)
		gtk_image_set_from_stock(img, GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
	else if (status == JOB_STATUS_INITIAL)
		gtk_image_set_from_stock(img, GTK_STOCK_NETWORK, GTK_ICON_SIZE_DIALOG);
	else if (status == JOB_STATUS_RUNNING)
		gtk_image_set_from_stock(img, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
	else if (status == JOB_STATUS_CANCELED)
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
	else if (status == JOB_STATUS_FAILED)
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
	else if (status == JOB_STATUS_QUEUED) {
		GebrJob *parent;
		GtkImage *wait_img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "subheader_button_img"));
		GtkLabel *wait_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_button_lbl"));

		parent = gebr_job_control_find(jc, gebr_job_get_queue(job));
		if (parent) {
			gtk_image_set_from_stock(img, "chronometer", GTK_ICON_SIZE_DIALOG);
			gtk_image_set_from_stock(wait_img, job_control_get_icon_for_job(parent), GTK_ICON_SIZE_BUTTON);
			gtk_label_set_text(wait_label, gebr_job_get_title(parent));
			gtk_widget_show(GTK_WIDGET(queued_button));
		}
	}
}

static void
gebr_job_control_include_cmd_line(GebrJobControl *jc,
                                  GebrJob *job)
{
	GtkTextBuffer *buffer;
	GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_builder_get_object(jc->priv->builder, "cmd_line_window"));
	PangoFontDescription *font;
	font = pango_font_description_new();
	pango_font_description_set_family(font, "monospace");
	GtkWrapMode wrapmode = gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE;

	GtkWidget *child = gtk_bin_get_child(GTK_BIN(scroll));
	if (child)
		gtk_widget_destroy(child);

	g_list_free(jc->priv->cmd_views);
	jc->priv->cmd_views = NULL;

	gint total;
	const gchar *run_type = gebr_job_get_run_type(job);
	GebrJobTask *job_tasks = gebr_job_get_tasks(job, &total);
	if (!job_tasks)
		return;

	if (total > 1 && g_strcmp0(run_type, "mpi") != 0) {
		GtkWidget *vbox = gtk_vbox_new(FALSE, 8);

		for (int i = 0; i < total; i++) {
			GebrJobTask *task = job_tasks + i;

			if (!task->cmd_line)
				continue;

			gchar *title = g_strdup_printf(_("Command line for task %d of %d (node: %s)"),
						       task->frac, total, task->server);
			GtkWidget *expander = gtk_expander_new(title);
			gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
			g_free(title);

			GtkWidget *view = gtk_text_view_new();
			gtk_widget_modify_font(view, font);
			g_object_set(view, "wrap-mode", wrapmode, NULL);
			g_object_set(view, "editable", FALSE, NULL);
			jc->priv->cmd_views = g_list_prepend(jc->priv->cmd_views, view);
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
			gtk_text_buffer_set_text(buffer, task->cmd_line, -1);

			g_signal_connect(view, "populate-popup",
					 G_CALLBACK(on_text_view_populate_popup), jc);

			gtk_container_add(GTK_CONTAINER(expander), view);
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
		}

		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox);
		gtk_widget_show_all(vbox);
	} else if (job_tasks->cmd_line){
		GtkWidget *view = gtk_text_view_new();
		gtk_widget_modify_font(view, font);
		g_object_set(view, "wrap-mode", wrapmode, NULL);
		g_object_set(view, "editable", FALSE, NULL);
		jc->priv->cmd_views = g_list_prepend(jc->priv->cmd_views, view);
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
		gtk_text_buffer_set_text(buffer, job_tasks->cmd_line, -1);

		g_signal_connect(view, "populate-popup",
				 G_CALLBACK(on_text_view_populate_popup), jc);

		gtk_container_add(GTK_CONTAINER(scroll), view);
		gtk_widget_show_all(view);
	}

	pango_font_description_free(font);
}

static void
gebr_job_control_load_details(GebrJobControl *jc,
			      GebrJob *job)
{
	g_return_if_fail(job != NULL);

	GString *info = g_string_new("");
	GtkTextIter end_iter;
	GtkImage *info_button_image;
	GtkLabel *input_file, *output_file, *log_file, *job_group;
	gchar *input_file_str, *output_file_str, *log_file_str;
	GebrCommJobStatus status = gebr_job_get_status(job);

	input_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "input_label"));
	output_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "output_label"));
	log_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "error_label"));
	job_group = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "job_group"));
	info_button_image = GTK_IMAGE(gtk_bin_get_child(GTK_BIN(jc->priv->info_button)));

	gebr_job_get_io(job, &input_file_str, &output_file_str, &log_file_str);

	gchar *markup;
	gchar *result;

	markup = g_markup_printf_escaped (_("<b>Input file</b>: %s"), input_file_str ? input_file_str : _("None"));
	gtk_label_set_markup(input_file, markup);
	g_free(markup);

	if (input_file_str && gebr_validator_evaluate(gebr.validator, input_file_str,
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, NULL)) {
		gtk_widget_set_tooltip_text(GTK_WIDGET(input_file), result);
		g_free(result);
	}

	markup = g_markup_printf_escaped (_("<b>Output file</b>: %s"), output_file_str ? output_file_str : _("None"));
	gtk_label_set_markup (output_file, markup);
	g_free(markup);

	if (output_file_str && gebr_validator_evaluate(gebr.validator, output_file_str,
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, NULL)) {
		gtk_widget_set_tooltip_text(GTK_WIDGET(output_file), result);
		g_free(result);
	}

	markup = g_markup_printf_escaped (_("<b>Log File</b>: %s"), log_file_str ? log_file_str : _("None"));
	gtk_label_set_markup (log_file, markup);
	g_free(markup);

	if (log_file_str && gebr_validator_evaluate(gebr.validator, log_file_str,
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, NULL)) {
		gtk_widget_set_tooltip_text(GTK_WIDGET(log_file), result);
		g_free(result);
	}

	gchar *msg = g_strdup(gebr_job_get_server_group(job));
	GString *msg_final = g_string_new("");

	const gchar *maddr = gebr_job_get_maestro_address(job);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);

	if (!g_strcmp0(msg, ""))
		msg = g_strdup_printf("%s", gebr_maestro_server_get_display_address(maestro));
	if (!g_strcmp0(msg, "127.0.0.1"))
		msg = g_strdup(maddr); 

	g_string_append(msg_final, msg);
	if(g_utf8_strlen(msg, 16) > 15) {
		g_string_erase(msg_final, 13, -1);
		g_string_append(msg_final,"...");
	}
	gtk_label_set_markup(job_group, g_string_free(msg_final,FALSE));
	gtk_widget_set_tooltip_text(GTK_WIDGET(job_group), msg);

	gdouble speed = gebr_job_get_exec_speed(job);
	const gchar *icon = gebr_interface_get_speed_icon(gebr_ui_flow_execution_calculate_slider_from_speed(speed));
	if (icon)
		gtk_image_set_from_stock(info_button_image, icon, GTK_ICON_SIZE_LARGE_TOOLBAR);

	job_control_fill_servers_info(jc);
	gebr_jc_update_status_and_time(jc, job, status);
	update_control_buttons(jc, gebr_job_can_close(job), gebr_job_can_kill(job), TRUE);

	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "header_label"));
	const gchar *description = gebr_job_get_description(job);
	if (description && *description){
		markup = g_strdup_printf ("<span size=\"x-large\">%s</span>", description);
		gtk_widget_set_sensitive(GTK_WIDGET(label), TRUE);
	}
	else{
		markup = g_strdup_printf ("<span size=\"x-large\">No description available</span>");
		gtk_widget_set_sensitive(GTK_WIDGET(label), FALSE);
	}
	gtk_label_set_markup (label, markup);
	gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
	g_free (markup);

	gtk_text_buffer_set_text(jc->priv->text_buffer, "", 0);

	gebr_job_control_include_cmd_line(jc, job);

	/* output */
	g_string_append(info, gebr_job_get_output(job));

	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->priv->text_buffer, &end_iter, info->str, info->len);
	if (gebr.config.job_log_auto_scroll)
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(jc->priv->text_view), &end_iter,
					     0, FALSE, 0, 0);

	g_string_free(info, TRUE);
}

static gboolean
update_tree_view(gpointer data)
{
	GtkTreeIter iter;
	GebrJobControl *jc = data;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	GebrJob *job = gebr_job_control_get_selected_job(jc);
	if (job && gebr_job_get_status(job) == JOB_STATUS_RUNNING)
		gebr_jc_update_status_and_time(jc, job, JOB_STATUS_RUNNING);

	return TRUE;
}

static gint
tree_sort_func(GtkTreeModel *model,
	       GtkTreeIter *a,
	       GtkTreeIter *b,
	       gpointer user_data)
{
	GebrJobControl *jc = user_data;

	GebrJob *ja, *jb;
	gboolean ca, cb;
	TimesType tca, tcb;

	gtk_tree_model_get(model, a,
	                   JC_STRUCT, &ja,
	                   JC_IS_CONTROL, &ca,
	                   JC_CONTROL_TYPE, &tca,
	                   -1);
	gtk_tree_model_get(model, b,
	                   JC_STRUCT, &jb,
	                   JC_IS_CONTROL, &cb,
	                   JC_CONTROL_TYPE, &tcb,
	                   -1);

	if ((!ja && !ca) && (!jb && !cb))
		return 0;

	if ((!ja && !ca))
		return 1;

	if ((!jb && !cb))
		return -1;

	gdouble da, db;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	gint clocks_diff = gebr_maestro_server_get_clocks_diff(maestro);

	if (ca)
		da = (gebr_get_lower_bound_for_type(tca) - clocks_diff);
	else
		g_free(compute_relative_time(ja, NULL, &da, jc));

	if (cb)
		db = (gebr_get_lower_bound_for_type(tcb) - clocks_diff);
	else
		g_free(compute_relative_time(jb, NULL, &db, jc));

	if (db - da < 0)
		return -1;

	if (db - da > 0)
		return 1;

	return 0;
}

static void
gebr_jc_populate_status_cb(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkListStore *store = jc->priv->status_model;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(jc->priv->status_model), &iter);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_TEXT, _("Any"),
	                   ST_STATUS, -1,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_APPLY,
	                   ST_TEXT, _("Finished"),
	                   ST_STATUS, JOB_STATUS_FINISHED,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_CANCEL,
	                   ST_TEXT, _("Canceled/Failed"),
	                   ST_STATUS, JOB_STATUS_CANCELED,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_EXECUTE,
	                   ST_TEXT, _("Running"),
	                   ST_STATUS, JOB_STATUS_RUNNING,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, "chronometer",
	                   ST_TEXT, _("Queued"),
	                   ST_STATUS, JOB_STATUS_QUEUED,
	                   -1);
}

static void
on_job_remove(GebrJob *job, GebrJobControl *jc)
{
	BLOCK_SELECTION_CHANGED_SIGNAL(jc);
	gebr_job_control_remove(jc, job);
	UNBLOCK_SELECTION_CHANGED_SIGNAL(jc);
	job_control_on_cursor_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), jc);
}

static void
on_job_disconnected(GebrJob *job, GebrJobControl *jc)
{
	BLOCK_SELECTION_CHANGED_SIGNAL(jc);
	gebr_job_control_remove(jc, job);
	UNBLOCK_SELECTION_CHANGED_SIGNAL(jc);
}

static void
wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GebrJobControl * jc)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	GtkWrapMode mode = gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE;

	g_object_set(G_OBJECT(jc->priv->text_view), "wrap-mode", mode, NULL);

	for (GList *i = jc->priv->cmd_views; i; i = i->next)
		g_object_set(i->data, "wrap-mode", mode, NULL);
}

static void
autoscroll_toggled(GtkCheckMenuItem * check_menu_item)
{
	gebr.config.job_log_auto_scroll = gtk_check_menu_item_get_active(check_menu_item);
}

static void
on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu, GebrJobControl *jc)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(wordwrap_toggled), jc);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);

	menu_item = gtk_check_menu_item_new_with_label(_("Auto-scroll"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(autoscroll_toggled), NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_auto_scroll);
}

static GtkMenu *
job_control_popup_menu(GtkWidget * widget, GebrJobControl *jc)
{
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	menu = gtk_menu_new();

	if (rows) {

		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop")));

		gtk_widget_show_all(menu);

		g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(rows);

		return GTK_MENU(menu);
	}
	return NULL;
}

static void
on_filter_show_unselect_jobs(GtkToggleButton *button,
			     GebrJobControl *jc)
{
	if (gtk_toggle_button_get_active(button))
		gebr_job_control_select_job(jc, NULL);
}

static void
on_reset_filter(GtkButton  *button,
                gpointer    user_data)
{
	GebrJobControl *jc = user_data;
	gebr_job_control_reset_filters(jc);
	update_user_defined_filters(jc);
	update_tree_view(jc);
}

static void
on_job_define(GebrMaestroController *mc,
	      GebrMaestroServer *maestro,
	      GebrJob *job,
	      GebrJobControl *jc)
{
	gebr_job_control_add(jc, job);
	on_maestro_server_filter_changed(jc->priv->server_combo, jc);
	on_maestro_flow_filter_changed(jc->priv->flow_combo, jc);
}


static gint
servers_filter_sort_func(GtkTreeModel *model,
                         GtkTreeIter *a,
                         GtkTreeIter *b,
                         gpointer user_data)
{
	gchar *g1, *g2;
	GebrMaestroServerGroupType t1, t2;

	gtk_tree_model_get(model, a,
	                   1, &t1,
	                   2, &g1,
	                   -1);

	gtk_tree_model_get(model, b,
	                   1, &t2,
	                   2, &g2,
	                   -1);

	if (!g1) {
		g_free(g2);
		return -1;
	} else if (!g2) {
		g_free(g1);
		return 1;
	}

	if (t1 == MAESTRO_SERVER_TYPE_GROUP) {
		if (t1 == t2) {
			if (g_strcmp0(g1, "") == 0) {
				g_free(g1);
				g_free(g2);
				return -1;
			} else if (g_strcmp0(g2, "") == 0) {
				g_free(g1);
				g_free(g2);
				return 1;
			}
			g_free(g1);
			g_free(g2);
			return g_strcmp0(g1, g2);
		}
		g_free(g1);
		g_free(g2);
		return -1;
	}
	else if (t2 == MAESTRO_SERVER_TYPE_GROUP) {
		g_free(g1);
		g_free(g2);
		return 1;
	} else {
		gint ret = g_strcmp0(g1, g2);
		g_free(g1);
		g_free(g2);
		return ret;
	}
}

static void
on_maestro_flow_filter_changed(GtkComboBox *combo,
                               GebrJobControl *jc)
{
	g_signal_handlers_block_by_func(combo, on_cb_changed, jc);
	GtkTreeIter iter, it, active;

	gchar *prev_selected = NULL;
	if (gtk_combo_box_get_active_iter(combo, &active))
		gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->flow_filter), &active,
		                   1, &prev_selected, -1);

	gtk_list_store_clear(jc->priv->flow_filter);

	gtk_list_store_append(jc->priv->flow_filter, &iter);
	gtk_list_store_set(jc->priv->flow_filter, &iter,
			   0, _("Any"),// Display name
			   1, NULL,    // ID
			   -1);

	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);
	gebr_gui_gtk_tree_model_foreach(it, model) {
		gboolean control;
		GebrJob *job;
		const gchar *flow_id, *flow_name;

		gtk_tree_model_get(model, &it,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			continue;

		flow_id = gebr_job_get_flow_id(job);
		flow_name = gebr_job_get_title(job);

		if (get_flow_iter(jc, flow_id, &iter))
			continue;

		gtk_list_store_append(jc->priv->flow_filter, &iter);
		gtk_list_store_set(jc->priv->flow_filter, &iter,
		                   0, flow_name,
		                   1, flow_id,
		                   -1);
	}

	GtkTreeIter new_it;
	gint index = 0;
	gint select_index = 0;
	gboolean find_index = FALSE;

	if (prev_selected) {
		GtkTreeModel *flows_model = GTK_TREE_MODEL(jc->priv->flow_filter);
		gebr_gui_gtk_tree_model_foreach_hyg(new_it, flows_model, combo) {
			gchar *id;

			gtk_tree_model_get(flows_model, &new_it,
			                   1, &id, -1);

			if (g_strcmp0(id, prev_selected) == 0) {
				find_index = TRUE;
				select_index = index;
				break;
			}
			index++;
			g_free(id);
		}
	}
	gtk_combo_box_set_active(jc->priv->flow_combo, select_index);

	if (!find_index)
		on_reset_filter(NULL, jc);

	g_signal_handlers_unblock_by_func(combo, on_cb_changed, jc);
}

static void
on_maestro_server_filter_changed(GtkComboBox *combo,
                                 GebrJobControl *jc)
{
	g_signal_handlers_block_by_func(jc->priv->server_combo, on_cb_changed, jc);
	GtkTreeIter iter, it;

	gchar *prev_selected = gtk_combo_box_get_active_text(jc->priv->server_combo);

	gtk_list_store_clear(jc->priv->server_filter);

	gtk_list_store_append(jc->priv->server_filter, &iter);
	gtk_list_store_set(jc->priv->server_filter, &iter,
			   0, _("Any"),// Display name
			   1, -1,      // Type
			   2, NULL,    // Name
			   -1);

	GebrMaestroServer *maestro;
	maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (!maestro)
		return;

	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);
	gebr_gui_gtk_tree_model_foreach(it, model) {
		gboolean control;
		GebrJob *job;
		const gchar *group, *type_str, *display;
		GebrMaestroServerGroupType type;

		gtk_tree_model_get(model, &it,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			continue;

		group = gebr_job_get_server_group(job);
		type_str = gebr_job_get_server_group_type(job);
		type = gebr_maestro_server_group_str_to_enum(type_str);

		if (get_server_group_iter(jc, group, type_str, &iter))
			continue;

		if (!*group && type == MAESTRO_SERVER_TYPE_GROUP)
			display = gebr_maestro_server_get_display_address(maestro);
		else
			display = group;

		gtk_list_store_append(jc->priv->server_filter, &iter);
		gtk_list_store_set(jc->priv->server_filter, &iter,
		                   0, display,
		                   1, type,
		                   2, group);

		if (type == MAESTRO_SERVER_TYPE_GROUP) {
			gint n;
			gchar **servers = gebr_job_get_servers(job, &n);
			const gchar *tstr = gebr_maestro_server_group_enum_to_str(MAESTRO_SERVER_TYPE_DAEMON);

			for (gint i = 0; i < n; i++) {
				if (get_server_group_iter(jc, servers[i], tstr, &iter))
					continue;

				gtk_list_store_append(jc->priv->server_filter, &iter);
				gtk_list_store_set(jc->priv->server_filter, &iter,
				                   0, servers[i],
				                   1, MAESTRO_SERVER_TYPE_DAEMON,
				                   2, servers[i]);
			}
		}
	}

	GtkTreeIter new_it;
	gint index = 0;
	gint select_index = 0;
	gboolean find_index = FALSE;

	GtkTreeModel *servers_model = GTK_TREE_MODEL(jc->priv->server_filter);
	gebr_gui_gtk_tree_model_foreach_hyg(new_it, servers_model, combo) {
		gchar *addr;

		gtk_tree_model_get(servers_model, &new_it, 0, &addr, -1);

		if (g_strcmp0(addr, prev_selected) == 0) {
			find_index = TRUE;
			select_index = index;
			break;
		}
		index++;
	}
	gtk_combo_box_set_active(jc->priv->server_combo, select_index);

	if (!find_index)
		on_reset_filter(NULL, jc);

	g_signal_handlers_unblock_by_func(jc->priv->server_combo, on_cb_changed, jc);
}

static void
on_group_changed(GebrMaestroController *mc,
                 GebrMaestroServer *maestro,
                 GebrJobControl *jc)
{
}

static void
clear_jobs_for_maestro(GebrJobControl *jc,
		       GebrMaestroServer *maestro)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);
	const gchar *addr1 = gebr_maestro_server_get_address(maestro);
	const gchar *addr2;

	gebr_job_control_select_job(jc, NULL);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gboolean control;
		GebrJob *job;
		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			continue;

		addr2 = gebr_job_get_maestro_address(job);

		if (g_strcmp0(addr1, addr2) == 0)
			gebr_job_control_remove(jc, job);
	}
}

static void
on_maestro_state_changed(GebrMaestroController *mc,
			 GebrMaestroServer *maestro,
			 GebrJobControl *jc)
{
	if (gebr_maestro_server_get_state(maestro) == SERVER_STATE_DISCONNECTED)
		clear_jobs_for_maestro(jc, maestro);
	on_reset_filter(NULL, jc);
}

static gboolean
view_selection_func(GtkTreeSelection *selection,
                    GtkTreeModel *model,
                    GtkTreePath *path,
                    gboolean path_currently_selected,
                    GebrJobControl *jc)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);

	gboolean control;
	gtk_tree_model_get(model, &iter,
	                   JC_IS_CONTROL, &control,
	                   -1);

	if (control)
		return FALSE;

	return TRUE;
}

/* Public methods {{{1 */
GebrJobControl *
gebr_job_control_new(void)
{
	GebrJobControl *jc;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	jc = g_new(GebrJobControl, 1);
	jc->priv = g_new0(GebrJobControlPriv, 1);

	g_signal_connect(gebr.maestro_controller, "job-define",
	                 G_CALLBACK(on_job_define), jc);
	g_signal_connect(gebr.maestro_controller, "group-changed",
	                 G_CALLBACK(on_group_changed), jc);
	g_signal_connect(gebr.maestro_controller, "maestro-state-changed",
			 G_CALLBACK(on_maestro_state_changed), jc);

	jc->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(jc->priv->builder, GEBR_GLADE_DIR"/gebr-job-control.glade", NULL);

	jc->priv->widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "top-level-widget"));

	/*
	 * Left side
	 */

	jc->priv->filter_info_bar = gtk_info_bar_new();

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GtkWidget *close_button = gtk_button_new();
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	GtkWidget *button_box = gtk_vbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(close_button), image);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	gtk_box_pack_end(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(content), button_box, FALSE, FALSE, 0);
	g_signal_connect(close_button, "clicked", G_CALLBACK(on_reset_filter), jc);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(_("Jobs filtered by:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);

	GtkBox *left_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "left-side-box"));
	gtk_box_pack_start(left_box, jc->priv->filter_info_bar, FALSE, TRUE, 0);
	gtk_container_child_set(GTK_CONTAINER(left_box), jc->priv->filter_info_bar, "position", 0, NULL);

	jc->priv->use_filter_flow = FALSE;
	jc->priv->use_filter_servers = FALSE;
	jc->priv->use_filter_status = FALSE;

	jc->priv->user_server_name_filter = NULL;
	jc->priv->user_server_filter_id = NULL;
	jc->priv->user_flow_filter_id = NULL;

	jc->priv->restore_mode = FALSE;

	jc->priv->store = gtk_list_store_new(JC_N_COLUMN,
	                                     G_TYPE_POINTER,
	                                     G_TYPE_STRING,
	                                     G_TYPE_BOOLEAN,
	                                     G_TYPE_INT);

	GtkTreeIter iter;
	for (gint type = 0; type < TIME_N_TYPES; type++) {
		gchar *text = gebr_get_control_text_for_type(type);

		gtk_list_store_append(GTK_LIST_STORE(jc->priv->store), &iter);
		gtk_list_store_set(GTK_LIST_STORE(jc->priv->store), &iter,
		                   JC_STRUCT, NULL,
		                   JC_IS_CONTROL, TRUE,
		                   JC_CONTROL_TYPE, type,
		                   JC_TEXT_CONTROL, text,
		                   -1);

		g_free(text);
	}

	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(jc->priv->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       (GtkTreeModelFilterVisibleFunc)jobs_visible_func,
					       jc, NULL);

	GtkTreeModel *sort = gtk_tree_model_sort_new_with_model(filter);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sort), 0, tree_sort_func, jc, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort), 0, GTK_SORT_DESCENDING);

	GtkTreeView *treeview;
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(jc->priv->builder, "treeview_jobs"));
	gtk_tree_view_set_model(treeview, sort);
	g_object_unref(filter);
	g_object_unref(sort);

	gtk_widget_set_size_request(GTK_WIDGET(treeview), 200, -1);

	jc->priv->view = GTK_WIDGET(treeview);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(jc->priv->view), FALSE);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	g_signal_connect(selection, "changed", G_CALLBACK(job_control_on_cursor_changed), jc);

	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)view_selection_func, jc, NULL);


	/* Icon/Title column */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_append_column(treeview, col);

	/* Icon Renderer */
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, icon_column_data_func, NULL, NULL);

	/* Title Renderer */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	g_object_set(renderer, "ellipsize-set", TRUE, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
	gtk_tree_view_column_set_cell_data_func(col, renderer, title_column_data_func, NULL, NULL);

	/* Snapshot icon column */
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(treeview, col);
	gtk_tree_view_column_set_cell_data_func(col, renderer, snap_icon_column_data_func, NULL, NULL);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(jc->priv->view),
	                                          (GebrGuiGtkPopupCallback) job_control_popup_menu, jc);

	/*
	 * Filter
	 */

	GtkCellRenderer *cell;

	GtkComboBox *flow_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-flow-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(flow_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(flow_cb), cell, "text", 0);
	jc->priv->flow_combo = flow_cb;

	GtkComboBox *server_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-servers-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(server_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(server_cb), cell, "text", 0);
	jc->priv->server_combo = server_cb;

	GtkComboBox *status_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-status-cb"));
	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(status_cb), cell, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(status_cb), cell, "stock-id", ST_ICON);
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(status_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(status_cb), cell, "text", ST_TEXT);
	jc->priv->status_combo = status_cb;

//	g_signal_connect(gebr.maestro_controller, "maestro-list-changed",
//			 G_CALLBACK(on_maestro_list_changed), jc);

	/* by Flow */
	jc->priv->flow_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(flow_cb, GTK_TREE_MODEL(jc->priv->flow_filter));
	g_signal_connect(jc->priv->flow_combo, "changed", G_CALLBACK(on_cb_changed), jc);

	/* by Servers */
	jc->priv->server_filter = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(jc->priv->server_filter), 0, servers_filter_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(jc->priv->server_filter), 0, GTK_SORT_ASCENDING);
	gtk_combo_box_set_model(server_cb, GTK_TREE_MODEL(jc->priv->server_filter));
	g_signal_connect(jc->priv->server_combo, "changed", G_CALLBACK(on_cb_changed), jc);

	/* by Status of job */
	GtkListStore *store = gtk_list_store_new(ST_N_COLUMN,
	                                         G_TYPE_STRING,
	                                         G_TYPE_STRING,
	                                         G_TYPE_INT);
	jc->priv->status_model = store;

	gebr_jc_populate_status_cb(jc);
	gtk_combo_box_set_model(status_cb, GTK_TREE_MODEL(jc->priv->status_model));
	g_signal_connect(jc->priv->status_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	gtk_combo_box_set_active(jc->priv->status_combo, 0);


	/*
	 * Right side
	 */
	GtkButton *dismiss;
	dismiss = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_dismiss"));
	g_signal_connect(dismiss, "clicked", G_CALLBACK(on_dismiss_clicked), jc);

	GtkButton *show_issues;
	show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	g_signal_connect(show_issues, "clicked", G_CALLBACK(on_show_issues_clicked), jc);

	GtkButton *wait_button;
	wait_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	g_signal_connect(wait_button, "clicked", G_CALLBACK(on_job_wait_button), jc);

	jc->priv->info_button = gebr_gui_tool_button_new();
	gtk_button_set_relief(GTK_BUTTON(jc->priv->info_button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(jc->priv->info_button), gtk_image_new());
	g_object_set(jc->priv->info_button, "has-tooltip",TRUE, NULL);
	g_signal_connect(jc->priv->info_button, "query-tooltip", G_CALLBACK(detail_button_query_tooltip), jc);

	gtk_widget_show_all(jc->priv->info_button);
	GtkBox *box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "info_box"));
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "servers_widget"));
	gebr_gui_tool_button_add(GEBR_GUI_TOOL_BUTTON(jc->priv->info_button), widget);
	gtk_box_pack_start(box, jc->priv->info_button, FALSE, TRUE, 0);

	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	if (!gtk_widget_get_visible(GTK_WIDGET(info)))
		gtk_widget_show(GTK_WIDGET(show_issues));
	else
		gtk_widget_hide(GTK_WIDGET(show_issues));

	GtkWidget *text_view;
	GtkTextIter iter_end;
	
	GtkToggleButton *details = GTK_TOGGLE_BUTTON(gtk_builder_get_object(jc->priv->builder, "more_details"));
	g_signal_connect(details, "toggled", G_CALLBACK(on_toggled_more_details), jc->priv->builder);
	gtk_toggle_button_set_active(details, FALSE);

	/* Text view of output*/
	jc->priv->text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &iter_end);
	jc->priv->text_mark = gtk_text_buffer_create_mark(jc->priv->text_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "textview_output"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), jc->priv->text_buffer);
	g_object_set(text_view, "wrap-mode", gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);

	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	PangoFontDescription *font;

	font = pango_font_description_new();
	pango_font_description_set_family(font, "monospace");
	gtk_widget_modify_font(text_view, font);

	pango_font_description_free(font);

	jc->priv->text_view = text_view;
	g_signal_connect(jc->priv->text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), jc);

	jc->priv->output_window = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "output_window"));

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save"), FALSE);

	return jc;
}

void
gebr_job_control_free(GebrJobControl *jc)
{
	g_object_unref(jc->priv->builder);
	g_free(jc->priv->servers_info.percentages);
	g_free(jc->priv);
	g_free(jc);
}

GtkWidget *
gebr_job_control_get_widget(GebrJobControl *jc)
{
	return jc->priv->widget;
}

static void
on_status_update_toolbar_buttons(GebrJob *job,
                                 GebrCommJobStatus old_status,
                                 GebrCommJobStatus new_status,
                                 const gchar *parameter,
                                 GebrJobControl *jc)
{
	gboolean can_close, can_kill;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);

	gebr_jc_get_jobs_state(jc, rows, &can_close, &can_kill);

	update_control_buttons(jc, can_close, can_kill, TRUE);

	GtkTreeModel *sort = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
	GtkTreeModel *filter = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));
}

void
gebr_job_control_add(GebrJobControl *jc, GebrJob *job)
{
	GebrJob *tmp = gebr_job_control_find(jc, gebr_job_get_id(job));
	if (tmp) {
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(jc->priv->store), gebr_job_get_iter(tmp));
		gtk_tree_model_row_changed(GTK_TREE_MODEL(jc->priv->store), path, gebr_job_get_iter(tmp));
		gebr_job_control_load_details(jc, job);
		gtk_tree_path_free(path);
		return;
	}

	gtk_list_store_append(jc->priv->store, gebr_job_get_iter(job));
	gtk_list_store_set(jc->priv->store, gebr_job_get_iter(job),
	                   JC_STRUCT, job,
	                   JC_TEXT_CONTROL, NULL,
	                   JC_IS_CONTROL, FALSE,
	                   JC_CONTROL_TYPE, TIME_NONE,
	                   -1);
	g_signal_connect(job, "disconnect", G_CALLBACK(on_job_disconnected), jc);
	g_signal_connect(job, "job-remove", G_CALLBACK(on_job_remove), jc);
	g_signal_connect(job, "status-change", G_CALLBACK(on_status_update_toolbar_buttons), jc);
}

struct JobControlFindData {
	const gchar *rid;
	GebrJob *job;
};

static gboolean
job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	GebrJob *i;
	struct JobControlFindData *data = user_data;

	gtk_tree_model_get(model, iter, JC_STRUCT, &i, -1);

	if (!i)
		return FALSE;

	if (g_strcmp0(gebr_job_get_id(i), data->rid) == 0) {
		data->job = i;
		return TRUE;
	}
	return FALSE;
}

GebrJob *
gebr_job_control_find(GebrJobControl *jc, const gchar *rid)
{
	struct JobControlFindData data;

	g_return_val_if_fail(rid != NULL, NULL);

	data.job = NULL;
	data.rid = rid;

	gtk_tree_model_foreach(GTK_TREE_MODEL(jc->priv->store), job_find_foreach_func, &data);

	return data.job;
}

gboolean
gebr_job_control_save_selected(GebrJobControl *jc)
{
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;
	gchar *fname;
	FILE *fp;

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

	GtkTreeIter iter;
	GtkTreeModel *model;
	GebrJob *job;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, jc->priv->view) {
		gboolean control;
		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);
		
		if (control)
			continue;

		gchar * title;
		title = g_strdup_printf("---------- %s ---------\n", gebr_job_get_title(job));
		fputs(title, fp);
		g_free(title);

		/* Start and Finish dates */
		const gchar *start_date = gebr_job_get_start_date(job);
		const gchar *finish_date = gebr_job_get_finish_date(job);
		gchar *dates;
		dates = g_strdup_printf(_("\nStart date: %s\nFinish date: %s\n"), start_date? gebr_localized_date(start_date): _("(None)"),
					finish_date? gebr_localized_date(finish_date) : _("(None)"));
		fputs(dates, fp);
		g_free(dates);

		/* Issues */
		gchar *issues;
		gchar *job_issue = gebr_job_get_issues(job);
		issues = g_strdup_printf(_("\nIssues:\n%s"), job_issue? job_issue : _("(None)\n"));
		fputs(issues, fp);
		g_free(issues);
		g_free(job_issue);

		/* Input, output and log files */
		gchar *io, *input, *output, *log;
		gebr_job_get_io(job, &input, &output, &log);
		io = g_strdup_printf(_("\nInput File: %s\nOutput File: %s\nLog File: %s\n"), input? input : "(None)",
				     output? output : "(None)", log? log : "(None)");
		fputs(io, fp);
		g_free(io);
		g_free(input);
		g_free(output);
		g_free(log);

		/* Command line */
		gchar *cmd_line;
		gchar *command = gebr_job_get_command_line(job);
		cmd_line = g_strdup_printf("\n%s\n", strlen(command)? command : "(None)");
		fputs(cmd_line, fp);
		g_free(cmd_line);
		g_free(command);

		/* Output */
		gchar *text;
		gchar *output_text = gebr_job_get_output(job);
		text = g_strdup_printf(_("Output:\n%s\n\n"), strlen(output_text)? output_text : "(None)");
		fputs(text, fp);
		g_free(text);
		g_free(output_text);

		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Saved Job information at \"%s\"."), fname);
	}
	
	fclose(fp);
	g_free(fname);
	return TRUE;
}

void
gebr_job_control_stop_selected(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	GebrJob *job;
	
	gboolean asked = FALSE;
	gint selected_rows = 0;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	selected_rows =	gtk_tree_selection_count_selected_rows(selection);

	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows)
		return;

	for (GList *i = rows; i; i = i->next) {
		if (!gtk_tree_model_get_iter(model, &iter, i->data))
			return;

		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &job,
		                   -1);
		
		if (!job)
			continue;

		if (selected_rows == 1) {
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel Job \"%s\"?"),
							   gebr_job_get_title(job)) == FALSE)
				return;
		} else if (!asked) {
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel the selected Jobs?")) == FALSE)
				return;
			asked = TRUE;
		}

		gint n;
		gchar *servers = g_strjoinv(", ", gebr_job_get_servers(job, &n));

		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking node to cancel Job."));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking node \"%s\" to cancel Job \"%s\"."),
			     servers, gebr_job_get_title(job));
		g_free(servers);

		gebr_job_kill(job);
	}
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}

void
gebr_job_control_close_selected(GebrJobControl *jc)
{
	GebrJob *job;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *rows;
	GList *rowrefs = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows->next) {
		gboolean control;
		gtk_tree_model_get_iter(model, &iter, rows->data);
		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			goto free_rows;

		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear Job \"%s\"?"), gebr_job_get_title(job)))
			goto free_rows;
	} else {
		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear the selected Jobs?")))
			goto free_rows;
	}

	for (GList *i = rows; i; i = i->next) {
		GtkTreeRowReference *rowref = gtk_tree_row_reference_new(model, i->data);
		rowrefs = g_list_prepend(rowrefs, rowref);
	}

	jc->priv->last_selection.job = NULL;

	for (GList *i = rowrefs; i; i = i->next) {
		GtkTreePath *path = gtk_tree_row_reference_get_path(i->data);

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			g_warn_if_reached();
			continue;
		}
		gboolean control;
		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			continue;

		gebr_job_close(job);
		gtk_tree_path_free(path);
	}

	g_list_foreach(rowrefs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free(rowrefs);

free_rows:
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}

void
gebr_job_control_select_job_by_rid(GebrJobControl *jc, const gchar *rid)
{
	gebr_job_control_select_job(jc, gebr_job_control_find(jc, rid));
}

void
gebr_job_control_select_job(GebrJobControl *jc, GebrJob *job)
{
	if (job) {
		GtkTreeModel *sort = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
		GtkTreeModel *filter = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
		GtkTreeIter *iter = gebr_job_get_iter(job), filter_iter, sort_iter;

		if (!gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter), &filter_iter, iter))
			return;

		if (!gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT(sort), &sort_iter, &filter_iter))
			g_return_if_reached();

		GtkTreePath *path = gtk_tree_model_get_path(sort, &sort_iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(jc->priv->view), path, NULL, FALSE);
		gtk_tree_path_free(path);
	} else {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
		gtk_tree_selection_unselect_all(selection);
	}
}

typedef enum {
	JC_COMBO_TYPE_FLOW = 0,
	JC_COMBO_TYPE_SERVER,
	JC_COMBO_TYPE_STATUS,
} JobControlComboType;

static void
restore_user_defined_filters(GebrJobControl *jc)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;
	gboolean got_row = FALSE;
	GtkTreeModel *model;
	gchar *flow_id;

	jc->priv->restore_mode = TRUE;

	//##########################################################
	//FLOW
	model = GTK_TREE_MODEL(jc->priv->flow_filter);
	valid = FALSE;

	if (gtk_tree_model_get_iter_first(model, &iter) && gtk_tree_model_iter_next(model, &iter))
		valid = TRUE;

	while (valid) {
		gtk_tree_model_get(model, &iter,
				   1, &flow_id,
				   -1);

		if (!jc->priv->user_flow_filter_id && !flow_id)
			got_row = TRUE;
		else if (g_strcmp0(jc->priv->user_flow_filter_id, flow_id) == 0)
			got_row = TRUE;

		if (got_row) {
			gtk_combo_box_set_active_iter(jc->priv->flow_combo, &iter);
			break;
		}

		valid = gtk_tree_model_iter_next(model, &iter);
	}



	//##########################################################
	//STATUS
	model = GTK_TREE_MODEL(jc->priv->status_model);
	valid = FALSE;
	got_row = FALSE;

	if (gtk_tree_model_get_iter_first(model, &iter) && gtk_tree_model_iter_next(model, &iter))
	    valid = TRUE;

	while (valid) {
		GebrCommJobStatus status;
		gtk_tree_model_get(model, &iter,
				   ST_STATUS, &status,
				   -1);

		if (jc->priv->user_status_filter_id == status) {
			gtk_combo_box_set_active_iter(jc->priv->status_combo, &iter);
			break;
		}
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	//##########################################################
	//SERVER
	model = GTK_TREE_MODEL(jc->priv->server_filter);
	valid = FALSE;
	got_row = FALSE;
	if (gtk_tree_model_get_iter_first(model, &iter) && gtk_tree_model_iter_next(model, &iter))
	    valid = TRUE;

	while (valid) {
		gchar *server_name;
		GebrMaestroServerGroupType server_type;
		gtk_tree_model_get(model, &iter,
				   1, &server_type,
				   2, &server_name,
				   -1);

		if (g_strcmp0(jc->priv->user_server_name_filter, server_name) == 0 &&
		    server_type == jc->priv->user_server_type_filter) {
			gtk_combo_box_set_active_iter(jc->priv->server_combo, &iter);
			break;
		}
		valid = gtk_tree_model_iter_next(model, &iter);
	}
	jc->priv->restore_mode = FALSE;
}

GebrJob *
gebr_job_control_get_selected_job(GebrJobControl *jc)
{
	GebrJob *job = NULL;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows || rows->next)
		goto free_and_return;

	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, rows->data))
		g_return_val_if_reached(NULL);

	gtk_tree_model_get(model, &iter,
	                   JC_STRUCT, &job,
	                   -1);

free_and_return:

	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return job;
}

gboolean static
update_user_defined_filters(GebrJobControl *jc)
{
	GtkTreeIter active;

	gebr_job_control_free_user_defined_filter(jc);

	//Server
	if (gtk_combo_box_get_active_iter(jc->priv->server_combo, &active)) {
		gchar *name;
		GebrMaestroServerGroupType type;
		gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->server_filter), &active,
				   1, &type,
				   2, &name,
				   -1);
		jc->priv->user_server_type_filter = type;
		jc->priv->user_server_name_filter = g_strdup(name);
	}

	//Flow
	if (gtk_combo_box_get_active_iter(jc->priv->flow_combo, &active)) {
		gchar *id;
		gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->flow_filter), &active,
				   1, &id,
				   -1);
		jc->priv->user_flow_filter_id = g_strdup(id);
	}

	//Status
	if (gtk_combo_box_get_active_iter(jc->priv->status_combo, &active)) {
		GebrCommJobStatus status;
		gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->status_model), &active,
				   ST_STATUS, &status,
				   -1);
		jc->priv->user_status_filter_id = status;
	}

	return TRUE;
}

void
gebr_job_control_show(GebrJobControl *jc)
{
	if (!jc->priv->automatic_filter)
		restore_user_defined_filters(jc);

	jc->priv->automatic_filter = FALSE;
	jc->priv->timeout_source_id = g_timeout_add(1000, update_tree_view, jc);

	update_tree_view(jc);

	gtk_widget_reparent(jc->priv->text_view, jc->priv->output_window);
}

void
gebr_job_control_hide(GebrJobControl *jc)
{
	if (jc->priv->timeout_source_id)
		g_source_remove(jc->priv->timeout_source_id);

	jc->priv->automatic_filter = TRUE;
	gebr_job_control_reset_filters(jc);
	jc->priv->automatic_filter = FALSE;
}

void
gebr_job_control_setup_filter_button(GebrJobControl *jc,
				     GebrGuiToolButton *tool_button)
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "tl-filter"));
	g_signal_connect(tool_button, "toggled", G_CALLBACK(on_filter_show_unselect_jobs), jc);
	gebr_gui_tool_button_add(tool_button, widget);
}

gboolean
detail_button_query_tooltip(GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       GtkTooltip *tooltip,
			       GebrJobControl *jc)
{
	GebrJob *job = gebr_job_control_get_selected_job(jc);
	if (!job)
		return FALSE;

	gdouble value = gebr_job_get_exec_speed(job);
	gdouble tmp = gebr_ui_flow_execution_calculate_slider_from_speed(value);
	const gchar *text_tooltip = gebr_ui_flow_execution_set_text_for_performance(tmp);
	gtk_tooltip_set_text (tooltip, text_tooltip);
	return TRUE;
}

void
gebr_job_control_remove(GebrJobControl *jc,
			GebrJob *job)
{
	gtk_list_store_remove(jc->priv->store, gebr_job_get_iter(job));

	on_maestro_server_filter_changed(jc->priv->server_combo, jc);
	on_maestro_flow_filter_changed(jc->priv->flow_combo, jc);
}

GtkTreeModel *
gebr_job_control_get_model(GebrJobControl *jc)
{
	return GTK_TREE_MODEL(jc->priv->store);
}

GebrJob *
gebr_job_control_get_recent_job_from_flow(GebrGeoXmlDocument *flow,
                                          GebrJobControl *jc)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(flow);

	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->store);

	GebrJob *job = NULL;
	const gchar *last_date = NULL;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		GebrJob *curr_job;
		gboolean control;

		gtk_tree_model_get(model, &iter,
		                   JC_STRUCT, &curr_job,
		                   JC_IS_CONTROL, &control,
		                   -1);

		if (control)
			continue;

		const gchar *id = gebr_job_get_flow_id(curr_job);
		if (!g_strcmp0(flow_id, id)) {
			const gchar *last_job_date = gebr_job_get_last_run_date(curr_job);
			if (!last_date || g_strcmp0(last_date, last_job_date) < 0) {
				last_date = last_job_date;
				job = curr_job;
			}
		}
	}
	return job;
}

void
gebr_job_control_apply_flow_filter(GebrGeoXmlFlow *flow,
                                   GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->flow_filter);

	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gchar *id;
		gtk_tree_model_get(model, &iter,
		                   1, &id,
		                   -1);

		if (!g_strcmp0(id, flow_id)) {
			gtk_combo_box_set_active_iter(jc->priv->flow_combo, &iter);
			break;
		}
		g_free(id);
	}
	on_maestro_flow_filter_changed(jc->priv->flow_combo, jc);
}

void
gebr_job_control_free_user_defined_filter(GebrJobControl *jc)
{
	jc->priv->user_status_filter_id = -1;
	g_free(jc->priv->user_server_name_filter);
	g_free(jc->priv->user_flow_filter_id);
}

void
gebr_job_control_reset_filters(GebrJobControl *jc)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));

	gtk_combo_box_set_active(jc->priv->status_combo, 0);
	gtk_combo_box_set_active(jc->priv->server_combo, 0);
	gtk_combo_box_set_active(jc->priv->flow_combo, 0);

	gtk_widget_hide(GTK_WIDGET(jc->priv->filter_info_bar));
	job_control_on_cursor_changed(selection, jc);
}

GtkWidget *
gebr_job_control_get_output_view(GebrJobControl *jc)
{
	return jc->priv->text_view;
}

void
gebr_job_control_set_automatic_filter(GebrJobControl *jc, gboolean value)
{
	jc->priv->automatic_filter = value;
}
