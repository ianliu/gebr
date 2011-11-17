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
	GtkComboBox *group_combo;
	GtkComboBox *server_combo;
	GtkComboBox *status_combo;
	GtkListStore *server_filter;
	GtkListStore *status_model;
	GtkListStore *store;
	GtkTextBuffer *text_buffer;
	GtkListStore *group_filter;
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
                                           enum JobStatus status);

static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu, GebrJobControl *jc);


static GtkMenu *job_control_popup_menu(GtkWidget * widget, GebrJobControl *job_control);

static void job_control_fill_servers_info(GebrJobControl *jc);

static void job_control_disconnect_signals(GebrJobControl *jc);

static void gebr_job_control_info_set_visible(GebrJobControl *jc,
					      gboolean visible,
					      const gchar *txt);


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
			msg = g_strdup(_("There are no jobs here! Execute a flow to populate this list."));
		else if (virt_n < real_n)
			msg = g_strdup_printf(_("%d jobs of %d are hidden because of the filter."),
					      real_n - virt_n, real_n);
		else
			msg = g_strdup(_("Select a job at the list on the left."));
	} else
		msg = g_strdup("Multiple jobs are selected.");

	gebr_job_control_info_set_visible(jc, FALSE, msg);
	g_free(msg);
}

static gboolean
get_server_group_iter(GebrJobControl *jc, const gchar *group, GtkTreeIter *iter)
{
	GtkTreeIter i;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->group_filter);
	gchar *g;
	gboolean valid = gtk_tree_model_get_iter_first(model, &i);

	if (valid)
		valid = gtk_tree_model_iter_next(model, &i);

	while (valid) {
		gtk_tree_model_get(model, &i, 1, &g, -1);
		if (g_strcmp0(g, group) == 0) {
			if (iter)
				*iter = i;
			g_free(g);
			return TRUE;
		}
		g_free(g);
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
jobs_visible_for_group(GtkTreeModel *model,
                       GtkTreeIter *iter,
                       GebrJobControl *jc)
{
	GtkTreeIter active;
	gchar *combo_group;
	gboolean visible = FALSE;

	if (!gtk_combo_box_get_active_iter (jc->priv->group_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->group_filter), &active,
			   1, &combo_group, -1);

	gchar *tmp = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(jc->priv->group_filter),
							 &active);
	gint index = atoi(tmp);
	g_free(tmp);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));

	if (index == 0) { // Any
		jc->priv->use_filter_group = FALSE;
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Group:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}

	GebrJob *job;
	const gchar *group;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job) {
		g_list_free(box);
		g_list_free(labels);
		return FALSE;
	}
	group = gebr_job_get_server_group(job);

	if (!g_strcmp0(combo_group, group))
		visible = TRUE;

	if (!jc->priv->use_filter_group) {
		const gchar *text = g_strdup_printf(_("Group: %s"), combo_group);
		gtk_box_pack_start(GTK_BOX(box->data), gtk_label_new(text), FALSE, FALSE, 0);
		jc->priv->use_filter_group = TRUE;
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Group:")) {
				const gchar *new_text = g_strdup_printf(_("Group: %s"), combo_group);
				gtk_label_set_text(i->data, new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);
	g_free(combo_group);
	return visible;
}

static gboolean
jobs_visible_for_servers(GtkTreeModel *model,
                         GtkTreeIter *iter,
                         GebrJobControl *jc)
{
	gboolean visible = FALSE;
	GtkTreeIter active;
	GebrServer *combo_server;

	if (!gtk_combo_box_get_active_iter (jc->priv->server_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->server_filter), &active,
	                   1, &combo_server, -1);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));

	if (!combo_server) {
		jc->priv->use_filter_servers = FALSE;
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Server:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}
	GebrJob *job;
	gchar **servers;
	gint n_servers;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job) {
		g_list_free(box);
		g_list_free(labels);
		return FALSE;
	}
	servers = gebr_job_get_servers(job, &n_servers);

	for (gint i = 0; i < n_servers; i++) {
		if (!g_strcmp0(servers[i], combo_server->comm->address->str))
			visible = TRUE;
	}

	if (!jc->priv->use_filter_servers) {
		const gchar *text = g_strdup_printf(_("Server: %s"), combo_server->comm->address->str);
		gtk_box_pack_start(GTK_BOX(box->data), gtk_label_new(text), FALSE, FALSE, 0);
		jc->priv->use_filter_servers = TRUE;
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Server:")) {
				const gchar *new_text = g_strdup_printf(_("Server: %s"), combo_server->comm->address->str);
				gtk_label_set_text(i->data, new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);
	return visible;
}

