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

/* File: cb_flow.c
 * Callbacks for the flows manipulation
 */
#include "cb_flow.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <misc.h>

#include "gebr.h"
#include "interface.h"
#include "server.h"
#include "cb_line.h"
#include "cb_help.h"
#include "menus.h"
#include "callbacks.h"
#include "ui_help.h"

/* errors strings. */
extern const char *no_line_selected_error;
const char *no_flow_selected_error    = "No flow selected";
const char *no_program_selected_error = "No program selected";

/* Function: flow_load_path
 * Load a flow from its path, handling errors
 */
GeoXmlFlow *
flow_load_path (gchar * path)
{
	GeoXmlDocument * doc;
	int ret;

	/* TODO: handle errors in a different way, maybe using statusbar */
	if ((ret = geoxml_document_load (&doc, path))){
	   switch (ret) {
	   case GEOXML_RETV_DTD_SPECIFIED:
	      fprintf(stderr, "%s: dtd specified\n", path);
	      break;
	   case GEOXML_RETV_INVALID_DOCUMENT:
	      fprintf(stderr, "%s: invalid document\n", path);
	      break;
	   case GEOXML_RETV_CANT_ACCESS_FILE:
	      fprintf(stderr, "%s: can't access file\n", path);
	      break;
	   case GEOXML_RETV_CANT_ACCESS_DTD:
	      fprintf(stderr, "%s: can't access dtd\n", path);
	      break;
	   default:
	      fprintf(stderr, "%s: unspecified error\n", path);
	      break;
	   }
	   return NULL;
	}

	return GEOXML_FLOW(doc);
}

/*
 * Function: flow_save
 * Save one flow file
 */
int
flow_save   (void)
{
	GString *fname;

	data_fname(geoxml_document_get_filename (GEOXML_DOC(flow)), &fname);
	return geoxml_document_save( GEOXML_DOC(flow), fname->str );
}

/*
 * Fucntion: flow_export
 * Export current flow to a file
 */
void
flow_export        (GtkMenuItem *menuitem,
		    gpointer     user_data)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel     *	model;
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;
	gchar *			filename;
	gchar *                 oldfilename;


	if (flow == NULL){
	   log_message(INTERFACE, no_flow_selected_error, TRUE);
	   return;
	}

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(	"Choose file", NULL,
							GTK_FILE_CHOOSER_ACTION_SAVE,
							GTK_STOCK_SAVE, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, "System flow files (*.mnu)");
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* export current flow to disk */
	oldfilename = geoxml_document_get_filename(GEOXML_DOC(flow));
	geoxml_document_set_filename(GEOXML_DOC(flow), filename);
	geoxml_document_save( GEOXML_DOC(flow), path );
	geoxml_document_set_filename(GEOXML_DOC(flow), oldfilename);

	g_free(path);
	g_free(filename);

out:
	gtk_widget_destroy(chooser_dialog);

}

void
flow_add_programs_to_view   (GeoXmlFlow * f)
{
	GeoXmlProgram *program;

	for (geoxml_flow_get_program(f, &program, 0); program != NULL; geoxml_program_next(&program)) {
		gchar  *menu;
		gulong  prog_index;

		geoxml_program_get_menu(program, &menu, &prog_index);

		/* Add to the GUI */
		GtkTreeIter piter;
		gtk_list_store_append (W.fseq_store, &piter);
		gtk_list_store_set (W.fseq_store, &piter,
				FSEQ_TITLE_COLUMN, geoxml_program_get_title(program),
				FSEQ_MENU_FILE_NAME_COLUMN, menu,
				FSEQ_MENU_INDEX, prog_index,
				-1);

		{
		   GdkPixbuf    *pixbuf;

		   if (strcmp(geoxml_program_get_status(program), "unconfigured") == 0)
		      pixbuf = W.pixmaps.unconfigured_icon;
		   else if (strcmp(geoxml_program_get_status(program), "configured") == 0)
		      pixbuf = W.pixmaps.configured_icon;
		   else if (strcmp(geoxml_program_get_status(program), "disabled") == 0)
		      pixbuf = W.pixmaps.disabled_icon;
		   else
		      continue;

		   gtk_list_store_set (W.fseq_store, &piter,
				       FSEQ_STATUS_COLUMN, pixbuf, -1);
		}
	}
}

