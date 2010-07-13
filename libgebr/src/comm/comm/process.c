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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "process.h"

/*
 * Prototypes
 */

static void __gebr_comm_process_stop_state(GebrCommProcess * process);

/*
 * gobject stuff
 */

enum {
	READY_READ_STDOUT,
	READY_READ_STDERR,
	FINISHED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void gebr_comm_process_class_init(GebrCommProcessClass * class)
{
	/* signals */
	object_signals[READY_READ_STDOUT] = g_signal_new("ready-read-stdout", GEBR_COMM_PROCESS_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommProcessClass, ready_read_stdout), NULL, NULL,	/* acumulators */
							 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[READY_READ_STDERR] = g_signal_new("ready-read-stderr", GEBR_COMM_PROCESS_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommProcessClass, ready_read_stderr), NULL, NULL,	/* acumulators */
							 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[FINISHED] = g_signal_new("finished", GEBR_COMM_PROCESS_TYPE, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrCommProcessClass, finished), NULL, NULL,	/* acumulators */
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gebr_comm_process_init(GebrCommProcess * process)
{
	__gebr_comm_process_stop_state(process);
	process->stdin_io_channel = NULL;
	process->stdout_io_channel = NULL;
	process->stderr_io_channel = NULL;
}

G_DEFINE_TYPE(GebrCommProcess, gebr_comm_process, G_TYPE_OBJECT)

/*
 * internal functions
 */
static void __gebr_comm_process_io_channel_free(GIOChannel ** io_channel)
{
	if (*io_channel != NULL) {
		GError *error;

		error = NULL;
		g_io_channel_shutdown(*io_channel, TRUE, &error);
		g_io_channel_unref(*io_channel);
		*io_channel = NULL;
	}
}

static void __gebr_comm_process_free(GebrCommProcess * process)
{
	if (process->stdout_watch_id) {
		if (gebr_comm_process_stdout_bytes_available(process))
			g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);
		if (gebr_comm_process_stderr_bytes_available(process))
			g_signal_emit(process, object_signals[READY_READ_STDERR], 0);
		g_source_remove(process->stdout_watch_id);
		g_source_remove(process->stderr_watch_id);
		g_source_remove(process->finish_watch_id);
		process->stdout_watch_id = 0;
		process->stderr_watch_id = 0;
		process->finish_watch_id = 0;
	}
	__gebr_comm_process_io_channel_free(&process->stdin_io_channel);
	__gebr_comm_process_io_channel_free(&process->stdout_io_channel);
	__gebr_comm_process_io_channel_free(&process->stderr_io_channel);
}

static void __gebr_comm_process_stop_state(GebrCommProcess * process)
{
	process->pid = 0;
	process->is_running = FALSE;
}

static gboolean
__gebr_comm_process_read_stdout_watch(GIOChannel * source, GIOCondition condition, GebrCommProcess * process)
{
	if (gebr_comm_process_stdout_bytes_available(process) && (condition & G_IO_HUP))
		g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);

	if (condition & G_IO_NVAL) {
		/* probably a fd change */
		return FALSE;
	}
	if (condition & G_IO_ERR) {
		/* TODO: */
		return FALSE;
	}
	if (condition & G_IO_HUP) {
		return FALSE;
	}

	g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);

	return TRUE;
}

static gboolean
__gebr_comm_process_read_stderr_watch(GIOChannel * source, GIOCondition condition, GebrCommProcess * process)
{
	if (gebr_comm_process_stderr_bytes_available(process) && (condition & G_IO_HUP))
		g_signal_emit(process, object_signals[READY_READ_STDERR], 0);

	if (condition & G_IO_NVAL) {
		/* probably a fd change */
		return FALSE;
	}
	if (condition & G_IO_ERR) {
		/* TODO: */
		return FALSE;
	}
	if (condition & G_IO_HUP) {
		return FALSE;
	}

	g_signal_emit(process, object_signals[READY_READ_STDERR], 0);

	return TRUE;
}

