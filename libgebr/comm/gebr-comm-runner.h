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
#include <libgebr/comm/gebr-comm-server.h>

G_BEGIN_DECLS

typedef struct _GebrCommRunner GebrCommRunner;
typedef struct _GebrCommRunnerPriv GebrCommRunnerPriv;

struct _GebrCommRunner {
	GebrCommRunnerPriv *priv;
};

/**
 * gebr_comm_runner_new:
 *
 * Returns: A newly created #GebrCommRunner.
 */
GebrCommRunner *gebr_comm_runner_new(GebrGeoXmlDocument *flow,
				     GList *servers,
				     const gchar *gid,
				     const gchar *parent_rid,
				     const gchar *speed,
				     const gchar *nice,
				     const gchar *group,
				     GebrValidator *validator);

/**
 * gebr_comm_runner_set_ran_func:
 *
 * Set @data to be called when this #GebrCommRunner finishes submitting its
 * job.
 */
void gebr_comm_runner_set_ran_func(GebrCommRunner *self,
				   void (*func) (GebrCommRunner *runner,
						 gpointer data),
				   gpointer data);

/**
 * gebr_comm_runner_free:
 */
void gebr_comm_runner_free(GebrCommRunner *self);

/**
 * gebr_comm_runner_run_async:
 * @self:
 */
void gebr_comm_runner_run_async(GebrCommRunner *self,
				const gchar *id);

GebrValidator *gebr_comm_runner_get_validator(GebrCommRunner *self);

const gchar *gebr_comm_runner_get_ncores(GebrCommRunner *self);

const gchar *gebr_comm_runner_get_servers_list(GebrCommRunner *self);

gint gebr_comm_runner_get_total(GebrCommRunner *self);

G_END_DECLS

#endif /*  __GEBR_COMM_RUNNER_H__ */