/*
 * Function: flow_load
 * Load a selected flow from file
 */
void
flow_load (void)
{

    GtkTreeIter       iter;
    GtkTreeSelection *selection;
    GtkTreeModel     *model;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

       gchar    *filename;
       GString  *path;

       /* Free previous flow */
       if (flow != NULL)
	  geoxml_document_free ( GEOXML_DOC(flow));
       gtk_list_store_clear (W.fseq_store);

       gtk_tree_model_get (GTK_TREE_MODEL (W.flow_store), &iter,
 			   FB_FILENAME, &filename,
 			   -1);

       data_fname(filename, &path);
       flow = flow_load_path (path->str);
       g_string_free(path, TRUE);

       if (flow != NULL) {
	  flow_add_programs_to_view (flow);
       }

       g_free(filename);
    }
    else if (flow != NULL) {
	gtk_list_store_clear(W.fseq_store);
	geoxml_document_free (GEOXML_DOC (flow));
	flow = NULL;
    }
}

/*
 * Function: flow_info_update
 * Update information shown about the selected flow
 */
void
flow_info_update (void){

   if (flow){

      /* Title in bold */
      {
	 char *markup;

	 markup = g_markup_printf_escaped ("<b>%s</b>", geoxml_document_get_title (GEOXML_DOC(flow)));
	 gtk_label_set_markup (GTK_LABEL (W.flow_info.title), markup);
	 g_free (markup);
      }

      /* Description in italic */
      {
	 char *markup;

	 markup = g_markup_printf_escaped ("<i>%s</i>", geoxml_document_get_description (GEOXML_DOC(flow)));
	 gtk_label_set_markup (GTK_LABEL (W.flow_info.description), markup);
	 g_free (markup);
      }

      /* I/O labels */
      {
	 char *markup;
	 markup = g_markup_printf_escaped ("<b>Input:</b>");
	 gtk_label_set_markup (GTK_LABEL (W.flow_info.input_label), markup);
	 g_free (markup);

	 markup = g_markup_printf_escaped ("<b>Output:</b>");
	 gtk_label_set_markup (GTK_LABEL (W.flow_info.output_label), markup);
	 g_free (markup);

	 markup = g_markup_printf_escaped ("<b>Error log:</b>");
	 gtk_label_set_markup (GTK_LABEL (W.flow_info.error_label), markup);
	 g_free (markup);
      }

      /* Input file */
      if (strlen(geoxml_flow_io_get_input(flow)) > 0)
	 gtk_label_set_text (GTK_LABEL (W.flow_info.input), geoxml_flow_io_get_input(flow));
      else
	 gtk_label_set_text (GTK_LABEL (W.flow_info.input), "(none)");


      /* Output file */
      if (strlen(geoxml_flow_io_get_output(flow)) > 0)
	 gtk_label_set_text (GTK_LABEL (W.flow_info.output), geoxml_flow_io_get_output(flow));
      else
	 gtk_label_set_text (GTK_LABEL (W.flow_info.output), "(none)");


      /* Error file */
      if (strlen(geoxml_flow_io_get_error(flow)) > 0)
	 gtk_label_set_text (GTK_LABEL (W.flow_info.error), geoxml_flow_io_get_error(flow));
      else
	 gtk_label_set_text (GTK_LABEL (W.flow_info.error), "(none)");


      /* Author and email */
      {
	 GString *str;

	 str = g_string_new(NULL);

	 g_string_printf(str, "%s <%s>",
			 geoxml_document_get_author (GEOXML_DOC(flow)),
			 geoxml_document_get_email  (GEOXML_DOC(flow)));

	 gtk_label_set_text (GTK_LABEL (W.flow_info.author), str->str);
	 g_string_free(str, TRUE);
      }

      /* Info button */
      g_object_set(W.flow_info.help, "sensitive", TRUE, NULL);

   }
   else{
      gtk_label_set_text( GTK_LABEL(W.flow_info.title), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.description), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.input_label), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.input), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.output_label), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.output), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.error_label), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.error), "");
      gtk_label_set_text( GTK_LABEL(W.flow_info.author), "");

      g_object_set(W.flow_info.help, "sensitive", FALSE, NULL);
   }
}

