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

#include <gtk/gtk.h>

#include <libgebr/comm/protocol.h>
#include <libgebr/comm/server.h>

struct server {
	struct gebr_comm_server *	comm;

	GtkTreeIter			iter;

	/* last error showed on tooltip */
	GString *			last_error;
};

struct server *
server_new(const gchar * address, gboolean autoconnect);

void
server_free(struct server * server);

gboolean
server_find(struct server * server, GtkTreeIter * iter);

gboolean
server_find_address(const gchar * address, GtkTreeIter * iter);

#endif //__SERVER_H
