/*
 * gebr-flow-edition.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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
#include "ui_flow_program.c"

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
#include "ui_flow_execution.h"
#include "ui_parameters.h"
#include "callbacks.h"
#include "ui_document.h"
#include "gebr-maestro-server.h"
#include "ui_flows_io.h"

/*
 * Prototypes
 */

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

static void flow_edition_set_edited_io(gchar *path, gchar *new_text);
static void flow_edition_component_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path);
static void flow_edition_component_editing_canceled(GtkCellRenderer *renderer, gpointer user_data);
static void flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text);
static void flow_edition_component_selected(void);
static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_flow_edition);

static gboolean on_flow_sequence_query_tooltip(GtkTreeView * treeview,
					       gint x,
					       gint y,
					       gboolean keyboard_tip,
					       GtkTooltip * tooltip,
					       GebrFlowEdition *ui_flow_edition);


GebrFlowEdition *
flow_edition_setup_ui(void)
{
	GebrFlowEdition *fe;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* alloc */
	fe = g_new(GebrFlowEdition, 1);

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

	/* Space key pressed on flow component changes its configured status */
	g_signal_connect(fe->fseq_view, "key-press-event",
			 G_CALLBACK(flow_edition_component_key_pressed), fe);

	/* Double click on flow component open its parameter window */
	g_signal_connect(fe->fseq_view, "row-activated",
			 G_CALLBACK(flow_edition_component_activated), fe);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(fe->fseq_view)), "changed",
			 G_CALLBACK(flow_edition_component_selected), fe);

	return fe;
}

void flow_edition_load_components(void)
{
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	if (!flow_browse_get_selected(NULL, FALSE))
		return;

	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->error_iter);

	/* now into GUI */
//	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);
//	flow_add_program_sequence_to_view(first_program, FALSE, FALSE);

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
flow_edition_set_edited_io(gchar *path, gchar *new_text)
{
	GtkTreeIter iter;

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
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);
}

static void
flow_edition_component_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text)
{
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);

	flow_edition_set_edited_io(path, new_text);

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
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);

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
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);

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
	can_move_up = gebr_gui_gtk_list_store_can_move_up(GTK_TREE_STORE(ui_flow_edition->fseq_store), &iter);
	can_move_down = gebr_gui_gtk_list_store_can_move_down(GTK_TREE_STORE(ui_flow_edition->fseq_store), &iter);

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

const gchar *
gebr_flow_get_error_tooltip_from_id(GebrIExprError errorid)
{
	const gchar *error_message;

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
	default:
		error_message = "";
		break;
	}

	return error_message;
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

	GebrUiFlowProgram *ui_program = gebr_ui_flow_program_new(program);
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

	const gchar *error_message;
	if (tooltip_text && *tooltip_text)
		error_message = tooltip_text;
	else
		error_message = _("Unknown error");

	const gchar *err_msg = gebr_ui_flow_program_get_error_tooltip(ui_program);
	if (*err_msg)
		error_message = err_msg;

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
	GtkTreeIter iter, parent;
	GebrGeoXmlProgramControl control;
	gboolean has_control;
	GebrGeoXmlProgram *first_prog = NULL;

	GtkTreeModel *model = GTK_TREE_MODEL (gebr.ui_flow_browse->store);

	gboolean valid = gtk_tree_model_get_iter_first (model, &parent);
	while (valid) {
		valid = gtk_tree_model_iter_children(model, &iter, &parent);
		if (valid)
			break;
		valid = gtk_tree_model_iter_next(model, &parent);
	}


	GtkTreeIter output_iter;
	GebrUiFlowBrowseType type;
	GebrUiFlowProgram *ui_program;
	GebrUiFlowsIo *ui_io;
	while (valid) {
		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT_TYPE, &type,
		                   -1);

		if (type == STRUCT_TYPE_IO) {
			gtk_tree_model_get(model, &iter,
			                   FB_STRUCT, &ui_io,
			                   -1);

			if (gebr_ui_flows_io_get_io_type(ui_io) == GEBR_IO_TYPE_OUTPUT) {
				output_iter = iter;
				break;
			}
		}

		if (type != STRUCT_TYPE_PROGRAM || first_prog) {
			valid = gtk_tree_model_iter_next(model, &iter);
			continue;
		}

		gtk_tree_model_get(model, &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);

		if (!first_prog)
			first_prog = gebr_ui_flow_program_get_xml(ui_program);

		valid = gtk_tree_model_iter_next(model, &iter);
	}

	control = gebr_geoxml_program_get_control (first_prog);
	has_control = control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;

	// Reference this program so _sequence_next don't destroy it
	gebr_geoxml_object_ref(program);
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
		control = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));

		if (!has_control && control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
			gtk_tree_store_prepend(gebr.ui_flow_browse->store, &iter, &parent);
			has_control = TRUE;
		} else
			gtk_tree_store_insert_before(gebr.ui_flow_browse->store, &iter, &parent, &output_iter);

		gebr_geoxml_object_ref(program);
		GebrUiFlowProgram *ui_program = gebr_ui_flow_program_new(GEBR_GEOXML_PROGRAM(program));
		gebr_ui_flow_program_set_flag_opened(ui_program, never_opened);
		gtk_tree_store_set(gebr.ui_flow_browse->store, &iter,
		                   FB_STRUCT_TYPE, STRUCT_TYPE_PROGRAM,
		                   FB_STRUCT, ui_program,
				   -1);

		GebrIExprError undef;
		gebr_geoxml_program_get_error_id(GEBR_GEOXML_PROGRAM(program), &undef);
		if (never_opened || undef == GEBR_IEXPR_ERROR_PATH)
			continue;
		gebr_geoxml_program_is_valid(GEBR_GEOXML_PROGRAM(program), gebr.validator, NULL);
	}

	if (select_last)
		flow_browse_select_iter(&iter);
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

void
gebr_flow_edition_get_iter_for_program(GebrGeoXmlProgram *prog,
                                       GtkTreeIter *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store);

	gboolean valid = gtk_tree_model_get_iter_first(model, iter);
	while (valid) {
		GebrGeoXmlProgram *program;

		gtk_tree_model_get(model, iter,
		                   FSEQ_GEBR_GEOXML_POINTER, &program, -1);

		if (prog == program)
			return;

		valid = gtk_tree_model_iter_next(model, iter);
	}
	iter = NULL;
}

GtkWidget *
gebr_flow_edition_get_programs_view(GebrFlowEdition *fe)
{
	return fe->fseq_view;
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
