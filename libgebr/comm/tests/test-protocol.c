/*   libgebr - GêBR Library
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

#include <glib.h>

#include <gebr-comm-protocol.h>
#include <gebr-comm-protocol_p.h>

void test_comm_build_message()
{
	GString * message;
	struct gebr_comm_message_def msg_def = gebr_comm_message_def_create("FOO", TRUE, 2);

	message = gebr_comm_protocol_build_message(msg_def, 2, NULL, NULL);
	g_assert_cmpstr(message->str, ==, "FOO 5 0| 0|\n");

	message = gebr_comm_protocol_build_message(msg_def, 2, "teste1", "123");
	g_assert_cmpstr(message->str, ==, "FOO 14 6|teste1 3|123\n");
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/comm/protocol/build-message", test_comm_build_message);

	return g_test_run();
}
