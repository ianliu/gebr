/*   libgebr - GÍBR Library
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

#include "gprocess.h"

/*
 * Prototypes
 */

static void
__g_process_stop_state(GProcess * process);

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

static void
g_process_class_init(GProcessClass * class)
{
	/* signals */
	object_signals[READY_READ_STDOUT] = g_signal_new("ready-read-stdout",
		G_PROCESS_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GProcessClass, ready_read_stdout),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[READY_READ_STDERR] = g_signal_new("ready-read-stderr",
		G_PROCESS_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GProcessClass, ready_read_stderr),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[FINISHED] = g_signal_new("finished",
		G_PROCESS_TYPE,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET (GProcessClass, finished),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
g_process_init(GProcess * process)
{
	__g_process_stop_state(process);
}

G_DEFINE_TYPE(GProcess, g_process, G_TYPE_OBJECT)

/*
 * internal functions
 */

static void
__g_process_free(GProcess * process)
{
	GError *	error;

	error = NULL;
	g_io_channel_shutdown(process->stdin_io_channel, TRUE, &error);
	g_io_channel_unref(process->stdin_io_channel);
	g_io_channel_shutdown(process->stdout_io_channel, TRUE, &error);
	g_io_channel_unref(process->stdout_io_channel);
	g_io_channel_shutdown(process->stderr_io_channel, TRUE, &error);
	g_io_channel_unref(process->stderr_io_channel);
}

static void
__g_process_stop_state(GProcess * process)
{
	process->pid = 0;
	process->is_running = FALSE;
	process->stdin_io_channel = NULL;
	process->stdout_io_channel = NULL;
	process->stderr_io_channel = NULL;
}

static gboolean
__g_process_read_stdout_watch(GIOChannel * source, GIOCondition condition, GProcess * process)
{
	if (g_process_stdout_bytes_available(process) && (condition & G_IO_HUP))
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
		/* TODO: */
		/* using g_child_add_watch instead */
		return FALSE;
	}

	g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);

	return TRUE;
}

static gboolean
__g_process_read_stderr_watch(GIOChannel * source, GIOCondition condition, GProcess * process)
{
	if (g_process_stderr_bytes_available(process) && (condition & G_IO_HUP))
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
		/* TODO: */
		/* FIXME: use g_child_add_watch */
// 		__g_process_free(process);
// 		__g_process_stop_state(process);
// 		g_signal_emit(process, object_signals[FINISHED], 0);
		return FALSE;
	}

	g_signal_emit(process, object_signals[READY_READ_STDERR], 0);

	return TRUE;
}

static void
__g_process_finished_watch(GPid pid, gint status, GProcess * process)
{
	gint			exit_code;
	enum GProcessExitStatus	exit_status;

	if (process->stdout_watch_id) {
		if (g_process_stdout_bytes_available(process))
			g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);
		g_source_remove(process->stdout_watch_id);

		if (g_process_stderr_bytes_available(process))
			g_signal_emit(process, object_signals[READY_READ_STDERR], 0);
		g_source_remove(process->stderr_watch_id);

		__g_process_free(process);
	}
	__g_process_stop_state(process);

	exit_code = WEXITSTATUS(status);
	exit_status =  WIFEXITED(status) ? G_PROCESS_NORMAL_EXIT : G_PROCESS_CRASH_EXIT;
	g_signal_emit(process, object_signals[FINISHED], 0);
}

static GByteArray *
__g_process_read(GIOChannel * io_channel, gsize max_size)
{
	guint8		buffer[max_size];
	size_t		read_bytes;
	GByteArray *	byte_array;

	read_bytes = read(g_io_channel_unix_get_fd(io_channel), buffer, max_size);
	if (read_bytes == -1)
		return NULL;

	byte_array = g_byte_array_new();
	g_byte_array_append(byte_array, buffer, read_bytes);

	return byte_array;
}

