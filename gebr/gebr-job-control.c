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
	GtkComboBox *maestro_combo;
	GtkComboBox *server_combo;
	GtkComboBox *status_combo;
	GtkListStore *server_filter;
	GtkListStore *status_model;
	GtkListStore *store;
	GtkTextBuffer *text_buffer;
	GtkListStore *maestro_filter;
	GList *cmd_views;
	GtkWidget *filter_info_bar;
	GtkWidget *label;
	GtkWidget *text_view;
	GtkWidget *view;
	GtkWidget *widget;
	GtkWidget *info_button;
	LastSelection last_selection;
	guint timeout_source_id;

	gboolean use_filter_status;
	gboolean use_filter_servers;
	gboolean use_filter_group;

	struct {
		gchar **servers;
		gdouble *percentages;
	} servers_info;
};

enum {
	JC_STRUCT,
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

static void gebr_jc_update_status_and_time(GebrJobControl *jc,
                                           GebrJob 	  *job,
                                           GebrCommJobStatus status);

static void on_maestro_filter_changed(GtkComboBox *combo,
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
		if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data)) {
			gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);

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

	gchar *msg;
	gint real_n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(jc->priv->store), NULL);
	gint virt_n = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view)), NULL);

	if (!rows) {
		if (real_n == 0)
			msg = g_strdup(_("No jobs! Try out one of our demos (go to Help->Samples)."));
		else if (virt_n < real_n)
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

static GebrJob *
get_selected_job(GebrJobControl *jc)
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

	gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);

free_and_return:

	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return job;
}

static gboolean
jobs_visible_for_maestro(GtkTreeModel *model,
			 GtkTreeIter *iter,
			 GebrJobControl *jc)
{
	GtkTreeIter active;
	gchar *combo_group;
	gchar *combo_name;
	gboolean visible = FALSE;

	if (!gtk_combo_box_get_active_iter (jc->priv->maestro_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->maestro_filter), &active,
			   0, &combo_name,
	                   1, &combo_group, -1);

	gchar *tmp = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(jc->priv->maestro_filter),
							 &active);
	gint index = atoi(tmp);
	g_free(tmp);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));
	labels = labels->next;

	if (index == 0) { // Any
		jc->priv->use_filter_group = FALSE;
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Maestro:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}

	if (!jc->priv->use_filter_group) {
		gchar *text = g_markup_printf_escaped("<span size='x-small'>Maestro: %s</span>", combo_name);
		GtkWidget *label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), text);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_box_pack_start(GTK_BOX(box->data), label, FALSE, FALSE, 0);
		jc->priv->use_filter_group = TRUE;
		g_free(text);
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Maestro:")) {
				gchar *new_text = g_markup_printf_escaped("<span size='x-small'>Maestro: %s</span>", combo_name);
				gtk_label_set_markup(i->data, new_text);
				g_free(new_text);
			}
		}
	}

	g_list_free(box);
	g_list_free(labels);
	g_free(combo_name);

	GebrJob *job;
	const gchar *maddr;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job)
		return FALSE;

	maddr = gebr_job_get_maestro_address(job);

	if (!g_strcmp0(combo_group, maddr))
		visible = TRUE;

	g_free(combo_group);

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
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Group/Server:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}


	if (!jc->priv->use_filter_servers) {
		gchar *text = g_markup_printf_escaped("<span size='x-small'>Group/Server: %s</span>", display);
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
				gchar *new_text = g_markup_printf_escaped("<span size='x-small'>Group/Server: %s</span>", display);
				gtk_label_set_markup(i->data, new_text);
				g_free(new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);
	GebrJob *job;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job) {
		g_free(name);
		g_free(display);
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

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

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
jobs_visible_func(GtkTreeModel *model,
		  GtkTreeIter *iter,
		  GebrJobControl *jc)
{
	gboolean visible = TRUE;

	if (!jobs_visible_for_status(model, iter, jc))
		visible = FALSE;

	if (!jobs_visible_for_servers(model, iter, jc))
		visible = FALSE;

	if (!jobs_visible_for_maestro(model, iter, jc))
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
	    jc->priv->use_filter_group == FALSE &&
	    jc->priv->use_filter_servers == FALSE)
		gtk_widget_hide(jc->priv->filter_info_bar);
	else
		gtk_widget_show_all(jc->priv->filter_info_bar);

	on_select_non_single_job(jc, NULL);
}

