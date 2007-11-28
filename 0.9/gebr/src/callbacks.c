/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/* File: callbacks.c
 * Main callbacks.
 */
#include "callbacks.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <misc/protocol.h>

#include "menus.h"
#include "gebr.h"
#include "interface.h"
#include "cb_proj.h"
#include "server.h"

extern struct ggopt arg;

/*
 * Function: show_widget
 * Call gtk_widget_show_all
 */
void
show_widget   (GtkWidget *widget,
	       GtkWidget *data)
{
   gtk_widget_show_all (data);
}

/*
 * Function: hide_widget
 * Call gtk_widget_hide
 */
void
hide_widget   (GtkWidget *widget,
	       GtkWidget *data)
{
   gtk_widget_hide (data);
}

/*
 * Function: data_fname
 * Prefix filename with data dir path
 */
void
data_fname (const char *document,
	    GString    **fname   )
{
   *fname = g_string_new(W.config.data_arg);
   g_string_append(*fname, "/");
   g_string_append(*fname, document);
}

/*
 * Function: file_browse
 * Pop-up file browse dialog.
 *
 * The file browse dialog is initialize with the
 * string in entry.
 *
 * Input:
 * button - button pressed to call this function
 * entry  - entry that should be modified
 */
void
file_browse                    (GtkButton  *button,
                                GtkWidget  *entry)
{
   gchar *filename;

   gtk_widget_show (W.filesel);

   W.fileselentry = entry;
   filename = (gchar *)gtk_entry_get_text (GTK_ENTRY (entry));
   if (strlen (filename) > 0){
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (W.filesel), filename);
   }
}

/*
 * Function: file_browse_ok
 * Retrive file selection.
 *
 * Store the selected filename in W.filesel and emmits
 * the signal activate.
 */
void
file_browse_ok                  (GtkButton  *button,
                                 GtkWidget  *entry)
{
   gtk_entry_set_text (GTK_ENTRY (W.fileselentry),
		       gtk_file_selection_get_filename
		       (GTK_FILE_SELECTION (W.filesel)));

   gtk_signal_emit_by_name (GTK_OBJECT (W.fileselentry), "activate");
}

/*
  Function: pref_actions
  Take the appropriate action when the parameter dialog emmits
  a response signal.
 */
void
pref_actions                (GtkDialog *dialog,
			     gint       arg1,
			     gpointer   user_data)
{

   switch (arg1){
   case GTK_RESPONSE_OK:
      /* Save preferences to file and the relod */
      g_string_assign(W.pref.username_value,
		      gtk_entry_get_text (GTK_ENTRY (W.pref.username)));
      g_string_assign(W.pref.email_value,
		      gtk_entry_get_text (GTK_ENTRY (W.pref.email)));
      g_string_assign(W.pref.usermenus_value,
		      gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (W.pref.usermenus)));
      g_string_assign(W.pref.data_value,
		      gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (W.pref.data)));
      g_string_assign(W.pref.editor_value,
		      gtk_entry_get_text (GTK_ENTRY (W.pref.editor)));
      g_string_assign(W.pref.browser_value,
		      gtk_combo_box_get_active_text(GTK_COMBO_BOX(W.pref.browser)));

      gebr_config_save();
      gebr_config_reload();
      break;
   case GTK_RESPONSE_CANCEL: /* does nothing */
   default:
      break;
   }

   gtk_widget_destroy(GTK_WIDGET(dialog));
   W.pref.win = NULL;
}



/*
 * Function: switch_menu
 * Hide/Show the corresponding menu to the selected page
 *
 */
void
switch_menu     (GtkNotebook     *notebook,
		 GtkNotebookPage *page,
		 guint            page_num,
		 gpointer         user_data)
{
   switch (page_num){
   case 0: /* Project page */
      g_object_set (W.menu[MENUBAR_PROJECT], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_LINE], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
      break;
   case 1: /* Flow browse page */
      g_object_set (W.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
      break;
   case 2: /* Flow edit page */
      g_object_set (W.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", TRUE, NULL);
      break;
   case 3: /* Job control page*/
      g_object_set (W.menu[MENUBAR_PROJECT], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_LINE], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW], "sensitive", FALSE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", FALSE, NULL);
      break;
   default: /* Anything else */
      g_object_set (W.menu[MENUBAR_PROJECT], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_LINE], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW], "sensitive", TRUE, NULL);
      g_object_set (W.menu[MENUBAR_FLOW_COMPONENTS], "sensitive", TRUE, NULL);
   }
}
