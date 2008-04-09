/*   GeBR Daemon - Process and control execution of flows
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

#ifndef __JOB_H
#define __JOB_H

#include <geoxml.h>
#include <comm/gprocess.h>

#include "client.h"

struct job {
	GProcess *	process;
	GeoXmlFlow *	flow;
	gboolean	user_finished;

	/* the hostname of the client that ran it */
	GString *	hostname;
	GString *	status;
	GString *	jid;

	GString *	title;
	GString *	start_date;
	GString *	finish_date;
	GString *	issues;
	GString *	cmd_line;
	GString *	output;
};

gboolean
job_new(struct job ** _job, struct client * client, GString * xml);

void
job_free(struct job * job);

void
job_run_flow(struct job * job, struct client * client);

struct job *
job_find(GString * jid);

void
job_clear(struct job * job);

void
job_end(struct job * job);

void
job_kill(struct job * job);

void
job_list(struct client * client);

void
job_send_clients_job_notify(struct job * job);

#endif //__JOB_H
