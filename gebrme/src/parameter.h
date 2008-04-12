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

#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml.h>
#include <gui/parameter.h>
#include <gui/valuesequenceedit.h>

#include "enumoptionedit.h"
struct group_parameters_data;

/* Persintant pointer of GeoXmlParameter. As
 * it may change (because of geoxml_parameter_set_type)
 * we must keep a container for it and share this container beetween signals.
 */
struct parameter_data {
	GeoXmlParameter *				parameter;
	GtkWidget *					frame;
	GtkWidget *					label;
	GtkWidget *					specific_table;

	/* parent parameters data (maybe be a group_parameters_data) */
	struct parameters_data *		 	parameters_data;
	/* for in-group parameter */
	GtkWidget *					radio_button;

	union {
		struct parameter_widget *		widget;
		struct {
			struct group_parameters_data *	parameters_data;
			GtkWidget *			exclusive_check_button;
		} group;
	} specific;
};

GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, struct parameters_data * parameters_data, gboolean expanded);

void
parameter_create_ui_type_specific(GtkWidget * table, struct parameter_data * data);

#endif //__PARAMETER_H