/*
 * Function: flow_new
 * Create a new flow
 */
void
flow_new     (GtkMenuItem *menuitem,
	      gpointer     user_data)
{
   GtkTreeSelection *selection;
   GtkTreeIter       iter;
   GtkTreeModel     *model;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));
   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

      gchar    *name;
      gchar    *lne_filename;

      gtk_tree_model_get ( GTK_TREE_MODEL(W.proj_line_store), &iter,
			   PL_NAME, &name,
			   PL_FILENAME, &lne_filename,
			   -1);

      if (gtk_tree_store_iter_depth(W.proj_line_store, &iter) < 1)
	 log_message(INTERFACE, no_line_selected_error, TRUE);
      else {
	 time_t     t;
	 struct tm *lt;
	 char      *flw_filename[21];
	 GString   *message;
	 GString   *lne_path;
	 GString   *flw_path;
	 static const char *title = "New Flow";

	 /* some feedback */
	 message = g_string_new(NULL);
	 g_string_printf (message, "Adding flow to line %s", name);
	 log_message(INTERFACE, message->str, TRUE);
	 g_string_free(message, TRUE);

	 time (&t);
	 lt = localtime (&t);
	 strftime (flw_filename, STRMAX, "%Y_%m_%d", lt);
	 strcat (flw_filename, "_XXXXXX");
	 mktemp (flw_filename);
	 strcat (flw_filename, ".flw");

	 data_fname(lne_filename, &lne_path);
	 data_fname(flw_filename, &flw_path);

	 /* Add flow to the line */
	 GeoXmlLine * line;
	 line = line_load(lne_path->str);
	 if (line == NULL) {
	    printf("FIXME: %s:%d", __FILE__, __LINE__);
	    g_string_free(lne_path, TRUE);
	    g_string_free(flw_path, TRUE);
	    goto out;
	 }
	 geoxml_line_add_flow (line, flw_filename);
	 geoxml_document_save (GEOXML_DOC(line), lne_path->str);
	 geoxml_document_free (GEOXML_DOC(line));

	 /* Create a new flow */
	 GeoXmlFlow * f;
	 f = geoxml_flow_new ();
	 geoxml_document_set_filename (GEOXML_DOC(f), flw_filename);
	 geoxml_document_set_title (GEOXML_DOC(f), title );
	 geoxml_document_set_author (GEOXML_DOC(f), W.pref.username_value->str);
	 geoxml_document_set_email (GEOXML_DOC(f), W.pref.email_value->str);
	 geoxml_document_save (GEOXML_DOC(f), flw_path->str);
	 geoxml_document_free (GEOXML_DOC(f));

	 /* Add to the GUI */
	 GtkTreeIter fiter;
	 gtk_list_store_append (W.flow_store, &fiter);
	 gtk_list_store_set (W.flow_store, &fiter,
			     FB_NAME, title,
			     FB_FILENAME, flw_filename,
			     -1);

	 g_string_free(flw_path, TRUE);
	 g_string_free(lne_path, TRUE);
      }

out:
      g_free(name);
      g_free(lne_filename);
   } else
      log_message(INTERFACE, "Select a line to which a flow will be added to", TRUE);
}

/*
 * Function: flow_delete
 * Delete the selected flow in flow browser
 *
 */