static GString *
__g_process_read_string(GIOChannel * io_channel, gsize max_size)
{
	gchar		buffer[max_size+1];
	size_t		read_bytes;
	GString *	string;

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

GProcess *
g_process_new(void)
{
	return (GProcess*)g_object_new(G_PROCESS_TYPE, NULL);
}

void
g_process_free(GProcess * process)
{
	if (process->is_running)
		g_process_kill(process);
	if (process->stdin_io_channel != NULL)
		__g_process_free(process);
	g_object_unref(G_OBJECT(process));
}

gboolean
g_process_is_running(GProcess * process)
{
	return process->is_running;
}

gboolean
g_process_start(GProcess * process, GString * cmd_line)
{
	gboolean	ret;
	gchar **	argv;
	gint		argc;
	GPid		pid;
	int		stdin_pipe[2];
	int		stdout_pipe[2];
	int		stderr_pipe[2];
	gint		stdin_fd, stdout_fd, stderr_fd;
	GError *	error;

	/* free previous start stuff */
	if (process->stdin_io_channel != NULL)
		__g_process_free(process);

	error = NULL;
	g_shell_parse_argv(cmd_line->str, &argc, &argv, &error);
// 	ret = g_spawn_async_with_pipes(
// 		NULL, /* working_directory */
// 		argv,
// 		NULL, /* envp */
// 		G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
// 		NULL,
// 		process,
// 		&process->pid,
// 		&stdin_fd, &stdout_fd, &stderr_fd,
// 		&error);
// 	if (ret == FALSE) {
//
// 		goto out;
// 	}
	pipe(stdin_pipe);
	pipe(stdout_pipe);
	pipe(stderr_pipe);
	pid = fork();
	if (pid == 0) {
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);
		dup2(stdin_pipe[0], 0);
		dup2(stdout_pipe[1], 1);
		dup2(stderr_pipe[1], 2);

		if (execvp(argv[0], argv) == -1) {
			ret = FALSE;
			goto out;
		}
	}
	stdin_fd = stdin_pipe[1];
	stdout_fd = stdout_pipe[0];
	stderr_fd = stderr_pipe[0];

	process->stdout_watch_id = process->stderr_watch_id = 0;
	g_child_watch_add(process->pid, (GChildWatchFunc)__g_process_finished_watch, process);
	/* create io channels */
	process->stdin_io_channel = g_io_channel_unix_new(stdin_fd);
	process->stdout_io_channel = g_io_channel_unix_new(stdout_fd);
	process->stderr_io_channel = g_io_channel_unix_new(stderr_fd);
	/* watches */
	process->stdout_watch_id = g_io_add_watch(process->stdout_io_channel,
		G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		(GIOFunc)__g_process_read_stdout_watch, process);
	process->stderr_watch_id = g_io_add_watch(process->stderr_io_channel,
		G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		(GIOFunc)__g_process_read_stderr_watch, process);
	/* nonblock operations */
	g_io_channel_set_flags(process->stdin_io_channel,
		g_io_channel_get_flags(process->stdin_io_channel) | G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_flags(process->stdout_io_channel,
		g_io_channel_get_flags(process->stdout_io_channel) | G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_flags(process->stderr_io_channel,
		g_io_channel_get_flags(process->stderr_io_channel) | G_IO_FLAG_NONBLOCK, &error);

	/* there is already something available now? */
	if (g_process_stdout_bytes_available(process))
		g_signal_emit(process, object_signals[READY_READ_STDOUT], 0);
	if (g_process_stderr_bytes_available(process))
		g_signal_emit(process, object_signals[READY_READ_STDERR], 0);

out:	g_strfreev(argv);
	return ret;
}

GPid
g_process_get_pid(GProcess * process)
{
	return process->pid;
}

void
g_process_kill(GProcess * process)
{
	if (!process->pid)
		return;
	kill(-process->pid, SIGKILL);
	g_spawn_close_pid(process->pid);
}

void
g_process_terminate(GProcess * process)
{
	if (!process->pid)
		return;
	kill(-process->pid, SIGTERM);
	g_spawn_close_pid(process->pid);
}

gulong
g_process_stdout_bytes_available(GProcess * process)
{
	/* Adapted from QNativeProcessEnginePrivate::nativeBytesAvailable()
	 * (qnativeprocessengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t	nbytes = 0;
	gulong	available = 0;

	if (ioctl(g_io_channel_unix_get_fd(process->stdout_io_channel), FIONREAD, (char *) &nbytes) >= 0)
		available = (gulong)*((int *)&nbytes);

	return available;
}

gulong
g_process_stderr_bytes_available(GProcess * process)
{
	/* Adapted from QNativeProcessEnginePrivate::nativeBytesAvailable()
	 * (qnativeprocessengine_unix.cpp:528 of Qt 4.3.0)
	 */
	size_t	nbytes = 0;
	gulong	available = 0;

	if (ioctl(g_io_channel_unix_get_fd(process->stderr_io_channel), FIONREAD, (char *) &nbytes) >= 0)
		available = (gulong)*((int *)&nbytes);

	return available;
}

GByteArray *
g_process_read_stdout(GProcess * process, gsize max_size)
{
	return __g_process_read(process->stdout_io_channel, max_size);
}

GString *
g_process_read_stdout_string(GProcess * process, gsize max_size)
{
	return __g_process_read_string(process->stdout_io_channel, max_size);
}

GByteArray *
g_process_read_stdout_all(GProcess * process)
{
	/* trick for lazyness */
	return g_process_read_stdout(
		process, g_process_stdout_bytes_available(process));
}

GString *
g_process_read_stdout_string_all(GProcess * process)
{
	/* trick for lazyness */
	return g_process_read_stdout_string(
		process, g_process_stdout_bytes_available(process));
}

GByteArray *
g_process_read_stderr(GProcess * process, gsize max_size)
{
	return __g_process_read(process->stderr_io_channel, max_size);
}

GString *
g_process_read_stderr_string(GProcess * process, gsize max_size)
{
	return __g_process_read_string(process->stderr_io_channel, max_size);
}

GByteArray *
g_process_read_stderr_all(GProcess * process)
{
	/* trick for lazyness */
	return g_process_read_stderr(
		process, g_process_stderr_bytes_available(process));
}

GString *
g_process_read_stderr_string_all(GProcess * process)
{
	/* trick for lazyness */
	return g_process_read_stderr_string(
		process, g_process_stderr_bytes_available(process));
}

gsize
g_process_write_stdin(GProcess * process, GByteArray * byte_array)
{
	size_t	written_bytes;

	written_bytes = write(g_io_channel_unix_get_fd(process->stdin_io_channel),
		byte_array->data, byte_array->len);
	if (written_bytes == -1)
		return 0;

	return written_bytes;
}

gsize
g_process_write_stdin_string(GProcess * process, GString * string)
{
	GByteArray	byte_array;
	size_t		written_bytes;

	byte_array = (GByteArray) {
		.data = (guint8*)string->str,
		.len = string->len
	};
	written_bytes = g_process_write_stdin(process, &byte_array);

	return written_bytes;
}
