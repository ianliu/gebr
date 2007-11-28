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

#ifndef _GEBR_CALLBACKS_H_
#define _GEBR_CALLBACKS_H_

#include <gtk/gtk.h>
#include <glib.h>

void
show_widget   (GtkWidget *widget,
	       GtkWidget *data   );
void
hide_widget   (GtkWidget *widget,
	       GtkWidget *data   );

void
data_fname (const char *document,
	    GString    **fname   );

void
file_browse                    (GtkButton  *button,
                                GtkWidget  *entry);
void
file_browse_ok                  (GtkButton  *button,
                                 GtkWidget  *entry);

void
pref_actions                (GtkDialog *dialog,
			     gint       arg1,
			     gpointer   user_data);

void
switch_menu     (GtkNotebook     *notebook,
		 GtkNotebookPage *page,
		 guint            page_num,
		 gpointer         user_data);

extern const char* browser[];

#endif //_GEBR_CALLBACKS_H_