void
flow_delete     (GtkMenuItem *menuitem,
		 gpointer     user_data)
{
   GtkTreeSelection *selection;
   GtkTreeIter       iter, liter;
   GtkTreeModel     *model;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

      gchar *name, *flw_filename, *lne_filename;
      GString *lne_path;
      GString *flw_path;
      GString *message;

      gtk_tree_model_get ( GTK_TREE_MODEL(W.flow_store), &iter,
			   FB_NAME, &name,
			   FB_FILENAME, &flw_filename,
			   -1);

      /* Get the line filename */
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));
      gtk_tree_selection_get_selected (selection, &model, &liter);
      gtk_tree_model_get ( model, &liter,
			   PL_FILENAME, &lne_filename,
			   -1);

      /* Some feedback */
      message = g_string_new(NULL);
      g_string_printf (message, "Erasing flow %s", name);
      log_message(INTERFACE, message->str, TRUE);
      g_string_free(message, TRUE);

      data_fname(lne_filename, &lne_path);
      data_fname(flw_filename, &flw_path);

      /* Remove flow from its line */
      GeoXmlLine     *line;
      GeoXmlLineFlow *line_flow;
      line = line_load(lne_path->str);
      if (line == NULL) {
         printf("FIXME: %s:%d", __FILE__, __LINE__);
	 g_string_free(lne_path, TRUE);
	 g_string_free(flw_path, TRUE);
         goto out;
      }

      /* Seek and destroy */
      geoxml_line_get_flow(line, &line_flow, 0);
      while (line_flow != NULL) {
         if (g_ascii_strcasecmp(flw_filename, geoxml_line_get_flow_source(line_flow)) == 0) {
            geoxml_line_remove_flow(line, line_flow);
            geoxml_document_save(GEOXML_DOC(line), lne_path->str);
            break;
         }

	 geoxml_line_next_flow(&line_flow);
      }
      geoxml_document_free(GEOXML_DOC(line));

      /* Frees and delete flow from the disk */
      geoxml_document_free (GEOXML_DOC(flow));
      flow = NULL;
      unlink(flw_path->str);
      g_string_free(lne_path, TRUE);
      g_string_free(flw_path, TRUE);

      /* Finally, from the GUI */
      gtk_list_store_remove (GTK_LIST_STORE (W.flow_store), &iter);
      gtk_list_store_clear(W.fseq_store);
      flow_info_update();

out:
      g_free(name);
      g_free(flw_filename);
      g_free(lne_filename);
   } else
      log_message(INTERFACE, no_flow_selected_error, TRUE);
}

/*
 * Function: flow_rename
 * Rename a flow upon double click.
 */
void
flow_rename  (GtkCellRendererText *cell,
	      gchar               *path_string,
	      gchar               *new_text,
	      gpointer             user_data)
{

   GtkTreeIter iter;


   gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (W.flow_store),
 					&iter,
 					path_string);
   gtk_list_store_set (W.flow_store, &iter,
 		       FB_NAME, new_text,
 		       -1);

   /* Update XML */
   geoxml_document_set_title (GEOXML_DOC(flow), new_text);
   flow_save();
}

/*
 * Function: program_add_to_flow
 * Add selected menu to flow sequence
 *
 */
void
program_add_to_flow      (GtkButton *button,
			  gpointer user_data)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	if (flow == NULL) {
		log_message(INTERFACE, no_flow_selected_error, TRUE);
		return;
	}
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.menu_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_menu_selected_error, TRUE);
		return;
	}
	if (!gtk_tree_store_iter_depth(W.menu_store, &iter)) {
		log_message(INTERFACE, selected_menu_instead_error, TRUE);
		return;
	}

	gchar *				name;
	gchar *				menufn;
	GString *			fname;
	GeoXmlFlow *			menu;
	GeoXmlProgram *			program;
	GeoXmlProgramParameter *	program_parameter;

	gtk_tree_model_get ( GTK_TREE_MODEL (W.menu_store), &iter,
			      MENU_TITLE_COLUMN, &name,
			      MENU_FILE_NAME_COLUMN, &menufn,
			      -1);

	if (menus_fname(menufn, &fname) != EXIT_SUCCESS)
		goto out;
	menu = flow_load_path (fname->str);
	g_string_free(fname, TRUE);
	if (menu == NULL) {
		goto out;
	}

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	geoxml_flow_get_program(menu, &program, 0);
	while (program != NULL) {
		program_parameter = geoxml_program_get_first_parameter(program);
		while (program_parameter != NULL) {
			geoxml_program_parameter_set_value(program_parameter,
				geoxml_program_parameter_get_default(program_parameter));

			geoxml_program_parameter_next(&program_parameter);
		}

		geoxml_program_next(&program);
	}

	/* add it to the file and to the view */
	geoxml_flow_add_flow (flow, menu);
	flow_add_programs_to_view (menu);

	geoxml_document_free (GEOXML_DOC(menu));
	flow_save();

