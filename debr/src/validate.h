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

#ifndef __VALIDATE_H
#define __VALIDATE_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>

enum {
	VALIDATE_ICON = 0,
	VALIDATE_FILENAME,
	VALIDATE_POINTER,
	VALIDATE_N_COLUMN
};

struct ui_validate {
	GtkWidget *widget;

	GtkListStore *list_store;
	GtkWidget *tree_view;

	GtkWidget *text_view_vbox;
};

void validate_setup_ui(void);

void validate_menu(GtkTreeIter * iter, GebrGeoXmlFlow * menu);

void validate_close(void);

void validate_clear(void);

#endif				//__VALIDATE_H
