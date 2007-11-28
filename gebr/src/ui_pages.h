/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 G\ufffdBR core team (http://gebr.sourceforge.net)
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

#ifndef _GEBR_UI_PAGES_H_
#define _GEBR_UI_PAGES_H_

#include <gtk/gtk.h>

void
add_project     (GtkNotebook    *notebook);

void
add_flow_browse (GtkNotebook    *notebook);

void
add_flow_edit   (GtkNotebook    *notebook);

#endif //_GEBR_UI_PAGES_H_
