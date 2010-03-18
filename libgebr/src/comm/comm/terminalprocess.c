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
 *   Partly inspired on Qt 4.3 version of QProcess, by Trolltech
 */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>

#include "terminalprocess.h"

/*
 * Prototypes
 */

static void __gebr_comm_terminal_process_stop_state(GebrCommTerminalProcess * terminal_process);

/*
 * gobject stuff
 */

enum {
	READY_READ,
	FINISHED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void gebr_comm_terminal_process_class_init(GebrCommTerminalProcessClass * class)
{
	/* signals */
	object_signals[READY_READ] = g_signal_new("ready-read", GEBR_COMM_TERMINAL_PROCESS_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommTerminalProcessClass, ready_read), NULL, NULL,	/* acumulators */
						  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[FINISHED] = g_signal_new("finished", GEBR_COMM_TERMINAL_PROCESS_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommTerminalProcessClass, finished), NULL, NULL,	/* acumulators */
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE, 0 /*2, G_TYPE_INT, G_TYPE_INT */ );
}

static void gebr_comm_terminal_process_init(GebrCommTerminalProcess * terminal_process)
{
	__gebr_comm_terminal_process_stop_state(terminal_process);
	terminal_process->ptm_io_channel = NULL;
}

G_DEFINE_TYPE(GebrCommTerminalProcess, gebr_comm_terminal_process, G_TYPE_OBJECT)

/*
 * internal functions
 */
static void __gebr_comm_terminal_process_free(GebrCommTerminalProcess * terminal_process)
{
	if (terminal_process->ptm_watch_id) {
		if (gebr_comm_terminal_process_bytes_available(terminal_process))
			g_signal_emit(terminal_process, object_signals[READY_READ], 0);
		g_source_remove(terminal_process->ptm_watch_id);
		g_source_remove(terminal_process->finish_watch_id);
		terminal_process->ptm_watch_id = 0;
		terminal_process->finish_watch_id = 0;
	}
	if (terminal_process->ptm_io_channel != NULL) {
		g_io_channel_unref(terminal_process->ptm_io_channel);
		terminal_process->ptm_io_channel = NULL;
	}
}

static void __gebr_comm_terminal_process_stop_state(GebrCommTerminalProcess * terminal_process)
{
	terminal_process->pid = 0;
	terminal_process->is_running = FALSE;
}

static gboolean
__gebr_comm_terminal_process_read_watch(GIOChannel * source, GIOCondition condition,
					GebrCommTerminalProcess * terminal_process)
{
	if (condition & G_IO_ERR) {
		/* TODO: */
		return FALSE;
	}
	if (condition & G_IO_HUP) {
		/* using g_child_watch_add */
		return FALSE;
	}
	if (condition & G_IO_NVAL) {
		/* probably a fd change or end of process */
		return FALSE;
	}

	g_signal_emit(terminal_process, object_signals[READY_READ], 0);

	return TRUE;
}

static void
__gebr_comm_terminal_process_finished_watch(GPid pid, gint status, GebrCommTerminalProcess * terminal_process)
{
	__gebr_comm_terminal_process_free(terminal_process);
	__gebr_comm_terminal_process_stop_state(terminal_process);

	g_signal_emit(terminal_process, object_signals[FINISHED], 0);
}

static GByteArray *__gebr_comm_terminal_process_read(GIOChannel * io_channel, gsize max_size)
{
	guint8 buffer[max_size];
	ssize_t read_bytes;
	GByteArray *byte_array;

	read_bytes = read(g_io_channel_unix_get_fd(io_channel), buffer, max_size);
	if (read_bytes == -1)
		return NULL;

	byte_array = g_byte_array_new();
	g_byte_array_append(byte_array, buffer, read_bytes);

	return byte_array;
}

static GString *__gebr_comm_terminal_process_read_string(GIOChannel * io_channel, gsize max_size)
{
	gchar buffer[max_size + 1];
	ssize_t read_bytes;
	GString *string;

	read_bytes = read(g_io_channel_unix_get_fd(io_channel), buffer, max_size);
	if (read_bytes == -1)
		return NULL;

	buffer[read_bytes] = '\0';
	string = g_string_new(NULL);
	g_string_assign(string, buffer);

	return string;
}

/*
 * private functions
 */

/*
 * user functions
 */

GebrCommTerminalProcess *gebr_comm_terminal_process_new(void)
{
	return (GebrCommTerminalProcess *) g_object_new(GEBR_COMM_TERMINAL_PROCESS_TYPE, NULL);
}

void gebr_comm_terminal_process_free(GebrCommTerminalProcess * terminal_process)
{
	if (terminal_process->is_running)
		gebr_comm_terminal_process_kill(terminal_process);
	__gebr_comm_terminal_process_free(terminal_process);
	g_object_unref(G_OBJECT(terminal_process));
}

gboolean gebr_comm_terminal_process_is_running(GebrCommTerminalProcess * terminal_process)
{
	return terminal_process->is_running;
}

