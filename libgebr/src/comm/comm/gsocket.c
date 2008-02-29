/*   libgebr - GÍBR Library
 *   Copyright (C) 2007-2008 GÍBR core team (http://gebr.sourceforge.net)
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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>

#include "gsocket.h"
#include "gsocketprivate.h"

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

static void
g_socket_class_init(GSocketClass * class)
{
	/* virtual */
	class->connected = NULL;
	class->disconnected = NULL;
	class->new_connection = NULL;

	/* signals */
	object_signals[READY_READ] = g_signal_new ("ready-read",
		G_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GSocketClass, ready_read),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[READY_WRITE] = g_signal_new ("ready-write",
		G_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GSocketClass, ready_write),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[ERROR] = g_signal_new ("error",
		G_SOCKET_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GSocketClass, error),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__INT,
		G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
g_socket_init(GSocket * socket)
{
	socket->io_channel = NULL;
}

G_DEFINE_TYPE(GSocket, g_socket, G_TYPE_OBJECT)

/*
 * internal functions
 */

static gboolean
__g_socket_read(GIOChannel * source, GIOCondition condition, GSocket * socket)
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
		GSocketClass *	class;

		class = G_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case G_SOCKET_STATE_CONNECTED:
			socket->state = G_SOCKET_STATE_UNCONNECTED;
				if (class->disconnected != NULL)
					class->disconnected(socket);
			break;
		default:
			break;
		}

		return FALSE;
	}
	if (!g_socket_bytes_available(socket)) {
		GSocketClass *	class;
		gboolean	ret;

		class = G_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case G_SOCKET_STATE_LISTENING:
			if (class->new_connection != NULL)
				class->new_connection(socket);

			ret = TRUE;
			break;
		case G_SOCKET_STATE_CONNECTED:
			socket->state = G_SOCKET_STATE_UNCONNECTED;
			if (class->disconnected != NULL)
				class->disconnected(socket);

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

static gboolean
__g_socket_write(GIOChannel * source, GIOCondition condition, GSocket * socket)
{
	if (condition & G_IO_NVAL) {
		/* probably a fd change */
		goto out;
	}
	if (condition & G_IO_ERR) {
		switch (errno) {
		case ECONNREFUSED:
			_g_socket_emit_error(socket, G_SOCKET_ERROR_CONNECTION_REFUSED);
			break;
		case EINPROGRESS:
			return TRUE;
		case 0:
			return FALSE;
		default:
			_g_socket_emit_error(socket, G_SOCKET_ERROR_UNKNOWN);
			break;
		}
		if (socket->state == G_SOCKET_STATE_CONNECTED) {
			GSocketClass *	class;

			class = G_SOCKET_GET_CLASS(socket);
			socket->state = G_SOCKET_STATE_UNCONNECTED;
			if (class->disconnected != NULL)
				class->disconnected(socket);
		}
		goto out;
	}
	if (condition & G_IO_HUP) {
		/* TODO: */
		goto out;
	}
	if (!g_socket_bytes_available(socket)) {
		GSocketClass *	class;

		class = G_SOCKET_GET_CLASS(socket);
		switch (socket->state) {
		case G_SOCKET_STATE_CONNECTING:
			socket->state = G_SOCKET_STATE_CONNECTED;
			if (class->connected != NULL)
				class->connected(socket);
			break;
		default:
			break;
		}

		goto out;
	}

	/* write queud bytes */
	if (socket->queue_write_bytes->len) {
		size_t	written_bytes;

		written_bytes = send(_g_socket_get_fd(socket),
			socket->queue_write_bytes->data, socket->queue_write_bytes->len, 0);
		if (written_bytes == -1)
			goto out;

		g_byte_array_remove_range(socket->queue_write_bytes, 0, written_bytes);
		/* there is still bytes left */
		if (socket->queue_write_bytes->len)
			_g_socket_enable_write_watch(socket);
	}

	g_signal_emit(socket, object_signals[READY_WRITE], 0);
out:	return FALSE;
}

/*
 * private functions
 */

void
_g_socket_init(GSocket * socket, int fd)
{
	GError *	error;

	if (socket->io_channel != NULL)
		_g_socket_close(socket);

	error = NULL;
	socket->state = G_SOCKET_STATE_NONE;
	socket->last_error = G_SOCKET_ERROR_NONE;
	/* IO channel */
	socket->io_channel = g_io_channel_unix_new(fd);
	g_io_channel_set_encoding(socket->io_channel, NULL, &error);
	g_io_channel_set_close_on_unref(socket->io_channel, TRUE);
	/* byte array */
	socket->queue_write_bytes = g_byte_array_new();
}

void
_g_socket_close(GSocket * socket)
{
	if (socket->io_channel != NULL) {
		GError *	error;

		error = NULL;
		g_io_channel_shutdown(socket->io_channel, FALSE, &error);
		g_io_channel_unref(socket->io_channel);
		g_byte_array_free(socket->queue_write_bytes, TRUE);
	}
}

int
_g_socket_get_fd(GSocket * socket)
{
	return g_io_channel_unix_get_fd(socket->io_channel);
}

void
_g_socket_enable_read_watch(GSocket * socket)
{
	g_io_add_watch(socket->io_channel, G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		(GIOFunc)__g_socket_read, socket);
}

void
_g_socket_enable_write_watch(GSocket * socket)
{
	g_io_add_watch(socket->io_channel, G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		(GIOFunc)__g_socket_write, socket);
}

void
_g_socket_emit_error(GSocket * socket, enum GSocketError error)
{
	socket->last_error = error;
	g_signal_emit(socket, object_signals[ERROR], 0, error);
}

/*
 * user functions
 */

void
g_socket_close(GSocket * socket)
{
	_g_socket_close(socket);

	g_object_unref(G_OBJECT(socket));
}

enum GSocketState
g_socket_get_state(GSocket * socket)
{
	return socket->state;
}

enum GSocketError
g_socket_get_last_error(GSocket * socket)
{
	return socket->last_error;
}

gulong
g_socket_bytes_available(GSocket * socket)
{
	/* Adapted from QNativeSocketEnginePrivate::nativeBytesAvailable()
	 * (qnativesocketengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t	nbytes = 0;
	gulong	available = 0;

	if (ioctl(_g_socket_get_fd(socket), FIONREAD, (char *) &nbytes) >= 0)
		available = (gulong)*((int *)&nbytes);

	return available;
}

GByteArray *
g_socket_read(GSocket * socket, gsize max_size)
{
	guint8		buffer[max_size];
	size_t		read_bytes;
	GByteArray *	byte_array;

	read_bytes = recv(_g_socket_get_fd(socket), buffer, max_size, 0);
	if (read_bytes == -1)
		return NULL;

	byte_array = g_byte_array_new();
	g_byte_array_append(byte_array, buffer, read_bytes);

	return byte_array;
}

GString *
g_socket_read_string(GSocket * socket, gsize max_size)
{
	gchar		buffer[max_size+1];
	size_t		read_bytes;
	GString *	string;

	read_bytes = recv(_g_socket_get_fd(socket), buffer, max_size, 0);
	if (read_bytes == -1)
		return NULL;

	buffer[read_bytes] = '\0';
	string = g_string_new(NULL);
	g_string_assign(string, buffer);

	return string;
}

GByteArray *
g_socket_read_all(GSocket * socket)
{
	/* trick for lazyness */
	return g_socket_read(socket, g_socket_bytes_available(socket));
}

GString *
g_socket_read_string_all(GSocket * socket)
{
	/* trick for lazyness */
	return g_socket_read_string(socket, g_socket_bytes_available(socket));
}

gsize
g_socket_write(GSocket * socket, GByteArray * byte_array)
{
	size_t	written_bytes;

	/* enable signal for writting queued bytes */
	_g_socket_enable_write_watch(socket);

	written_bytes = send(_g_socket_get_fd(socket), byte_array->data, byte_array->len, 0);
	if (written_bytes == -1)
		return 0;
	if (written_bytes < byte_array->len) {
		g_byte_array_append(socket->queue_write_bytes,
			byte_array->data + written_bytes, byte_array->len - written_bytes);
	}

	return written_bytes;
}

gsize
g_socket_write_string(GSocket * socket, GString * string)
{
	GByteArray	byte_array;
	size_t		written_bytes;

	byte_array = (GByteArray) {
		.data = (guint8*)string->str,
		.len = string->len
	};
	written_bytes = g_socket_write(socket, &byte_array);

	return written_bytes;
}
