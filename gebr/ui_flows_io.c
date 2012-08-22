/*
 * ui_flows_io.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Team
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

#include <config.h>
#include "ui_flows_io.h"

#include <libgebr/utils.h>
#include <libgebr/gebr-version.h>
#include <gebr/ui_flows_io.h>
#include <libgebr/comm/gebr-comm.h>

struct _GebrUiFlowsIoPriv {
	gchar *id;
	GebrFlowsIoType type;
	gchar *value;
	gboolean overwrite;
	gboolean active;
};


/*
 * Global variables to implement GebrUiFlowsIoSingleton methods.
 */
static GebrUiFlowsIoFactory 	__factory = NULL;
static gpointer           	__data = NULL;
static GebrUiFlowsIo           *__io = NULL;

/*
 * Prototypes
 */

G_DEFINE_TYPE(GebrUiFlowsIo, gebr_ui_flows_io, G_TYPE_OBJECT);

/*
 * Static Functions
 */

static void
gebr_ui_flows_io_finalize(GObject *object)
{
	GebrUiFlowsIo *io = GEBR_UI_FLOWS_IO(object);
	/*
	 * Frees
	 */
	g_free(io->priv->id);
	g_free(io->priv->value);
	G_OBJECT_CLASS(gebr_ui_flows_io_parent_class)->finalize(object);
}

GebrUiFlowsIo *
gebr_ui_flows_io_new(void)
{
	return g_object_new(GEBR_TYPE_UI_FLOWS_IO, NULL);
}

static void
gebr_ui_flows_io_class_init(GebrUiFlowsIoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebr_ui_flows_io_finalize;
	g_type_class_add_private(klass, sizeof(GebrUiFlowsIoPriv));
}


static void
gebr_ui_flows_io_init(GebrUiFlowsIo *io)
{
	io->priv = G_TYPE_INSTANCE_GET_PRIVATE(io,
					       GEBR_TYPE_UI_FLOWS_IO,
					       GebrUiFlowsIoPriv);
	io->priv->id = NULL;
	io->priv->type = GEBR_IO_TYPE_NONE;
	io->priv->overwrite = FALSE;
	io->priv->value = NULL;
	io->priv->active = FALSE;
}

void
gebr_ui_flows_io_singleton_set_factory(GebrUiFlowsIoFactory fac,
				       gpointer data)
{
	__factory = fac;
	__data = data;
}

GebrUiFlowsIo *
gebr_ui_flows_io_singleton_get(void)
{
	if (__factory)
		return (*__factory)(__data);

	if (!__io)
		__io = gebr_ui_flows_io_new();

	return __io;
}


/*
 * Public Functions
 */

void
gebr_ui_flows_io_set_id(GebrUiFlowsIo *io, const gchar *id)
{
	io->priv->id = g_strdup(id);
}

const gchar*
gebr_ui_flows_io_get_id(GebrUiFlowsIo *io)
{
	return io->priv->id;
}

void
gebr_ui_flows_io_set_io_type(GebrUiFlowsIo *io, GebrFlowsIoType type)
{
	io->priv->type = type;
}

const gchar*
gebr_ui_flows_io_get_value(GebrUiFlowsIo *io)
{
	return io->priv->value;
}

void
gebr_ui_flows_io_set_value(GebrUiFlowsIo *io, const gchar *value)
{
	io->priv->value = g_strdup(value);
}

GebrFlowsIoType
gebr_ui_flows_io_get_io_type(GebrUiFlowsIo *io)
{
	return io->priv->type;
}

void
gebr_ui_flows_io_set_overwrite(GebrUiFlowsIo *io, gboolean overwrite)
{
	io->priv->overwrite = overwrite;
}

gboolean
gebr_ui_flows_io_get_overwrite(GebrUiFlowsIo *io)
{
	return io->priv->overwrite;
}

void
gebr_ui_flows_io_set_active(GebrUiFlowsIo *io, gboolean active)
{
	io->priv->active = active;
}

gboolean
gebr_ui_flows_io_get_active(GebrUiFlowsIo *io)
{
	return io->priv->active;
}

gboolean
gebr_ui_flows_io_set_value_from_flow(GebrUiFlowsIo *io, GebrGeoXmlDocument *flow)
{
	gchar *value = NULL;
	GebrFlowsIoType type = gebr_ui_flows_io_get_io_type(io);

	switch(type) {
	case GEBR_IO_TYPE_INPUT:
		value = gebr_geoxml_flow_io_get_input(GEBR_GEOXML_FLOW(flow));
		break;
	case GEBR_IO_TYPE_OUTPUT:
		value = gebr_geoxml_flow_io_get_output(GEBR_GEOXML_FLOW(flow));
		break;
	case GEBR_IO_TYPE_ERROR:
		value = gebr_geoxml_flow_io_get_error(GEBR_GEOXML_FLOW(flow));
		break;
	case GEBR_IO_TYPE_NONE:
		break;
	}

	if (value)
		return TRUE;
	else
		return FALSE;
}

