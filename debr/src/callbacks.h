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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

void
do_navigation_bar_update(void);

void
on_new_activate(void);
void
on_copy_activate(void);
void
on_paste_activate(void);
void
on_quit_activate(void);

void
on_menu_new_activate(void);
void
on_menu_open_activate(void);
void
on_menu_save_activate(void);
void
on_menu_save_as_activate(void);
void
on_menu_save_all_activate(void);
void
on_menu_revert_activate(void);
void
on_menu_delete_activate(void);
void
on_menu_properties_activate(void);
void
on_menu_validate_activate(void);
void
on_menu_install_activate(void);
void
on_menu_close_activate(void);

void
on_program_new_activate(void);
void
on_program_delete_activate(void);
void
on_program_properties_activate(void);
void
on_program_preview_activate(void);
void
on_program_top_activate(void);
void
on_program_bottom_activate(void);
void
on_program_copy_activate(void);
void
on_program_paste_activate(void);

void
on_parameter_new_activate(void);
void
on_parameter_delete_activate(void);
void
on_parameter_properties_activate(void);
void
on_parameter_top_activate(void);
void
on_parameter_bottom_activate(void);
void
on_parameter_change_type_activate(void);
void
on_parameter_type_activate(GtkRadioAction * first_action);
void
on_parameter_copy_activate(void);
void
on_parameter_paste_activate(void);

void
on_validate_close_activate(void);
void
on_validate_clear_activate(void);

void
on_configure_preferences_activate(void);
void
on_help_contents_activate(void);
void
on_help_about_activate(void);


#endif //__CALLBACKS_H
