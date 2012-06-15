/*
 * gebr-flow-edition.c
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

#include "gebr-flow-edition.h"

#include <string.h>
#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-iexpr.h>
#include <libgebr/gebr-expr.h>
#include <gdk/gdkkeysyms.h>

#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_parameters.h"
#include "ui_help.h"
#include "callbacks.h"
#include "ui_document.h"
#include "gebr-maestro-server.h"

/*
 * Prototypes
 */

struct _GebrFlowEditionPriv {
	GtkWidget *server_combobox;
	GtkWidget *queue_combobox;

	/* Group selection */
	gchar *name;
	GebrMaestroServerGroupType type;
};

static void on_controller_maestro_state_changed(GebrMaestroController *mc,
						GebrMaestroServer *maestro,
						GebrFlowEdition *fe);

static void on_daemons_changed(GebrMaestroController *mc,
                               GebrFlowEdition *fe);

static void on_group_changed(GebrMaestroController *mc,
			     GebrMaestroServer *maestro,
			     GebrFlowEdition *fe);

static gboolean flow_edition_find_by_group(GebrFlowEdition *fe,
					   GebrMaestroServerGroupType type,
					   const gchar *name,
					   GtkTreeIter *iter);

static gboolean flow_edition_find_flow_server(GebrFlowEdition *fe,
					      GebrGeoXmlFlow *flow,
					      GtkTreeModel   *model,
					      GtkTreeIter    *iter);

static gboolean flow_edition_may_reorder(GtkTreeView *tree_view,
					 GtkTreeIter *iter,
					 GtkTreeIter * position,
					 GtkTreeViewDropPosition drop_position,
					 GebrFlowEdition *ui_flow_edition);

static gboolean flow_edition_reorder(GtkTreeView * tree_view,
				     GtkTreeIter * iter,
				     GtkTreeIter * position,
				     GtkTreeViewDropPosition drop_position,
				     GebrFlowEdition *ui_flow_edition);

static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path);
static void flow_edition_component_editing_canceled(GtkCellRenderer *renderer, gpointer user_data);
static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text);
static void flow_edition_component_selected(void);
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected);

static void flow_edition_menu_add(void);
static void flow_edition_menu_show_help(void);

static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_flow_edition);
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_flow_edition);
static void on_groups_combobox_changed(GtkComboBox *combobox, GebrFlowEdition *fe);

static gboolean on_flow_sequence_query_tooltip(GtkTreeView * treeview,
					       gint x,
					       gint y,
					       gboolean keyboard_tip,
					       GtkTooltip * tooltip,
					       GebrFlowEdition *ui_flow_edition);

static void on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
						       GtkCellRenderer *cell,
						       GtkTreeModel    *tree_model,
						       GtkTreeIter     *iter,
						       gpointer         data);

static void on_queue_set_text(GtkCellLayout   *cell_layout,
                              GtkCellRenderer *cell,
                              GtkTreeModel    *tree_model,
                              GtkTreeIter     *iter,
                              gpointer         data);

static void on_queue_combobox_changed (GtkComboBox *combo, GebrFlowEdition *fe);

static void select_file_column(GtkTreeView *tree_view, GebrFlowEdition *fe);

static gboolean
menu_search_func(GtkTreeModel *model,
		 gint column,
		 const gchar *key,
		 GtkTreeIter *iter,
		 gpointer data)
{
	gchar *title, *desc;
	gchar *lt, *ld, *lk; // Lower case strings
	gboolean match;

	if (!key)
		return FALSE;

	gtk_tree_model_get(model, iter,
			   MENU_TITLE_COLUMN, &title,
			   MENU_DESC_COLUMN, &desc,
			   -1);

	lt = title ? g_utf8_strdown(title, -1) : g_strdup("");
	ld = desc ?  g_utf8_strdown(desc, -1)  : g_strdup("");
	lk = g_utf8_strdown(key, -1);

	match = gebr_utf8_strstr(lt, lk) || gebr_utf8_strstr(ld, lk);

	g_free(title);
	g_free(desc);
	g_free(lt);
	g_free(ld);
	g_free(lk);

	return !match;
}