static gboolean
jobs_visible_for_status(GtkTreeModel *model,
                        GtkTreeIter *iter,
                        GebrJobControl *jc)
{
	GtkTreeIter active;
	enum JobStatus combo_status;
	gchar *combo_text;

	if (!gtk_combo_box_get_active_iter (jc->priv->status_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->status_model), &active,
	                   ST_STATUS, &combo_status,
	                   ST_TEXT, &combo_text, -1);

	GtkWidget *content = gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar));
	GList *box = gtk_container_get_children(GTK_CONTAINER(content));
	GList *labels = gtk_container_get_children(GTK_CONTAINER(box->data));

	if (combo_status == -1) {
		for (GList *i = labels; i; i = i->next)
			if (g_str_has_prefix(gtk_label_get_text(i->data), "Status:"))
				gtk_widget_destroy(GTK_WIDGET(i->data));
		jc->priv->use_filter_status = FALSE;
		g_list_free(box);
		g_list_free(labels);
		return TRUE;
	}
	GebrJob *job;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job) {
		g_list_free(box);
		g_list_free(labels);
		return FALSE;
	}
	enum JobStatus status = gebr_job_get_status(job);
	gboolean visible = FALSE;

	if (status == combo_status)
		visible = TRUE;
	else if (status == JOB_STATUS_FAILED && combo_status == JOB_STATUS_CANCELED)
		visible = TRUE;

	if (!jc->priv->use_filter_status) {
		const gchar *text = g_strdup_printf(_("Status: %s"), combo_text);
		gtk_box_pack_start(GTK_BOX(box->data), gtk_label_new(text), FALSE, FALSE, 0);
		jc->priv->use_filter_status = TRUE;
	} else {
		for (GList *i = labels; i; i = i->next) {
			const gchar *filter = gtk_label_get_text(i->data);
			if (g_str_has_prefix(filter, "Status:")) {
				const gchar *new_text = g_strdup_printf(_("Status: %s"), combo_text);
				gtk_label_set_text(i->data, new_text);
			}
		}
	}
	g_list_free(box);
	g_list_free(labels);
	return visible;
}

static gboolean
jobs_visible_func(GtkTreeModel *model,
		  GtkTreeIter *iter,
		  GebrJobControl *jc)
{
	if (!jobs_visible_for_status(model, iter, jc))
		return FALSE;

	if (!jobs_visible_for_servers(model, iter, jc))
		return FALSE;

	if (!jobs_visible_for_group(model, iter, jc))
		return FALSE;

	return TRUE;
}

static void
on_group_popup(GtkComboBox *group, GebrJobControl *jc)
{
	g_debug("POPUP");
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

static void
build_servers_filter_list(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_server_list->common.store);

	gtk_list_store_clear(jc->priv->server_filter);

	GtkTreeIter it;
	gtk_list_store_append(jc->priv->server_filter, &it);
	gtk_list_store_set(jc->priv->server_filter, &it, 0, _("Any"), -1);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gboolean is_autochoose;
		GebrServer *server;

		gtk_tree_model_get(model, &iter,
				   SERVER_IS_AUTO_CHOOSE, &is_autochoose,
				   SERVER_POINTER, &server,
				   -1);

		if (!server || !server->comm)
			continue;

		if (!gebr_comm_server_is_logged(server->comm) || is_autochoose)
			continue;

		gtk_list_store_append(jc->priv->server_filter, &it);
		gtk_list_store_set(jc->priv->server_filter, &it,
				   0, server->comm->address->str,
				   1, server,
				   -1);
	}

	gtk_combo_box_set_active(jc->priv->server_combo, 0);
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

	gtk_label_set_text(label, issues);
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
	      GebrTask *task,
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
	      enum JobStatus old_status,
	      enum JobStatus new_status,
	      const gchar *parameter,
	      GebrJobControl *jc)
{
	gebr_job_control_load_details(jc, job);
}

