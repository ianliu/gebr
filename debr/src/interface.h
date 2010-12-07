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
 * \file interface.c Interface creation: mainwindow, actions and callbacks assignmment.
 * \see callbacks.c
 */

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Accel Group entries */
/* The order here are similar to the notebook pages*/
enum {
    ACCEL_MENU = 0,
    ACCEL_PROGRAM,
    ACCEL_PARAMETER,
    ACCEL_VALIDATE,
    ACCEL_GENERAL,
    ACCEL_COMMON,
    ACCEL_PARAMETER_CHANGE_TYPE,
    ACCEL_N
};
/**
 * Creates the main window for DeBR and initialize all structures.
 */
void debr_setup_ui(void);

/**
 * Sets the actions sensitiveness given in the list \p names to \p sensitive.
 *
 * \param names A #NULL terminated array of strings, containing the actions names.
 * \param sensitive Whether to make the actions sensitive or not.
 */
void debr_set_actions_sensitive(gchar ** names, gboolean sensitive);

G_END_DECLS
#endif				//__INTERFACE_H
