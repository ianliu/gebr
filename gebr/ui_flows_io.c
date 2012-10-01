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
#include "gebr.h"
#include "document.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgebr/comm/gebr-comm.h>
#include <libgebr/gebr-version.h>
#include <libgebr/gebr-validator.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-maestro-info.h>

struct _GebrUiFlowsIoPriv {
	gchar *id;
	GebrUiFlowsIoType type;
	gchar *value;
	gboolean overwrite;
	gboolean active;
	gchar *tooltip;
	gchar *stock_id;
};

typedef struct {
	GebrGeoXmlDocument *doc;
	GebrUiFlowsIo *io;
	GtkTreeIter *iter;
} GebrUiFlowsDai;

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
	g_free(io->priv->tooltip);
	g_free(io->priv->stock_id);
	G_OBJECT_CLASS(gebr_ui_flows_io_parent_class)->finalize(object);
}

GebrUiFlowsIo *
gebr_ui_flows_io_new(GebrUiFlowsIoType type)
{
	GebrUiFlowsIo *io = g_object_new(GEBR_TYPE_UI_FLOWS_IO, NULL);
	io->priv->type = type;

	return io;
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
	io->priv->overwrite = FALSE;
	io->priv->value = NULL;
	io->priv->active = FALSE;
	io->priv->tooltip = NULL;
	io->priv->stock_id = NULL;
}

/*
 * Public Functions
 */

void
gebr_ui_flows_io_set_id(GebrUiFlowsIo *io,
                        const gchar *id)
{
	io->priv->id = g_strdup(id);
}

const gchar*
gebr_ui_flows_io_get_id(GebrUiFlowsIo *io)
{
	return io->priv->id;
}

void
gebr_ui_flows_io_set_io_type(GebrUiFlowsIo *io,
                             GebrUiFlowsIoType type)
{
	io->priv->type = type;
}

const gchar*
gebr_ui_flows_io_get_value(GebrUiFlowsIo *io)
{
	return io->priv->value;
}

void
gebr_ui_flows_io_set_value(GebrUiFlowsIo *io,
                           const gchar *value)
{
	io->priv->value = g_strdup(value);
}

GebrUiFlowsIoType
gebr_ui_flows_io_get_io_type(GebrUiFlowsIo *io)
{
	return io->priv->type;
}

void
gebr_ui_flows_io_set_overwrite(GebrUiFlowsIo *io,
                               gboolean overwrite)
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

void gebr_ui_flows_io_update_active(GebrUiFlowsIo *io,
				    GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlProgram *first_program = NULL;
	GebrGeoXmlProgram *last_program = NULL;
	gboolean has_some_error_output = FALSE;
	gboolean has_configured = FALSE;

	gebr_geoxml_flow_get_program(flow, &program, 0);
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
		io->priv->active = TRUE;
		gebr_geoxml_object_unref(first_program);
		gebr_geoxml_object_unref(last_program);
	}
}

void
gebr_ui_flows_io_set_tooltip(GebrUiFlowsIo *io, const gchar *tooltip)
{
	io->priv->tooltip = g_strdup(tooltip);
}

void
gebr_ui_flows_io_update_tooltip(GebrUiFlowsIo *io,
				const gchar *path,
				const gchar *evaluated,
				GError *err,
				const gchar *err_msg)
{
	gboolean active = io->priv->active;
	gchar *tooltip = NULL;

	if (!*path || !active) {
		tooltip = NULL;
	} else if (err) {
		tooltip = g_strdup(err->message);
	} else if (err_msg) {
		tooltip = g_strdup(err_msg);
	} else {
		switch(io->priv->type) {
		case GEBR_IO_TYPE_INPUT:
			tooltip = g_strdup_printf(_("Input file \"%s\""), evaluated);
			break;
		case GEBR_IO_TYPE_OUTPUT:
			tooltip = g_strdup_printf(_("%s output file \"%s\""),
						  io->priv->overwrite ? _("Append to") : _("Overwrite"), evaluated);
			break;
		case GEBR_IO_TYPE_ERROR:
			tooltip = g_strdup_printf(_("%s log file \"%s\""),
						  io->priv->overwrite ? _("Append to") : _("Overwrite"), evaluated);
			break;
		case GEBR_IO_TYPE_NONE:
			break;
		}
	}
	io->priv->tooltip = g_strdup(tooltip);
}


const gchar *
gebr_ui_flows_io_get_tooltip(GebrUiFlowsIo *io)
{
	return io->priv->tooltip;
}