static void __gebr_comm_process_finished_watch(GPid pid, gint status, GebrCommProcess * process)
{
	__gebr_comm_process_free(process);
	__gebr_comm_process_stop_state(process);

	g_signal_emit(process, object_signals[FINISHED], 0);
}

static GByteArray *__gebr_comm_process_read(GIOChannel * io_channel, gsize max_size)
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

static GString *__gebr_comm_process_read_string(GIOChannel * io_channel, gsize max_size)
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

GebrCommProcess *gebr_comm_process_new(void)
{
	return (GebrCommProcess *) g_object_new(GEBR_COMM_PROCESS_TYPE, NULL);
}

void gebr_comm_process_free(GebrCommProcess * process)
{
	if (process->is_running)
		gebr_comm_process_kill(process);
	__gebr_comm_process_free(process);
	g_object_unref(G_OBJECT(process));
}

gboolean gebr_comm_process_is_running(GebrCommProcess * process)
{
	return process->is_running;
}

gboolean gebr_comm_process_start(GebrCommProcess * process, GString * cmd_line)
{
	gboolean ret;
	gchar **argv;
	gint argc;
	gint stdin_fd, stdout_fd, stderr_fd;
	int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
	GError *error;

	__gebr_comm_process_free(process);
	ret = FALSE;
	error = NULL;
	g_shell_parse_argv(cmd_line->str, &argc, &argv, &error);

	if (pipe(stdin_pipe) == -1)
		g_warning("%s:%d: Error when creating stdin pipe with error code %d",
			  __FILE__, __LINE__, errno);

	if (pipe(stdout_pipe) == -1)
		g_warning("%s:%d: Error when creating stdout pipe with error code %d",
			  __FILE__, __LINE__, errno);

	if (pipe(stderr_pipe) == -1)
		g_warning("%s:%d: Error when creating stderr pipe with error code %d",
			  __FILE__, __LINE__, errno);

	process->pid = fork();
	if (process->pid == -1)
		goto out;
	if (process->pid == 0) {
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);
		dup2(stdin_pipe[0], 0);
		dup2(stdout_pipe[1], 1);
		dup2(stderr_pipe[1], 2);

		if (execvp(argv[0], argv) == -1)
			exit(0);
	} else
		setpgid(process->pid, getpid());
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);
	close(stderr_pipe[1]);
	stdin_fd = stdin_pipe[1];
	stdout_fd = stdout_pipe[0];
	stderr_fd = stderr_pipe[0];

	process->is_running = TRUE;
	process->finish_watch_id =
	    g_child_watch_add(process->pid, (GChildWatchFunc) __gebr_comm_process_finished_watch, process);
	/* create io channels */
	process->stdin_io_channel = g_io_channel_unix_new(stdin_fd);
	process->stdout_io_channel = g_io_channel_unix_new(stdout_fd);
	process->stderr_io_channel = g_io_channel_unix_new(stderr_fd);
	/* watches */
	process->stdout_watch_id = g_io_add_watch(process->stdout_io_channel,
						  G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
						  (GIOFunc) __gebr_comm_process_read_stdout_watch, process);
	process->stderr_watch_id = g_io_add_watch(process->stderr_io_channel,
						  G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
						  (GIOFunc) __gebr_comm_process_read_stderr_watch, process);
	/* nonblock operations */
	g_io_channel_set_flags(process->stdin_io_channel,
			       g_io_channel_get_flags(process->stdin_io_channel) | G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_flags(process->stdout_io_channel,
			       g_io_channel_get_flags(process->stdout_io_channel) | G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_flags(process->stderr_io_channel,
			       g_io_channel_get_flags(process->stderr_io_channel) | G_IO_FLAG_NONBLOCK, &error);

	/* there is already something available now? */
	if (gebr_comm_process_stdout_bytes_available(process))
		g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);
	if (gebr_comm_process_stderr_bytes_available(process))
		g_signal_emit(process, object_signals[READY_READ_STDERR], 0);

	ret = TRUE;
 out:	g_strfreev(argv);
	return ret;
}