static void
on_job_issued(GebrJob *job,
	      const gchar *issues,
	      GebrJobControl *jc)
{
	fill_issues_properties(jc, issues, job);
}

static void
gebr_job_control_info_set_visible(GebrJobControl *jc,
				  gboolean visible,
				  const gchar *txt)
{
	GtkWidget *job_info = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "job_info_widget"));
	GtkLabel *empty_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "empty_job_selection_label"));

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
	if (!g_strcmp0(jc->priv->servers_info.servers[i], "127.0.0.1"))
		server = server_get_name_from_address(jc->priv->servers_info.servers[i]);
	else
		server = jc->priv->servers_info.servers[i];

	gchar *t = g_strdup_printf("%s\n%d%% of total", server, (int)round((jc->priv->servers_info.percentages[i])*100));
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
	gchar *nprocs, *niceness;
	gint n_servers, i;
	gchar **servers;

	servers = gebr_job_get_servers(job, &n_servers);

	gebr_job_get_resources(job, &nprocs, &niceness);

	if (!nprocs || !niceness)
		g_string_printf(resources, _("Waiting for server(s) details"));
	else {
		const gchar *groups = gebr_job_get_server_group(job);
		gchar *markup;

		if (g_strcmp0(groups, "") == 0)
			groups = _("All Servers");

		markup = g_markup_printf_escaped (_("Job submitted by <b>%s</b> to group <b>%s</b>.\n"
						  "Executed using %s<b>%s</b> processor(s)\ndistributed on <b>%d</b> servers.\n"),
						  gebr_job_get_hostname(job), groups,
						  g_strcmp0(niceness, "0")? _("idle time of ") : "", nprocs, n_servers);
		g_string_append(resources, markup);

		g_free(nprocs);
		g_free(niceness);
		g_free(markup);
	}
	gtk_label_set_markup (res_label, resources->str);

	g_string_free(resources, TRUE);

	guint *values = g_new(guint, n_servers);
	GtkWidget *piechart = gebr_gui_pie_new(values, 0);

	jc->priv->servers_info.servers = servers;
	if (jc->priv->servers_info.percentages)
		g_free(jc->priv->servers_info.percentages);
	jc->priv->servers_info.percentages = g_new(gdouble, n_servers);

	g_signal_connect(piechart, "query-tooltip", G_CALLBACK(on_pie_tooltip), jc);

	gtk_widget_set_has_tooltip(piechart, TRUE);
	gtk_widget_set_size_request(piechart, 150, 150);

	GtkBox *pie_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "pie_box"));
	GtkBox *servers_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "servers_box"));

	gtk_container_foreach(GTK_CONTAINER(servers_box), (GtkCallback)gtk_widget_destroy, NULL);
	gtk_container_foreach(GTK_CONTAINER(pie_box), (GtkCallback)gtk_widget_destroy, NULL);

	gtk_box_pack_start(pie_box, piechart, TRUE, FALSE, 0);
	gtk_widget_show_all(piechart);

	for (i = 0; servers[i]; i++) {
		GebrTask *task = gebr_job_get_task_from_server(job, servers[i]);
		if (task) {
			gdouble percentage = gebr_task_get_percentage(task);
			values[i] = (guint)(percentage * 1000);
			jc->priv->servers_info.percentages[i] = percentage;
		}
	}
	gebr_gui_pie_set_data(GEBR_GUI_PIE(piechart), values, n_servers);
	g_free(values);

	for (i = 0; servers[i]; i++) {
		const gchar *server;
		if (!g_strcmp0(servers[i], "127.0.0.1"))
			 server = server_get_name_from_address(servers[i]);
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
	const gchar *stock_id;

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
	g_get_current_time(&curr_time);


	gchar *str_aux = gebr_calculate_relative_time(&start_time, &curr_time);
	relative_time_msg = g_strconcat( str_aux, _(" ago"), NULL );

	g_object_set(cell, "text", relative_time_msg, NULL);
	g_free(str_aux);
	g_free(relative_time_msg);

	return;
}

static void
gebr_jc_update_status_and_time(GebrJobControl *jc,
                               GebrJob 	      *job,
                               enum JobStatus status)
{
	const gchar *start_date = gebr_job_get_start_date(job);
	const gchar *finish_date = gebr_job_get_finish_date(job);
	GString *start = g_string_new(NULL);
	GString *finish = g_string_new(NULL);

