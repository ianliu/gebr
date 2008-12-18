/*   GeBR - An environment for seismic processing.
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

#ifndef __UI_FLOW_COMPONENT_H
#define __UI_FLOW_COMPONENT_H

#include <gtk/gtk.h>

/* Flow sequence store fields */
enum {
	FSEQ_STATUS_COLUMN = 0,
	FSEQ_TITLE_COLUMN,
	FSEQ_GEOXML_POINTER,
	FSEQ_MENU_FILE_NAME_COLUMN,
	FSEQ_MENU_INDEX,
	FSEQ_N_COLUMN
};

/* Menu store fields */
enum {
	MENU_TITLE_COLUMN = 0,
	MENU_DESC_COLUMN,
	MENU_FILE_NAME_COLUMN,
	MENU_N_COLUMN
};

struct ui_flow_edition {
	GtkWidget *	widget;

	/* Sequence of programs of a flow */
	GtkListStore *	fseq_store;
	GtkWidget *	fseq_view;

	/* available system and user's menus*/
	GtkWidget *	menu_view;
	GtkTreeStore *	menu_store;
};

struct ui_flow_edition *
flow_edition_setup_ui(void);

void
flow_edition_component_change_parameters(void);

void
flow_edition_set_status(GtkRadioAction * action);

#endif //__UI_FLOW_COMPONENT_H
