/*   libgebr - GeBR Library
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
 *
 *   Inspired on Qt 4.3 version of QAbstractSocket, by Trolltech
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>

#include "gebr-comm-socket.h"
#include "gebr-comm-socketprivate.h"
#include "gebr-comm-socketaddressprivate.h"

/*
 * gobject stuff
 */

enum {
	READY_READ,
	READY_WRITE,
	ERROR,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void gebr_comm_socket_class_init(GebrCommSocketClass * class)
{
	/* virtual */
	class->connected = NULL;
	class->disconnected = NULL;
	class->new_connection = NULL;

	/* signals */
	object_signals[READY_READ] = g_signal_new("ready-read", GEBR_COMM_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommSocketClass, ready_read), NULL, NULL,	/* acumulators */
						  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[READY_WRITE] = g_signal_new("ready-write", GEBR_COMM_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommSocketClass, ready_write), NULL, NULL,	/* acumulators */
						   g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[ERROR] = g_signal_new("error", GEBR_COMM_SOCKET_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommSocketClass, error), NULL, NULL,	/* acumulators */
					     g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

static void gebr_comm_socket_init(GebrCommSocket * socket)
{
	socket->io_channel = NULL;
}

G_DEFINE_TYPE(GebrCommSocket, gebr_comm_socket, G_TYPE_OBJECT)

/*
 * internal functions
 */
static gboolean __gebr_comm_socket_read(GIOChannel * source, GIOCondition condition, GebrCommSocket * socket)
{
	if (condition & G_IO_NVAL) {
		/* probably a fd change */
		return FALSE;
	}
	if (condition & G_IO_ERR) {
		/* TODO: */
		return FALSE;
	}
	if (condition & G_IO_HUP) {
		GebrCommSocketClass *klass;

		klass = GEBR_COMM_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case GEBR_COMM_SOCKET_STATE_CONNECTED:
			socket->state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
			if (klass->disconnected != NULL)
				klass->disconnected(socket);
			break;
		default:
			break;
		}

		return FALSE;
	}
	if (!gebr_comm_socket_bytes_available(socket)) {
		GebrCommSocketClass *klass;
		gboolean ret;

		klass = GEBR_COMM_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case GEBR_COMM_SOCKET_STATE_LISTENING:
			if (klass->new_connection != NULL)
				klass->new_connection(socket);

			ret = TRUE;
			break;
		case GEBR_COMM_SOCKET_STATE_CONNECTED:
			socket->state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
			if (klass->disconnected != NULL)
				klass->disconnected(socket);

			ret = FALSE;
			break;
		default:
			ret = TRUE;
			break;
		}

		return ret;
	}

	g_signal_emit(socket, object_signals[READY_READ], 0);

	return TRUE;
}

void __gebr_comm_socket_write_queue(GebrCommSocket * socket)
{
	g_return_if_fail(socket->state == GEBR_COMM_SOCKET_STATE_CONNECTED);
	/* write queud bytes */
	if (socket->queue_write_bytes->len) {
		ssize_t written_bytes;

		_gebr_comm_socket_enable_write_watch(socket);

		written_bytes = send(_gebr_comm_socket_get_fd(socket), socket->queue_write_bytes->data,
				     socket->queue_write_bytes->len > 4096 ? 4096 : socket->queue_write_bytes->len, 0);

		if (written_bytes != -1)
			g_byte_array_remove_range(socket->queue_write_bytes, 0, written_bytes);
	}
}

