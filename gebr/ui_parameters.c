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
static void	parameters_actions			(GtkDialog          *dialog,
							 gint                arg1,
							 GebrGuiProgramEdit *program_edit);

static gboolean	parameters_on_delete_event		(GtkDialog          *dialog,
							 GdkEventAny        *event,
							 GebrGuiProgramEdit *program_edit);

/* Public functions {{{1*/
void
parameters_configure_setup_ui(void)
{
	GtkWidget *dialog;
	GtkWidget *button;
	GebrGuiProgramEdit *program_edit;

	if (flow_browse_get_selected(NULL, FALSE) == FALSE)
		return;

	dialog = gtk_dialog_new_with_buttons(_("Parameters"),
					     GTK_WINDOW(gebr.window),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     NULL);

	g_object_set(dialog, "type-hint", GDK_WINDOW_TYPE_HINT_NORMAL, NULL);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	gchar *tmp_help_p = gebr_geoxml_program_get_help(gebr.program);
	if (strlen(tmp_help_p) == 0)	
		gtk_widget_set_sensitive(button, FALSE);
	g_free(tmp_help_p);

	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Default"), GTK_RESPONSE_APPLY);
	g_object_set(G_OBJECT(button), "image",
		     gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_BUTTON), NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line );
	GebrMaestroInfo *info;
	if (maestro)
		info = gebr_maestro_server_get_info(maestro);
	else
		info = NULL;

	GebrGeoXmlSequence *clone = gebr_geoxml_sequence_append_clone(GEBR_GEOXML_SEQUENCE(gebr.program));

	gchar *name;
	GebrMaestroServerGroupType type;
	gdouble speed = gebr_interface_get_execution_speed();
	gebr_flow_browse_get_current_group(gebr.ui_flow_browse, &type, &name);

	gebr_ui_flow_update_prog_mpi_nprocess(GEBR_GEOXML_PROGRAM(clone), maestro, speed, name, type);

	program_edit = gebr_gui_program_edit_setup_ui(GEBR_GEOXML_PROGRAM(clone), NULL,
						      FALSE, gebr.validator, info);

	g_signal_connect(dialog, "response", G_CALLBACK(parameters_actions), program_edit);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(parameters_on_delete_event), program_edit);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), program_edit->widget, TRUE, TRUE, 0);
	gtk_widget_show(dialog);

	/* adjust window size to fit parameters horizontally */
	gdouble width;
	width = GTK_BIN(GTK_BIN(program_edit->scrolled_window)->child)->child->allocation.width + 30;
	if (width >= gdk_screen_get_width(gdk_screen_get_default()))
		gtk_window_maximize(GTK_WINDOW(dialog));
	else
		gtk_widget_set_size_request(dialog, width, 500);
}

gboolean
validate_selected_program(GError **error)
{
	GtkTreeIter iter;
	flow_browse_get_selected(&iter, FALSE);
	return validate_program_iter(&iter, error);
}

gboolean
validate_program_iter(GtkTreeIter *iter, GError **error)
{
	GebrUiFlowBrowseType type;
	GebrUiFlowProgram *ui_program;
	gboolean never_opened;
	GebrGeoXmlProgram *program;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), iter,
			   FB_STRUCT_TYPE, &type,
	                   FB_STRUCT, &ui_program,
			   -1);

	if (type != STRUCT_TYPE_PROGRAM)
		return FALSE;

	never_opened = gebr_ui_flow_program_get_flag_opened(ui_program);
	program = gebr_ui_flow_program_get_xml(ui_program);

	if (never_opened)
		return FALSE;

	if (gebr_geoxml_program_is_valid(program, gebr.validator, error))
		return TRUE;

	return FALSE;
}

/* Private functions {{{1*/
static void
parameters_actions(GtkDialog *dialog, gint response, GebrGuiProgramEdit *program_edit)
{
	switch (response) {
	case GTK_RESPONSE_OK:{
		GtkTreeIter iter;
		const gchar * icon;

		gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(program_edit->program),
						 GEBR_GEOXML_SEQUENCE(gebr.program));
		gebr_geoxml_object_ref(gebr.program);
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(gebr.program));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

		icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(program_edit->program));

		/* Update interface */
		flow_browse_get_selected(&iter, FALSE);

		GebrUiFlowProgram *ui_program;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
		                   FB_STRUCT, &ui_program,
		                   -1);

		gebr_geoxml_object_ref(program_edit->program);
		gebr_ui_flow_program_set_xml(ui_program, program_edit->program);

		if (gebr_ui_flow_program_get_flag_opened(ui_program))
			gebr_ui_flow_program_set_flag_opened(ui_program, FALSE);

		flow_browse_reload_selected();

		flow_browse_select_iter(&iter);

		GebrGeoXmlProgram *program = program_edit->program;
		if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			if (gebr_geoxml_program_get_status(program) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
				gebr_geoxml_flow_insert_iter_dict(gebr.flow);
				GebrGeoXmlSequence *parameter = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
				gebr_validator_insert(gebr.validator, GEBR_GEOXML_PARAMETER(parameter), NULL, NULL);
			} else {
				gebr_geoxml_flow_update_iter_dict_value(gebr.flow);
				GebrGeoXmlProgramParameter *dict_iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
				const gchar *value = gebr_geoxml_program_parameter_get_first_value(dict_iter, FALSE);
				gebr_validator_change_value(gebr.validator, GEBR_GEOXML_PARAMETER(dict_iter), value, NULL, NULL);
			}
		}

		if (validate_selected_program(NULL))
			flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED, &iter, gebr.ui_flow_browse);
		else
			flow_browse_change_iter_status(GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED, &iter, gebr.ui_flow_browse);

		flow_browse_revalidate_programs(gebr.ui_flow_browse);
		flow_browse_validate_io(gebr.ui_flow_browse);
		gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);

		/* Update parameters review on Flow Browse */
		gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);

		break;
	}
	case GTK_RESPONSE_APPLY: {
		GebrGeoXmlParameters *params;
		params = gebr_geoxml_program_get_parameters(program_edit->program);
		gebr_geoxml_parameters_reset_to_default(params);
		gebr_gui_program_edit_reload(program_edit, NULL);
		gebr_geoxml_object_unref(params);
		return;
	}
	case GTK_RESPONSE_HELP:
		gebr_help_show_selected_program_help();
		return;
	case GTK_RESPONSE_CANCEL:
	default:
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(program_edit->program));
		break;
	}

	gebr_gui_program_edit_destroy(program_edit);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean
parameters_on_delete_event(GtkDialog *dialog,
			   GdkEventAny *event,
			   GebrGuiProgramEdit *program_edit)
{
	parameters_actions(dialog, GTK_RESPONSE_CANCEL, program_edit);
	return FALSE;
}
