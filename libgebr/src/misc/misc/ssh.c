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

#include <stdlib.h>

#include "ssh.h"
#include "gtcpserver.h"

struct ssh_tunnel
ssh_tunnel_new(guint16 start_port, const gchar * destination, guint16 remote_port)
{
	struct ssh_tunnel	ssh_tunnel;
	GString *		cmd_line;

	/* define the port available for tunneling */
	ssh_tunnel.port = start_port;
	while (!g_tcp_server_is_local_port_available(ssh_tunnel.port))
		++ssh_tunnel.port;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "bash -c \"ssh -f -L %d:127.0.0.1:%d %s sleep 999d\"", ssh_tunnel.port, remote_port, destination);

// 	ssh_tunnel.process = g_process_new();
// 	g_process_start(ssh_tunnel.process, cmd_line);
	system(cmd_line->str);

	/* frees */
	g_string_free(cmd_line, TRUE);

	return ssh_tunnel;
}

void
ssh_tunnel_free(struct ssh_tunnel ssh_tunnel)
{
// 	g_process_kill(ssh_tunnel.process);
// 	g_process_free(ssh_tunnel.process);
}

struct ssh_tunnel
ssh_tunnel_x11_new(const gchar * destination, guint16 remote_port)
{
	struct ssh_tunnel	ssh_tunnel;
	GString *		cmd_line;

	/* define the port available for tunneling */
	ssh_tunnel.port = 6000;
	while (!g_tcp_server_is_local_port_available(ssh_tunnel.port))
		++ssh_tunnel.port;

	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "bash -c \"ssh -f -R %d:127.0.0.1:%d %s sleep 999d\"", ssh_tunnel.port, remote_port, destination);

// 	ssh_tunnel.process = g_process_new();
// 	g_process_start(ssh_tunnel.process, cmd_line);
	system(cmd_line->str);

	/* frees */
	g_string_free(cmd_line, TRUE);

	return ssh_tunnel;
}

void
ssh_tunnel_x11_free(struct ssh_tunnel ssh_tunnel)
{
	ssh_tunnel_free(ssh_tunnel);
}
