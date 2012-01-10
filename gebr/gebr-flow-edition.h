/*
 * gebr-flow-edition.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_FLOW_EDITION_H__
#define __GEBR_FLOW_EDITION_H__

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include "gebr-maestro-server.h"

G_BEGIN_DECLS

/**
 * Flow sequence store fields
 */ 
enum {
	FSEQ_ICON_COLUMN = 0,
	FSEQ_TITLE_COLUMN,
	FSEQ_GEBR_GEOXML_POINTER,
	FSEQ_EDITABLE,
	FSEQ_ELLIPSIZE,
	FSEQ_SENSITIVE,
	FSEQ_TOOLTIP,
	FSEQ_NEVER_OPENED,
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
typedef struct _GebrFlowEdition GebrFlowEdition;
typedef struct _GebrFlowEditionPriv GebrFlowEditionPriv;

struct _GebrFlowEdition {
	GebrFlowEditionPriv *priv;

	GtkWidget *widget;

	GtkTreeIter input_iter;
	GtkTreeIter output_iter;
	GtkTreeIter error_iter;

	/* Sequence of programs of a flow */
	GtkListStore *fseq_store;
	GtkWidget *fseq_view;
	GtkCellRenderer *text_renderer;

	/* available system and user's menus */
	GtkWidget *menu_view;
	GtkTreeStore *menu_store;

	GtkWidget *nice_button_high;
	GtkWidget *nice_button_low;
};

/**
 * Assembly the flow edit ui_flow_edition->widget.
 *
 * @return The structure containing relevant data.
 */
GebrFlowEdition *flow_edition_setup_ui(void);

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
 * flow_edition_change_iter_status:
 * @status: new status for program
 * @iter: #GtkTreeIter pointing to a program
 *
 * Change the flow status for @iter.
 */
void flow_edition_change_iter_status(GebrGeoXmlProgramStatus status, GtkTreeIter *iter);

/**
 * Change the flow status when select the status from the "Flow Component" menu.
 */
void flow_edition_status_changed(guint status);

/**
 * flow_edition_revalidate_programs:
 */
void flow_edition_revalidate_programs(void);

/**
 * flow_add_program_sequence_to_view:
 * @program: A #GebrGeoXmlSequence of #GebrGeoXmlProgram to be added to the view.
 * @select_last: Whether to select the last program.
 *
 * Adds all programs in the sequence @program into the flow edition view.
 */
void flow_add_program_sequence_to_view(GebrGeoXmlSequence * program,
				       gboolean select_last,
				       gboolean never_opened);

/**
 * Checks if the first program has input entrance, the last one has output exit and if even one of then has error exit.
 * If one of this is false, so the respective component are made insensitive. 
 */
void flow_program_check_sensitiveness (void);

void flow_edition_set_run_widgets_sensitiveness(GebrFlowEdition *fe,
                                                gboolean sensitive,
                                                gboolean maestro_err);

void gebr_flow_edition_hide(GebrFlowEdition *self);

void gebr_flow_edition_show(GebrFlowEdition *self);

void gebr_flow_edition_select_queue(GebrFlowEdition *self);

void gebr_flow_edition_update_server(GebrFlowEdition *fe,
				     GebrMaestroServer *maestro);

const gchar *gebr_flow_edition_get_selected_queue(GebrFlowEdition *fe);

const gchar *gebr_flow_edition_get_selected_server(GebrFlowEdition *fe);

void gebr_flow_edition_get_current_group(GebrFlowEdition *fe,
					 GebrMaestroServerGroupType *type,
					 gchar **name);

void gebr_flow_edition_get_server_hostname(GebrFlowEdition *fe,
                                           gchar **host);

void gebr_flow_edition_select_group_for_flow(GebrFlowEdition *fe,
					     GebrGeoXmlFlow *flow);

G_END_DECLS

#endif /* __GEBR_FLOW_EDITION_H__ */
