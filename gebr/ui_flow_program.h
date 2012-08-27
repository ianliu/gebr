/*
 * ui_flow_program.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Team
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

#ifndef __GEBR_UI_FLOW_PROGRAM_H__
#define  __GEBR_UI_FLOW_PROGRAM_H__

#include <glib-object.h>
#include <libgebr/geoxml/geoxml.h>
#include <gtk/gtk.h>

#define GEBR_TYPE_UI_FLOW_PROGRAM            (gebr_ui_flow_program_get_type())
#define GEBR_UI_FLOW_PROGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_TYPE_UI_FLOW_PROGRAM, GebrUiFlowProgram))
#define GEBR_UI_FLOW_PROGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GEBR_TYPE_UI_FLOW_PROGRAM, GebrUiFlowProgramClass))
#define GEBR_IS_UI_FLOW_PROGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_TYPE_UI_FLOW_PROGRAM))
#define GEBR_UI_FLOW_PROGRAM_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GEBR_TYPE_UI_FLOW_PROGRAM))
#define GEBR_UI_FLOW_PROGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GEBR_TYPE_UI_FLOW_PROGRAM, GebrUiFlowProgramClass))

G_BEGIN_DECLS

typedef struct _GebrUiFlowProgram GebrUiFlowProgram;
typedef struct _GebrUiFlowProgramPriv GebrUiFlowProgramPriv;
typedef struct _GebrUiFlowProgramClass GebrUiFlowProgramClass;

struct _GebrUiFlowProgram {
	GObject parent;
	GebrUiFlowProgramPriv *priv;
};

struct _GebrUiFlowProgramClass {
	GObjectClass parent_class;
};

/*----------------------------------------------------------------------------------------------*/

GebrUiFlowProgram *gebr_ui_flow_program_new(GebrGeoXmlProgram *program);

void gebr_ui_flow_program_set_xml (GebrUiFlowProgram *program, GebrGeoXmlProgram *prog_xml);

GebrGeoXmlProgram *gebr_ui_flow_program_get_xml (GebrUiFlowProgram *program);

void gebr_ui_flow_program_set_flag_opened (GebrUiFlowProgram *program, gboolean is_never_opened);

gboolean gebr_ui_flow_program_get_flag_opened (GebrUiFlowProgram *program);

void gebr_ui_flow_program_set_status (GebrUiFlowProgram *program, GebrGeoXmlProgramStatus status);

GebrGeoXmlProgramStatus gebr_ui_flow_program_get_status (GebrUiFlowProgram *program);

void gebr_ui_flow_program_set_error_id (GebrUiFlowProgram *program, GebrIExprError error_id);

GebrIExprError gebr_ui_flow_program_get_error_id (GebrUiFlowProgram *program);

const gchar *gebr_ui_flow_program_get_tooltip(GebrUiFlowProgram *program);

void gebr_ui_flow_program_update_tooltip(GebrUiFlowProgram *program);

GtkMenu *gebr_ui_flow_program_popup_menu(GebrUiFlowProgram *program,
					 gboolean can_move_up,
					 gboolean can_move_down);
G_END_DECLS

#endif /* __GEBR_UI_FLOW_PROGRAM_H__ */
