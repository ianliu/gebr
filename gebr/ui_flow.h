/*
 * ui_flow.h
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

#ifndef __UI_FLOW_H__
#define __UI_FLOW_H__

#include <libgebr/geoxml/geoxml.h>
#include <gtk/gtk.h>

#define GEBR_TYPE_UI_FLOW		(gebr_ui_flow_get_type())
#define GEBR_UI_FLOW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_UI_FLOW, GebrUiFlow))
#define GEBR_UI_FLOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_UI_FLOW, GebrUiFlowClass))
#define GEBR_IS_UI_FLOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_UI_FLOW))
#define GEBR_IS_UI_FLOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_UI_FLOW))
#define GEBR_UI_FLOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_UI_FLOW, GebrUiFlowClass))

G_BEGIN_DECLS

typedef struct _GebrUiFlow GebrUiFlow;
typedef struct _GebrUiFlowPriv GebrUiFlowPriv;
typedef struct _GebrUiFlowClass GebrUiFlowClass;


struct _GebrUiFlow {
	GObject parent;
	GebrUiFlowPriv *priv;
};

struct _GebrUiFlowClass {
	GObjectClass parent_class;
};

GType gebr_ui_flow_get_type() G_GNUC_CONST;

GebrUiFlow *gebr_ui_flow_new(GebrGeoXmlFlow *flow,
                             GebrGeoXmlLineFlow *line_flow);

GebrGeoXmlFlow *gebr_ui_flow_get_flow(GebrUiFlow *ui_flow);

GebrGeoXmlLineFlow *gebr_ui_flow_get_line_flow(GebrUiFlow *ui_flow);

const gchar *gebr_ui_flow_get_filename(GebrUiFlow *ui_flow);

const gchar *gebr_ui_flow_get_last_modified(GebrUiFlow *ui_flow);

void gebr_ui_flow_set_last_modified(GebrUiFlow *ui_flow,
                                    const gchar *last_modified);

gboolean gebr_ui_flow_get_is_selected(GebrUiFlow *ui_flow);

void gebr_ui_flow_set_is_selected(GebrUiFlow *ui_flow,
                                  gboolean is_selected);

GtkMenu *gebr_ui_flow_popup_menu(GebrUiFlow *ui_flow,
                                 gboolean move_up,
                                 gboolean move_down);

G_END_DECLS

#endif /* __UI_FLOW_H__ */