void
gebr_ui_flows_io_set_stock_id(GebrUiFlowsIo *io, const gchar *stock_id)
{
	io->priv->stock_id = g_strdup(stock_id);
}

const gchar*
gebr_ui_flows_io_get_stock_id(GebrUiFlowsIo *io)
{
	return io->priv->stock_id;
}

void
gebr_ui_flows_io_load_from_xml(GebrUiFlowsIo *io,
			       GebrGeoXmlLine *line,
			       GebrGeoXmlFlow *flow,
			       GebrMaestroServer *maestro,
			       GebrValidator *validator)
{
	GError *err = NULL;
	gchar *err_msg = NULL, *result = NULL, *tmp = NULL, *mount_point = NULL, *path = NULL, *path_real = NULL, *resolved_path = NULL;
	const gchar *aux = NULL;

	gchar ***paths = gebr_geoxml_line_get_paths(line);

	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;

	gebr_ui_flows_io_update_active(io, flow);
	// Get the path
	switch(io->priv->type) {
	case GEBR_IO_TYPE_INPUT:
		aux = gebr_geoxml_flow_io_get_input(flow);
		break;
	case GEBR_IO_TYPE_OUTPUT:
		aux = gebr_geoxml_flow_io_get_output(flow);
		break;
	case GEBR_IO_TYPE_ERROR:
		aux = gebr_geoxml_flow_io_get_error(flow);
		break;
	case GEBR_IO_TYPE_NONE:
		break;
	}

	path_real = gebr_relativise_path(aux, mount_point, paths);
	path = g_markup_escape_text(path_real, -1);

	g_return_if_fail(path);

	// Validation
	resolved_path = gebr_resolve_relative_path(path_real, paths);
	gebr_validator_evaluate(validator, resolved_path, GEBR_GEOXML_PARAMETER_TYPE_STRING,
				GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);

	gebr_validator_evaluate_interval(validator, path_real,
					 GEBR_GEOXML_PARAMETER_TYPE_STRING,
					 GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
					 FALSE, &tmp, NULL);

	gebr_validate_path(tmp, paths, &err_msg);

	gebr_ui_flows_io_update_tooltip(io, path, result, err, err_msg);
	gebr_ui_flows_io_update_stock_id(io, path, result, err, err_msg);

	if (*path && io->priv->active)
		io->priv->value = g_strdup(path);
	else
		io->priv->value = g_strdup("");

	g_free(tmp);
	g_free(mount_point);
	g_free(path);
	g_free(path_real);
	g_free(resolved_path);
}

const gchar *
gebr_ui_flows_io_get_label_markup(GebrUiFlowsIo *io)
{
	if (!io)
		return NULL;

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(io);
	switch (type) {
	case GEBR_IO_TYPE_INPUT:
		return _("<i>Input file</i>");
	case GEBR_IO_TYPE_OUTPUT:
		return _("<i>Output file</i>");
	case GEBR_IO_TYPE_ERROR:
		return _("<i>Log file</i>");
	default:
		return NULL;
	}
}

const gchar *
gebr_ui_flows_io_get_icon_str(GebrUiFlowsIo *io)
{
	if (!io)
		return NULL;

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(io);

	switch (type) {
	case GEBR_IO_TYPE_INPUT:
		return "gebr-stdin";
	case GEBR_IO_TYPE_OUTPUT:
		return io->priv->overwrite ? "gebr-stdout" : "gebr-append-stdout";
	case GEBR_IO_TYPE_ERROR:
		return io->priv->overwrite ? "gebr-stderr" : "gebr-append-stderr";
	default:
		return NULL;
	}
}

void
gebr_ui_flows_io_update_stock_id(GebrUiFlowsIo *io,
				 const gchar *path,
				 const gchar *evaluated,
				 GError *error_1,
				 const gchar *err_msg)
{
	const gchar *icon = gebr_ui_flows_io_get_icon_str(io);

	if (error_1 || err_msg || !icon)
		if (io->priv->active)
			icon = GTK_STOCK_DIALOG_WARNING;

	io->priv->stock_id = g_strdup(icon);
	return;
}

