/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

void
on_project_new_activate(void);

void
on_project_delete_activate(void);

void
on_project_properties_activate(void);

void
on_project_refresh_activate(void);

void
on_line_new_activate(void);

void
on_line_delete_activate(void);

void
on_line_path_activate(void);

void
on_line_properties_activate(void);

void
on_flow_new_activate(void);

void
on_flow_import_activate(void);

void
on_flow_export_activate(void);

void
on_flow_export_as_menu_activate(void);

void
on_flow_delete_activate(void);

void
on_flow_properties_activate(void);

void
on_flow_io_activate(void);

void
on_flow_execute_activate(void);

void
on_flow_component_properties_activate(void);

void
on_flow_component_refresh_activate(void);

void
on_flow_component_status_activate(GtkRadioAction * action);

void
on_configure_preferences_activate(void);

void
on_configure_servers_activate(void);

void
on_help_about_activate(void);

#endif //__CALLBACKS_H
