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

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Menubar entries */
enum {
	MENUBAR_PROJECT = 0,
	MENUBAR_LINE,
	MENUBAR_FLOW,
	MENUBAR_FLOW_COMPONENTS,
	MENUBAR_N
};

/* Accel Group entries */
/* The order here are similar to the notebook pages*/
enum {
    ACCEL_PROJECT_AND_LINE = 0,
    ACCEL_FLOW,
    ACCEL_FLOW_EDITION,
    ACCEL_JOB_CONTROL,
    ACCEL_GENERAL,
    ACCEL_STATUS,
    ACCEL_SERVER,
    ACCEL_N
};
void gebr_setup_ui(void);

G_END_DECLS
#endif				//__INTERFACE_H