	/* start date (may have failed, never started) */
	if (start_date && strlen(start_date))
		g_string_append_printf(start, "%s %s", _("Started at"),
		                       gebr_localized_date(start_date));

	/* finish date */
	if (finish_date && strlen(finish_date))
		g_string_append_printf(finish, "%s %s", status == JOB_STATUS_FINISHED ?
				       _("Finished at") : _("Canceled at"),
				       gebr_localized_date(finish_date));

	GtkImage *img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "status_image"));
	GtkLabel *subheader = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_label"));
	GtkButton *queued_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	GtkLabel *details_start_date = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "detail_start_date"));
	gtk_widget_hide(GTK_WIDGET(queued_button));

	job_control_fill_servers_info(jc);

	if (status == JOB_STATUS_FINISHED) {
		gchar *elapsed_time = g_strdup_printf("%s\nElapsed time: %s", finish->str, gebr_job_get_elapsed_time(job));
		gtk_image_set_from_stock(img, GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, elapsed_time);
		gtk_label_set_text(details_start_date, start->str);
		gtk_widget_show(GTK_WIDGET(details_start_date));
		g_free(elapsed_time);
	}

	else if (status == JOB_STATUS_RUNNING) {
		gchar *running = g_strdup_printf("%s\nElapsed time: %s", start->str, gebr_job_get_running_time(job, start_date));
		gtk_image_set_from_stock(img, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, running);
		gtk_widget_hide(GTK_WIDGET(details_start_date));
		g_free(running);
	}

	else if (status == JOB_STATUS_CANCELED) {
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, finish->str);
		gtk_label_set_text(details_start_date, start->str);
	}

	else if (status == JOB_STATUS_FAILED) {
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, _("Job failed"));
		gtk_label_set_text(details_start_date, start->str);
	}

	else if (status == JOB_STATUS_QUEUED) {
		GebrJob *parent;
		GtkImage *wait_img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "subheader_button_img"));
		GtkLabel *wait_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_button_lbl"));

		parent = gebr_job_control_find(jc, gebr_job_get_queue(job));

		if (parent) {
			gtk_image_set_from_stock(img, "chronometer", GTK_ICON_SIZE_DIALOG);
			gtk_label_set_text(subheader, "Waiting for job");

			gtk_image_set_from_stock(wait_img, job_control_get_icon_for_job(parent), GTK_ICON_SIZE_BUTTON);
			gtk_label_set_text(wait_label, gebr_job_get_title(parent));

			gtk_widget_show(GTK_WIDGET(queued_button));
		}
	}
	else if (status == JOB_STATUS_INITIAL) {
		gchar *remainings = gebr_job_get_remaining_servers(job);
		gchar *wait_text = g_strdup_printf(_("Waiting for servers: %s"), remainings? remainings : "");
		gtk_image_set_from_stock(img, GTK_STOCK_NETWORK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, wait_text);
		gtk_widget_hide(GTK_WIDGET(details_start_date));
		if (remainings)
			g_free(remainings);
		g_free(wait_text);
	}

	g_string_free(start, FALSE);
	g_string_free(finish, FALSE);
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

	GList *tasks = gebr_job_get_list_of_tasks(job);
	if (!tasks)
		g_return_if_reached();

	if (g_list_length(tasks) > 1) {
		GtkWidget *vbox = gtk_vbox_new(FALSE, 8);

		for (GList *i = tasks; i; i = i->next) {
			GebrTask *task = i->data;

			gint frac, total;
			gebr_task_get_fraction(task, &frac, &total);
			gchar *title = g_strdup_printf(_("Command line for task %d of %d (Server: %s)"),
						       frac, total, gebr_task_get_server(task)->comm->address->str);
			GtkWidget *expander = gtk_expander_new(title);
			gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
			g_free(title);

			GtkWidget *view = gtk_text_view_new();
			gtk_widget_modify_font(view, font);
			g_object_set(view, "wrap-mode", wrapmode, NULL);
			g_object_set(view, "editable", FALSE, NULL);
			jc->priv->cmd_views = g_list_prepend(jc->priv->cmd_views, view);
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
			gtk_text_buffer_set_text(buffer, gebr_task_get_cmd_line(task), -1);

			g_signal_connect(view, "populate-popup",
					 G_CALLBACK(on_text_view_populate_popup), jc);

			gtk_container_add(GTK_CONTAINER(expander), view);
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
		}

		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox);
		gtk_widget_show_all(vbox);
	} else {
		GtkWidget *view = gtk_text_view_new();
		gtk_widget_modify_font(view, font);
		g_object_set(view, "wrap-mode", wrapmode, NULL);
		g_object_set(view, "editable", FALSE, NULL);
		jc->priv->cmd_views = g_list_prepend(jc->priv->cmd_views, view);
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
		gtk_text_buffer_set_text(buffer, gebr_task_get_cmd_line(tasks->data), -1);

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
	enum JobStatus status = gebr_job_get_status(job);

	input_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "input_label"));
	output_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "output_label"));
	log_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "error_label"));
	job_group = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "job_group"));
	info_button_image = GTK_IMAGE(gtk_bin_get_child(GTK_BIN(jc->priv->info_button)));

	gebr_job_get_io(job, &input_file_str, &output_file_str, &log_file_str);
	update_control_buttons(jc, gebr_job_can_close(job), gebr_job_can_kill(job), TRUE);

	gchar *markup;

	markup = g_markup_printf_escaped ("<b>Input file</b>: %s", input_file_str ? input_file_str : _("None"));
	gtk_label_set_markup (input_file, markup);
	g_free(markup);

	markup = g_markup_printf_escaped ("<b>Output file</b>: %s", output_file_str ? output_file_str : _("None"));
	gtk_label_set_markup (output_file, markup);
	g_free(markup);

	markup = g_markup_printf_escaped ("<b>Log File</b>: %s", log_file_str ? log_file_str : _("None"));
	gtk_label_set_markup (log_file, markup);
	g_free(markup);

	gchar *msg = g_strdup(gebr_job_get_server_group(job));
	if (!g_strcmp0(msg, ""))
		msg = g_strdup("All servers");
	gtk_label_set_markup (job_group, msg);

	switch (gebr_job_get_exec_speed(job))
	{
	case 1:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-low", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	case 2: case 3: case 4:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-medium", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	case 5:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-high", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	default:
		g_warn_if_reached();
	}

	g_object_set(info_button_image, "has-tooltip",TRUE, NULL);
	g_signal_connect(info_button_image, "query-tooltip", G_CALLBACK(detail_button_query_tooltip), jc);

	gebr_jc_update_status_and_time(jc, job, status);

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
	if (job) {
		enum JobStatus status = gebr_job_get_status(job);
		if (status == JOB_STATUS_RUNNING)
			gebr_jc_update_status_and_time(jc, job, status);
	}
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

	return tva.tv_sec - tvb.tv_sec;
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
on_row_changed_model(GtkTreeModel *tree_model,
                     GtkTreePath  *path,
                     GtkTreeIter  *iter,
                     GebrJobControl *jc)
{
	build_servers_filter_list(jc);
}

