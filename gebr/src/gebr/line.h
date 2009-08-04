/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __LINE_H
#define __LINE_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>

gboolean
line_new(void);
gboolean
line_delete(gboolean confirm);
GeoXmlLine *
line_import(const gchar * line_filename, const gchar * at_dir);

GtkTreeIter
line_append_flow(GeoXmlLineFlow * line_flow);
void
line_load_flows(void);

void
line_move_flow_top(void);
void
line_move_flow_bottom(void);

#endif //__LINE_H
