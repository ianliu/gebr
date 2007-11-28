/*   GÍBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef __SSH_H
#define __SSH_H

#include <glib.h>

#include "gprocess.h"

struct ssh_tunnel {
	GProcess *	process;
	guint16		port;
};

struct ssh_tunnel
ssh_tunnel_new(guint16 start_port, const gchar * destination, guint16 remote_port);

void
ssh_tunnel_free(struct ssh_tunnel ssh_tunnel);

#endif //__SSH_H
