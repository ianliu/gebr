/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#ifndef __PROGRAM_H
#define __PROGRAM_H

#include <gtk/gtk.h>
#include <geoxml.h>

void
program_add(void);

void
program_create_ui(GeoXmlProgram * program, gboolean hidden);

void
program_remove(GtkButton * button, GeoXmlProgram * program);

void
program_up(GtkButton * button, GeoXmlProgram * program);

void
program_down(GtkButton * button, GeoXmlProgram * program);

void
program_stdin_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

void
program_stdout_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

void
program_stderr_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

gboolean
program_info_title_changed(GtkEntry * entry, GeoXmlProgram * program);

gboolean
program_info_binary_changed(GtkEntry * entry, GeoXmlProgram * program);

gboolean
program_info_desc_changed(GtkEntry * entry, GeoXmlProgram * program);

void
program_info_help_view(GtkButton * button, GeoXmlProgram * program);

void
program_info_help_edit(GtkButton * button, GeoXmlProgram * program);

#endif //__PROGRAM_H