GPid gebr_comm_process_get_pid(GebrCommProcess * process)
{
	return process->pid;
}

void gebr_comm_process_kill(GebrCommProcess * process)
{
	if (!process->pid)
		return;
	killpg(process->pid, SIGKILL);
}

void gebr_comm_process_terminate(GebrCommProcess * process)
{
	if (!process->pid)
		return;
	killpg(process->pid, SIGTERM);
}

void gebr_comm_process_close_stdin(GebrCommProcess * process)
{
	if (process->is_running == FALSE)
		return;
	__gebr_comm_process_io_channel_free(&process->stdin_io_channel);
}

gulong gebr_comm_process_stdout_bytes_available(GebrCommProcess * process)
{
	/* Adapted from QNativeProcessEnginePrivate::nativeBytesAvailable()
	 * (qnativeprocessengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t nbytes = 0;
	gulong available = 0;

	if (ioctl(g_io_channel_unix_get_fd(process->stdout_io_channel), FIONREAD, (char *)&nbytes) >= 0)
		available = (gulong) * ((int *)&nbytes);

	return available;
}

gulong gebr_comm_process_stderr_bytes_available(GebrCommProcess * process)
{
	/* Adapted from QNativeProcessEnginePrivate::nativeBytesAvailable()
	 * (qnativeprocessengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t nbytes = 0;
	gulong available = 0;

	if (ioctl(g_io_channel_unix_get_fd(process->stderr_io_channel), FIONREAD, (char *)&nbytes) >= 0)
		available = (gulong) * ((int *)&nbytes);

	return available;
}

GByteArray *gebr_comm_process_read_stdout(GebrCommProcess * process, gsize max_size)
{
	return __gebr_comm_process_read(process->stdout_io_channel, max_size);
}

GString *gebr_comm_process_read_stdout_string(GebrCommProcess * process, gsize max_size)
{
	return __gebr_comm_process_read_string(process->stdout_io_channel, max_size);
}

GByteArray *gebr_comm_process_read_stdout_all(GebrCommProcess * process)
{
	/* trick for lazyness */
	return gebr_comm_process_read_stdout(process, gebr_comm_process_stdout_bytes_available(process));
}

GString *gebr_comm_process_read_stdout_string_all(GebrCommProcess * process)
{
	/* trick for lazyness */
	return gebr_comm_process_read_stdout_string(process, gebr_comm_process_stdout_bytes_available(process));
}

GByteArray *gebr_comm_process_read_stderr(GebrCommProcess * process, gsize max_size)
{
	return __gebr_comm_process_read(process->stderr_io_channel, max_size);
}

GString *gebr_comm_process_read_stderr_string(GebrCommProcess * process, gsize max_size)
{
	return __gebr_comm_process_read_string(process->stderr_io_channel, max_size);
}

GByteArray *gebr_comm_process_read_stderr_all(GebrCommProcess * process)
{
	/* trick for lazyness */
	return gebr_comm_process_read_stderr(process, gebr_comm_process_stderr_bytes_available(process));
}

GString *gebr_comm_process_read_stderr_string_all(GebrCommProcess * process)
{
	/* trick for lazyness */
	return gebr_comm_process_read_stderr_string(process, gebr_comm_process_stderr_bytes_available(process));
}

gsize gebr_comm_process_write_stdin(GebrCommProcess * process, GByteArray * byte_array)
{
	ssize_t written_bytes;

	written_bytes = write(g_io_channel_unix_get_fd(process->stdin_io_channel), byte_array->data, byte_array->len);
	if (written_bytes == -1)
		return 0;

	return written_bytes;
}

gsize gebr_comm_process_write_stdin_string(GebrCommProcess * process, GString * string)
{
	GByteArray byte_array;
	ssize_t written_bytes;

	byte_array.data = (guint8 *) string->str;
	byte_array.len = string->len;
	written_bytes = gebr_comm_process_write_stdin(process, &byte_array);

	return written_bytes;
}
