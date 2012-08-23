/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/**
 * \file ui_flow_browse.c Responsible for UI for browsing a line's flows.
 */

#ifndef __UI_FLOW_BROWSE_H
#define __UI_FLOW_BROWSE_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include "libgebr/comm/gebr-comm-process.h"
#include "gebr-maestro-server.h"

G_BEGIN_DECLS

/*
 * Store fields
 */
enum {
	FB_STRUCT_TYPE = 0,
	FB_STRUCT,
	FB_N_COLUMN
};

typedef enum {
	STRUCT_TYPE_FLOW,
	STRUCT_TYPE_PROGRAM,
	STRUCT_TYPE_IO,
	STRUCT_TYPE_COLUMN
} GebrUiFlowBrowseType;

/**
 * Menu store fields
 */
enum {
	MENU_TITLE_COLUMN = 0,
	MENU_DESC_COLUMN,
	MENU_FILEPATH_COLUMN,
	MENU_N_COLUMN
};

/**
 */
typedef struct {
	GtkWidget *widget;

	GtkTreeStore *store;
	GtkWidget *view;
	GtkCellRenderer *icon_renderer;
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *action_renderer;

	GtkWidget *queue_combobox;
	GtkWidget *server_combobox;
	gchar *name;
	GebrMaestroServerGroupType type;

	GtkWidget *menu_window;
	GtkTreeStore *menu_store;
	GtkWidget *menu_view;

	GtkWidget *info_window;
	GtkWidget *warn_window;

	GtkWidget *revisions_button;

	GtkWidget *revpage_main;
	GtkWidget *revpage_warn;
	GtkWidget *revpage_warn_label;
	gboolean update_graph;
	GebrCommProcess *graph_process;

	GList *select_flows;

	/* Context Actions */
	GtkToggleButton *properties_ctx_button;
	GtkWidget *properties_ctx_box;
	GtkWidget *html_parameters;

	GtkToggleButton *snapshots_ctx_button;
	GtkWidget *snapshots_ctx_box;

	GtkWidget *jobs_ctx_box;

	/* Info Bar */
	GtkWidget *info_jobs;
	GtkWidget *jobs_status_box;
	gint last_info_width;

	struct ui_flow_browse_info {
		GtkBuilder *builder_flow;

		GtkWidget *status;
		GtkWidget *description;

		GtkWidget *modified;
		GtkWidget *lastrun;

		GtkWidget *help_view;
	} info;

	GHashTable *flow_jobs;

	GtkWidget *nice_button_high;
	GtkWidget *nice_button_low;
	GtkWidget *speed_button;
	GtkWidget *speed_slider;
	GtkWidget *ruler;
} GebrUiFlowBrowse;

/**
 * Assembly the flow browse page.
 * Return:
 * The structure containing relevant data.
 */
GebrUiFlowBrowse *flow_browse_setup_ui();

/**
 * Update information shown about the selected flow
 */
void flow_browse_info_update(void);

/**
 * Set to _iter_ the current selected flow
 */
gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected);

/**
 */
void flow_browse_reload_selected(void);

/**
 * Select flow at _iter_
 */
void flow_browse_select_iter(GtkTreeIter * iter);

/**
 * Turn multiple selection into single.
 */
void flow_browse_single_selection(void);

void flow_browse_show_help(void);

void flow_browse_edit_help(void);

void gebr_flow_browse_hide(GebrUiFlowBrowse *self);

void gebr_flow_browse_show(GebrUiFlowBrowse *self);

void gebr_flow_browse_status_icon(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  gpointer data);

void gebr_flow_browse_action_icon (GtkTreeViewColumn *tree_column,
                                   GtkCellRenderer *cell,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer data);

void gebr_flow_browse_text(GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell,
                           GtkTreeModel *model,
                           GtkTreeIter *iter,
                           gpointer data);

void gebr_flow_browse_select_snapshot_column(GtkTreeView *tree_view,
                                             GebrUiFlowBrowse *ui_flow_browse);

void gebr_flow_browse_select_job(GebrUiFlowBrowse *fb);

void gebr_flow_browse_append_job_on_flow(GebrGeoXmlFlow *flow,
                                         const gchar *job_id,
                                         GebrUiFlowBrowse *fb);

GList *gebr_flow_browse_get_jobs_from_flow(GebrGeoXmlFlow *flow,
                                           GebrUiFlowBrowse *fb);

void gebr_flow_browse_update_jobs_info(GebrGeoXmlFlow *flow,
                                       GebrUiFlowBrowse *fb,
                                       gint n_max);

void gebr_flow_browse_reset_jobs_from_flow(GebrGeoXmlFlow *flow,
                                           GebrUiFlowBrowse *fb);

void gebr_flow_browse_load_parameters_review(GebrGeoXmlFlow *flow,
                                             GebrUiFlowBrowse *fb);

void gebr_flow_browse_info_job(GebrUiFlowBrowse *fb,
                               const gchar *job_id);

const gchar *gebr_flow_browse_get_selected_queue(GebrUiFlowBrowse *fb);

void gebr_flow_browse_get_current_group(GebrUiFlowBrowse *fb,
                                        GebrMaestroServerGroupType *type,
                                        gchar **name);

void gebr_flow_browse_get_server_hostname(GebrUiFlowBrowse *fb,
                                          gchar **host);

void gebr_flow_browse_update_server(GebrUiFlowBrowse *fb,
                                    GebrMaestroServer *maestro);

void flow_browse_set_run_widgets_sensitiveness(GebrUiFlowBrowse *fb,
                                               gboolean sensitive,
                                               gboolean maestro_err);

void gebr_flow_browse_show_menu_list(GebrUiFlowBrowse *fb);

gint gebr_flow_browse_calculate_n_max(GebrUiFlowBrowse *fb);

void gebr_flow_browse_select_job_output(const gchar *job_id,
                                        GebrUiFlowBrowse *fb);

G_END_DECLS
#endif				//__UI_FLOW_BROWSE_H