static const gchar *
get_issues_image_for_status(GebrJob *job)
{
	return GTK_STOCK_DIALOG_WARNING;
}

static void
fill_issues_properties(GebrJobControl *jc,
                       const gchar *issues,
                       GebrJob *job)
{
	g_debug("on %s", __func__);
	const gchar *stock_id;
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_label"));
	GtkImage *image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_image"));

	gtk_widget_show(GTK_WIDGET(info));
	GtkButton *show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	gtk_widget_hide(GTK_WIDGET(show_issues));

	stock_id = get_issues_image_for_status(job);

	if (g_strcmp0(stock_id, GTK_STOCK_DIALOG_ERROR) == 0)
		gtk_info_bar_set_message_type(info, GTK_MESSAGE_ERROR);
	else
		gtk_info_bar_set_message_type(info, GTK_MESSAGE_WARNING);

	gtk_label_set_markup(label, issues);
	gtk_label_set_line_wrap(label, TRUE);
	gtk_label_set_line_wrap_mode(label, PANGO_WRAP_WORD);
	gtk_image_set_from_stock(image, stock_id, GTK_ICON_SIZE_DIALOG);
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
	GebrJob *job = get_selected_job(jc);
	fill_issues_properties(jc, gebr_job_get_issues(job), job);
}

static void
on_job_wait_button(GtkButton *button,
                   GebrJobControl *jc)
{
	GebrJob *job, *parent;

	job = get_selected_job(jc);
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
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(jc->priv->text_view), &end,
					     0, FALSE, 0, 0);
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
	g_debug("on %s", __func__);
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

	GebrJob *job = get_selected_job(jc);

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
	GebrJob *job = get_selected_job(jc);
	GString *resources = g_string_new(NULL);
	GtkLabel *res_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "resources_text"));
	const gchar *nprocs;
	const gchar *niceness;
	gint n_servers, i;
	gchar **servers;
	gint total_procs;

	if (!job)
		return;

	servers = gebr_job_get_servers(job, &n_servers);
	total_procs = gebr_job_get_total_procs(job);
	gebr_job_get_resources(job, &nprocs, &niceness);

	const gchar *maddr = gebr_job_get_maestro_address(job);
	if (!nprocs || !niceness)
		g_string_printf(resources, _("Waiting for server(s) details"));
	else {
		const gchar *type_str = gebr_job_get_server_group_type(job);
		const gchar *groups = gebr_job_get_server_group(job);
		GebrMaestroServerGroupType type = gebr_maestro_server_group_str_to_enum(type_str);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);

		gchar *markup;

		if (type == MAESTRO_SERVER_TYPE_GROUP)
			if (g_strcmp0(groups, "") == 0)
				groups = g_strdup_printf(_("%s"), gebr_maestro_server_get_display_address(maestro));

		markup = g_markup_printf_escaped(_("Job submitted by <b>%s</b> to Maestro <b>%s</b>.\n"
						   "Executed on total <b>%d</b> processes on %s <b>%s</b>,\n"
						   "%s"
						   "distributed on <b>%d</b> servers.\n"),
						  gebr_job_get_hostname(job), gebr_job_get_maestro_address(job),
						  total_procs, type == MAESTRO_SERVER_TYPE_DAEMON? "server" : "group", groups,
						  g_strcmp0(niceness, "0")? _("using the machines idle time, ") : "",
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
	for (i = 0; i < len; i++)
		values[i] = (guint)(percs[i] * 1000);

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

	if (!rows->next) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)rows->data))
			gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
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
		gtk_image_set_from_stock(image, get_issues_image_for_status(job), GTK_ICON_SIZE_DND);
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
	GebrJob *job;
	const gchar *stock_id;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);

	stock_id = job_control_get_icon_for_job(job);

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

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);
	g_object_set(cell, "text", gebr_job_get_title(job), NULL);
}

