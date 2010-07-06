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
#include <glib-object.h>
#include <glib/gstdio.h>

#include <listensocket.h>
#include <channelsocket.h>
#include <streamsocket.h>
#include <socketaddress.h>
#include <socket.h>

GMainLoop *loop;

void test_comm_socket_tcpunix_channel()
{
	GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
	GByteArray *data_read = g_byte_array_new();

	/*fill with random data*/
	guchar data[10001001]; 
	for (int i = 0; i < sizeof(data); ++i)
		data[i] = g_random_int();

	GebrCommSocketAddress tcpaddress = gebr_comm_socket_address_ipv4_local(61234);
	GebrCommSocketAddress unixaddress = gebr_comm_socket_address_unix("myunixsocket");

	GebrCommListenSocket *unixsocket = gebr_comm_listen_socket_new();
	gebr_comm_listen_socket_listen(unixsocket, &unixaddress);
	void ready_read(GebrCommStreamSocket * newconn)
	{
		GByteArray *array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(newconn));
		g_byte_array_append(data_read, array->data, array->len);
		if (data_read->len == sizeof(data)) {
			g_assert(memcmp(data_read->data, data, sizeof(data)) == 0);
			g_main_loop_quit(loop);
		}
	}
	void new_connection(GebrCommListenSocket * unixsocket)
	{
		GebrCommStreamSocket *newconn = gebr_comm_listen_socket_get_next_pending_connection(unixsocket);
		g_signal_connect(newconn, "ready-read",
				 G_CALLBACK(ready_read), NULL);
	}
	g_signal_connect(unixsocket, "new-connection",
			 G_CALLBACK(new_connection), NULL);
	
	GebrCommChannelSocket *channel = gebr_comm_channel_socket_new();
	gebr_comm_channel_socket_start(channel, &tcpaddress, &unixaddress);

	GebrCommStreamSocket *tcp = gebr_comm_stream_socket_new();
	gebr_comm_stream_socket_connect(tcp, &tcpaddress, TRUE);
	GByteArray *array = g_byte_array_new();
	g_byte_array_append(array, data, sizeof(data));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(tcp), array);
	g_byte_array_free(array, TRUE);

	g_main_loop_run(loop);

	g_unlink("myunixsocket");
	g_checksum_free(checksum);
	g_byte_array_free(data_read, TRUE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);
	loop = g_main_loop_new(NULL, FALSE);
	g_type_init();
	g_test_add_func("/comm/socket/tcpunix-channel", test_comm_socket_tcpunix_channel);
	return g_test_run();
}
