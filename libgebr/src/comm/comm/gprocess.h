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

#ifndef __LIBGEBR_COMM_GPROCESS_H
#define __LIBGEBR_COMM_GPROCESS_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

typedef struct _GProcess	GProcess;
typedef struct _GProcessClass	GProcessClass;

G_BEGIN_DECLS

GType
g_process_get_type(void);

#define G_PROCESS_TYPE			(g_process_get_type())
#define G_PROCESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_PROCESS_TYPE, GProcess))
#define G_PROCESS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), G_PROCESS_TYPE, GProcessClass))
#define G_IS_PROCESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_PROCESS_TYPE))
#define G_IS_PROCESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_PROCESS_TYPE))
#define G_PROCESS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_PROCESS_TYPE, GProcessClass))

enum GProcessExitStatus {
	G_PROCESS_NORMAL_EXIT,
	G_PROCESS_CRASH_EXIT
};

struct _GProcess {
	GObject			parent;

	GPid			pid;
	gboolean		is_running;
	GIOChannel *		stdin_io_channel;
	GIOChannel *		stdout_io_channel;
	GIOChannel *		stderr_io_channel;
};
struct _GProcessClass {
	GObjectClass		parent;

	/* signals */
	void			(*ready_read_stdout)(GProcess * self);
	void			(*ready_read_stderr)(GProcess * self);
//	void			(*finished)(GProcess * self, gint exit_code, enum GProcessExitStatus exit_status);
	void			(*finished)(GProcess * self);
};

/*
 * user functions
 */

GProcess *
g_process_new(void);

void
g_process_free(GProcess *);

gboolean
g_process_is_running(GProcess *);

gboolean
g_process_start(GProcess *, GString *);

GPid
g_process_get_pid(GProcess *);

void
g_process_kill(GProcess *);

void
g_process_terminate(GProcess *);

gulong
g_process_stdout_bytes_available(GProcess *);

gulong
g_process_stderr_bytes_available(GProcess *);

GByteArray *
g_process_read_stdout(GProcess *, gsize);

GString *
g_process_read_stdout_string(GProcess *, gsize);

GByteArray *
g_process_read_stdout_all(GProcess *);

GString *
g_process_read_stdout_string_all(GProcess *);

GByteArray *
g_process_read_stderr(GProcess *, gsize);

GString *
g_process_read_stderr_string(GProcess *, gsize);

GByteArray *
g_process_read_stderr_all(GProcess *);

GString *
g_process_read_stderr_string_all(GProcess *);

gsize
g_process_write_stdin(GProcess *, GByteArray *);

gsize
g_process_write_stdin_string(GProcess *, GString *);

G_END_DECLS

#endif //__LIBGEBR_COMM_GPROCESS_H