static void
time_column_data_func(GtkTreeViewColumn *tree_column,
		      GtkCellRenderer *cell,
		      GtkTreeModel *tree_model,
		      GtkTreeIter *iter,
		      gpointer data)
{
	GebrJob *job;
	const gchar *start_time_str;
	gchar *relative_time_msg;

	GTimeVal start_time, curr_time;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);

	start_time_str = gebr_job_get_last_run_date(job);
	if (!start_time_str)
		return;
	g_time_val_from_iso8601(start_time_str, &start_time);

	const gchar *maddr = gebr_job_get_maestro_address(job);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);

	g_get_current_time(&curr_time);
	gint clocks_diff = gebr_maestro_server_get_clocks_diff(maestro);
	curr_time.tv_sec -= clocks_diff;

	gchar *str_aux = gebr_calculate_relative_time(&start_time, &curr_time);
	relative_time_msg = g_strconcat( str_aux, _(" ago"), NULL );

	g_object_set(cell, "text", relative_time_msg, NULL);
	g_free(str_aux);
	g_free(relative_time_msg);

	return;
}

static void
compute_subheader_label(GebrJob *job,
			GebrCommJobStatus status,
			gchar **subheader,
			gchar **start_detail)
{
	const gchar *start_date = gebr_job_get_start_date(job);
	const gchar *finish_date = gebr_job_get_finish_date(job);
	GString *start = g_string_new(NULL);
	GString *finish = g_string_new(NULL);

	/* start date (may have failed, never started) */
	if (start_date && strlen(start_date))
		g_string_append_printf(start, _("Started at %s"), gebr_localized_date(start_date));

	/* finish date */
	if (finish_date && strlen(finish_date)) {
		const gchar *tmp = gebr_localized_date(finish_date);
		if (status == JOB_STATUS_FINISHED)
			g_string_append_printf(finish, _("Finished at %s"), tmp);
		else
			g_string_append_printf(finish, _("Canceled at %s"), tmp);
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
		*subheader = g_string_free(finish, FALSE);
		*start_detail = g_string_free(start, FALSE);
		return;
	}

	if (status == JOB_STATUS_FAILED) {
		*subheader = g_strdup(_("Job failed"));
		*start_detail = g_string_free(start, FALSE);
		g_string_free(finish, TRUE);
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
		*subheader = _("Waiting for servers");
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
	gchar *subheader_str, *start_detail_str;
	GtkImage *img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "status_image"));
	GtkLabel *subheader = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_label"));
	GtkButton *queued_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	GtkLabel *details_start_date = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "detail_start_date"));
	gtk_widget_hide(GTK_WIDGET(queued_button));

	compute_subheader_label(job, status, &subheader_str, &start_detail_str);

	if (subheader_str) {
		gtk_label_set_text(subheader, subheader_str);
		gtk_widget_show(GTK_WIDGET(subheader));
	} else
		gtk_widget_hide(GTK_WIDGET(subheader));

	if (start_detail_str) {
		gtk_label_set_text(details_start_date, start_detail_str);
		gtk_widget_show(GTK_WIDGET(details_start_date));
	} else
		gtk_widget_hide(GTK_WIDGET(details_start_date));

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
	GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_builder_get_object(jc->priv->builder, "scrolledwindow2"));
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

			gchar *title = g_strdup_printf(_("Command line for task %d of %d (Server: %s)"),
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

	const gchar *maddr = gebr_job_get_maestro_address(job);
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, maddr);

	if (!g_strcmp0(msg, ""))
		msg = g_strdup_printf("%s", gebr_maestro_server_get_display_address(maestro));
	if (!g_strcmp0(msg, "127.0.0.1"))
		msg = g_strdup(maddr); 


	gtk_label_set_markup (job_group, msg);

	gdouble speed = gebr_job_get_exec_speed(job);
	const gchar *icon = gebr_interface_get_speed_icon(gebr_interface_calculate_slider_from_speed(speed));
	if (icon)
		gtk_image_set_from_stock(info_button_image, icon, GTK_ICON_SIZE_LARGE_TOOLBAR);

	job_control_fill_servers_info(jc);
	gebr_jc_update_status_and_time(jc, job, status);
	update_control_buttons(jc, gebr_job_can_close(job), gebr_job_can_kill(job), TRUE);

	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "header_label"));
	const gchar *title = gebr_job_get_title(job);
	markup = g_markup_printf_escaped ("<span size=\"large\"><b>%s</b></span>", title);
	gtk_label_set_markup (label, markup);
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
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid)
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free(path);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	GebrJob *job = get_selected_job(jc);
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
	GebrJob *ja, *jb;

	gtk_tree_model_get(model, a, JC_STRUCT, &ja, -1);
	gtk_tree_model_get(model, b, JC_STRUCT, &jb, -1);

	if (!ja && !jb)
		return 0;

	if (!ja)
		return 1;

	if (!jb)
		return -1;

	const gchar *ta = gebr_job_get_last_run_date(ja);
	const gchar *tb = gebr_job_get_last_run_date(jb);

	if (!ta && !tb)
		return 0;

	if (!ta)
		return 1;

	if (!tb)
		return -1;

	GTimeVal tva;
	GTimeVal tvb;
	g_time_val_from_iso8601(ta, &tva);
	g_time_val_from_iso8601(tb, &tvb);

	glong timestamp = tva.tv_sec - tvb.tv_sec;

	if (timestamp == 0)
		return tva.tv_usec - tvb.tv_usec;
	return timestamp;
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

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));

	gtk_combo_box_set_active(jc->priv->status_combo, 0);
	gtk_combo_box_set_active(jc->priv->server_combo, 0);
	gtk_combo_box_set_active(jc->priv->maestro_combo, 0);

	gtk_widget_hide(GTK_WIDGET(jc->priv->filter_info_bar));

	job_control_on_cursor_changed(selection, jc);
}

