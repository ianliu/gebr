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

#ifndef __PARAMETER_H
#define __PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml.h>
#include <gui/gtkfileentry.h>

struct parameter_data;
struct parameter_ui_data;

GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, gboolean hidden);

void
parameter_create_ui_type_specific(GtkWidget * table, struct parameter_data * data);

void
parameter_add(GtkButton * button, GeoXmlProgram * program);

void
parameter_up(GtkButton * button, struct parameter_data * data);

void
parameter_down(GtkButton * button, struct parameter_data * data);

void
parameter_remove(GtkButton * button, struct parameter_data * data);

void
parameter_data_free(GtkObject * expander, struct parameter_data * data);

void
parameter_type_changed(GtkComboBox * combo, struct parameter_data * data);

void
parameter_required_changed(GtkToggleButton *togglebutton, struct parameter_data * data);

void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_default_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_flag_default_changed(GtkToggleButton * togglebutton, struct parameter_data * data);

void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data);

void
parameter_file_default_changed(GtkFileEntry * file_entry, struct parameter_data * data);

gboolean
parameter_range_default_changed(GtkSpinButton * spinbutton, struct parameter_data * data);

void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data);

void
parameter_uilabel_update(struct parameter_data * data);

#endif //__PARAMETER_H