void
on_ui_flows_io_overwrite_toggled(GtkCheckMenuItem *item,
                                 GebrUiFlowsDai *dai)
{
	gboolean overwrite = gtk_check_menu_item_get_active(item);
	gebr_ui_flows_io_set_overwrite(dai->io, overwrite);

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(dai->io);

	if (type == GEBR_IO_TYPE_OUTPUT)
		gebr_geoxml_flow_io_set_output_append(GEBR_GEOXML_FLOW(dai->doc), !overwrite);
	else if (type == GEBR_IO_TYPE_ERROR)
		gebr_geoxml_flow_io_set_error_append(GEBR_GEOXML_FLOW(dai->doc), !overwrite);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	gebr_ui_flows_io_load_from_xml(dai->io, gebr.line, GEBR_GEOXML_FLOW(dai->doc), maestro, gebr.validator);

	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_browse->store);
	GtkTreePath *path = gtk_tree_model_get_path(model, dai->iter);
	gtk_tree_model_row_changed(model, path, dai->iter);
	gtk_tree_path_free(path);
}

GtkMenu *
gebr_ui_flows_io_popup_menu(GebrUiFlowsIo *io,
			    GebrGeoXmlFlow *flow,
			    GtkTreeIter *iter)
{
	GtkWidget *menu = gtk_menu_new();
	GtkWidget * menu_item;
	gboolean overwrite;
	GCallback overwrite_callback = G_CALLBACK(on_ui_flows_io_overwrite_toggled);
	const gchar *label;

	if (!io->priv->active)
		return NULL;

	GebrUiFlowsIoType type = io->priv->type;

	switch(type) {
	case (GEBR_IO_TYPE_OUTPUT):
		label = _("Overwrite existing output file");
		break;
	case (GEBR_IO_TYPE_ERROR):
		label = _("Overwrite existing log file");
		break;
	default:
		return NULL;
	}

	overwrite = gebr_ui_flows_io_get_overwrite(io);
	menu_item = gtk_check_menu_item_new_with_label (label);

	GebrUiFlowsDai *dai = g_new0(GebrUiFlowsDai, 1);
	dai->doc = GEBR_GEOXML_DOCUMENT(flow);
	dai->io  = io;
	dai->iter = iter;

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), overwrite);
	g_signal_connect(menu_item, "toggled", overwrite_callback, dai);
	gtk_menu_shell_append(GTK_MENU_SHELL (menu), menu_item);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
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

void
gebr_ui_flows_io_start_editing(GebrUiFlowsIo *io,
                               GtkEntry *entry)
{
	GtkTreeModel *completion_model;
	const gchar *input;
	const gchar *output;
	const gchar *error;

	completion_model = gebr_gui_parameter_get_completion_model(GEBR_GEOXML_DOCUMENT (gebr.flow),
								   GEBR_GEOXML_DOCUMENT (gebr.line),
								   GEBR_GEOXML_DOCUMENT (gebr.project),
								   GEBR_GEOXML_PARAMETER_TYPE_FILE);
	gebr_gui_parameter_set_entry_completion(entry, completion_model, GEBR_GEOXML_PARAMETER_TYPE_FILE);

	input = gebr_geoxml_flow_io_get_input(gebr.flow);
	output = gebr_geoxml_flow_io_get_output(gebr.flow);
	error = gebr_geoxml_flow_io_get_error(gebr.flow);

	if ((io->priv->type == GEBR_IO_TYPE_INPUT && !*input) ||
	    (io->priv->type == GEBR_IO_TYPE_OUTPUT && !*output) ||
	    (io->priv->type == GEBR_IO_TYPE_ERROR && !*error))
		gtk_entry_set_text(entry, "");

	g_signal_connect(entry, "populate-popup", G_CALLBACK(populate_io_popup), NULL);
}

void
gebr_ui_flows_io_edited(GebrUiFlowsIo *io,
                        gchar *new_text)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	gchar *mount_point;

	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *tmp = gebr_relativise_path(new_text, mount_point, paths);
	gebr_pairstrfreev(paths);

	gebr_ui_flows_io_set_value(io, tmp);

	if (new_text) {
		if (io->priv->type == GEBR_IO_TYPE_INPUT)
			gebr_geoxml_flow_io_set_input(gebr.flow, tmp);
		else if (io->priv->type == GEBR_IO_TYPE_OUTPUT)
			gebr_geoxml_flow_io_set_output(gebr.flow, tmp);
		else if (io->priv->type == GEBR_IO_TYPE_ERROR)
			gebr_geoxml_flow_io_set_error(gebr.flow, tmp);
	}

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	gebr_ui_flows_io_load_from_xml(io, gebr.line, gebr.flow, maestro, gebr.validator);
}