static gboolean __gebr_comm_socket_write(GIOChannel * source, GIOCondition condition, GebrCommSocket * socket)
{
	if (condition & G_IO_NVAL) {
		/* probably a fd change */
		goto out;
	}
	if (condition & G_IO_ERR) {
		switch (errno) {
		case ECONNREFUSED:
			_gebr_comm_socket_emit_error(socket, GEBR_COMM_SOCKET_ERROR_CONNECTION_REFUSED);
			break;
		case EINPROGRESS:
			return TRUE;
		case 0:
			return FALSE;
		default:
			_gebr_comm_socket_emit_error(socket, GEBR_COMM_SOCKET_ERROR_UNKNOWN);
			break;
		}
		if (socket->state == GEBR_COMM_SOCKET_STATE_CONNECTED) {
			GebrCommSocketClass *klass;

			klass = GEBR_COMM_SOCKET_GET_CLASS(socket);
			socket->state = GEBR_COMM_SOCKET_STATE_UNCONNECTED;
			if (klass->disconnected != NULL)
				klass->disconnected(socket);
		}
		goto out;
	}
	if (condition & G_IO_HUP) {
		/* TODO: */
		goto out;
	}
	if (socket->state != GEBR_COMM_SOCKET_STATE_CONNECTED && !gebr_comm_socket_bytes_available(socket)) {
		GebrCommSocketClass *klass;

		klass = GEBR_COMM_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case GEBR_COMM_SOCKET_STATE_CONNECTING:
			socket->state = GEBR_COMM_SOCKET_STATE_CONNECTED;
			if (klass->connected != NULL)
				klass->connected(socket);
			break;
		default:
			break;
		}

		goto out;
	}

	__gebr_comm_socket_write_queue(socket);

 out:	return FALSE;
}

/*
 * private functions
 */

void _gebr_comm_socket_init(GebrCommSocket * socket, int fd, enum GebrCommSocketAddressType address_type)
{
	GError *error;

	/* free previous stuff */
	_gebr_comm_socket_close(socket);

	error = NULL;
	socket->write_watch_id = 0;
	socket->address_type = address_type;
	socket->state = GEBR_COMM_SOCKET_STATE_NONE;
	socket->last_error = GEBR_COMM_SOCKET_ERROR_NONE;
	/* IO channel */
	socket->io_channel = g_io_channel_unix_new(fd);
	g_io_channel_set_encoding(socket->io_channel, NULL, &error);
	g_io_channel_set_close_on_unref(socket->io_channel, TRUE);
	/* byte array */
	socket->queue_write_bytes = g_byte_array_new();
}

void _gebr_comm_socket_close(GebrCommSocket * socket)
{
	if (socket->io_channel != NULL) {
		GError *error;

		if (socket->write_watch_id)
			g_source_remove(socket->write_watch_id);

		error = NULL;
		g_io_channel_shutdown(socket->io_channel, FALSE, &error);
		g_io_channel_unref(socket->io_channel);
		socket->io_channel = NULL;
		g_byte_array_free(socket->queue_write_bytes, TRUE);
	}
}

int _gebr_comm_socket_get_fd(GebrCommSocket * socket)
{
	return g_io_channel_unix_get_fd(socket->io_channel);
}

void _gebr_comm_socket_enable_read_watch(GebrCommSocket * socket)
{
	g_io_add_watch(socket->io_channel, G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		       (GIOFunc) __gebr_comm_socket_read, socket);
}

void _gebr_comm_socket_enable_write_watch(GebrCommSocket * socket)
{
	socket->write_watch_id = g_io_add_watch(socket->io_channel, G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
						(GIOFunc) __gebr_comm_socket_write, socket);
}

void _gebr_comm_socket_emit_error(GebrCommSocket * socket, enum GebrCommSocketError error)
{
	socket->last_error = error;
	g_signal_emit(socket, object_signals[ERROR], 0, error);
}

/*
 * user functions
 */

void gebr_comm_socket_close(GebrCommSocket * socket)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	_gebr_comm_socket_close(socket);

	g_object_unref(G_OBJECT(socket));
}

void gebr_comm_socket_flush(GebrCommSocket * socket)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	GError *error;
	gint flags;

	flags = g_io_channel_get_flags(socket->io_channel);

	error = NULL;
	g_io_channel_set_flags(socket->io_channel, 0, &error);
	error = NULL;
	g_io_channel_flush(socket->io_channel, &error);

	error = NULL;
	g_io_channel_set_flags(socket->io_channel, flags, &error);
}

void gebr_comm_socket_set_blocking(GebrCommSocket * socket, gboolean blocking_operations)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	GError *error;

	error = NULL;
	if (!blocking_operations)
		g_io_channel_set_flags(socket->io_channel, g_io_channel_get_flags(socket->io_channel) | G_IO_FLAG_NONBLOCK, &error);
	else
		g_io_channel_set_flags(socket->io_channel, g_io_channel_get_flags(socket->io_channel) & ~G_IO_FLAG_NONBLOCK, &error);

}

