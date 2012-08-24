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

void
gebr_ui_flows_io_set_tooltip(GebrUiFlowsIo *io, const gchar *tooltip)
{
	io->priv->tooltip = g_strdup(tooltip);
}

const gchar*
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
			       GebrGeoXmlProject *project,
			       GebrGeoXmlLine *line,
			       GebrGeoXmlFlow *flow,
			       GebrMaestroServer *maestro,
			       GebrValidator *validator)
{
	GError *err = NULL;
	gchar *err_msg, *result, *tooltip, *tmp, *mount_point;
	gchar *path, *path_real;
	const gchar *icon, *title, *aux;
	gboolean ok = FALSE;
	gchar ***paths = gebr_geoxml_line_get_paths(line);

	if (maestro)
		mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
	else
		mount_point = NULL;

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(io);
	gboolean active = io->priv->active;
	gboolean overwrite = io->priv->overwrite;

	// Get the path
	switch(type) {
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
	gchar *resolved_path = gebr_resolve_relative_path(path_real, paths);
	gebr_validator_evaluate(validator, resolved_path, GEBR_GEOXML_PARAMETER_TYPE_STRING,
				GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &result, &err);

	ok = gebr_validator_evaluate_interval(validator, path_real,
					      GEBR_GEOXML_PARAMETER_TYPE_STRING,
					      GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
					      FALSE, &tmp, NULL);

	title = path;
	if (!*path || !active) {
		title = gebr_ui_flows_io_get_label_markup(io);
		icon = gebr_ui_flows_io_get_icon_str(io);
		tooltip = NULL;
	} else if (err) {
		tooltip = g_strdup(err->message);
		icon = GTK_STOCK_DIALOG_WARNING;
	} else if (!gebr_validate_path(tmp, paths, &err_msg)) {
		tooltip = err_msg;
		icon = GTK_STOCK_DIALOG_WARNING;
	} else {
		switch(type) {
		case GEBR_IO_TYPE_INPUT:
			tooltip = g_strdup_printf(_("Input file \"%s\""), result);
			icon = "gebr-stdin";
			break;
		case GEBR_IO_TYPE_OUTPUT:
			tooltip = g_strdup_printf(_("%s output file \"%s\""),
						  overwrite ? _("Append to") : _("Overwrite"), result);
			break;
		case GEBR_IO_TYPE_ERROR:
			tooltip = g_strdup_printf(_("%s log file \"%s\""),
						  overwrite ? _("Append to") : _("Overwrite"), result);
			break;
		case GEBR_IO_TYPE_NONE:
			break;
		}
	}

	io->priv->value = g_strdup(title);
	io->priv->tooltip = g_strdup(tooltip);
	io->priv->stock_id = g_strdup(icon);

	g_free(tmp);
	g_free(mount_point);
	g_free(path);
	g_free(path_real);
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
on_ui_flows_io_overwrite_toggled(GtkCheckMenuItem *item,
					     GebrUiFlowsDai *dai)
{
	gboolean overwrite = !gtk_check_menu_item_get_active(item);
	gebr_ui_flows_io_set_overwrite(dai->io, overwrite);

	GebrUiFlowsIoType type = gebr_ui_flows_io_get_io_type(dai->io);

	if (type == GEBR_IO_TYPE_OUTPUT)
		gebr_geoxml_flow_io_set_output_append(GEBR_GEOXML_FLOW(dai->doc), overwrite);
	else if (type == GEBR_IO_TYPE_OUTPUT)
		gebr_geoxml_flow_io_set_error_append(GEBR_GEOXML_FLOW(dai->doc), overwrite);
}

GtkMenu *
gebr_ui_flows_io_popup_menu(GebrUiFlowsIo *io,
			    GebrGeoXmlFlow *flow)
{
	GtkWidget *menu = gtk_menu_new();
	GtkWidget * menu_item;
	gboolean overwrite;
	GCallback overwrite_callback = G_CALLBACK(on_ui_flows_io_overwrite_toggled);
	const gchar *label;

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

	g_signal_connect (menu_item, "toggled", overwrite_callback, dai);
	//gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), !overwrite);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show(menu_item);

	return GTK_MENU(menu);
}