gboolean gebr_comm_terminal_process_start(GebrCommTerminalProcess * terminal_process, GString * cmd_line)
{
	gboolean ret;
	gchar **argv;
	gint argc;
	gint ptm_fd;
	GPid pid;
	GError *error;

	/* free previous start stuff */
	if (terminal_process->ptm_io_channel != NULL)
		__gebr_comm_terminal_process_free(terminal_process);

	error = NULL;
	g_shell_parse_argv(cmd_line->str, &argc, &argv, &error);
	pid = forkpty(&ptm_fd, NULL, NULL, NULL);
	if (pid == -1) {
		ret = FALSE;
		goto out;
	}
	if (pid == 0) {
		setpgrp();
		if (execvp(argv[0], argv) == -1) {
			ret = FALSE;
			goto out;
		}
	}

	ret = TRUE;
	terminal_process->pid = pid;
	/* monitor exit */
	terminal_process->ptm_io_channel = NULL;
	terminal_process->finish_watch_id =
		g_child_watch_add(terminal_process->pid, (GChildWatchFunc) __gebr_comm_terminal_process_finished_watch,
				  terminal_process);
	/* ptm */
	terminal_process->ptm_io_channel = g_io_channel_unix_new(ptm_fd);
	g_io_channel_set_close_on_unref(terminal_process->ptm_io_channel, TRUE);
	terminal_process->ptm_watch_id = g_io_add_watch(terminal_process->ptm_io_channel,
							G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
							(GIOFunc) __gebr_comm_terminal_process_read_watch,
							terminal_process);
	/* nonblock operation */
	g_io_channel_set_flags(terminal_process->ptm_io_channel,
			       g_io_channel_get_flags(terminal_process->ptm_io_channel) | G_IO_FLAG_NONBLOCK, &error);

	/* there is already something available now? */
	if (gebr_comm_terminal_process_bytes_available(terminal_process))
		g_signal_emit(terminal_process, object_signals[READY_READ], 0);

 out:	g_strfreev(argv);
	return ret;
}

GPid gebr_comm_terminal_process_get_pid(GebrCommTerminalProcess * terminal_process)
{
	return terminal_process->pid;
}

void gebr_comm_terminal_process_kill(GebrCommTerminalProcess * terminal_process)
{
	if (!terminal_process->pid)
		return;
	killpg(terminal_process->pid, SIGKILL);
}

void gebr_comm_terminal_process_terminate(GebrCommTerminalProcess * terminal_process)
{
	if (!terminal_process->pid)
		return;
	killpg(terminal_process->pid, SIGTERM);
}

gulong gebr_comm_terminal_process_bytes_available(GebrCommTerminalProcess * terminal_process)
{
	/* Adapted from QNativeProcessEnginePrivate::nativeBytesAvailable()
	 * (qnativeterminal_processengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t nbytes = 0;
	gulong available = 0;

	if (ioctl(g_io_channel_unix_get_fd(terminal_process->ptm_io_channel), FIONREAD, (char *)&nbytes) >= 0)
		available = (gulong) * ((int *)&nbytes);

	return available;
}

GByteArray *gebr_comm_terminal_process_read(GebrCommTerminalProcess * terminal_process, gsize max_size)
{
	return __gebr_comm_terminal_process_read(terminal_process->ptm_io_channel, max_size);
}

GString *gebr_comm_terminal_process_read_string(GebrCommTerminalProcess * terminal_process, gsize max_size)
{
	return __gebr_comm_terminal_process_read_string(terminal_process->ptm_io_channel, max_size);
}

GByteArray *gebr_comm_terminal_process_read_all(GebrCommTerminalProcess * terminal_process)
{
	/* trick for lazyness */
	return gebr_comm_terminal_process_read(terminal_process,
					       gebr_comm_terminal_process_bytes_available(terminal_process));
}

GString *gebr_comm_terminal_process_read_string_all(GebrCommTerminalProcess * terminal_process)
{
	/* trick for lazyness */
	return gebr_comm_terminal_process_read_string(terminal_process,
						      gebr_comm_terminal_process_bytes_available(terminal_process));
}

gsize gebr_comm_terminal_process_write(GebrCommTerminalProcess * terminal_process, GByteArray * byte_array)
{
	ssize_t written_bytes;

	written_bytes = write(g_io_channel_unix_get_fd(terminal_process->ptm_io_channel),
			      byte_array->data, byte_array->len);
	if (written_bytes == -1)
		return 0;

	return written_bytes;
}

gsize gebr_comm_terminal_process_write_string(GebrCommTerminalProcess * terminal_process, GString * string)
{
	GByteArray byte_array;
	size_t written_bytes;

	byte_array = (GByteArray) {
	.data = (guint8 *) string->str,.len = string->len};
	written_bytes = gebr_comm_terminal_process_write(terminal_process, &byte_array);

	return written_bytes;
}
