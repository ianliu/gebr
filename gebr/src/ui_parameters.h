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

struct parameter_data {
	GeoXmlParameter *			parameter;
	union {
		/* program parameter */
		struct parameter_widget	*	widget;
		/* group */
		struct {
			GList *			parameters;
			GtkWidget *		vbox;
			GSList *		radio_group;
		} group;
	} data;
};

/*
 *
 */
struct ui_parameters {
	GtkWidget *					dialog;

	GeoXmlProgram *					program;
	int						program_index;
	GtkWidget *					root_vbox;
	GList *						to_free_list;
	GtkWidget *					deinstanciate_button;

	/* list of struct parameter_data */
	GList *						parameters;
};

struct ui_parameters *
parameters_configure_setup_ui(void);

#endif //__PARAMETERS_H