GebrFlowEdition *
flow_edition_setup_ui(void)
{
	GebrFlowEdition *fe;

	GtkWidget *hpanel;
	GtkWidget *frame;
	GtkWidget *alignment;
	GtkWidget *label;
	GtkWidget *left_vbox;
	GtkWidget *scrolled_window;
	GtkWidget *vbox;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* alloc */
	fe = g_new(GebrFlowEdition, 1);
	fe->priv = g_new0(GebrFlowEditionPriv, 1);
	fe->priv->name = NULL;
	fe->priv->type = -1;

	g_signal_connect_after(gebr.maestro_controller, "group-changed",
			       G_CALLBACK(on_group_changed), fe);
	g_signal_connect_after(gebr.maestro_controller, "maestro-state-changed",
			       G_CALLBACK(on_controller_maestro_state_changed), fe);
	g_signal_connect(gebr.maestro_controller, "daemons-changed",
			       G_CALLBACK(on_daemons_changed), fe);

	/* Create flow edit fe->widget */
	fe->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(fe->widget), hpanel);

	/*
	 * Left side: flow components
	 */
	left_vbox = gtk_vbox_new(FALSE, 5);
	gtk_paned_pack1(GTK_PANED(hpanel), left_vbox, FALSE, FALSE);
	gtk_widget_set_size_request(left_vbox, 300, -1);

	/*
	 * Creates the QUEUE combobox
	 */
	fe->priv->queue_combobox = gtk_combo_box_new();
	g_signal_connect(fe->priv->queue_combobox, "changed",
			 G_CALLBACK(on_queue_combobox_changed), fe);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fe->priv->queue_combobox), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(fe->priv->queue_combobox), renderer,
	                                   on_queue_set_text, NULL, NULL);
	gtk_widget_show(fe->priv->queue_combobox);

	/*
	 * Creates the SERVER combobox
	 */
	fe->priv->server_combobox = gtk_combo_box_new();
	g_signal_connect(fe->priv->server_combobox, "changed",
			 G_CALLBACK(on_groups_combobox_changed), fe);
	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fe->priv->server_combobox), renderer, FALSE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(fe->priv->server_combobox), renderer,
					   on_server_disconnected_set_row_insensitive, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fe->priv->server_combobox), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(fe->priv->server_combobox), renderer,
					   on_server_disconnected_set_row_insensitive, NULL, NULL);

	frame = gtk_frame_new(NULL);
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	label = gtk_label_new_with_mnemonic(_("Run"));
	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(vbox), alignment);
	gtk_container_add(GTK_CONTAINER(vbox), fe->priv->queue_combobox);
	gtk_container_add(GTK_CONTAINER(vbox), fe->priv->server_combobox);
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Flow sequence"));
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	fe->fseq_store = gtk_list_store_new(FSEQ_N_COLUMN,
	                                    G_TYPE_STRING,
	                                    G_TYPE_STRING,
	                                    G_TYPE_POINTER,
	                                    G_TYPE_INT,
	                                    G_TYPE_BOOLEAN,
	                                    G_TYPE_BOOLEAN,
	                                    G_TYPE_STRING,
	                                    G_TYPE_BOOLEAN,
	                                    G_TYPE_STRING);

	fe->fseq_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fe->fseq_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(fe->fseq_view)),
				    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(fe->fseq_view),
						  (GebrGuiGtkPopupCallback) flow_edition_component_popup_menu,
						  fe);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(fe->fseq_view),
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_reorder,
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_may_reorder,
						    fe);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fe->fseq_view), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(fe->fseq_view), FALSE);

	/* Icon column */
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(fe->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", FSEQ_ICON_COLUMN);

	g_object_set(G_OBJECT(fe->fseq_view), "has-tooltip", TRUE, NULL);
	g_signal_connect(G_OBJECT(fe->fseq_view), "query-tooltip", G_CALLBACK(on_flow_sequence_query_tooltip), fe);

	/* Text column */
	fe->text_renderer = renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(fe->fseq_view), col);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", FSEQ_TITLE_COLUMN);
	gtk_tree_view_column_add_attribute(col, renderer, "editable", FSEQ_EDITABLE);
	gtk_tree_view_column_add_attribute(col, renderer, "ellipsize", FSEQ_ELLIPSIZE);
	gtk_tree_view_column_add_attribute(col, renderer, "sensitive", FSEQ_SENSITIVE);

	g_signal_connect(renderer, "edited", G_CALLBACK(flow_edition_component_edited), NULL);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(flow_edition_component_editing_started), NULL);
	g_signal_connect(renderer, "editing-canceled", G_CALLBACK(flow_edition_component_editing_canceled), NULL);

	/* Directories column */
	fe->file_renderer = renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(fe->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "sensitive", FSEQ_SENSITIVE);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", FSEQ_FILE_COLUMN);

	g_signal_connect(fe->fseq_view, "cursor-changed", G_CALLBACK(select_file_column), fe);

	/* Space key pressed on flow component changes its configured status */
	g_signal_connect(fe->fseq_view, "key-press-event",
			 G_CALLBACK(flow_edition_component_key_pressed), fe);

	/* Double click on flow component open its parameter window */
	g_signal_connect(fe->fseq_view, "row-activated",
			 G_CALLBACK(flow_edition_component_activated), fe);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(fe->fseq_view)), "changed",
			 G_CALLBACK(flow_edition_component_selected), fe);

	gtk_container_add(GTK_CONTAINER(scrolled_window), fe->fseq_view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);

	/*
	 * Right side: Menu list
	 */
	frame = gtk_frame_new(_("Menus"));
	gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, FALSE);
	gtk_widget_set_size_request(frame, 150, -1);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);

	fe->menu_store = gtk_tree_store_new(MENU_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fe->menu_store), MENU_TITLE_COLUMN, GTK_SORT_ASCENDING);
	fe->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fe->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), fe->menu_view);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(fe->menu_view),
						  (GebrGuiGtkPopupCallback) flow_edition_menu_popup_menu,
						  fe);
	g_signal_connect(fe->menu_view, "key-press-event",
			 G_CALLBACK(flow_edition_component_key_pressed), fe);
	g_signal_connect(GTK_OBJECT(fe->menu_view), "row-activated",
			 G_CALLBACK(flow_edition_menu_add), fe);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(fe->menu_view), MENU_TITLE_COLUMN);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(fe->menu_view),
					    menu_search_func, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(fe->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(fe->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	gtk_paned_set_position(GTK_PANED(hpanel), 150);

	return fe;
}

void
gebr_flow_edition_update_speed_slider_sensitiveness(GebrFlowEdition *fe)
{
	gboolean sensitive;
	GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_first_mpi_program(gebr.flow);

	if ((gebr_geoxml_flow_is_parallelizable(gebr.flow, gebr.validator))
	 || (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED))
		sensitive = TRUE;
	else
		sensitive = FALSE;
	
	gebr_geoxml_object_unref(prog);

	gebr_interface_update_speed_sensitiveness(fe->speed_button,
						  fe->speed_slider,
						  fe->ruler,
						  sensitive);
}

void
flow_edition_set_run_widgets_sensitiveness(GebrFlowEdition *fe,
                                           gboolean sensitive,
                                           gboolean maestro_err)
{
	if (gebr_geoxml_line_get_flows_number(gebr.line) == 0 && sensitive)
		sensitive = FALSE;

	if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0 && sensitive)
		sensitive = FALSE;

	const gchar *tooltip_disconn;
	const gchar *tooltip_execute;

	if (!gebr.line) {
		if (!gebr.project)
			tooltip_disconn = _("Select a line to execute a flow");
		else
			tooltip_disconn = _("Select a line of this project to execute a flow");
	} else {
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
		if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
			tooltip_disconn = _("The Maestro of this line is disconnected.\nConnecting it to execute a flow.");
		else if (gebr_geoxml_line_get_flows_number(gebr.line) == 0)
			tooltip_disconn = _("This line does not contain flows\nCreate a flow to execute this line");
		else if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
			tooltip_disconn = _("This flow does not contain programs\nAdd at least one to execute this flow");
		else
			tooltip_disconn = _("Execute");
	}
	tooltip_execute = _("Execute");

	gtk_widget_set_sensitive(fe->priv->queue_combobox, sensitive);
	gtk_widget_set_sensitive(fe->priv->server_combobox, sensitive);

	GtkAction *action = gtk_action_group_get_action(gebr.action_group_flow, "flow_execute");
	const gchar *tooltip = sensitive ? tooltip_execute : tooltip_disconn;

	gtk_action_set_stock_id(action, "gtk-execute");
	gtk_action_set_sensitive(action, sensitive);
	gtk_action_set_tooltip(action, tooltip);

	action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_execute");
	gtk_action_set_stock_id(action, "gtk-execute");
	gtk_action_set_sensitive(action, sensitive);
	gtk_action_set_tooltip(action, tooltip);
}

