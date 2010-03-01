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

#ifndef __PARAMETERS_H
#define __PARAMETERS_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libgebr/gui/programedit.h>

struct ui_parameters {
	GtkWidget *dialog;
	struct gebr_gui_program_edit *program_edit;
};

/**
 * Assembly a dialog to configure the current selected program's parameters.
 *
 * \return The structure containing relevant data. It will be automatically freed when the dialog closes.
 */
struct ui_parameters *parameters_configure_setup_ui(void);

/**
 * Change all parameters' values from \p parameters to their default value.
 */
void parameters_reset_to_default(GebrGeoXmlParameters * parameters);

/**
 * This function checks if the selected program has a required parameter unfiled 
 * \ref parameter_required_is_unfiled
 */
gboolean parameters_check_has_required_unfiled(void);

/**
 * This function checks if the selected program has a required parameter unfiled 
 * \ref parameter_required_is_unfiled
 */
gboolean parameters_check_has_required_unfiled_for_iter(GtkTreeIter * iter);

#endif				//__PARAMETERS_H
