/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * parameters_configure_setup_ui:
 *
 * When the selected program is edited, a copy is created at the bottom of its
 * flow. That copy is modified instead the original flow and if the operation
 * is cancelled, ie the user press the Cancel button, that copy is deleted.
 * Otherwise, if the user confirm by pressing Ok, the original program is
 * deleted and the copy is moved into its position.
 */
GebrGuiProgramEdit *parameters_configure_setup_ui(void);

/**
 */
gboolean validate_selected_program(GError **error);

/**
 */
gboolean validate_program_iter(GtkTreeIter *iter, GError **error);

G_END_DECLS

#endif /* __PARAMETERS_H__ */
