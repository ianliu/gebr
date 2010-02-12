/**
 * \file validate.h Validate API.
 */

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

/**
 * Columns for validate #GtkListStore model.
 */
enum {
	VALIDATE_ICON = 0,
	VALIDATE_FILENAME,
	VALIDATE_POINTER,
	VALIDATE_N_COLUMN
};

/**
 * Structure for validate's user interface.
 */
struct ui_validate {
	GtkWidget *widget;

	GtkListStore *list_store;
	GtkWidget *tree_view;

	GtkWidget *text_view_vbox;
};

/**
 * Assembly the job control page.
 *
 * \return The structure containing relevant data.
 */
void validate_setup_ui(void);

/**
 * Validate \p menu adding it to the validated list.
 * \param iter The item on the menu list.
 */
void validate_menu(GtkTreeIter * iter, GebrGeoXmlFlow * menu);

/**
 * Clear selecteds validated menus.
 */
void validate_close(void);

/**
 * Clear all the list of validated menus.
 */
void validate_clear(void);

#endif				//__VALIDATE_H
