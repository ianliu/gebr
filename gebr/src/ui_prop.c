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

/* File: ui_prop.c
 * Program's parameter window stuff
 */
#include "ui_prop.h"

#include <geoxml.h>
#include <stdlib.h>
#include <string.h>

#include "gebr.h"
#include "interface.h"
#include "callbacks.h"
#include "cb_prop.h"

/* Function: progpar_config_window
 * Assembly a program's parameter input window
 */
void
progpar_config_window    (void)
{
	GtkTreeSelection *		selection;
	GtkTreeModel *			model;
	GtkTreeIter			iter;

	GtkWidget *			label;
	GtkWidget *			vbox;
	GtkWidget *			scrolledwin;
	GtkWidget *			viewport;

	GeoXmlProgram *			program;
	GeoXmlProgramParameter *	program_parameter;
	long int			i;

	/* Which program ? */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		GtkTreePath * path;

		path = gtk_tree_model_get_path (model, &iter);
		W.program_index = (int) atoi(gtk_tree_path_to_string(path));
		gtk_tree_path_free(path);
	} else {
		gtk_statusbar_push (GTK_STATUSBAR (W.statusbar), 0,
				"No program selected");
		return;
	}

	W.parameters = gtk_dialog_new_with_buttons ("Parameters",
						GTK_WINDOW(W.mainwin),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK,     GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	gtk_dialog_add_button(GTK_DIALOG(W.parameters), "Default", GTK_RESPONSE_DEFAULT);
	gtk_dialog_add_button(GTK_DIALOG(W.parameters), "Help", GTK_RESPONSE_HELP);
	gtk_widget_set_size_request (W.parameters, 490, 350);

	/* Take the apropriate action when a button is pressed */
	g_signal_connect_swapped (W.parameters, "response",
				G_CALLBACK (properties_action),
				W.parameters);
	g_signal_connect (GTK_OBJECT (W.parameters), "delete_event",
			GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL );

	gtk_box_set_homogeneous( GTK_BOX(GTK_DIALOG (W.parameters)->vbox), FALSE);

	/* Flow Title */
	label = gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (W.parameters)->vbox), label, FALSE, TRUE, 5);
	gtk_misc_set_alignment( GTK_MISC(label), 0.5, 0);

	/* Scrolled window for parameters */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	viewport = gtk_viewport_new(NULL, NULL);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (W.parameters)->vbox), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (scrolledwin), viewport);
	gtk_container_add (GTK_CONTAINER (viewport), vbox);

	/* Starts reading program and its parameters
	 */
	geoxml_flow_get_program(flow, &program, W.program_index);

	/* Program title in bold */
	{
		char *markup;

		markup = g_markup_printf_escaped ("<big><b>%s</b></big>", geoxml_program_get_title(program));
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	W.parwidgets_number = geoxml_program_get_parameters_number(program);
	W.parwidgets = (GtkWidget**) malloc(sizeof(GtkWidget*) * W.parwidgets_number);

	program_parameter = geoxml_program_get_first_parameter(program);
	for(i=0; i<W.parwidgets_number; i++,  geoxml_program_parameter_next(&program_parameter)) {
		gchar * label = (gchar *)geoxml_program_parameter_get_label(program_parameter);

		switch (geoxml_program_parameter_get_type(program_parameter)) {
		case GEOXML_PARAMETERTYPE_FLOAT:
			W.parwidgets[i] =
			progpar_add_input_float (vbox, label,
						geoxml_program_parameter_get_required (program_parameter));

			gtk_entry_set_text(GTK_ENTRY(W.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_INT:
			W.parwidgets[i] =
			progpar_add_input_int(vbox, label,
						geoxml_program_parameter_get_required (program_parameter));
			gtk_entry_set_text(GTK_ENTRY(W.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_STRING:
			W.parwidgets[i] =
			progpar_add_input_string(vbox, label,
						geoxml_program_parameter_get_required (program_parameter));

			gtk_entry_set_text(GTK_ENTRY(W.parwidgets[i]),
					geoxml_program_parameter_get_value(program_parameter));
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			{
			gchar *min;
			gchar *max;
			gchar *step;

			geoxml_program_parameter_get_range_properties(program_parameter,
									&min, &max, &step);

			W.parwidgets[i] =
				progpar_add_input_range(vbox, label,
							atof(min), atof(max), atof(step),
							geoxml_program_parameter_get_required (program_parameter));

			gtk_spin_button_set_value(GTK_SPIN_BUTTON(W.parwidgets[i]),
							atof(geoxml_program_parameter_get_value(program_parameter)));
			break;
			}
		case GEOXML_PARAMETERTYPE_FLAG:
			W.parwidgets[i] = progpar_add_input_flag (vbox, label);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(W.parwidgets[i]),
							geoxml_program_parameter_get_flag_status(program_parameter));
		break;
		case GEOXML_PARAMETERTYPE_FILE: {
			W.parwidgets[i] = progpar_add_input_file (vbox, label,
				geoxml_program_parameter_get_file_be_directory(program_parameter),
				geoxml_program_parameter_get_required(program_parameter),
				geoxml_program_parameter_get_value(program_parameter));
		} break;
		default:
		break;
		}
	}

	gtk_widget_show_all(W.parameters);
	gtk_dialog_run(GTK_DIALOG(W.parameters));
}

/*
 * Function: progpar_add_input_float
 * Add an input entry to a float parameter
 */
GtkWidget *
progpar_add_input_float    (GtkWidget    *wg,
			    gchar        *label_str,
			    gboolean      required)
{
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *entry;

   hbox = gtk_hbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

   label = gtk_label_new(label_str);
   gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

   if (required) {
      char *markup;

      markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
   }

   entry = gtk_entry_new();
   gtk_widget_set_size_request (entry, 90, 30);
   gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

   return entry;

}

/*
 * Function: progpar_add_input_int
 * Add an input entry to an integer parameter
 */
GtkWidget *
progpar_add_input_int      (GtkWidget    *wg,
			    gchar        *label_str,
			    gboolean      required)
{
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *entry;

   hbox = gtk_hbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

   label = gtk_label_new(label_str);
   gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

   if (required) {
      char *markup;

      markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
   }

   entry = gtk_entry_new();
   gtk_widget_set_size_request (entry, 90, 30);
   gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

   return entry;

}

/*
 * Function: progpar_add_input_string
 * Add an input entry to a string parameter
 */
GtkWidget *
progpar_add_input_string   (GtkWidget    *wg,
			    gchar        *label_str,
			    gboolean      required )
{
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *entry;

   hbox = gtk_hbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

   label = gtk_label_new(label_str);
   gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

   if (required) {
      char *markup;

      markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
   }

   entry = gtk_entry_new();
   gtk_widget_set_size_request (entry, 140, 30);
   gtk_box_pack_end(GTK_BOX (hbox), entry, FALSE, FALSE, 0);

   return entry;
}

/*
 * Function: progpar_add_input_range
 * Add an input entry to a range parameter
 */
GtkWidget *
progpar_add_input_range    (GtkWidget    *wg,
			    gchar        *label_str,
			    gdouble       min,
			    gdouble       max,
			    gdouble       step,
			    gboolean      required )
{
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *spin;

   hbox = gtk_hbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

   label = gtk_label_new(label_str);
   gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);

   if (required) {
      char *markup;

      markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
   }

   spin = gtk_spin_button_new_with_range(min, max, step);
   gtk_widget_set_size_request (spin, 90, 30);
   gtk_box_pack_end(GTK_BOX (hbox), spin, FALSE, FALSE, 0);

   return spin;
}

/*
 * Function: progpar_add_input_flag
 * Add an input entry to a flag parameter
 */
GtkWidget *
progpar_add_input_flag     (GtkWidget    *wg,
			    gchar        *label_str)
{
   GtkWidget *hbox;
   GtkWidget *checkbutton;

   hbox = gtk_hbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

   checkbutton = gtk_check_button_new_with_label( label_str );
   gtk_box_pack_start(GTK_BOX (hbox), checkbutton, FALSE, TRUE, 0);

   return checkbutton;
}

/*
 * Function: progpar_add_input_file
 * Add an input entry and button to browse for a file or directory
 */
GtkWidget *
progpar_add_input_file	(	GtkWidget *		wg,
				gchar *			label_str,
				gboolean		is_directory,
				gboolean		required,
				const gchar *			path
			)
{
	GtkWidget *hbox;
	GtkWidget *label;

	/* hbox */
	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX (wg), hbox, FALSE, TRUE, 0);

	/* label */
	label = gtk_label_new (label_str);
	gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, TRUE, 0);
	if (required) {
		char *markup;

		markup = g_markup_printf_escaped ("<b>%s</b><sup>*</sup>", label_str);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}

	/* file chooser */
	if (is_directory) {
		GtkWidget *file_chooser;

		if (!required) {
			GtkWidget *checkbutton;
			GtkWidget *hbox2;

			hbox2 = gtk_hbox_new(FALSE, 10);

			checkbutton = gtk_check_button_new();
			gtk_box_pack_start(GTK_BOX (hbox2), checkbutton, FALSE, TRUE, 0);
			file_chooser = gtk_file_chooser_button_new ("Browse", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
			gtk_box_pack_start(GTK_BOX (hbox2), file_chooser, TRUE, TRUE, 0);

 			g_signal_connect(checkbutton, "clicked",
					 G_CALLBACK (properties_toogle_file_browse),
					 file_chooser );

			if (strlen(path)) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), path);
			}
			else{
			   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), FALSE);
			   g_object_set(file_chooser, "sensitive", FALSE, NULL);
			}

			gtk_widget_set_size_request (hbox2, 180, 30);
			gtk_box_pack_end(GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

			return hbox2;
		} else {
			file_chooser = gtk_file_chooser_button_new ("Browse", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

			if (!strlen(path))
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), getenv("HOME"));
			else
				gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), path);

			gtk_widget_set_size_request (file_chooser, 180, 30);
			gtk_box_pack_end(GTK_BOX (hbox), file_chooser, FALSE, FALSE, 0);

			return file_chooser;
		}
	} else {
		gebr_save_widget_t save_widget;

		save_widget = save_widget_create();
		gtk_entry_set_text (GTK_ENTRY (save_widget.entry), path);

		gtk_widget_set_size_request (save_widget.hbox, 180, 30);
		gtk_box_pack_end(GTK_BOX (hbox), save_widget.hbox, FALSE, FALSE, 0);

		return save_widget.entry;
	}
}