void flow_edition_load_components(void)
{
	GebrGeoXmlSequence *first_program;

	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	if (!flow_browse_get_selected(NULL, FALSE))
		return;

	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter);

	/* now into GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);
	flow_add_program_sequence_to_view(first_program, FALSE, FALSE);

	flow_edition_set_io();
}

gboolean flow_edition_get_selected_component(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No program selected."));
		return FALSE;
	}

	return TRUE;
}

void flow_edition_select_component_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter);
	flow_edition_component_selected();
}

void flow_edition_set_io(void)
{
	GError *err = NULL;
	gchar *title;
	gchar *result;
	gchar *tooltip;
	const gchar *icon;
	gboolean is_append;
	GtkTreeModel *model;
	gboolean sensitivity;

	flow_program_check_sensitiveness();
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));

	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	const gchar *tmp;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *mount_point;
	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;

	tmp = gebr_geoxml_flow_io_get_input(gebr.flow);
	gchar *input_real = gebr_relativise_path(tmp, mount_point, paths);
	gchar* input = g_markup_escape_text(input_real, -1);
	tmp = gebr_geoxml_flow_io_get_output(gebr.flow);
	gchar *output_real = gebr_relativise_path(tmp, mount_point, paths);
	gchar* output = g_markup_escape_text(output_real, -1);
	tmp = gebr_geoxml_flow_io_get_error(gebr.flow);
	gchar *error_real = gebr_relativise_path(tmp, mount_point, paths);
	gchar* error = g_markup_escape_text(error_real, -1);

	/* Set the INPUT properties.
	 * INPUT does not have 'Append/Overwrite' so there is only 2 possible icons:
	 *  - gebr-stdin
	 *  - GTK_STOCK_DIALOG_WARNING
	 */
	gtk_tree_model_get(model, &gebr.ui_flow_edition->input_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*input || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Input file"));
		icon = "gebr-stdin";
		tooltip = NULL;
	} else {
		title = g_strdup(input);
		gchar *resolved_input = gebr_resolve_relative_path(input_real, paths);
		gebr_validator_evaluate(gebr.validator, resolved_input, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		g_free(resolved_input);

		gchar *tmp;
		gboolean ok;
		ok = gebr_validator_evaluate_interval(gebr.validator, input_real,
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
						      FALSE, &tmp, NULL);

		gchar *err_msg;
		if (err) {
			tooltip = g_strdup(err->message);
			icon = GTK_STOCK_DIALOG_WARNING;
			g_clear_error(&err);
		} else if (!gebr_validate_path(tmp, paths, &err_msg)) {
			tooltip = err_msg;
			icon = GTK_STOCK_DIALOG_WARNING;
		} else {
			tooltip = g_strdup_printf(_("Input file \"%s\""), result);
			icon = "gebr-stdin";
			g_free(result);
		}

		if (ok)
			g_free(tmp);
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   FSEQ_FILE_COLUMN, GTK_STOCK_DIRECTORY,
			   -1);
	g_free(title);
	g_free(tooltip);

	/* Set the OUTPUT properties.
	 * OUTPUT have 'Append/Overwrite' so there is 3 possible icons:
	 *  - gebr-stdout
	 *  - gebr-append-stdout
	 *  - GTK_STOCK_DIALOG_WARNING
	 */
	icon = NULL;
	result = NULL;
	is_append = gebr_geoxml_flow_io_get_output_append(gebr.flow);
	gtk_tree_model_get(model, &gebr.ui_flow_edition->output_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*output || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Output file"));
		tooltip = NULL;
	} else {
		title = g_strdup(output);
		gchar *resolved_output = gebr_resolve_relative_path(output_real, paths);
		gebr_validator_evaluate(gebr.validator, resolved_output, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		g_free(resolved_output);

		gchar *tmp;
		gboolean ok;
		ok = gebr_validator_evaluate_interval(gebr.validator, output_real,
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
						      FALSE, &tmp, NULL);

		gchar *err_msg;
		if (err) {
			tooltip = g_strdup(err->message);
		} else if (!gebr_validate_path(tmp, paths, &err_msg)) {
			tooltip = err_msg;
			icon = GTK_STOCK_DIALOG_WARNING;
		} else {
			if (is_append)
				tooltip = g_strdup_printf(_("Append to output file \"%s\""), result);
			else
				tooltip = g_strdup_printf(_("Overwrite output file \"%s\""), result);
			g_free(result);
		}

		if (ok)
			g_free(tmp);
	}

	if (!err && !icon)
		icon = is_append ? "gebr-append-stdout":"gebr-stdout";
	else {
		icon = GTK_STOCK_DIALOG_WARNING;
		g_clear_error(&err);
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   FSEQ_FILE_COLUMN, GTK_STOCK_DIRECTORY,
			   -1);
	g_free(title);
	g_free(tooltip);

	/* Set the ERROR properties. */
	icon = NULL;
	result = NULL;
	is_append = gebr_geoxml_flow_io_get_error_append(gebr.flow);
	gtk_tree_model_get(model, &gebr.ui_flow_edition->error_iter, FSEQ_SENSITIVE, &sensitivity, -1);
	if (!*error || !sensitivity) {
		title = g_markup_printf_escaped("<i>%s</i>", _("Log file"));
		tooltip = NULL;
	} else {
		title = g_strdup(error);
		gchar *resolved_error = gebr_resolve_relative_path(error_real, paths);
		gebr_validator_evaluate(gebr.validator, resolved_error, GEBR_GEOXML_PARAMETER_TYPE_STRING,
		                        GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);
		g_free(resolved_error);

		gchar *tmp;
		gboolean ok;
		ok = gebr_validator_evaluate_interval(gebr.validator, error_real,
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
						      FALSE, &tmp, NULL);

		gchar *err_msg;
		if (err) {
			tooltip = g_strdup(err->message);
		} else if (!gebr_validate_path(tmp, paths, &err_msg)) {
			tooltip = err_msg;
			icon = GTK_STOCK_DIALOG_WARNING;
		} else {
			if (is_append)
				tooltip = g_strdup_printf(_("Append to log file \"%s\""), result);
			else
				tooltip = g_strdup_printf(_("Overwrite log file \"%s\""), result);
			g_free(result);
		}

		if (ok)
			g_free(tmp);
	}

	if (!err && !icon)
		icon = is_append ? "gebr-append-stderr":"gebr-stderr";
	else {
		icon = GTK_STOCK_DIALOG_WARNING;
		g_clear_error(&err);
	}

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
			   FSEQ_ICON_COLUMN, icon,
			   FSEQ_TITLE_COLUMN, title, 
			   FSEQ_EDITABLE, sensitivity? TRUE : FALSE,
			   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_START,
			   FSEQ_TOOLTIP, tooltip,
			   FSEQ_FILE_COLUMN, GTK_STOCK_DIRECTORY,
			   -1);

	flow_browse_info_update();

	g_free(title);
	g_free(tooltip);
	g_free(input);
	g_free(output);
	g_free(error);
	g_free(input_real);
	g_free(output_real);
	g_free(error_real);
	gebr_pairstrfreev(paths);
}

void flow_edition_component_activated(void)
{
	GtkTreeIter iter;
	gchar *title;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, FSEQ_TITLE_COLUMN, &title, -1);
	gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Configuring program '%s'."), title);
	parameters_configure_setup_ui();
	g_free(title);
}

