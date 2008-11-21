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

#ifndef __PROGRAM_H
#define __PROGRAM_H

#include <gtk/gtk.h>
#include <geoxml.h>

struct ui_program {
	GtkWidget *		widget;

	GtkListStore *		list_store;
	GtkWidget *		tree_view;

	struct ui_program_details {
		GtkWidget *	title_label;
		GtkWidget *	description_label;
	} details;
};

void
program_setup_ui(void);

void
program_load_menu(void);

void
program_new(void);

void
program_remove(void);

void
program_up(void);

void
program_down(void);

void
program_dialog_setup_ui(void);

#endif //__PROGRAM_H
