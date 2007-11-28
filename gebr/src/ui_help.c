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

#include "ui_help.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
/* #include <gtkmozembed.h> */

#include "gebr.h"
#include "callbacks.h"

void
show_help  (char *help,
	    char *title,
	    char *fname)
{

   GtkWidget *		dialog;
   GtkWidget *		scrolled_window;
   GtkWidget *		html;
   FILE *               htmlfp;
   GString *            htmlfname;
   GString *            url;
   GString *ghelp;

   if (help == NULL || strlen(help) < 1)
      return;

   dialog = gtk_dialog_new_with_buttons (title,
					 GTK_WINDOW(W.mainwin),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 NULL);
   g_signal_connect (GTK_OBJECT (dialog), "delete_event",
		     GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL );


   htmlfname = g_string_new("/tmp/");
   g_string_append(htmlfname, fname);
   g_string_append(htmlfname, ".html");

   /* Gambiarra */
   {
      ghelp = g_string_new(help);
      char *gebrcsspos;
      int pos;

      if ( (gebrcsspos = strstr(ghelp->str, "gebr.css")) != NULL){
	 pos = (gebrcsspos - ghelp->str)/sizeof(char);
	 g_string_erase(ghelp, pos, 8);
	 g_string_insert(ghelp, pos, "file://" GEBRDATADIR "/gebr.css");
      }
   }

   htmlfp = fopen(htmlfname->str,"w");
   if (htmlfp == NULL){
      /* Mensagem de erro */
      return;
   }
   fwrite(ghelp->str, sizeof(char), strlen(ghelp->str), htmlfp);
   fclose(htmlfp);
   g_string_free(ghelp, TRUE);

   /* Add file to list of files to be removed */
   W.tmpfiles = g_slist_append(W.tmpfiles, htmlfname->str);

   url = g_string_new("file://");
   g_string_append(url, htmlfname->str);

   /* Launch an external browser */
   {
      GString *cmdline;

      cmdline = g_string_new (W.config.browser_arg);
      g_string_append(cmdline, " ");
      g_string_append(cmdline, htmlfname->str);
      g_string_append(cmdline, " &");
      system(cmdline->str);
   }

   g_string_free(url, FALSE);
   g_string_free(htmlfname, FALSE);
}