static void
on_job_define(GebrMaestroController *mc,
	      GebrMaestroServer *maestro,
	      GebrJob *job,
	      GebrJobControl *jc)
{
	gebr_job_control_add(jc, job);
	on_maestro_filter_changed(jc->priv->maestro_combo, jc);
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
on_maestro_filter_changed(GtkComboBox *combo,
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
		GebrJob *job;
		const gchar *group, *type_str, *display;
		GebrMaestroServerGroupType type;

		gtk_tree_model_get(model, &it, JC_STRUCT, &job, -1);

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

	GtkTreeModel *servers_model = GTK_TREE_MODEL(jc->priv->server_filter);
	gebr_gui_gtk_tree_model_foreach_hyg(new_it, servers_model, combo) {
		gchar *addr;

		gtk_tree_model_get(servers_model, &new_it, 0, &addr, -1);

		if (g_strcmp0(addr, prev_selected) == 0) {
			select_index = index;
			break;
		}
		index++;
	}
	gtk_combo_box_set_active(jc->priv->server_combo, select_index);

	g_signal_handlers_unblock_by_func(jc->priv->server_combo, on_cb_changed, jc);
}

static void
on_maestro_list_changed(GebrMaestroController *mc,
			GebrJobControl *jc)
{
	GtkTreeIter iter;

	gtk_list_store_clear(jc->priv->maestro_filter);

	gtk_list_store_append(jc->priv->maestro_filter, &iter);
	gtk_list_store_set(jc->priv->maestro_filter, &iter, 0, _("Any"), -1);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(mc);
	gtk_list_store_append(jc->priv->maestro_filter, &iter);
	gtk_list_store_set(jc->priv->maestro_filter, &iter,
			   0, gebr_maestro_server_get_display_address(maestro),
			   1, gebr_maestro_server_get_address(maestro),
			   -1);

	gtk_combo_box_set_active(jc->priv->maestro_combo, 0);
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
		GebrJob *job;
		gtk_tree_model_get(model, &iter, 0, &job, -1);
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
	gtk_box_pack_start(GTK_BOX(content), vbox, FALSE, TRUE, 0);

	GtkBox *left_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "left-side-box"));
	gtk_box_pack_start(left_box, jc->priv->filter_info_bar, FALSE, TRUE, 0);
	gtk_container_child_set(GTK_CONTAINER(left_box), jc->priv->filter_info_bar, "position", 0, NULL);

	jc->priv->use_filter_group = FALSE;
	jc->priv->use_filter_servers = FALSE;
	jc->priv->use_filter_status = FALSE;

	jc->priv->store = gtk_list_store_new(JC_N_COLUMN, G_TYPE_POINTER);

	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(jc->priv->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       (GtkTreeModelFilterVisibleFunc)jobs_visible_func,
					       jc, NULL);

	GtkTreeModel *sort = gtk_tree_model_sort_new_with_model(filter);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sort), 0, tree_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort), 0, GTK_SORT_DESCENDING);

	GtkTreeView *treeview;
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(jc->priv->builder, "treeview_jobs"));
	gtk_tree_view_set_model(treeview, sort);
	g_object_unref(filter);
	g_object_unref(sort);

	gtk_widget_set_size_request(GTK_WIDGET(treeview), 150, -1);

	jc->priv->view = GTK_WIDGET(treeview);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(jc->priv->view), FALSE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), "changed",
			 G_CALLBACK(job_control_on_cursor_changed), jc);

	col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(jc->priv->builder, "tv_column"));

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_icon_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, icon_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_title_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, title_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_time_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, time_column_data_func, NULL, NULL);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(jc->priv->view),
	                                          (GebrGuiGtkPopupCallback) job_control_popup_menu, jc);

	/*
	 * Filter
	 */

	GtkCellRenderer *cell;

	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "label5"));
	gtk_widget_hide(GTK_WIDGET(label));
	GtkComboBox *maestro_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-servers-group-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(maestro_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(maestro_cb), cell, "text", 0);
	jc->priv->maestro_combo = maestro_cb;
	gtk_widget_hide(GTK_WIDGET(maestro_cb));

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

	g_signal_connect(gebr.maestro_controller, "maestro-list-changed",
			 G_CALLBACK(on_maestro_list_changed), jc);

	/* by Maestro */
	jc->priv->maestro_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(maestro_cb, GTK_TREE_MODEL(jc->priv->maestro_filter));
	//g_signal_connect(jc->priv->maestro_combo, "changed",
	//		 G_CALLBACK(on_maestro_filter_changed), jc);

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
	gtk_text_buffer_create_mark(jc->priv->text_buffer, "end", &iter_end, FALSE);

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
	gtk_list_store_set(jc->priv->store, gebr_job_get_iter(job), JC_STRUCT, job, -1);
	g_signal_connect(job, "disconnect", G_CALLBACK(on_job_disconnected), jc);
	g_signal_connect(job, "job-remove", G_CALLBACK(on_job_remove), jc);
	g_signal_connect(job, "status-change", G_CALLBACK(on_status_update_toolbar_buttons), jc);
}