static void _open_activated(GebrFlowEdition *fe,
                            const gchar *path)
{
	gchar *entry_text;
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(fe->fseq_store), &iter, path);

	GtkFileChooserAction action;
	gchar *stock;
	gchar *title = NULL;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter)) {
		entry_text = gebr_geoxml_flow_io_get_input(gebr.flow);

		action = GTK_FILE_CHOOSER_ACTION_OPEN;
		stock = GTK_STOCK_OPEN;
		title = g_strdup(_("Choose an input file"));
	} else {
		action = GTK_FILE_CHOOSER_ACTION_SAVE;
		stock = GTK_STOCK_SAVE;
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
			entry_text = gebr_geoxml_flow_io_get_output(gebr.flow);

			title = g_strdup(_("Choose an output file"));
		}
		else {
			entry_text = gebr_geoxml_flow_io_get_error(gebr.flow);
			title = g_strdup(_("Choose a log file"));
		}
	}
	if (!title)
		title = g_strdup(_("Choose file"));

	GtkWidget *dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW(gebr.window), action,
							stock, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *prefix = gebr_maestro_server_get_sftp_prefix(maestro);

	gchar *new_text;
	gint response = gebr_file_chooser_set_remote_navigation(dialog, entry_text,
	                                                        prefix, paths, TRUE,
	                                                        &new_text);
	if (response == GTK_RESPONSE_YES) {
		g_object_set(fe->text_renderer, "text", new_text, NULL);
		
		flow_edition_component_edited(GTK_CELL_RENDERER_TEXT(fe->text_renderer), g_strdup(path), new_text);
	}

	g_free(new_text);
	g_free(title);
	g_free(entry_text);
	gebr_pairstrfreev(paths);
}

static void
select_file_column(GtkTreeView *tree_view,
                   GebrFlowEdition *fe)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_turn_to_single_selection(tree_view);
	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;

	gebr_gui_gtk_tree_view_set_drag_source_dest(tree_view);

	if (!gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) &&
	    !gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) &&
	    !gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		return;

	gboolean sensitive;
	gtk_tree_model_get(GTK_TREE_MODEL(fe->fseq_store), &iter,
	                   FSEQ_SENSITIVE, &sensitive, -1);

	if (!sensitive)
		return;

	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor(tree_view, &path, &column);

	gint pos, wid;
	if(!gtk_tree_view_column_cell_get_position(column, fe->file_renderer, &pos, &wid))
		return;

	gchar *path_str = gtk_tree_path_to_string(path);

	gtk_tree_view_unset_rows_drag_source(tree_view);

	_open_activated(fe, path_str);

	g_free(path_str);
	gtk_tree_path_free(path);
}

static void populate_io_popup(GtkEntry *entry, GtkMenu *menu)
{
	GtkWidget *submenu;
	GtkWidget *item;

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);

	submenu = gebr_gui_parameter_add_variables_popup(entry,
							 GEBR_GEOXML_DOCUMENT (gebr.flow),
							 GEBR_GEOXML_DOCUMENT (gebr.line),
							 GEBR_GEOXML_DOCUMENT (gebr.project),
							 GEBR_GEOXML_PARAMETER_TYPE_STRING);
	item = gtk_menu_item_new_with_label(_("Insert Variable"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
}


static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path)
{
	GtkTreeIter iter;
	GtkTreeModel *completion_model;
	GtkEntry *entry = GTK_ENTRY(editable);
	const gchar *input;
	const gchar *output;
	const gchar *error;

	gtk_window_remove_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	g_object_set_data(G_OBJECT(entry), "path", g_strdup(path));
	g_object_set_data(G_OBJECT(entry), "renderer", renderer);

	if (!flow_edition_get_selected_component(&iter, TRUE))
		return;

	completion_model = gebr_gui_parameter_get_completion_model(GEBR_GEOXML_DOCUMENT (gebr.flow),
								   GEBR_GEOXML_DOCUMENT (gebr.line),
								   GEBR_GEOXML_DOCUMENT (gebr.project),
								   GEBR_GEOXML_PARAMETER_TYPE_FILE);
	gebr_gui_parameter_set_entry_completion(entry, completion_model, GEBR_GEOXML_PARAMETER_TYPE_FILE);

	input = gebr_geoxml_flow_io_get_input(gebr.flow);
	output = gebr_geoxml_flow_io_get_output(gebr.flow);
	error = gebr_geoxml_flow_io_get_error(gebr.flow);

	if ((gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) && (g_strcmp0(input, "") == 0))  ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) && (g_strcmp0(output, "") == 0)) ||
	    (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter) && (g_strcmp0(error, "") == 0)))
		gtk_entry_set_text(entry, "");

//	g_signal_connect(entry, "icon-release", G_CALLBACK(open_activated), NULL);
	g_signal_connect(entry, "populate-popup", G_CALLBACK(populate_io_popup), NULL);
}

static void flow_edition_component_editing_canceled(GtkCellRenderer *renderer, gpointer user_data)
{
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);
}


static void
flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text)
{
	GtkTreeIter iter;

	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, path);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *mount_point;

	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *tmp = gebr_relativise_path(new_text, mount_point, paths);
	gebr_pairstrfreev(paths);

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter))
		gebr_geoxml_flow_io_set_input(gebr.flow, tmp);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		gebr_geoxml_flow_io_set_output(gebr.flow, tmp);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		gebr_geoxml_flow_io_set_error(gebr.flow, tmp);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	flow_edition_set_io();

	gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);
}

gboolean flow_edition_component_key_pressed(GtkWidget *view, GdkEventKey *key)
{
	GList			* listiter;
	GList			* paths;
	GebrGeoXmlProgram	* program;
	GebrGeoXmlProgramStatus	  status;
	GtkTreeIter		  iter;
	GtkTreeModel		* model;
	GtkTreeSelection	* selection;
	gboolean		  has_disabled = FALSE;

	if (key->keyval != GDK_space)
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_edition->fseq_view));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	/* Removes the input/output/errors special iters from the paths list, and if there is no selection, return.
	 */
	GtkTreePath *in_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->input_iter);
	GtkTreePath *out_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->output_iter);
	GtkTreePath *error_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->error_iter);
	listiter = paths;
	while (listiter) {
		GList * remove_node = NULL;

		if (gtk_tree_path_compare (in_path, listiter->data) == 0 ||
		    gtk_tree_path_compare (out_path, listiter->data) == 0 ||
		    gtk_tree_path_compare (error_path, listiter->data) == 0)
			remove_node = listiter;

		if (remove_node) {
			listiter = listiter->next;
			paths = g_list_remove_link (paths, remove_node);
			gtk_tree_path_free (remove_node->data);
			g_list_free_1 (remove_node);
		} else
			listiter = listiter->next;
	}
	gtk_tree_path_free (in_path);
	gtk_tree_path_free (out_path);
	gtk_tree_path_free (error_path);

	if (!paths)
		return FALSE;

	listiter = paths;

	gtk_tree_model_get_iter (model, &iter, listiter->data);
	gtk_tree_model_get (model, &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &program,
			    -1);

	do {
		gboolean has_error;

		gtk_tree_model_get_iter (model, &iter, listiter->data);
		gtk_tree_model_get (model, &iter,
				    FSEQ_GEBR_GEOXML_POINTER, &program,
				    -1);

		status = gebr_geoxml_program_get_status (program);
		has_error = gebr_geoxml_program_get_error_id(program, NULL);

		if ((status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) ||
		    (status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED && !has_error))
		{
			has_disabled = TRUE;
			break;
		}

		listiter = listiter->next;
	} while (listiter);

	if (has_disabled) {
		status = GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED;
	} else {
		status = GEBR_GEOXML_PROGRAM_STATUS_DISABLED;
	}

	listiter = paths;
	while (listiter) {
		gtk_tree_model_get_iter(model, &iter, listiter->data);
		flow_edition_change_iter_status(status, &iter);
		listiter = listiter->next;
	}

	flow_program_check_sensitiveness();
	flow_edition_revalidate_programs();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	flow_edition_set_io();

	return TRUE;
}

