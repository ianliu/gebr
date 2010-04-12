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

/**
 * \file validate.h Validate API.
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
 */
struct validate {
	GtkWidget *widget;
	GtkWidget *text_view;

	GtkTreeIter iter;
	GtkTextBuffer *text_buffer;

	GebrGeoXmlFlow *menu;
	GtkTreeIter menu_iter;

	GebrGeoXmlValidate *geoxml_validate;
};

/**
 * Validate \p menu adding it to the validated list.
 * \param iter The item on the menu list.
 */
void validate_menu(GtkTreeIter * iter, GebrGeoXmlFlow * menu);

/**
 * Clear selected menu validations.
 */
void validate_close(void);

/**
 * Clear menu validation at \p iter.
 */
void validate_close_iter(GtkTreeIter *iter);

/**
 * Select \p iter.
 */
void validate_set_selected(GtkTreeIter * iter);

/**
 * Clear all the list of menus validations.
 */
void validate_clear(void);

#endif				//__VALIDATE_H
