/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* File: cb_prop.c
 * Callbacks for the component's properties manipulation
 */
#include "cb_prop.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* #include <gtkhtml/gtkhtml.h> */

#include "gebr.h"
#include "callbacks.h"
#include "cb_flow.h"
#include "menus.h"
#include "ui_help.h"
#include "ui_prop.h"

/*
  Take the appropriate action when the parameter dialog emmits
  a response signal.
 */
void
properties_action		(GtkDialog *dialog,
				 gint       arg1,
				 gpointer   user_data)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		properties_parameters_set_to_flow();
		g_free(W.parwidgets);
                gtk_widget_destroy(GTK_WIDGET(dialog));
		break;
	case GTK_RESPONSE_CANCEL:
                gtk_widget_destroy(GTK_WIDGET(dialog));
		g_free(W.parwidgets);
		break;
	case GTK_RESPONSE_DEFAULT:
		properties_set_to_default();
		break;
	case GTK_RESPONSE_HELP: {
		GtkWidget *		dialog;
		GtkWidget *		scrolled_window;
		GtkWidget *		html;

		GtkTreeIter		iter;
		GtkTreeSelection *	selection;
		GtkTreeModel *		model;
		GtkTreePath *		path;

		GeoXmlFlow *		menu;
		GeoXmlProgram *		program;
		gulong			index;
		gchar *			menu_fn;
		GString *               menu_path;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
		if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
			log_message(INTERFACE, "No flow component selected", TRUE);
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		index = (gulong)atoi(gtk_tree_path_to_string(path));
		gtk_tree_path_free(path);

		/* get the program and its path on menu */
		geoxml_flow_get_program(flow, &program, index);
		geoxml_program_get_menu(program, &menu_fn, &index);

		if (menus_fname(menu_fn, &menu_path) != EXIT_SUCCESS)
			break;
		menu = flow_load_path (menu_path->str);
		if (menu == NULL)
			break;

		/* go to menu's program index specified in flow */
		geoxml_flow_get_program(menu, &program, index);
		show_help((gchar*)geoxml_program_get_help(program),"Program help","prog");

		geoxml_document_free(GEOXML_DOC(menu));
		break;
	}
	default:
		break;
	}
}

void
properties_set_to_default		()
{
	int			 i;
	GeoXmlProgram		*program;
	GeoXmlProgramParameter	*program_parameter;

	geoxml_flow_get_program(flow, &program, W.program_index);
	program_parameter = geoxml_program_get_first_parameter(program);
	for (i=0; i < W.parwidgets_number; i++, geoxml_program_parameter_next(&program_parameter)) {
		const gchar * default_str = geoxml_program_parameter_get_default(program_parameter);
		if (default_str == NULL)
			continue;

		switch (geoxml_program_parameter_get_type(program_parameter)) {
		case GEOXML_PARAMETERTYPE_FLOAT:
		case GEOXML_PARAMETERTYPE_INT:
		case GEOXML_PARAMETERTYPE_STRING:
			gtk_entry_set_text(GTK_ENTRY(W.parwidgets[i]), default_str);
			break;
		case GEOXML_PARAMETERTYPE_RANGE: {
			char * endptr;
			double value = strtod(default_str, &endptr);
			if (endptr != default_str)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(W.parwidgets[i]), value);
			break;
		}
		case GEOXML_PARAMETERTYPE_FLAG:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(W.parwidgets[i]),
				geoxml_program_parameter_get_flag_default(program_parameter));
			break;
		case GEOXML_PARAMETERTYPE_FILE:
			if (geoxml_program_parameter_get_file_be_directory(program_parameter)) {
				if (geoxml_program_parameter_get_required(program_parameter))
					gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (W.parwidgets[i]), geoxml_program_parameter_get_value(program_parameter));
				else {
					GList *		list;
					GtkWidget *	file_chooser;
					GtkWidget *	checkbox;

					list = gtk_container_get_children(GTK_CONTAINER(W.parwidgets[i]));
					checkbox = (GtkWidget*)g_list_nth_data(list, 0);
					file_chooser = (GtkWidget*)g_list_nth_data(list, 1);

					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), FALSE);
					/* TODO: deactivate file_chooser */

					g_list_free(list);
				}
			} else
				gtk_entry_set_text(GTK_ENTRY(W.parwidgets[i]), default_str);
			break;
		default:
			break;
		}
	}

}

void
properties_parameters_set_to_flow	(void)
{
   int ii;

   GeoXmlProgram          *program;
   GeoXmlProgramParameter *parameter;

   geoxml_flow_get_program(flow, &program, W.program_index);
   parameter = geoxml_program_get_first_parameter(program);

   /* Set program state to configured */
   geoxml_program_set_status(program, "configured");

   /* Update interface */
   {
      GtkTreeIter       iter;
      GtkTreeSelection *selection;
      GtkTreeModel     *model;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
      gtk_tree_selection_get_selected (selection, &model, &iter);

      gtk_list_store_set (W.fseq_store, &iter,
			  FSEQ_STATUS_COLUMN, W.pixmaps.configured_icon, -1);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (W.configured_menuitem), TRUE);

   }

	for (ii=0; ii < W.parwidgets_number; ii++, geoxml_program_parameter_next(&parameter)){

		switch (geoxml_program_parameter_get_type(parameter)){
		case GEOXML_PARAMETERTYPE_FLOAT:
		case GEOXML_PARAMETERTYPE_INT:
		case GEOXML_PARAMETERTYPE_STRING:
			geoxml_program_parameter_set_value(parameter,
							gtk_entry_get_text(GTK_ENTRY(W.parwidgets[ii])));

			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			{
			char valuestr[30];

			sprintf(valuestr, "%f", gtk_spin_button_get_value(GTK_SPIN_BUTTON(W.parwidgets[ii])));
			geoxml_program_parameter_set_value(parameter, valuestr);
			break;
			}
		case GEOXML_PARAMETERTYPE_FLAG:
			geoxml_program_parameter_set_flag_state(parameter,
								gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(W.parwidgets[ii])));
			break;
		case GEOXML_PARAMETERTYPE_FILE: {
			if (geoxml_program_parameter_get_file_be_directory(parameter)) {
				if (geoxml_program_parameter_get_required(parameter)) {
					gchar * path;

					path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (W.parwidgets[ii]));
					geoxml_program_parameter_set_value(parameter, path);

					g_free(path);
				} else {
					GList *		list;
					GtkWidget *	file_chooser;
					GtkWidget *	checkbox;

					list = gtk_container_get_children(GTK_CONTAINER(W.parwidgets[ii]));
					checkbox = (GtkWidget*)g_list_nth_data(list, 0);
					file_chooser = (GtkWidget*)g_list_nth_data(list, 1);

					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox))) {
						gchar * path;

						path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_chooser));
						geoxml_program_parameter_set_value(parameter, path);

						g_free(path);
					} else
						geoxml_program_parameter_set_value(parameter, "");

					g_list_free(list);
				}
			} else
				geoxml_program_parameter_set_value(parameter,
					gtk_entry_get_text(GTK_ENTRY(W.parwidgets[ii])));
		} break;
		default:
			break;
		}
	}

   flow_save();
}

void
properties_toogle_file_browse      (GtkToggleButton *togglebutton,
				    gpointer         user_data)
{

   gboolean state;

   state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
   g_object_set((GtkWidget *) user_data, "sensitive", state, NULL);
}

