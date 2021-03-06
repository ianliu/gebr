/*   DeBR - GeBR Designer
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

/**
 * \file parametergroup.c Create dialog for editing a parameter of type group
 */

#ifndef __PARAMETER_GROUP_H
#define __PARAMETER_GROUP_H

#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

struct ui_parameter_group_dialog {
	GtkWidget *dialog;

	GtkWidget *instances_edit_vbox;

	GebrGeoXmlParameterGroup *parameter_group;
};

/**
 * Open a dialog to configure a group.
 */
gboolean parameter_group_dialog_setup_ui(gboolean new_parameter);

G_END_DECLS
#endif				//__PARAMETER_GROUP_H