void flow_edition_change_iter_status(GebrGeoXmlProgramStatus status, GtkTreeIter *iter)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	const gchar *icon;
	GtkTreePath *input_path, *output_path, *error_path;
	GebrGeoXmlProgram *program;
	gboolean has_error;

	model = GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store);
	input_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->input_iter);
	output_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->output_iter);
	error_path = gtk_tree_model_get_path (model, &gebr.ui_flow_edition->error_iter);

	path = gtk_tree_model_get_path (model, iter);
	if (gtk_tree_path_compare (path, input_path) == 0 ||
	    gtk_tree_path_compare (path, output_path) == 0 ||
	    gtk_tree_path_compare (path, error_path) == 0) {
		goto out;
	}

	gboolean never_opened;
	gtk_tree_model_get(model, iter, FSEQ_GEBR_GEOXML_POINTER, &program, FSEQ_NEVER_OPENED, &never_opened, -1);

	gboolean is_control = (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR);
	if (gebr_geoxml_program_get_status(program) == status) {
		// If program is control, it may invalidate other programs but not itself
		if (is_control)
			flow_edition_revalidate_programs();
		goto out;
	}

	if (never_opened) {
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, iter, FSEQ_NEVER_OPENED,
		                   FALSE, -1);
		validate_program_iter(iter, NULL);
	}
	has_error = gebr_geoxml_program_get_error_id(program, NULL);

	if (has_error && status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
		status = GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

	gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), status);
	icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, iter, FSEQ_ICON_COLUMN, icon, -1);

	if (is_control) {
		GebrGeoXmlSequence *parameter;

		if (status == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		} else {
			gebr_geoxml_flow_insert_iter_dict(gebr.flow);
			parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
			gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
		}
		flow_edition_revalidate_programs();
	}

out:
	gtk_tree_path_free (path);
	gtk_tree_path_free (input_path);
	gtk_tree_path_free (output_path);
	gtk_tree_path_free (error_path);
}

void flow_edition_status_changed(guint status)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		flow_edition_change_iter_status(status, &iter);
	}

	flow_program_check_sensitiveness();
	flow_edition_revalidate_programs();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

/**
 * \internal
 * Return TRUE if there is a selected menu and put it into _iter_
 * If _warn_unselected_ is TRUE then a error message is displayed if the FALSE is returned
 */
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 */
static gboolean
flow_edition_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, GebrFlowEdition *ui_flow_edition)
{
	GtkTreeModel *model;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramControl con;

	if (gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->error_iter))
		return FALSE;
	if (drop_position != GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->input_iter))
		return FALSE;
	if (drop_position == GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->output_iter))
		return FALSE;
	if (gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->error_iter))
		return FALSE;

	/* Check if the moving iter is a control program */
	model = gtk_tree_view_get_model (tree_view);

	if (!gtk_list_store_iter_is_valid(GTK_LIST_STORE(model), iter))
		return FALSE;

	gtk_tree_model_get (model, iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	con = gebr_geoxml_program_get_control (program);
	if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return FALSE;

	/* Check if the target iter is a control program */
	gtk_tree_model_get (model, position, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	con = gebr_geoxml_program_get_control (program);
	if (con != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return FALSE;

	return TRUE;
}

/**
 * \internal
 */
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, GebrFlowEdition *ui_flow_edition)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *position_program;

	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), position,
			   FSEQ_GEBR_GEOXML_POINTER, &position_program, -1);

	if (drop_position != GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_before(program, position_program);
		gtk_list_store_move_before(gebr.ui_flow_edition->fseq_store, iter, position);
	} else {
		gebr_geoxml_sequence_move_after(program, position_program);
		gtk_list_store_move_after(gebr.ui_flow_edition->fseq_store, iter, position);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	flow_program_check_sensitiveness();
	flow_edition_set_io();

	return FALSE;
}

/**
 * \internal
 * When a flow component (a program in the flow) is selected this funtions get the state of the program and set it on
 * Flow Component Menu.
 */
static void flow_edition_component_selected(void)
{
	gboolean has_error;
	GtkTreeIter iter;
	GtkAction * action;

	gebr.program = NULL;
	if (!flow_edition_get_selected_component(&iter, FALSE))
		return;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			   FSEQ_GEBR_GEOXML_POINTER, &gebr.program, -1);

	/* If the program has an error, disable the 'Configured' status */
	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	has_error = gebr_geoxml_program_get_error_id(gebr.program, NULL);
	gtk_action_set_sensitive(action, !has_error);

	action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_help");
	gchar *tmp_help_p = gebr_geoxml_program_get_help(gebr.program);
	gtk_action_set_sensitive(action, strlen(tmp_help_p) != 0);
	g_free(tmp_help_p);
}

/*
 * flow_edition_menu_add:
 * Add selected menu to flow sequence.
 */
static void flow_edition_menu_add(void)
{
	GtkTreeIter iter;
	gchar *name;
	gchar *filename;
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *menu_programs;
	gint menu_programs_index;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!flow_edition_get_selected_menu(&iter, FALSE)) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter);
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view), path, FALSE);
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_TITLE_COLUMN, &name, MENU_FILEPATH_COLUMN, &filename, -1);
	menu = menu_load_path(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	GebrGeoXmlProgramControl c1, c2;
	GebrGeoXmlProgram *first_prog;

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &first_prog, -1);

	gebr_geoxml_flow_get_program(menu, &program, 0);

	c1 = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));
	c2 = gebr_geoxml_program_get_control (first_prog);

	if (c1 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY && c2 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
		document_free(GEBR_GEOXML_DOC(menu));
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, _("This Flow already contains a loop"));
		goto out;
	}

	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		gebr_geoxml_parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	if (c1 == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		GebrGeoXmlParameter *dict_iter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
		gebr_validator_insert(gebr.validator, dict_iter, NULL, NULL);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	gebr_flow_set_toolbar_sensitive();
	flow_edition_set_run_widgets_sensitiveness(gebr.ui_flow_edition, TRUE, FALSE);
	gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE, TRUE);

	document_free(GEBR_GEOXML_DOC(menu));
 out:	g_free(name);
	g_free(filename);
}

/**
 * \internal
 * Show's menus help
 */
static void flow_edition_menu_show_help(void)
{
	GtkTreeIter iter;
	gchar *menu_filename;
	GebrGeoXmlFlow *menu;

	if (!flow_edition_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);

	menu = menu_load_path(menu_filename);
	if (menu == NULL)
		goto out;
	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);

out:	g_free(menu_filename);
}

static void on_output_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_output_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stdout", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-stdout", -1);
	}
	flow_edition_set_io();
}