out:
	g_free(name);
	g_free(menufn);
}

/*
 * Function: program_remove_from_flow
 * Remove selected program from flow process
 */
void
program_remove_from_flow      (GtkButton *button,
			       gpointer user_data)
{

   GtkTreeIter       iter;
   GtkTreeSelection *selection;
   GtkTreeModel     *model;

     selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter)){

	 GeoXmlProgram *pg;
	 gulong         nprogram;
	 gchar         *node;

	 node = gtk_tree_model_get_string_from_iter(model, &iter);
	 nprogram = (gulong) atoi(node);
	 g_free(node);

	 geoxml_flow_get_program(flow, &pg, nprogram);
	 geoxml_flow_remove_program(flow, pg);

	 flow_save();

	 gtk_list_store_remove (GTK_LIST_STORE (W.fseq_store), &iter);

      }
      else
	 log_message(INTERFACE, no_program_selected_error, TRUE);
}

/*
 * Function: program_move_down
 * Move selected program down in the processing flow
 */
void
program_move_down    (GtkButton *button,
		      gpointer user_data)
{

   GtkTreeIter iter;
   GtkTreeSelection *selection;
   GtkTreeModel     *model;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter)){

	 GtkTreeIter next;
	 next = iter;

	 if (gtk_tree_model_iter_next ( GTK_TREE_MODEL (W.fseq_store), &next) == FALSE)
		return;

	 GeoXmlProgram *pg;
	 gulong         nprogram;
	 gchar         *node;

	 /* Get index */
	 node = gtk_tree_model_get_string_from_iter(model, &iter);
	 nprogram = (gulong) atoi(node);
	 g_free(node);

	 /* Update flow */
	 geoxml_flow_get_program(flow, &pg, nprogram);
	 geoxml_flow_move_program_down(flow, pg);
	 flow_save();

	 /* Update GUI */
         gtk_list_store_move_after (W.fseq_store, &iter, &next);
      }
      else
	 log_message(INTERFACE, no_program_selected_error, TRUE);
}

/*
 * Function: program_move_up
 * Move selected program up in the processing flow
 */
void
program_move_up    (GtkButton *button,
		    gpointer user_data)
{

      GtkTreeIter       iter;
      GtkTreeSelection *selection;
      GtkTreeModel     *model;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter)){

	 GtkTreePath *path_to_pre =  gtk_tree_model_get_path  ( GTK_TREE_MODEL (W.fseq_store),
								&iter);
	 if ( gtk_tree_path_prev (path_to_pre) ){

	    GtkTreeIter    pre;
	    GeoXmlProgram *pg;
	    gulong         nprogram;
	    gchar         *node;

	    /* Get the index. */
	    node = gtk_tree_model_get_string_from_iter(model, &iter);
	    nprogram = (gulong) atoi(node);
	    g_free(node);

	    /* XML change */
	    geoxml_flow_get_program(flow, &pg, nprogram);
	    geoxml_flow_move_program_up(flow, pg);
	    flow_save();

	    /* View change */
	    gtk_tree_model_get_iter (GTK_TREE_MODEL (W.fseq_store),
				     &pre, path_to_pre );

	    gtk_list_store_move_before (W.fseq_store, &iter, &pre);
	 }
	 gtk_tree_path_free(path_to_pre);
      }
      else
	 log_message(INTERFACE, no_program_selected_error, TRUE);
}

