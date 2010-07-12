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
	GByteArray *data_read1 = g_byte_array_new();
	GByteArray *data_read2 = g_byte_array_new();
	gboolean finished_read1 = FALSE, finished_read2 = FALSE;

	/*fill with random data*/
	guchar data[4001001]; 
	for (int i = 0; i < sizeof(data); ++i)
		data[i] = g_random_int();

	GebrCommSocketAddress tcpaddress = gebr_comm_socket_address_ipv4_local(6011);
	GebrCommSocketAddress unixaddress = gebr_comm_socket_address_unix("/tmp/.X11-unix/X1");

	GebrCommListenSocket *unixsocket = gebr_comm_listen_socket_new();
	g_unlink("myunixsocket");
	gebr_comm_listen_socket_listen(unixsocket, &unixaddress);
	void ready_read_forward(GebrCommStreamSocket * newconn)
	{
		GByteArray *array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(newconn));
		g_byte_array_append(data_read1, array->data, array->len);
		g_byte_array_free(array, TRUE);
		if (data_read1->len == sizeof(data)) {
			g_assert(memcmp(data_read1->data, data, sizeof(data)) == 0);
			finished_read1 = TRUE;
		}
		if (finished_read1 && finished_read2)
			g_main_loop_quit(loop);
	}
	void new_connection(GebrCommListenSocket * unixsocket)
	{
		GebrCommStreamSocket *newconn = gebr_comm_listen_socket_get_next_pending_connection(unixsocket);
		g_signal_connect(newconn, "ready-read",
				 G_CALLBACK(ready_read_forward), NULL);
		GByteArray *array = g_byte_array_new();
		g_byte_array_append(array, data, sizeof(data));
		gebr_comm_socket_write(GEBR_COMM_SOCKET(newconn), array);
		g_byte_array_free(array, TRUE);
	}
	g_signal_connect(unixsocket, "new-connection",
			 G_CALLBACK(new_connection), NULL);
	
	GebrCommChannelSocket *channel = gebr_comm_channel_socket_new();
	gebr_comm_channel_socket_start(channel, &tcpaddress, &unixaddress);

	/* connection to be forwarded */
	void ready_read_source(GebrCommStreamSocket * tcp)
	{
		GByteArray *array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(tcp));
		g_byte_array_append(data_read2, array->data, array->len);
		g_byte_array_free(array, TRUE);
		if (data_read2->len == sizeof(data)) {
			g_assert(memcmp(data_read2->data, data, sizeof(data)) == 0);
			finished_read2 = TRUE;
		}
		if (finished_read1 && finished_read2)
			g_main_loop_quit(loop);
	}
	GebrCommStreamSocket *tcp = gebr_comm_stream_socket_new();
	g_signal_connect(tcp, "ready-read",
			 G_CALLBACK(ready_read_source), NULL);
	gebr_comm_stream_socket_connect(tcp, &tcpaddress, TRUE);
	GByteArray *array = g_byte_array_new();
	g_byte_array_append(array, data, sizeof(data));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(tcp), array);
	g_byte_array_free(array, TRUE);

	g_main_loop_run(loop);

	g_unlink("myunixsocket");
	g_checksum_free(checksum);
	g_byte_array_free(data_read1, TRUE);
	g_byte_array_free(data_read2, TRUE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);
	loop = g_main_loop_new(NULL, FALSE);
	g_type_init();
	g_test_add_func("/comm/socket/tcpunix-channel", test_comm_socket_tcpunix_channel);
	return g_test_run();
}