#if 0
void
ui_flows_on_io_entry_editing_started(GtkCellRenderer *renderer,
				      GtkCellEditable *editable,
				      gchar *path,
				      GtkAccelGroup *accel_group,
				      GebrGeoxmlDocument *project,
				      GebrGeoxmlDocument *line,
				      GebrGeoxmlDocument *flow
				      )
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

	if (!ui_flows_io_get_selected_component(&iter, TRUE))
		return;

	completion_model = gebr_gui_parameter_get_completion_model(flow, line, project,
								   GEBR_GEOXML_PARAMETER_TYPE_FILE);

	gebr_gui_parameter_set_entry_completion(entry, completion_model, GEBR_GEOXML_PARAMETER_TYPE_FILE);

	input = gebr_geoxml_flow_io_get_input(gebr.flow);
	output = gebr_geoxml_flow_io_get_output(gebr.flow);
	error = gebr_geoxml_flow_io_get_error(gebr.flow);

	g_signal_connect(entry, "populate-popup", G_CALLBACK(populate_io_popup), NULL);
}

void
populate_io_popup(GtkEntry *entry, GtkMenu *menu)
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

void on_output_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_output_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stdout", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->output_iter,
				   FSEQ_ICON_COLUMN, "gebr-stdout", -1);
	}
	ui_flows_io_set_io();
}

void on_error_append_toggled (GtkCheckMenuItem *item)
{
	gboolean is_append = !gtk_check_menu_item_get_active(item);

	gebr_geoxml_flow_io_set_error_append (gebr.flow, is_append);
	if (is_append == TRUE)
	{
		gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-append-stderr", -1);
	}
	else
	{
		gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->error_iter,
				   FSEQ_ICON_COLUMN, "gebr-stderr", -1);
	}
	ui_flows_io_set_io();
}

void ui_flows_io_set_io(void)
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
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_ui_flows_io->fseq_view));

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
	gtk_tree_model_get(model, &gebr.ui_ui_flows_io->input_iter, FSEQ_SENSITIVE, &sensitivity, -1);
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

	gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->input_iter,
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
	gtk_tree_model_get(model, &gebr.ui_ui_flows_io->output_iter, FSEQ_SENSITIVE, &sensitivity, -1);
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

	gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->output_iter,
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
	gtk_tree_model_get(model, &gebr.ui_ui_flows_io->error_iter, FSEQ_SENSITIVE, &sensitivity, -1);
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

	gtk_list_store_set(gebr.ui_ui_flows_io->fseq_store, &gebr.ui_ui_flows_io->error_iter,
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

GtkMenu *ui_flows_io_popup_menu(GtkWidget * widget, GebrFlowEdition *ui_ui_flows_io)
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

	if (!ui_flows_io_get_selected_component(&iter, FALSE))
		return NULL;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->input_iter))
	       return NULL;

	menu = gtk_menu_new();

	gboolean is_file = FALSE;
	gboolean is_append;
	const gchar *label;
	GCallback append_cb;

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->output_iter)) {
		is_file = TRUE;
		label = _("Overwrite existing output file");
		is_append = gebr_geoxml_flow_io_get_output_append (gebr.flow);
		append_cb = G_CALLBACK (on_output_append_toggled);
	} else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->error_iter)) {
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

	gtk_tree_model_get (GTK_TREE_MODEL (gebr.ui_ui_flows_io->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &program,
			    -1);
	control = gebr_geoxml_program_get_control (program);

	is_ordinary = control == GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;
	can_move_up = gebr_gui_gtk_list_store_can_move_up(ui_ui_flows_io->fseq_store, &iter);
	can_move_down = gebr_gui_gtk_list_store_can_move_down(ui_ui_flows_io->fseq_store, &iter);

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
	action = gtk_action_group_get_action(gebr.action_group_status, "ui_flows_io_status_configured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "ui_flows_io_status_disabled");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	action = gtk_action_group_get_action(gebr.action_group_status, "ui_flows_io_status_unconfigured");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_action_create_menu_item(action));

	/* separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_ui_flows_io, "ui_flows_io_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_ui_flows_io, "ui_flows_io_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_ui_flows_io, "ui_flows_io_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_ui_flows_io, "ui_flows_io_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_ui_flows_io, "ui_flows_io_help")));

 	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

void
ui_flows_on_io_editing_concluded(gchar *path, gchar *new_text)
{
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_ui_flows_io->fseq_store), &iter, path);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *mount_point;

	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *tmp = gebr_relativise_path(new_text, mount_point, paths);
	gebr_pairstrfreev(paths);

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->input_iter))
		gebr_geoxml_flow_io_set_input(gebr.flow, tmp);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->output_iter))
		gebr_geoxml_flow_io_set_output(gebr.flow, tmp);
	else if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_ui_flows_io->error_iter))
		gebr_geoxml_flow_io_set_error(gebr.flow, tmp);

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	ui_flows_io_set_io();

	gebr_ui_flows_io_update_speed_slider_sensitiveness(gebr.ui_ui_flows_io);
	gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);
}

void
ui_flows_io_editing_canceled(GtkCellRenderer *renderer,
			     gpointer user_data,
			     GtkWidget window,
			     GtkAccelGroup *accel_group)
{
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
}

#endif
