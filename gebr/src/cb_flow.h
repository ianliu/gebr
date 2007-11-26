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

#ifndef _GEBR_CB_FLOW_H_
#define _GEBR_CB_FLOW_H_

#include <gtk/gtk.h>
#include <geoxml.h>


GeoXmlFlow *
flow_load_path (gchar * path);

void
flow_load (void);

void
flow_info_update (void);

int
flow_save   (void);

void
flow_export        (GtkMenuItem *menuitem,
		    gpointer     user_data);

void
flow_new     (GtkMenuItem *menuitem,
	      gpointer     user_data);

void
flow_delete     (GtkMenuItem *menuitem,
		 gpointer     user_data);

void
flow_rename  (GtkCellRendererText *cell,
	      gchar               *path_string,
	      gchar               *new_text,
	      gpointer             user_data);

void
program_add_to_flow      (GtkButton *button,
			  gpointer user_data);

void
program_remove_from_flow      (GtkButton *button,
			       gpointer user_data);
void
program_move_down    (GtkButton *button,
		      gpointer user_data);
void
program_move_up    (GtkButton *button,
		    gpointer user_data);

void
flow_properties (void);

void
flow_properties_actions		(GtkDialog *dialog,
				 gint       arg1,
				 gpointer   user_data);

void
flow_io (void);

void
flow_io_actions			(GtkDialog *dialog,
				 gint       arg1,
				 gpointer   user_data);

void
flow_run(void);

void
flow_show_help                  (GtkButton *button,
				 gpointer   user_data);

#endif //_GEBR_CB_FLOW_H_
