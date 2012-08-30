/*
 * ui_flow_program.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Team
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
#include "ui_flow_program.h"

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-version.h>
#include <libgebr/gui/gui.h>
#include <flow.h>
#include "gebr.h"
#include <gebr/ui_flow_program.h>
#include <libgebr/comm/gebr-comm.h>


struct _GebrUiFlowProgramPriv {
	GebrGeoXmlProgram *program;
	gboolean never_opened;
	GebrGeoXmlProgramStatus status;
	GebrIExprError error_id;
	gchar *tooltip;
	// inserir ID que deve ser um indentificador unico de um programa
};

/*----------------------------------------------------------------------------------------------*/

G_DEFINE_TYPE(GebrUiFlowProgram, gebr_ui_flow_program, G_TYPE_OBJECT);


static void
gebr_ui_flow_program_finalize(GObject *object)
{
	G_OBJECT_CLASS(gebr_ui_flow_program_parent_class)->finalize(object);
}

static void
gebr_ui_flow_program_class_init(GebrUiFlowProgramClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebr_ui_flow_program_finalize;
	g_type_class_add_private(klass, sizeof(GebrUiFlowProgramPriv));
}


static void
gebr_ui_flow_program_init(GebrUiFlowProgram *prog)
{
	prog->priv = G_TYPE_INSTANCE_GET_PRIVATE(prog,
						 GEBR_TYPE_UI_FLOW_PROGRAM,
						 GebrUiFlowProgramPriv);
}

GebrUiFlowProgram *
gebr_ui_flow_program_new(GebrGeoXmlProgram *program)
{
	GebrUiFlowProgram *ui_prog = g_object_new(GEBR_TYPE_UI_FLOW_PROGRAM, NULL);

	ui_prog->priv->program = program;
	ui_prog->priv->status = gebr_geoxml_program_get_status(program);
	gebr_geoxml_program_get_error_id(program, &(ui_prog->priv->error_id));
	gebr_ui_flow_program_update_tooltip(ui_prog);

	return ui_prog;
}

/*----------------------------------------------------------------------------------------------*/

void
gebr_ui_flow_program_set_xml(GebrUiFlowProgram *program,
			     GebrGeoXmlProgram *prog_xml)
{
	if (program->priv->program)
		gebr_geoxml_object_unref(program->priv->program);

	program->priv->program = prog_xml;
}

GebrGeoXmlProgram *
gebr_ui_flow_program_get_xml(GebrUiFlowProgram *program)
{
	return program->priv->program;
}

void
gebr_ui_flow_program_set_flag_opened(GebrUiFlowProgram *program,
				     gboolean never_opened)
{
	program->priv->never_opened = never_opened;
}

gboolean
gebr_ui_flow_program_get_flag_opened(GebrUiFlowProgram *program)
{
	return program->priv->never_opened;
}

void
gebr_ui_flow_program_set_status(GebrUiFlowProgram *program,
				GebrGeoXmlProgramStatus status)
{
	program->priv->status = status;
}

GebrGeoXmlProgramStatus
gebr_ui_flow_program_get_status (GebrUiFlowProgram *program)
{
	return program->priv->status;
}

void
gebr_ui_flow_program_set_error_id(GebrUiFlowProgram *program,
				  GebrIExprError error_id)
{
	program->priv->error_id = error_id;
}

GebrIExprError
gebr_ui_flow_program_get_error_id (GebrUiFlowProgram *program)
{
	return program->priv->error_id;
}

const gchar *
gebr_ui_flow_program_get_tooltip(GebrUiFlowProgram *program)
{
	return program->priv->tooltip;
}

void
gebr_ui_flow_program_update_tooltip(GebrUiFlowProgram *program) {
	const gchar *tooltip;
	if (program->priv->status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED) {
		tooltip =  _("This program needs to be configured");
	} else {
		GebrIExprError errorid = program->priv->error_id;

		switch (errorid) {
		case GEBR_IEXPR_ERROR_SYNTAX:
		case GEBR_IEXPR_ERROR_TOOBIG:
		case GEBR_IEXPR_ERROR_RUNTIME:
		case GEBR_IEXPR_ERROR_INVAL_TYPE:
		case GEBR_IEXPR_ERROR_TYPE_MISMATCH:
			tooltip = _("This program has an invalid expression");
			break;
		case GEBR_IEXPR_ERROR_EMPTY_EXPR:
			tooltip = _("A required parameter is unfilled");
			break;
		case GEBR_IEXPR_ERROR_UNDEF_VAR:
		case GEBR_IEXPR_ERROR_UNDEF_REFERENCE:
			tooltip = _("An undefined variable is being used");
			break;
		case GEBR_IEXPR_ERROR_INVAL_VAR:
		case GEBR_IEXPR_ERROR_BAD_REFERENCE:
		case GEBR_IEXPR_ERROR_CYCLE:
			tooltip = _("A badly defined variable is being used");
			break;
		case GEBR_IEXPR_ERROR_PATH:
			tooltip = _("This program has cleaned their paths");
			break;
		case GEBR_IEXPR_ERROR_BAD_MOVE:
		case GEBR_IEXPR_ERROR_INITIALIZE:
		default:
			tooltip = NULL;
			break;
		}
	}

	if (program->priv->tooltip)
		g_free(program->priv->tooltip);
	program->priv->tooltip = g_strdup(tooltip);
}

GtkMenu *
gebr_ui_flow_program_popup_menu(GebrUiFlowProgram *program)
{
	GtkWidget * menu_item;
	GtkAction * action;

	if (!program)
		return NULL;

	GtkWidget *menu = gtk_menu_new();



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
gebr_ui_flow_program_get_error_tooltip(GebrUiFlowProgram *program)
{
	//FIXME: remove as soon as we remove gebr-flow-edition.c
	return NULL;
}