void
flow_properties (void)
{

   GtkTreeIter       iter;
   GtkTreeSelection *selection;
   GtkTreeModel     *model;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
   if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
	   log_message(INTERFACE, no_flow_selected_error, TRUE);
	   return;
   }

   GtkWidget *dialog;
   GtkWidget *table;
   GtkWidget *label;

   dialog = gtk_dialog_new_with_buttons ("Flow properties",
					 GTK_WINDOW(W.mainwin),
					 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					 GTK_STOCK_OK, GTK_RESPONSE_OK,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 NULL);

   W.flow_prop.win = GTK_WIDGET (dialog);

   g_signal_connect_swapped (dialog, "response",
                             G_CALLBACK (flow_properties_actions),
                             dialog);

   gtk_widget_set_size_request (dialog, 390, 260);

   g_signal_connect (GTK_OBJECT (dialog), "delete_event",
		     GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL );


   table = gtk_table_new (5, 2, FALSE);
   gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);

   /* Title */
   label = gtk_label_new ("Title");
   gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
   W.flow_prop.title = gtk_entry_new ();
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
   gtk_table_attach (GTK_TABLE (table), W.flow_prop.title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
   gtk_entry_set_text (GTK_ENTRY (W.flow_prop.title), geoxml_document_get_title ( GEOXML_DOC (flow)));

   /* Description */
   label = gtk_label_new ("Description");
   gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
   W.flow_prop.description = gtk_entry_new ();
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
   gtk_table_attach (GTK_TABLE (table), W.flow_prop.description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
   gtk_entry_set_text (GTK_ENTRY (W.flow_prop.description), geoxml_document_get_description ( GEOXML_DOC (flow)));

   /* Help */
   label = gtk_label_new ("Help");
   gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
   W.flow_prop.help = gtk_button_new_from_stock ( GTK_STOCK_EDIT );
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
   gtk_table_attach (GTK_TABLE (table), W.flow_prop.help, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
   g_signal_connect (GTK_OBJECT (W.flow_prop.help), "clicked",
		     GTK_SIGNAL_FUNC (help_edit), flow );

   /* Author */
   label = gtk_label_new ("Author");
   gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
   W.flow_prop.author = gtk_entry_new ();
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
   gtk_table_attach (GTK_TABLE (table), W.flow_prop.author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
   gtk_entry_set_text (GTK_ENTRY (W.flow_prop.author), geoxml_document_get_author ( GEOXML_DOC (flow)));

   /* User email */
   label = gtk_label_new ("Email");
   gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
   W.flow_prop.email = gtk_entry_new ();
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
   gtk_table_attach (GTK_TABLE (table), W.flow_prop.email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
   gtk_entry_set_text (GTK_ENTRY (W.flow_prop.email), geoxml_document_get_email ( GEOXML_DOC (flow)));

   gtk_widget_show_all (dialog);

   return;
}

/*
  Function: flow_properties_actions
  Take the appropriate action when the flow properties dialog emmits
  a response signal.
 */
void
flow_properties_actions                (GtkDialog *dialog,
					gint       arg1,
					gpointer   user_data)
{
    switch (arg1){
    case GTK_RESPONSE_OK:
	    geoxml_document_set_title (GEOXML_DOC (flow), gtk_entry_get_text ( GTK_ENTRY (W.flow_prop.title)));
	    geoxml_document_set_description (GEOXML_DOC (flow), gtk_entry_get_text ( GTK_ENTRY (W.flow_prop.description)));
	    geoxml_document_set_author (GEOXML_DOC (flow), gtk_entry_get_text ( GTK_ENTRY (W.flow_prop.author)));
	    geoxml_document_set_email (GEOXML_DOC (flow), gtk_entry_get_text ( GTK_ENTRY (W.flow_prop.email)));

	    /* Update flow title in flow_store */
	    {
	       GtkTreeIter iter;
	       GtkTreeSelection *selection;
	       GtkTreeModel     *model;

	       selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
	       if (gtk_tree_selection_get_selected (selection, &model, &iter)){

		  gtk_list_store_set (W.flow_store, &iter,
				      FB_NAME, geoxml_document_get_title (GEOXML_DOC (flow)),
				      -1);
	       }
	    }

	    flow_save ();
	    flow_info_update ();

       break;
    default:                  /* does nothing */
       break;
    }
    gtk_widget_destroy (GTK_WIDGET (W.flow_prop.win));
}

void
flow_io (void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel* 		model;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_selected_error, TRUE);
		return;
	}

	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons ("Flow input/output",
						GTK_WINDOW(W.mainwin),
						GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL);
	W.flow_io.win = GTK_WIDGET (dialog);
	gtk_widget_set_size_request (dialog, 390, 160);

	g_signal_connect_swapped (dialog, "response",
				G_CALLBACK (flow_io_actions),
				dialog);
	g_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL );

	table = gtk_table_new (5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);


	/* FIXME: change GTK_FILE_CHOOSER_ACTION_OPEN to GTK_FILE_CHOOSER_ACTION_SAVE */

	/* Input */
	label = gtk_label_new ("Input file");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.flow_io.input = save_widget_create();
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.flow_io.input.hbox, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL,  3, 3);
	gtk_entry_set_text (GTK_ENTRY (W.flow_io.input.entry), geoxml_flow_io_get_input (flow));
	gtk_widget_set_size_request (W.flow_io.input.hbox, 140, 30);

	/* Output */
	label = gtk_label_new ("Output file");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.flow_io.output = save_widget_create();
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.flow_io.output.hbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text (GTK_ENTRY (W.flow_io.output.entry), geoxml_flow_io_get_output (flow));
	gtk_widget_set_size_request (W.flow_io.output.hbox, 140, 30);

	/* Error */
	label = gtk_label_new ("Error log file");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.flow_io.error = save_widget_create();
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.flow_io.error.hbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text (GTK_ENTRY (W.flow_io.error.entry), geoxml_flow_io_get_error (flow));
	gtk_widget_set_size_request (W.flow_io.error.hbox, 140, 30);

	gtk_widget_show_all (dialog);

	return;
}

