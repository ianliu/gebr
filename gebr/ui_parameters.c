/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gebr-expr.h>

#include "ui_parameters.h"
#include "gebr.h"
#include "menu.h"
#include "flow.h"
#include "document.h"
#include "ui_help.h"
#include "ui_flow_execution.h"
#include "ui_flow_browse.h"
#include "ui_flow_program.h"

/* Prototypes {{{1 */
/*
static void	parameters_actions			(GtkDialog          *dialog,
							 gint                arg1,
							 GebrGuiProgramEdit *program_edit);

static gboolean	parameters_on_delete_event		(GtkDialog          *dialog,
							 GdkEventAny        *event,
							 GebrGuiProgramEdit *program_edit);
							 */

static gboolean
on_parameters_key_press(GtkWidget *widget,
                        GdkEvent *event,
                        GebrGuiProgramEdit *program_edit)
{
	if (event->key.keyval == GDK_Return) {
		flow_browse_revalidate_programs(gebr.ui_flow_browse);
		flow_browse_validate_io(gebr.ui_flow_browse);
		flow_browse_info_update();
	}

	return FALSE;
}

static void
on_parameters_default_button_clicked(GtkButton *button,
				     GebrGuiProgramEdit *program_edit)
{
	GebrGeoXmlParameters *params;
	params = gebr_geoxml_program_get_parameters(program_edit->program);
	gebr_geoxml_parameters_reset_to_default(params);
	gebr_gui_program_edit_reload(program_edit, NULL);
	gebr_geoxml_object_unref(params);
	return;
}

static void
on_parameters_help_button_clicked(GtkButton *button,
				  GebrGuiProgramEdit *program_edit)
{
	gebr_help_show(GEBR_GEOXML_OBJECT(program_edit->program), FALSE);

}

static void
on_validated(GebrGuiParameterWidget *widget,
	     gpointer user_data)
{
	flow_browse_revalidate_programs(gebr.ui_flow_browse);
	flow_browse_validate_io(gebr.ui_flow_browse);
	flow_browse_info_update();
}

/* Public functions {{{1*/
GebrGuiProgramEdit *
parameters_configure_setup_ui(void)
{
	GebrGuiProgramEdit *program_edit;

	GtkTreeIter iter;
	if (flow_browse_get_selected(&iter, FALSE) == FALSE)
		return NULL;

	GebrUiFlowBrowseType row_type;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
	                   FB_STRUCT_TYPE, &row_type, -1);

	if (row_type != STRUCT_TYPE_PROGRAM)
		return NULL;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line );
	GebrMaestroInfo *info;
	if (maestro)
		info = gebr_maestro_server_get_info(maestro);
	else
		info = NULL;

	gchar *name = NULL;
	GebrMaestroServerGroupType type = 0;
	gdouble speed = gebr_interface_get_execution_speed();

	gebr_ui_flow_update_prog_mpi_nprocess(gebr.program, maestro, speed, name, type);

	program_edit = gebr_gui_program_edit_setup_ui(gebr.program, NULL,
						      FALSE, gebr.validator, info, FALSE);
	gebr_gui_program_edit_set_validated_callback(program_edit, on_validated, NULL);

	GtkWidget *hbox_buttons = gtk_hbox_new(FALSE, 0);
	GtkWidget *button_default = gtk_button_new_with_mnemonic(_("Reset"));
	GtkWidget *button_help = gtk_button_new_with_mnemonic(_("_Help"));

	g_signal_connect(program_edit->widget, "event", G_CALLBACK(on_parameters_key_press), program_edit);
	g_signal_connect(button_default, "clicked", G_CALLBACK(on_parameters_default_button_clicked), program_edit);
	g_signal_connect(button_help, "clicked", G_CALLBACK(on_parameters_help_button_clicked), program_edit);
 
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_help, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_default, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(program_edit->widget), hbox_buttons, FALSE, TRUE, 5);

	gtk_widget_show_all(program_edit->widget);
	return program_edit;
}

gboolean
validate_selected_program(GError **error)
{
	GtkTreeIter iter;
	if (!flow_browse_get_selected(&iter, FALSE))
		return FALSE;
	return validate_program_iter(&iter, error);
}

gboolean
validate_program_iter(GtkTreeIter *iter, GError **error)
{
	GebrUiFlowBrowseType type;
	GebrUiFlowProgram *ui_program;
	GebrGeoXmlProgram *program;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), iter,
			   FB_STRUCT_TYPE, &type,
	                   FB_STRUCT, &ui_program,
			   -1);

	if (type != STRUCT_TYPE_PROGRAM)
		return FALSE;

	program = gebr_ui_flow_program_get_xml(ui_program);

	if (gebr_geoxml_program_is_valid(program, gebr.validator, error))
		return TRUE;

	return FALSE;
}
