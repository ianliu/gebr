/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __PARAMETERS_H
#define __PARAMETERS_H

#include <gtk/gtk.h>

#include <geoxml.h>

struct parameters_data {
	gboolean		is_group;
	GeoXmlParameters *	parameters;

	GtkWidget *		vbox;
};

GtkWidget *
parameters_create_ui(GeoXmlParameters * parameters, gboolean expanded);

void
parameters_add(GtkButton * button, struct parameters_data * parameters_data);

#endif //__PARAMETERS_H
