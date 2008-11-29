/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/parameter.h>

enum {
	PARAMETER_TYPE,
	PARAMETER_KEYWORD,
	PARAMETER_LABEL,
	PARAMETER_XMLPOINTER,
	PARAMETER_N_COLUMN
};

struct ui_parameter {
	GtkWidget *			widget;

	GtkTreeStore *			tree_store;
	GtkWidget *			tree_view;
};

struct ui_parameter_dialog {
	GtkWidget *			dialog;

	GtkWidget *			default_widget_hbox;
	struct parameter_widget *	parameter_widget;
	GtkWidget *			separator_entry;
};

void
parameter_setup_ui(void);

void
parameter_load_program(void);

void
parameter_load_selected(void);

void
parameter_new(void);

void
parameter_remove(void);

void
parameter_duplicate(void);

void
parameter_up(void);

void
parameter_down(void);

void
parameter_change_type_setup_ui(void);

void
parameter_dialog_setup_ui(void);

#endif //__PARAMETER_H
