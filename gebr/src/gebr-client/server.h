/*   GeBR - An environment for seismic processing.
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

#include <glib.h>

#include <libgebr/comm/server.h>

struct server {
	struct gebr_comm_server *comm;

};

/**
 * Create a new server with \p address
 */
struct server *server_new(const gchar * address);

/**
 * Just free \p server structure and its gebr_comm_server
 * If \p server is connected the disconnect signal will cause \p server's jobs to be freed.
 */
void server_free(struct server *server);

#endif				//__SERVER_H