static void on_error_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_error_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stderr", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-stderr", -1);
	}
	flow_edition_set_io();
}

/**
 * \internal
 * Show popup menu for flow component.
 */
static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget * menu;
	GtkWidget * menu_item;
	GtkAction * action;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramControl control;
	gboolean is_ordinary;
	gboolean can_move_up;
	gboolean can_move_down;

	if (!flow_edition_get_selected_component(&iter, FALSE))
		return NULL;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter))
	       return NULL;

	menu = gtk_menu_new();

	gboolean is_file = FALSE;
	gboolean is_append;
	const gchar *label;
	GCallback append_cb;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		is_file = TRUE;
		label = _("Overwrite existing output file");
		is_append = gebr_geoxml_flow_io_get_output_append (gebr.flow);
		append_cb = G_CALLBACK (on_output_append_toggled);
	} else
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter)) {
		is_file = TRUE;
		label = _("Overwrite existing log file");
		is_append = gebr_geoxml_flow_io_get_error_append (gebr.flow);
		append_cb = G_CALLBACK (on_error_append_toggled);
	}

	if (is_file) {
		menu_item = gtk_check_menu_item_new_with_label (label);
		g_signal_connect (menu_item, "toggled", append_cb, NULL);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), !is_append);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show(menu_item);
		return GTK_MENU (menu);
	}

	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &program,
			    -1);
	control = gebr_geoxml_program_get_control (program);

	is_ordinary = control == GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;
	can_move_up = gebr_gui_gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter);
	can_move_down = gebr_gui_gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter);

	/* Move top */
	if (is_ordinary && can_move_up) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_top), NULL);
	}
	/* Move bottom */
	if (is_ordinary && can_move_down) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_bottom), NULL);
	}
	/* separator */
	if (is_ordinary && (can_move_up || can_move_down))
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());


	/* status */
	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_configured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_disabled");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "flow_edition_status_unconfigured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	/* separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_help")));

 	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Show popup menu for menu list.
 */
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!flow_edition_get_selected_menu(&iter, FALSE)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_flow_edition->menu_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_flow_edition->menu_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_flow_edition->menu_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_flow_edition->menu_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_show_help), NULL);
	gchar *menu_filename;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
	GebrGeoXmlDocument *xml = GEBR_GEOXML_DOCUMENT(menu_load_path(menu_filename));
	gchar *tmp_help = gebr_geoxml_document_get_help(xml);
	if (xml != NULL && strlen(tmp_help) <= 1)
		gtk_widget_set_sensitive(menu_item, FALSE);
	if (xml)
		document_free(xml);
	g_free(menu_filename);
	g_free(tmp_help);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void
on_groups_combobox_changed(GtkComboBox *combobox,
			   GebrFlowEdition *fe)
{
	GtkTreeIter iter;
	GtkTreeIter flow_iter;

	if (!flow_browse_get_selected(&flow_iter, TRUE))
		return;

	if (!gtk_combo_box_get_active_iter(combobox, &iter))
		return;

	GtkTreeModel *model = gtk_combo_box_get_model(combobox);

	GebrMaestroServer *maestro =
		gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
							     gebr.line);

	if (!maestro)
		return;

	gchar *name;
	GebrMaestroServerGroupType type;
	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_TYPE, &type,
			   MAESTRO_SERVER_NAME, &name,
			   -1);

	if (fe->priv->name)
		g_free(fe->priv->name);
	fe->priv->name = name;
	fe->priv->type = type;

	flow_edition_set_io();
	flow_browse_info_update();
}

/**
 * \internal
 * Shows tooltips for each line in component flow tree view.
 */
static gboolean
on_flow_sequence_query_tooltip(GtkTreeView * treeview,
			       gint x,
			       gint y,
			       gboolean keyboard_tip,
			       GtkTooltip * tooltip,
			       GebrFlowEdition *ui_flow_edition)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GebrGeoXmlProgram *program;
	gchar *tooltip_text;

	if (!gtk_tree_view_get_tooltip_context(treeview, &x, &y, keyboard_tip, &model, NULL, &iter))
		return FALSE;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter)
	    || gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)
	    || gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter))
	{
		gtk_tree_model_get(model, &iter, FSEQ_TOOLTIP, &tooltip_text, -1);

		if (!tooltip_text)
			return FALSE;

		gtk_tooltip_set_text(tooltip, tooltip_text);
		g_free(tooltip_text);

		goto set_tooltip;
	}

	gboolean never_opened;
	gtk_tree_model_get(model, &iter,
	                   FSEQ_GEBR_GEOXML_POINTER, &program,
	                   FSEQ_NEVER_OPENED, &never_opened,
	                   FSEQ_TOOLTIP, &tooltip_text,
	                   -1);

	if (!program || gebr_geoxml_program_get_status(program) != GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED)
		return FALSE;

	GebrIExprError errorid;
	if (!gebr_geoxml_program_get_error_id(program, &errorid) && !never_opened)
		return FALSE;

	gchar *error_message;
	if (tooltip_text && *tooltip_text)
		error_message = tooltip_text;
	else
		error_message = _("Unknown error");

	switch (errorid) {
	case GEBR_IEXPR_ERROR_SYNTAX:
	case GEBR_IEXPR_ERROR_TOOBIG:
	case GEBR_IEXPR_ERROR_RUNTIME:
	case GEBR_IEXPR_ERROR_INVAL_TYPE:
	case GEBR_IEXPR_ERROR_TYPE_MISMATCH:
		error_message = _("This program has an invalid expression");
		break;
	case GEBR_IEXPR_ERROR_EMPTY_EXPR:
		error_message = _("A required parameter is unfilled");
		break;
	case GEBR_IEXPR_ERROR_UNDEF_VAR:
	case GEBR_IEXPR_ERROR_UNDEF_REFERENCE:	
		error_message = _("An undefined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_INVAL_VAR:
	case GEBR_IEXPR_ERROR_BAD_REFERENCE:
	case GEBR_IEXPR_ERROR_CYCLE:
		error_message = _("A badly defined variable is being used");
		break;
	case GEBR_IEXPR_ERROR_PATH:
		error_message = _("This program has cleaned their paths");
		break;
	case GEBR_IEXPR_ERROR_BAD_MOVE:
	case GEBR_IEXPR_ERROR_INITIALIZE:
		break;
	}

	gtk_tooltip_set_text(tooltip, error_message);
	g_free(tooltip_text);

set_tooltip:
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_set_tooltip_row(treeview, tooltip, path);
	gtk_tree_path_free(path);

	return TRUE;
}

/**
 * \internal
 * If the server was not connected, the combo box item stay insensitive
 */

static void
on_queue_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer         data)
{
	GebrJob *job;
	gchar *name_queue;

	gtk_tree_model_get(tree_model, iter, 0, &job, -1);

	if (!job)
		name_queue = g_strdup(_("Immediately"));
	else
		name_queue = g_strdup_printf(_("After %s"), gebr_job_get_title(job));

	g_object_set(cell, "text", name_queue, NULL);
	g_free(name_queue);
}