GebrJob *
gebr_job_control_find(GebrJobControl *jc, const gchar *rid)
{
	GebrJob *job = NULL;

	g_return_val_if_fail(rid != NULL, NULL);

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		GebrJob *i;

		gtk_tree_model_get(model, iter, JC_STRUCT, &i, -1);

		if (!i)
			return FALSE;

		if (g_strcmp0(gebr_job_get_id(i), rid) == 0) {
			job = i;
			return TRUE;
		}
		return FALSE;
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(jc->priv->store), job_find_foreach_func, NULL);

	return job;
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

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		
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
	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, jc->priv->view) {
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		
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

		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to cancel Job."));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server(s) \"%s\" to cancel Job \"%s\"."),
			     servers, gebr_job_get_title(job));
		g_free(servers);

		gebr_job_kill(job);
	}
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
		gtk_tree_model_get_iter(model, &iter, rows->data);
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
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
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
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

void
gebr_job_control_show(GebrJobControl *jc)
{
	jc->priv->timeout_source_id = g_timeout_add(1000, update_tree_view, jc);
}

void
gebr_job_control_hide(GebrJobControl *jc)
{
	if (jc->priv->timeout_source_id)
		g_source_remove(jc->priv->timeout_source_id);
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
	GebrJob *job = get_selected_job(jc);
	gdouble value = gebr_job_get_exec_speed(job);
	gdouble tmp = gebr_interface_calculate_slider_from_speed(value);
	const gchar *text_tooltip = gebr_interface_set_text_for_performance(tmp);
	gtk_tooltip_set_text (tooltip, text_tooltip);
	return TRUE;
}

void
gebr_job_control_remove(GebrJobControl *jc,
			GebrJob *job)
{
	gtk_list_store_remove(jc->priv->store, gebr_job_get_iter(job));

	on_maestro_filter_changed(jc->priv->server_combo, jc);
}

GtkTreeModel *
gebr_job_control_get_model(GebrJobControl *jc)
{
	return GTK_TREE_MODEL(jc->priv->store);
}