enum GebrCommSocketState gebr_comm_socket_get_state(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), GEBR_COMM_SOCKET_STATE_NONE); 

	return socket->state;
}

enum GebrCommSocketError gebr_comm_socket_get_last_error(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), GEBR_COMM_SOCKET_ERROR_NONE);

	return socket->last_error;
}

GebrCommSocketAddress gebr_comm_socket_get_address(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), _gebr_comm_socket_address_unknown());

	GebrCommSocketAddress address;

	_gebr_comm_socket_address_getsockname(&address, socket->address_type, _gebr_comm_socket_get_fd(socket));

	return address;
}

gulong gebr_comm_socket_bytes_available(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), 0);

	/* Adapted from QNativeSocketEnginePrivate::nativeBytesAvailable()
	 * (qnativesocketengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t nbytes = 0;
	gulong available = 0;

	if (ioctl(_gebr_comm_socket_get_fd(socket), FIONREAD, (char *)&nbytes) >= 0)
		available = (gulong) * ((int *)&nbytes);

	return available;
}

gulong gebr_comm_socket_bytes_to_write(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), 0);

	return socket->queue_write_bytes->len;
}

GByteArray *gebr_comm_socket_read(GebrCommSocket * socket, gsize max_size)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), NULL);

	if (socket->state != GEBR_COMM_SOCKET_STATE_CONNECTED)
		return NULL;

	guint8 buffer[max_size];
	size_t read_bytes;
	GByteArray *byte_array;

	read_bytes = recv(_gebr_comm_socket_get_fd(socket), buffer, max_size, 0);
	if (read_bytes == -1)
		return NULL;

	byte_array = g_byte_array_new();
	g_byte_array_append(byte_array, buffer, read_bytes);

	return byte_array;
}

GString *gebr_comm_socket_read_string(GebrCommSocket * socket, gsize max_size)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), NULL);

	if (socket->state != GEBR_COMM_SOCKET_STATE_CONNECTED)
		return NULL;

	gchar buffer[max_size + 1];
	size_t read_bytes;
	GString *string;

	read_bytes = recv(_gebr_comm_socket_get_fd(socket), buffer, max_size, 0);
	if (read_bytes == -1)
		return NULL;

	buffer[read_bytes] = '\0';
	string = g_string_new(NULL);
	g_string_assign(string, buffer);

	return string;
}

GByteArray *gebr_comm_socket_read_all(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), NULL);

	/* trick for lazyness */
	return gebr_comm_socket_read(socket, gebr_comm_socket_bytes_available(socket));
}

GString *gebr_comm_socket_read_string_all(GebrCommSocket * socket)
{
	g_return_val_if_fail(GEBR_COMM_IS_SOCKET(socket), NULL);

	/* trick for lazyness */
	return gebr_comm_socket_read_string(socket, gebr_comm_socket_bytes_available(socket));
}

void gebr_comm_socket_write(GebrCommSocket * socket, GByteArray * byte_array)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	g_byte_array_append(socket->queue_write_bytes, byte_array->data, byte_array->len);
	_gebr_comm_socket_enable_write_watch(socket);
}

void gebr_comm_socket_write_immediately(GebrCommSocket * socket, GByteArray * byte_array)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	g_byte_array_append(socket->queue_write_bytes, byte_array->data, byte_array->len);
	__gebr_comm_socket_write_queue(socket);
}

void gebr_comm_socket_write_string(GebrCommSocket * socket, GString * string)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	GByteArray byte_array;

	byte_array.data = (guint8 *)string->str;
	byte_array.len = string->len;

	gebr_comm_socket_write(socket, &byte_array);
}

void gebr_comm_socket_write_string_immediately(GebrCommSocket * socket, GString * string)
{
	g_return_if_fail(GEBR_COMM_IS_SOCKET(socket));

	GByteArray byte_array;

	byte_array.data = (guint8 *)string->str;
	byte_array.len = string->len;

	gebr_comm_socket_write_immediately(socket, &byte_array);
}
