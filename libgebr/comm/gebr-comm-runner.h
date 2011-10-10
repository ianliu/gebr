/*   libgebr - GeBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_COMM_RUNNER_H__
#define __GEBR_COMM_RUNNER_H__

#include <glib.h>
#include <libgebr/gebr-validator.h>

G_BEGIN_DECLS

typedef struct {
	GebrGeoXmlFlow *flow;
	gchar *flow_xml;
	guint run_id;
} GebrCommRunnerFlow;

/**
 * GebrCommRunner:
 * @flows: use gebr_comm_runner_add_flow() to add a flow
 * @parallel: whether this will be executed in a parallel environment
 * @account: account for moab servers
 * @queue: the queue this flow will be appended
 * @num_processes: the number of processes to run in parallel
 * @execution_speed: the absolute value means the number of cores, while its
 * sign means niceness 0 (+) and niceness 19 (-).
 */
typedef struct {
	/*< private >*/
	GList *flows;
	GList *servers;
	gboolean parallel;
	gchar *account;
	gchar *queue;
	gchar *num_processes;
	gchar *execution_speed;
} GebrCommRunner;

/**
 * gebr_comm_runner_new:
 *
 * Returns: A newly created #GebrCommRunner. See 
 */
GebrCommRunner *gebr_comm_runner_new(void);

/**
 * gebr_comm_runner_free:
 */
void gebr_comm_runner_free(GebrCommRunner *self);

/**
 * gebr_comm_runner_add_flow:
 */
guint gebr_comm_runner_add_flow(GebrCommRunner *self,
				GebrValidator  *vaildator,
				GebrGeoXmlFlow *flow);

/**
 * gebr_comm_runner_run:
 * @self:
 */
void gebr_comm_runner_run(GebrCommRunner *self);

G_END_DECLS

#endif /*  __GEBR_COMM_RUNNER_H__ */