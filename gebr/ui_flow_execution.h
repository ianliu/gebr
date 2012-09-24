/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_UI_FLOW_H__
#define __GEBR_UI_FLOW_H__

#include <glib.h>
#include <libgebr/geoxml/geoxml.h>

#include "gebr-maestro-server.h"

#define GEBR_TYPE_UI_FLOW_EXECUTION		(gebr_ui_flow_execution_get_type())
#define GEBR_UI_FLOW_EXECUTION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_UI_FLOW_EXECUTION, GebrUiFlowExecution))
#define GEBR_UI_FLOW_EXECUTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_UI_FLOW_EXECUTION, GebrUiFlowExecutionClass))
#define GEBR_IS_UI_FLOW_EXECUTION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_UI_FLOW_EXECUTION))
#define GEBR_IS_UI_FLOW_EXECUTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_UI_FLOW_EXECUTION))
#define GEBR_UI_FLOW_EXECUTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_UI_FLOW_EXECUTION, GebrUiFlowExecutionClass))

G_BEGIN_DECLS

typedef struct _GebrUiFlowExecution GebrUiFlowExecution;
typedef struct _GebrUiFlowExecutionPriv GebrUiFlowExecutionPriv;
typedef struct _GebrUiFlowExecutionClass GebrUiFlowExecutionClass;


struct _GebrUiFlowExecution {
	GObject parent;
	GebrUiFlowExecutionPriv *priv;
};

struct _GebrUiFlowExecutionClass {
	GObjectClass parent_class;
};

/**
 * gebr_ui_flow_run:
 *
 * This method runs the selected flows according to the interface setup.
 */
void gebr_ui_flow_run(GebrUiFlowExecution *ui_flow_execution,
                      gboolean is_parallel,
                      gboolean is_detailed);

void gebr_ui_flow_run_snapshots(GebrUiFlowExecution *ui_flow_execution,
                                GebrGeoXmlFlow *flow,
                                const gchar *snapshots,
                                gboolean is_parallel,
                                gboolean is_detailed);

void gebr_ui_flow_update_prog_mpi_nprocess(GebrGeoXmlProgram *prog,
                                           GebrMaestroServer *maestro,
                                           gdouble speed,
                                           const gchar *group_name,
                                           GebrMaestroServerGroupType group_type);

/*
 * gebr_ui_flow_execution_details_setup_ui:
 *
 * Create the interface of the execution details
 * */
GebrUiFlowExecution *gebr_ui_flow_execution_details_setup_ui(gboolean slider_sensitiviness,
							     gboolean multiple);

gdouble gebr_ui_flow_execution_calculate_speed_from_slider_value(gdouble x);

gdouble gebr_ui_flow_execution_calculate_slider_from_speed(gdouble speed);

void gebr_ui_flow_execution_get_current_group(GtkComboBox *combo,
					      GebrMaestroServerGroupType *type,
					      gchar **name,
					      GebrMaestroServer *maestro);

const gchar *gebr_ui_flow_execution_get_selected_queue(GtkComboBox *combo,
						       GebrMaestroServer *maestro);

void gebr_ui_flow_execution_get_server_hostname(GtkComboBox *combo,
					   GebrMaestroServer *maestro,
					   gchar **host);

const gchar *gebr_ui_flow_execution_set_text_for_performance(gdouble value);

G_END_DECLS

#endif /* __GEBR_UI_FLOW_H__ */
