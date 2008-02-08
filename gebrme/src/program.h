/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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
program_create_ui(GeoXmlProgram * program, gboolean hidden);

void
program_add(void);

void
program_remove(GtkMenuItem * menu_item, GeoXmlProgram * program);

GtkMenu *
program_popup_menu(GtkWidget * event_box, GeoXmlProgram * program);

GtkExpander *
program_previous(GtkExpander * program_expander, gint * position);

GtkExpander *
program_next(GtkExpander * program_expander, gint * position);

void
program_move_up(GtkMenuItem * menu_item, GeoXmlProgram * program);

void
program_move_down(GtkMenuItem * menu_item, GeoXmlProgram * program);

void
program_stdin_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

void
program_stdout_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

void
program_stderr_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program);

gboolean
program_summary_title_changed(GtkEntry * entry, GeoXmlProgram * program);

gboolean
program_summary_binary_changed(GtkEntry * entry, GeoXmlProgram * program);

gboolean
program_summary_desc_changed(GtkEntry * entry, GeoXmlProgram * program);

void
program_summary_help_view(GtkButton * button, GeoXmlProgram * program);

void
program_summary_help_edit(GtkButton * button, GeoXmlProgram * program);

gboolean
program_summary_url_changed(GtkEntry * entry, GeoXmlProgram * program);

#endif //__PROGRAM_H
