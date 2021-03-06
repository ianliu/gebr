/*
 * gebr-flow-edition.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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
	FSEQ_FILE_COLUMN,
	FSEQ_N_COLUMN
};

/**
 */
enum {
	FSEQ_PROGRAM_CONFIGURED,
	FSEQ_PROGRAM_UNCONFIGURED,
	FSEQ_PROGRAM_DISABLED,
};

typedef struct _GebrFlowEdition GebrFlowEdition;
typedef struct _GebrFlowEditionPriv GebrFlowEditionPriv;

struct _GebrFlowEdition {
	GtkTreeIter input_iter;
	GtkTreeIter output_iter;
	GtkTreeIter error_iter;

	/* Sequence of programs of a flow */
	GtkListStore *fseq_store;
	GtkWidget *fseq_view;
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *file_renderer;

	GtkWidget *speed_slider;
	GtkWidget *nice_button_high;
	GtkWidget *speed_button;
	GtkWidget *ruler;
};




/**
 * flow_edition_change_iter_status:
 * @status: new status for program
 * @iter: #GtkTreeIter pointing to a program
 *
 * Change the flow status for @iter.
 */
void flow_edition_change_iter_status(GebrGeoXmlProgramStatus status, GtkTreeIter *iter);

/**
 * Checks if the first program has input entrance, the last one has output exit and if even one of then has error exit.
 * If one of this is false, so the respective component are made insensitive. 
 */
void flow_program_check_sensitiveness (void);

void gebr_flow_edition_update_speed_slider_sensitiveness(GebrFlowEdition *fe);

G_END_DECLS

#endif /* __GEBR_FLOW_EDITION_H__ */
