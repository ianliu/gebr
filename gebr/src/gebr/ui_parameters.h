/*   GeBR - An environment for seismic processing.
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

#include <glib.h>
#include <gtk/gtk.h>

#include <gui/parameter.h>

/*
 *
 */
struct ui_parameters {
	GtkWidget *		dialog;
	GtkWidget *		vbox;

	/* cloned program for editing */
	GeoXmlProgram *		program;
	/* original program index */
	int			program_index;
};

struct ui_parameters *
parameters_configure_setup_ui(void);

void
parameters_reset_to_default(GeoXmlParameters * parameters);

#endif //__PARAMETERS_H
