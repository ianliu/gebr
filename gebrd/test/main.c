/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "fakeclient.h"

int
main(int argc, char ** argv)
{
	GMainContext *	main_context;

	main_context = g_main_context_new();
	fake_client.main_loop = g_main_loop_new(NULL, TRUE);
	g_type_init();

	/* for testing */
	g_print("Fake client connecting...\n");
	fake_client_connect_to_server();
	g_main_loop_run(fake_client.main_loop);

	return 0;
}
