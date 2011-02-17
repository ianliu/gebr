/*   GeBR Daemon - Process and control execution of flows
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

#ifndef __SERVER_H
#define __SERVER_H

#include "client.h"

G_BEGIN_DECLS

/**
 * Init daemon log file and run lock file.
 */
gboolean server_init(void);

/**
 * Free data allocated for clients.
 */
void server_free(void);

/**
 * Closes log file and delete run lock.
 */
void server_quit(void);

/**
 * Callback to #GebrCommListenSocket's new-connection signal.
 */
void server_new_connection(void);

/**
 * Switch messages received from \p client.
 * @return #TRUE if parsing went ok, #FALSE otherwise.
 */
gboolean server_parse_client_messages(struct client *client);

G_END_DECLS
#endif				//__SERVER_H
