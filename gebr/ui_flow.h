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

G_BEGIN_DECLS

/*
 * QueueTypes:
 * @AUTOMATIC_QUEUE: A queue created when a flow is executed
 * @USER_QUEUE: A queue created when the user chooses a name or when multiple
 * flows are executed.
 * @IMMEDIATELY_QUEUE: The immediately queue.
 */
typedef enum {
	AUTOMATIC_QUEUE,
	USER_QUEUE,
	IMMEDIATELY_QUEUE,
} GebrQueueTypes;

GebrQueueTypes gebr_get_queue_type(const gchar *queue_id);

/**
 * gebr_ui_flow_run:
 *
 * This method runs the selected flows according to the interface setup.
 */
void gebr_ui_flow_run(void);

G_END_DECLS

#endif /* __GEBR_UI_FLOW_H__ */
