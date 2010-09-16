/*   GeBR - An environment for seismic processing.
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
 * @file ui_flow_edition.c Interface functions and callbacks for the "Flow Edition" page.
 */

#ifndef __UI_FLOW_COMPONENT_H
#define __UI_FLOW_COMPONENT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * Flow sequence store fields
 */ 
enum {
	FSEQ_ICON_COLUMN = 0,
	FSEQ_TITLE_COLUMN,
	FSEQ_GEBR_GEOXML_POINTER,
	FSEQ_N_COLUMN
};

/**
 */
enum {
	FSEQ_PROGRAM_CONFIGURED,
	FSEQ_PROGRAM_UNCONFIGURED,
	FSEQ_PROGRAM_DISABLED,
};

/**
 * Menu store fields
 */
enum {
	MENU_TITLE_COLUMN = 0,
	MENU_DESC_COLUMN,
	MENU_FILEPATH_COLUMN,
	MENU_N_COLUMN
};

/**
 */
struct ui_flow_edition {
	GtkWidget *widget;
	GtkWidget *server_combobox;
	GtkWidget *queue_combobox;
	GtkBin *queue_bin;

	GtkTreeIter input_iter;
	GtkTreeIter output_iter;

	/* Sequence of programs of a flow */
	GtkListStore *fseq_store;
	GtkWidget *fseq_view;

	/* available system and user's menus */
	GtkWidget *menu_view;
	GtkTreeStore *menu_store;

};

/**
 * Assembly the flow edit ui_flow_edition->widget.
 *
 * @return The structure containing relevant data.
 */
struct ui_flow_edition *flow_edition_setup_ui(void);

/**
 * Load current flow's (gebr.flow) programs.
 */
void flow_edition_load_components(void);

/**
 * Return TRUE if there is a selected component (program) and put it into _iter_
 * If _warn_unselected_ is TRUE then a error message is displayed if the FALSE is returned
 */
gboolean flow_edition_get_selected_component(GtkTreeIter * iter, gboolean warn_unselected);

/**
 * Select \p iter and scroll to it.
 */
void flow_edition_select_component_iter(GtkTreeIter * iter);

/**
 * Set the XML IO into iterators
 */
void flow_edition_set_io(void);

/**
 * Show the current selected flow components parameters
 */
void flow_edition_component_activated(void);

gboolean flow_edition_component_key_pressed(GtkWidget *view, GdkEventKey *key);

/**
 * Change the flow status when select the status from the "Flow Component" menu.
 */
void flow_edition_status_changed(void);

/**
 * Update flow edition interface with information of the current selected server.
 */
void flow_edition_on_server_changed(void);



G_END_DECLS
#endif				//__UI_FLOW_COMPONENT_H