void
flow_io_actions			(GtkDialog *dialog,
				 gint       arg1,
				 gpointer   user_data)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:
		geoxml_flow_io_set_input (flow,
			gtk_entry_get_text (GTK_ENTRY (W.flow_io.input.entry)));
		geoxml_flow_io_set_output (flow,
			gtk_entry_get_text (GTK_ENTRY (W.flow_io.output.entry)));
		geoxml_flow_io_set_error (flow,
			gtk_entry_get_text (GTK_ENTRY (W.flow_io.error.entry)));

		flow_save ();
		flow_info_update ();
		break;
	default:                  /* does nothing */
		break;
	}

	gtk_widget_destroy (GTK_WIDGET (W.flow_io.win));
}

void
flow_run(void)
{
	GtkWidget *		dialog;
	GtkWidget *		view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;
	GtkTreeSelection *	selection;
	GtkTreeModel     *	model;
	GtkTreeIter		iter;
	GtkTreeIter		first_iter;
	gboolean		has_first;
	struct server *		server;

	/* check for a flow selected */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.flow_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_selected_error, TRUE);
		return;
	}

	has_first = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.server_store), &first_iter);
	if (!has_first) {
		GtkWidget *	dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(W.mainwin),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			"There is no servers available. Please add at least one at Configure->Server");
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));

		gtk_widget_destroy(dialog);

		return;
	}

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(W.server_store), NULL) == 1){
	   gtk_tree_model_get (GTK_TREE_MODEL(W.server_store), &first_iter,
			       SERVER_POINTER, &server,
			       -1);
	   server_run_flow(server);
	   return;
	}

	dialog = gtk_dialog_new_with_buttons("Choose server to run",
						GTK_WINDOW(W.mainwin),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						NULL );
	gtk_widget_set_size_request(dialog, 380, 300);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL (W.server_store));

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("Servers", renderer, NULL);
	gtk_tree_view_column_set_sort_column_id  (col, SERVER_ADDRESS);
	gtk_tree_view_column_set_sort_indicator  (col, TRUE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "text", SERVER_ADDRESS);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), view, TRUE, TRUE, 0);

	/* select the first server */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_select_iter(selection, &first_iter);

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter       	iter;

		gtk_tree_selection_get_selected(selection, &model, &iter);
		gtk_tree_model_get (GTK_TREE_MODEL(W.server_store), &iter,
				SERVER_POINTER, &server,
				-1);

		server_run_flow(server);

		break;
	}
	case GTK_RESPONSE_CANCEL:
		break;
	}

	gtk_widget_destroy(dialog);
}


void
flow_show_help                  (GtkButton *button,
				 gpointer   user_data)
{
   if (flow != NULL)
      show_help( (gchar*)geoxml_document_get_help(GEOXML_DOC(flow)),
		 "Flow help", (gchar*)geoxml_document_get_filename(GEOXML_DOC(flow)));
}