static void
on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
					   GtkCellRenderer *cell,
					   GtkTreeModel    *tree_model,
					   GtkTreeIter     *iter,
					   gpointer         data)
{
	gchar *name;
	GebrMaestroServerGroupType type;

	gtk_tree_model_get(tree_model, iter,
			   MAESTRO_SERVER_TYPE, &type,
			   MAESTRO_SERVER_NAME, &name,
			   -1);

	GebrDaemonServer *daemon = NULL;
	GebrMaestroServer *maestro;
	gboolean is_connected = TRUE;

	maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
							       gebr.line);

	if (!maestro) {
		if (GTK_IS_CELL_RENDERER_TEXT(cell))
			g_object_set(cell, "text", "", NULL);
		else
			g_object_set(cell, "stock-id", NULL, NULL);
		return;
	}

	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		daemon = gebr_maestro_server_get_daemon(maestro, name);
		if (!daemon)
			return;
		is_connected = gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED;
	}

	if (GTK_IS_CELL_RENDERER_TEXT(cell)) {
		const gchar *txt;

		if (type == MAESTRO_SERVER_TYPE_GROUP) {
			if (name && *name)
				txt = name;
			else
				txt = gebr_maestro_server_get_display_address(maestro);
		} else  {
			txt = gebr_daemon_server_get_hostname(daemon);
			if (!txt || !*txt)
				txt = gebr_daemon_server_get_address(daemon);
		}
		g_object_set(cell, "text", txt, NULL);
	} else {
		const gchar *stock_id;
		if (type == MAESTRO_SERVER_TYPE_GROUP)
			stock_id = GTK_STOCK_NETWORK;
		else {
			if (is_connected)
				stock_id = GTK_STOCK_CONNECT;
			else
				stock_id = GTK_STOCK_DISCONNECT;
		}

		g_object_set(cell, "stock-id", stock_id, NULL);
	}

	g_object_set(cell, "sensitive", is_connected, NULL);
}

static void on_queue_combobox_changed (GtkComboBox *combo, GebrFlowEdition *fe)
{
	gint index;
	GebrJob *job;
	GtkTreeIter iter;
	GtkTreeIter server_iter;
	GtkTreeModel *server_model;

	if (!flow_browse_get_selected (&iter, FALSE))
		return;

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX(fe->priv->server_combobox), &server_iter))
		return;

	server_model = gtk_combo_box_get_model (GTK_COMBO_BOX(fe->priv->server_combobox));

	gtk_tree_model_get (server_model, &server_iter,
			    0, &job,
			    -1);

	index = gtk_combo_box_get_active (combo);

	if (index < 0)
		index = 0;

	gtk_combo_box_set_active(combo, index);
}

static gboolean
flow_edition_find_by_group(GebrFlowEdition *fe,
			   GebrMaestroServerGroupType type,
			   const gchar *name,
			   GtkTreeIter *iter)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fe->priv->server_combobox));
	if (!model)
		return FALSE;

	gboolean valid = gtk_tree_model_get_iter_first(model, iter);

	while (valid)
	{
		GebrMaestroServerGroupType ttype;
		gchar *tname;

		gtk_tree_model_get(model, iter,
				   MAESTRO_SERVER_TYPE, &ttype,
				   MAESTRO_SERVER_NAME, &tname,
				   -1);

		if (g_strcmp0(tname, name) == 0 && ttype == type) {
			g_free(tname);
			return TRUE;
		}

		g_free(tname);
		valid = gtk_tree_model_iter_next(model, iter);
	}

	return gtk_tree_model_get_iter_first(model, iter);
}

static gboolean
flow_edition_find_flow_server(GebrFlowEdition *fe,
			      GebrGeoXmlFlow *flow,
			      GtkTreeModel   *model,
			      GtkTreeIter    *iter)
{
	if (!flow)
		return FALSE;

	gboolean ret;
	gchar *tmp, *name;
	GebrMaestroServerGroupType type;

	gebr_geoxml_flow_server_get_group(flow, &tmp, &name);
	type = gebr_maestro_server_group_str_to_enum(tmp);
	ret = flow_edition_find_by_group(fe, type, name, iter);

	g_free(tmp);
	g_free(name);

	return ret;
}

void
flow_edition_revalidate_programs(void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GebrGeoXmlProgram *program;

	model = GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);

		if (!program)
			continue;

		if (gebr_geoxml_program_get_status(program) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
			continue;

		if(gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			if (validate_program_iter(&iter, NULL))
				gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
			else
				gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
			const gchar *icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
			gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter, FSEQ_ICON_COLUMN, icon, -1);
			continue;
		}

		if (validate_program_iter(&iter, NULL))
			flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, &iter);
		else
			flow_edition_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, &iter);
	}

	gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);
}

void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program,
				       gboolean select_last,
				       gboolean never_opened)
{
	const gchar *icon;
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;
	gboolean has_control;
	GebrGeoXmlProgram *first_prog;

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &first_prog, -1);

	control = gebr_geoxml_program_get_control (first_prog);
	has_control = control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;

	// Reference this program so _sequence_next don't destroy it
	gebr_geoxml_object_ref(program);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

		if (!has_control && control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
			gtk_list_store_insert_after (gebr.ui_flow_edition->fseq_store, &iter, NULL);
			has_control = TRUE;
		} else
			gtk_list_store_insert_before(gebr.ui_flow_edition->fseq_store,
						     &iter, &gebr.ui_flow_edition->output_iter);

		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program));
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
				   FSEQ_TITLE_COLUMN, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)),
				   FSEQ_ICON_COLUMN, icon,
				   FSEQ_TOOLTIP, _("This program needs to be configure"),
				   FSEQ_GEBR_GEOXML_POINTER, program,
				   FSEQ_ELLIPSIZE, PANGO_ELLIPSIZE_NONE,
				   FSEQ_EDITABLE, FALSE,
				   FSEQ_SENSITIVE, TRUE,
				   FSEQ_NEVER_OPENED, never_opened,
				   -1);
		gebr_geoxml_object_ref(program);

		GebrIExprError undef;
		gebr_geoxml_program_get_error_id(GEBR_GEOXML_PROGRAM(program), &undef);
		if (never_opened || undef == GEBR_IEXPR_ERROR_PATH)
			continue;
		gebr_geoxml_program_is_valid(GEBR_GEOXML_PROGRAM(program), gebr.validator, NULL);
	}

	if (select_last)
		flow_edition_select_component_iter(&iter);
}