static void
on_reset_filter(GtkInfoBar *info_bar,
                gint        response_id,
                gpointer    user_data)
{
	GebrJobControl *jc = user_data;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));

	gtk_combo_box_set_active(jc->priv->status_combo, 0);
	gtk_combo_box_set_active(jc->priv->server_combo, 0);
	gtk_combo_box_set_active(jc->priv->group_combo, 0);

	gtk_widget_hide(GTK_WIDGET(info_bar));

	job_control_on_cursor_changed(selection, jc);
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

	jc->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(jc->priv->builder, GEBR_GLADE_DIR"/gebr-job-control.glade", NULL);

	jc->priv->widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "top-level-widget"));

	/*
	 * Left side
	 */

	jc->priv->filter_info_bar = gtk_info_bar_new_with_buttons(_("Show all"), GTK_RESPONSE_OK, NULL);
	g_signal_connect(jc->priv->filter_info_bar, "response", G_CALLBACK(on_reset_filter), jc);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gtk_info_bar_get_content_area(GTK_INFO_BAR(jc->priv->filter_info_bar))), vbox, TRUE, TRUE, 0);

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

	GtkComboBox *group_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-servers-group-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(group_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(group_cb), cell, "text", 0);
	jc->priv->group_combo = group_cb;

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

	/* by Servers */
	jc->priv->server_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_combo_box_set_model(server_cb, GTK_TREE_MODEL(jc->priv->server_filter));
	g_signal_connect(jc->priv->server_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	g_signal_connect(gebr.ui_server_list->common.store, "row-changed", G_CALLBACK(on_row_changed_model), jc);

	/* by Group of servers */
	jc->priv->group_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkTreeIter iter;
	gtk_list_store_append(jc->priv->group_filter, &iter);
	gtk_list_store_set(jc->priv->group_filter, &iter, 0, _("Any"), -1);
	gtk_combo_box_set_model(group_cb, GTK_TREE_MODEL(jc->priv->group_filter));
	g_signal_connect(jc->priv->group_combo, "popup", G_CALLBACK(on_group_popup), jc);
	g_signal_connect(jc->priv->group_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	gtk_combo_box_set_active(jc->priv->group_combo, 0);

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

void
gebr_job_control_add(GebrJobControl *jc, GebrJob *job)
{
	gtk_list_store_append(jc->priv->store, gebr_job_get_iter(job));
	gtk_list_store_set(jc->priv->store, gebr_job_get_iter(job), JC_STRUCT, job, -1);
	g_signal_connect(job, "disconnect", G_CALLBACK(on_job_disconnected), jc);

	const gchar *group = gebr_job_get_server_group(job);
	if (!group)
		group = "";

	if (!get_server_group_iter(jc, group, NULL)) {
		GtkTreeIter iter;
		const gchar *tmp;

		if (g_strcmp0(group, "") == 0)
			tmp  = _("All Servers");
		else
			tmp = group;

		gtk_list_store_append(jc->priv->group_filter, &iter);
		gtk_list_store_set(jc->priv->group_filter, &iter,
				   0, tmp,
				   1, group, -1);
	}
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
		dates = g_strdup_printf("\nStart date: %s\nFinish date: %s\n", start_date? gebr_localized_date(start_date): "(None)",
					finish_date? gebr_localized_date(finish_date) : "(None)");
		fputs(dates, fp);
		g_free(dates);

		/* Issues */
		gchar *issues;
		gchar *job_issue = gebr_job_get_issues(job);
		issues = g_strdup_printf("\nIssues:\n%s", strlen(job_issue)? job_issue : "(None)\n");
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
		cmd_line = g_strdup_printf(_("\nCommand Line:\n%s\n"), strlen(command)? command : "(None)");
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

		gchar *servers = g_strjoinv(", ", gebr_job_get_servers(job, NULL));

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

	BLOCK_SELECTION_CHANGED_SIGNAL(jc);
	for (GList *i = rowrefs; i; i = i->next) {
		GtkTreePath *path = gtk_tree_row_reference_get_path(i->data);

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			g_warn_if_reached();
			continue;
		}
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		if (gebr_job_close(job))
			gebr_job_control_remove(jc, job);
		gtk_tree_path_free(path);
	}
	UNBLOCK_SELECTION_CHANGED_SIGNAL(jc);
	job_control_on_cursor_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), jc);

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
	gint value = gebr_job_get_exec_speed(job);
	const gchar *text_tooltip = set_text_for_performance(value);
	gtk_tooltip_set_text (tooltip, text_tooltip);
	return TRUE;
}
void
gebr_job_control_remove(GebrJobControl *jc,
			GebrJob *job)
{
	GtkTreeIter iter;
	gboolean has_group = FALSE;
	const gchar *group = gebr_job_get_server_group(job);

	gtk_list_store_remove(jc->priv->store, gebr_job_get_iter(job));

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(jc->priv->store)) {
		GebrJob *j;
		gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->store), &iter, JC_STRUCT, &j, -1);
		if (!j)
			continue;
		if (g_strcmp0(gebr_job_get_server_group(j), group) == 0) {
			has_group = TRUE;
			break;
		}
	}

	if (!has_group && get_server_group_iter(jc, group, &iter)) {
		gtk_list_store_remove(jc->priv->group_filter, &iter);
		gtk_combo_box_set_active(jc->priv->group_combo, 0);
	}
	g_object_unref(job);
}

GtkTreeModel *
gebr_job_control_get_model(GebrJobControl *jc)
{
	return GTK_TREE_MODEL(jc->priv->store);
}
