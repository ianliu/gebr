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

/* File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preference and about
 * dialogs. All other subcomponents of the interface are implemented
 * in the files initiated by "ui_".
 */
#include <string.h>
#include "ui_server.h"
#include "cb_server.h"

#include "gebr.h"

/*------------------------------------------------------------------------*
 * Function: assembly_preference_win
 * Assembly preference window.
 *
 */
void
assembly_server_win (void){

   GtkWidget *dialog;
   GtkWidget *label;
   GtkWidget *entry;
   GtkTreeViewColumn *col;
   GtkCellRenderer *renderer;


   dialog = gtk_dialog_new_with_buttons ("Server",
					 GTK_WINDOW(W.mainwin),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_REMOVE, GEBR_SERVER_REMOVE,
					 GTK_STOCK_CLOSE, GEBR_SERVER_CLOSE,
					 NULL );

   /* Take the apropriate action when a button is pressed */
   g_signal_connect_swapped (dialog, "response",
			     G_CALLBACK (server_dialog_actions),
			     dialog);

   gtk_widget_set_size_request (dialog, 380, 300);

   label = gtk_label_new( "Remote server hostname:" );
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, TRUE, 0);

   entry = gtk_entry_new();
   g_signal_connect (GTK_OBJECT(entry), "activate", 
		     GTK_SIGNAL_FUNC(server_add), NULL);

   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, FALSE, TRUE, 0);

   W.server_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (W.server_store));

   renderer = gtk_cell_renderer_text_new ();
   col = gtk_tree_view_column_new_with_attributes ("Servers", renderer, NULL);
   gtk_tree_view_column_set_sort_column_id  (col, SERVER_ADDRESS);
   gtk_tree_view_column_set_sort_indicator  (col, TRUE);

   gtk_tree_view_append_column (GTK_TREE_VIEW (W.server_view), col);
   gtk_tree_view_column_add_attribute (col, renderer, "text", SERVER_ADDRESS);

   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), W.server_view, TRUE, TRUE, 0);


   gtk_widget_show_all(dialog);
   gtk_dialog_run(GTK_DIALOG(dialog));

}
