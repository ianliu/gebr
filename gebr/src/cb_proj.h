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

#ifndef _GEBR_CB_PROJ_H_
#define _GEBR_CB_PROJ_H_

#include <gtk/gtk.h>
#include <geoxml.h>

GeoXmlProject *
project_load (gchar * path);

void
projects_refresh (void);

void
project_new     (GtkMenuItem *menuitem,
		 gpointer     user_data);

void
project_delete     (GtkMenuItem *menuitem,
		    gpointer     user_data);

void
proj_line_rename  (GtkCellRendererText *cell,
		   gchar               *path_string,
		   gchar               *new_text,
		   gpointer             user_data);

#endif //_GEBR_CB_PROJ_H_