void flow_program_check_sensitiveness (void)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlProgram *first_program = NULL;
	GebrGeoXmlProgram *last_program = NULL;
	gboolean has_some_error_output = FALSE;
	gboolean has_configured = FALSE;

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
			   FSEQ_EDITABLE, FALSE,
			   FSEQ_SENSITIVE, FALSE, -1);
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
			   FSEQ_EDITABLE, FALSE,
			   FSEQ_SENSITIVE, FALSE, -1);
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter,
			   FSEQ_EDITABLE, FALSE,
			   FSEQ_SENSITIVE, FALSE, -1);

	gebr_geoxml_flow_get_program(gebr.flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {

		GebrGeoXmlProgramControl control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

		if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
			continue;

		if (gebr_geoxml_program_get_status (GEBR_GEOXML_PROGRAM(program)) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			if (!has_configured) {
				first_program = GEBR_GEOXML_PROGRAM(program);
				gebr_geoxml_object_ref(first_program);
				has_configured = TRUE;
			}
			if (!has_some_error_output && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))){
				has_some_error_output = TRUE;
			}

			if (last_program)
				gebr_geoxml_object_unref(last_program);

			last_program = GEBR_GEOXML_PROGRAM(program);
			gebr_geoxml_object_ref(last_program);
		}
	}

	if (has_configured) {
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
		gebr_geoxml_object_unref(first_program);
		gebr_geoxml_object_unref(last_program);
	}
	gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);
}

static void
restore_last_selection(GebrFlowEdition *fe)
{
	GtkTreeIter iter;

	if (flow_edition_find_by_group(fe, fe->priv->type,
				       fe->priv->name, &iter))
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fe->priv->server_combobox), &iter);
}

static void
on_group_changed(GebrMaestroController *mc,
		 GebrMaestroServer *maestro,
		 GebrFlowEdition *fe)
{
	GtkTreeIter iter;
	GtkComboBox *cb = GTK_COMBO_BOX(fe->priv->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(cb);

	if (!gebr.line)
		return;

	if (fe->priv->name) {
		restore_last_selection(fe);
		return;
	}

	if (gebr_maestro_controller_get_maestro_for_line(mc, gebr.line)) {
		if (flow_edition_find_flow_server(fe, gebr.flow, model, &iter)) {
			gchar *name;
			gtk_tree_model_get(model, &iter, MAESTRO_SERVER_NAME, &name, -1);
			gtk_combo_box_set_active_iter(cb, &iter);
		}
		gebr_flow_edition_select_queue(fe);
	}
}

static void
on_daemons_changed(GebrMaestroController *mc,
                   GebrFlowEdition *fe)
{
	gebr_flow_edition_select_group_for_flow(fe, gebr.flow);
}

static void
on_controller_maestro_state_changed(GebrMaestroController *mc,
				    GebrMaestroServer *maestro,
				    GebrFlowEdition *fe)
{
	if (!gebr.line)
		return;

	gchar *addr1 = gebr_geoxml_line_get_maestro(gebr.line);
	const gchar *addr2 = gebr_maestro_server_get_address(maestro);

	if (g_strcmp0(addr1, addr2) != 0)
		goto out;

	switch (gebr_maestro_server_get_state(maestro)) {
	case SERVER_STATE_DISCONNECTED:
		flow_edition_set_run_widgets_sensitiveness(fe, FALSE, TRUE);
		break;
	case SERVER_STATE_LOGGED:
		gebr_flow_edition_update_server(fe, maestro);
		break;
	default:
		break;
	}

out:
	g_free(addr1);
}

void
gebr_flow_edition_show(GebrFlowEdition *fe)
{
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	gebr_flow_edition_update_speed_slider_sensitiveness(fe);

	if (gebr.config.niceness == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fe->nice_button_high), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fe->nice_button_low), TRUE);
}

void
gebr_flow_edition_hide(GebrFlowEdition *self)
{
}

void
gebr_flow_edition_select_queue(GebrFlowEdition *self)
{
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(self->priv->queue_combobox)) == -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(self->priv->queue_combobox), 0);

	if (gebr.config.niceness == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_high), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_low), TRUE);
}

void
gebr_flow_edition_update_server(GebrFlowEdition *fe,
				GebrMaestroServer *maestro)
{
	gboolean sensitive = maestro != NULL;

	if (maestro) {
		const gchar *type, *msg;

		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (g_strcmp0(type, "error:none") == 0) {
			/* Set groups/servers model for Maestro */
			GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);
			gtk_combo_box_set_model(GTK_COMBO_BOX(fe->priv->server_combobox), model);

			/* Set queues model for Maestro */
			GtkTreeModel *queue_model = gebr_maestro_server_get_queues_model(maestro);
			gtk_combo_box_set_model(GTK_COMBO_BOX(fe->priv->queue_combobox), queue_model);

			if (gebr.flow)
				gebr_flow_edition_select_group_for_flow(fe, gebr.flow);
		}
		else
			sensitive = FALSE;
	}
	if (gebr_geoxml_line_get_flows_number(gebr.line) > 0)
		flow_edition_set_run_widgets_sensitiveness(fe, sensitive, TRUE);
	else
		flow_edition_set_run_widgets_sensitiveness(fe, FALSE, FALSE);
}

const gchar *
gebr_flow_edition_get_selected_queue(GebrFlowEdition *fe)
{
	GtkTreeIter iter;
	GtkComboBox *combo = GTK_COMBO_BOX(fe->priv->queue_combobox);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return "";
	else {
		GebrJob *job;
		GtkTreeModel *model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get(model, &iter, 0, &job, -1);
		return job ? gebr_job_get_id(job) : "";
	}
}

const gchar *
gebr_flow_edition_get_selected_server(GebrFlowEdition *fe)
{
	GtkTreeIter iter;
	GtkComboBox *combo = GTK_COMBO_BOX(fe->priv->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(combo);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return "";

	GebrDaemonServer *daemon;
	gtk_tree_model_get(model, &iter, 0, &daemon, -1);
	return gebr_daemon_server_get_address(daemon);
}

void 
gebr_flow_edition_get_current_group(GebrFlowEdition *fe,
				    GebrMaestroServerGroupType *type,
				    gchar **name)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fe->priv->server_combobox));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fe->priv->server_combobox), &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_TYPE, type,
			   MAESTRO_SERVER_NAME, name,
			   -1);
}

void
gebr_flow_edition_get_server_hostname(GebrFlowEdition *fe,
                                      gchar **host)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fe->priv->server_combobox));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fe->priv->server_combobox), &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_HOST, host,
			   -1);
}

void
gebr_flow_edition_select_group_for_flow(GebrFlowEdition *fe,
					GebrGeoXmlFlow *flow)
{
	GtkTreeIter iter;
	GtkComboBox *cb = GTK_COMBO_BOX(fe->priv->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(cb);

	if (model) {
		if (flow_edition_find_flow_server(fe, flow, model, &iter))
			gtk_combo_box_set_active_iter(cb, &iter);
		gebr_flow_edition_select_queue(fe);
	}
}

gchar *
gebr_maestro_server_translate_error(const gchar *error_type,
                                    const gchar *error_msg)
{
	gchar *message = NULL;

	if (g_strcmp0(error_type, "error:protocol") == 0)
		message = g_strdup_printf(_("Maestro protocol version mismatch: %s"), error_msg);
	else if (g_strcmp0(error_type, "error:ssh") == 0)
		message = g_strdup(error_msg);

	return message;
}
