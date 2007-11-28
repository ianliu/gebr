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

#ifndef _GEBR_UI_PROP_H_
#define _GEBR_UI_PROP_H_

#include <glib.h>
#include <gtk/gtk.h>

#define GTK_RESPONSE_DEFAULT GTK_RESPONSE_APPLY

void
progpar_config_window    (void);

GtkWidget *
progpar_add_input_float    (GtkWidget    *wg,
			    gchar * label_str,
			    gboolean     required  );

GtkWidget *
progpar_add_input_int      (GtkWidget    *wg,
			    gchar * label_str,
			    gboolean     required  );

GtkWidget *
progpar_add_input_string   (GtkWidget    *wg,
			    gchar * label_str,
			    gboolean     required  );

GtkWidget *
progpar_add_input_range    (GtkWidget    *wg,
			    gchar        *label_str,
			    gdouble       min,
			    gdouble       max,
			    gdouble       step,
			    gboolean      required );

GtkWidget *
progpar_add_input_flag     (GtkWidget    *wg,
			    gchar        *label_str);

GtkWidget *
progpar_add_input_file     (GtkWidget *		wg,
			    gchar *		label_str,
			    gboolean		is_directory,
			    gboolean		required,
			    const gchar *	path);

void
responde                (GtkDialog *dialog,
			 gint       arg1,
			 gpointer   user_data);

#endif //_GEBR_UI_PROP_H_
