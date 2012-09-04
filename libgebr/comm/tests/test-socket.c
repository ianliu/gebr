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

#include <gebr-comm-listensocket.h>
#include <gebr-comm-channelsocket.h>
#include <gebr-comm-streamsocket.h>
#include <gebr-comm-socketaddress.h>
#include <gebr-comm-socket.h>

GMainLoop *loop;

struct TestData {
	GByteArray *data_read1;
	GByteArray *data_read2;
	gboolean finished_read1;
	gboolean finished_read2;
	guchar data[4001001]; 
};

static void
ready_read_forward(GebrCommStreamSocket * newconn,
		   struct TestData *test_data)
{
	GByteArray *array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(newconn));
	g_byte_array_append(test_data->data_read1, array->data, array->len);
	g_byte_array_free(array, TRUE);
	if (test_data->data_read1->len == sizeof(test_data->data)) {
		g_assert(memcmp(test_data->data_read1->data, test_data->data, sizeof(test_data->data)) == 0);
		test_data->finished_read1 = TRUE;
	}
	if (test_data->finished_read1 && test_data->finished_read2)
		g_main_loop_quit(loop);
}

static void
new_connection(GebrCommListenSocket * unixsocket,
	       struct TestData *test_data)
{
	GebrCommStreamSocket *newconn = gebr_comm_listen_socket_get_next_pending_connection(unixsocket);
	g_signal_connect(newconn, "ready-read",
			 G_CALLBACK(ready_read_forward), test_data);
	GByteArray *array = g_byte_array_new();
	g_byte_array_append(array, test_data->data, sizeof(test_data->data));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(newconn), array);
	g_byte_array_free(array, TRUE);
}

static void
ready_read_source(GebrCommStreamSocket * tcp,
		  struct TestData *test_data)
{
	GByteArray *array = gebr_comm_socket_read_all(GEBR_COMM_SOCKET(tcp));
	g_byte_array_append(test_data->data_read2, array->data, array->len);
	g_byte_array_free(array, TRUE);
	if (test_data->data_read2->len == sizeof(test_data->data)) {
		g_assert(memcmp(test_data->data_read2->data, test_data->data, sizeof(test_data->data)) == 0);
		test_data->finished_read2 = TRUE;
	}
	if (test_data->finished_read1 && test_data->finished_read2)
		g_main_loop_quit(loop);
}

void test_comm_socket_tcpunix_channel()
{
	GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
	struct TestData test_data;

	test_data.data_read1 = g_byte_array_new();
	test_data.data_read2 = g_byte_array_new();
	test_data.finished_read1 = FALSE;
	test_data.finished_read2 = FALSE;

	/*fill with random data*/
	for (int i = 0; i < sizeof(test_data.data); ++i)
		test_data.data[i] = g_random_int();

	GebrCommSocketAddress tcpaddress = gebr_comm_socket_address_ipv4_local(6011);
	GebrCommSocketAddress unixaddress = gebr_comm_socket_address_unix("/tmp/.X11-unix/X1");

	GebrCommListenSocket *unixsocket = gebr_comm_listen_socket_new();
	g_unlink("myunixsocket");
	gebr_comm_listen_socket_listen(unixsocket, &unixaddress);
	g_signal_connect(unixsocket, "new-connection",
			 G_CALLBACK(new_connection), &test_data);
	
	GebrCommChannelSocket *channel = gebr_comm_channel_socket_new();
	gebr_comm_channel_socket_start(channel, &tcpaddress, &unixaddress);

	/* connection to be forwarded */
	GebrCommStreamSocket *tcp = gebr_comm_stream_socket_new();
	g_signal_connect(tcp, "ready-read",
			 G_CALLBACK(ready_read_source), &test_data);
	gebr_comm_stream_socket_connect(tcp, &tcpaddress, TRUE);
	GByteArray *array = g_byte_array_new();
	g_byte_array_append(array, test_data.data, sizeof(test_data.data));
	gebr_comm_socket_write(GEBR_COMM_SOCKET(tcp), array);
	g_byte_array_free(array, TRUE);

	g_main_loop_run(loop);

	g_unlink("myunixsocket");
	g_checksum_free(checksum);
	g_byte_array_free(test_data.data_read1, TRUE);
	g_byte_array_free(test_data.data_read2, TRUE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);
	loop = g_main_loop_new(NULL, FALSE);
	g_type_init();
	//g_test_add_func("/comm/socket/tcpunix-channel", test_comm_socket_tcpunix_channel);
	return g_test_run();
}
