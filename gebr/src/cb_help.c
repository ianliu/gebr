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

/* File: cb_help.c
 * Callbacks for the help manipulation
 */
#include "cb_help.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

#include "gebr.h"
#include "callbacks.h"

void        help_edit                  (GtkButton *button,
					gpointer   user_data)
{
   FILE *fp;

   gchar random[7];
   GString *tmpfn;

   gchar *buffer;
   GString *help;
   GString *cmdline;

   GeoXmlDocument *doc = (GeoXmlDocument *) user_data;

   /* Call an editor */
   if (!W.config.editor_given){
      log_message(ERROR, "No editor defined", TRUE);
      return;
   }

   /* Temporary file */
   tmpfn = g_string_new("/tmp/gebr_");

   sprintf (random, "XXXXXX");
   mktemp (random);

   g_string_append(tmpfn, random);
   g_string_append(tmpfn, ".html");

   /* Write current help to temporary file */
   fp = fopen(tmpfn->str, "w");

   buffer = geoxml_document_get_help(doc);
   if ((buffer != NULL) && (strlen(buffer)>1) )
      fputs(buffer, fp);
   fclose(fp);

   cmdline = g_string_new(NULL);
   g_string_printf(cmdline, "%s %s", W.pref.editor_value->str, tmpfn->str);
   system(cmdline->str);
   g_string_free(cmdline, TRUE);

   /* Read back the help from file */
   fp = fopen(tmpfn->str, "r");
   buffer = malloc (sizeof(char) * STRMAX);
   help = g_string_new(NULL);

   if (fgets(buffer, STRMAX, fp)!=NULL)
      g_string_append(help, buffer);

   while (!feof(fp)){
      if (fgets(buffer, STRMAX, fp) != NULL)
	 g_string_append(help, buffer);
   }

   free(buffer);
   fclose(fp);

   geoxml_document_set_help(doc, help->str);

   unlink(tmpfn->str);
   g_string_free(tmpfn, TRUE);
   g_string_free(help, TRUE);

}
