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

#ifndef __GEBR_COMM_PROCESS_H
#define __GEBR_COMM_PROCESS_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

typedef struct _GebrCommProcess GebrCommProcess;
typedef struct _GebrCommProcessClass GebrCommProcessClass;

G_BEGIN_DECLS GType gebr_comm_process_get_type(void);

#define GEBR_COMM_PROCESS_TYPE			(gebr_comm_process_get_type())
#define GEBR_COMM_PROCESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_PROCESS_TYPE, GebrCommProcess))
#define GEBR_COMM_PROCESS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_PROCESS_TYPE, GebrCommProcessClass))
#define GEBR_COMM_IS_PROCESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_PROCESS_TYPE))
#define GEBR_COMM_IS_PROCESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_PROCESS_TYPE))
#define GEBR_COMM_PROCESS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_PROCESS_TYPE, GebrCommProcessClass))

enum GebrCommProcessExitStatus {
	GEBR_COMM_PROCESS_NORMAL_EXIT,
	GEBR_COMM_PROCESS_CRASH_EXIT
};

struct _GebrCommProcess {
	GObject parent;

	GPid pid;
	gboolean is_running;
	gint exit_code;
	enum GebrCommProcessExitStatus exit_status;

	GIOChannel *stdin_io_channel;
	GIOChannel *stdout_io_channel;
	GIOChannel *stderr_io_channel;
	guint stdout_watch_id;
	guint stderr_watch_id;
	guint finish_watch_id;
};
struct _GebrCommProcessClass {
	GObjectClass parent;

	/* signals */
	void (*ready_read_stdout) (GebrCommProcess * self);
	void (*ready_read_stderr) (GebrCommProcess * self);
	void (*finished) (GebrCommProcess * self);
};

/*
 * user functions
 */

GebrCommProcess *gebr_comm_process_new(void);

void gebr_comm_process_free(GebrCommProcess *);

gboolean gebr_comm_process_is_running(GebrCommProcess *);

gboolean gebr_comm_process_start(GebrCommProcess *, GString *);

GPid gebr_comm_process_get_pid(GebrCommProcess *);

void gebr_comm_process_kill(GebrCommProcess *);

void gebr_comm_process_terminate(GebrCommProcess *);

void gebr_comm_process_close_stdin(GebrCommProcess *);

gulong gebr_comm_process_stdout_bytes_available(GebrCommProcess *);

gulong gebr_comm_process_stderr_bytes_available(GebrCommProcess *);

GByteArray *gebr_comm_process_read_stdout(GebrCommProcess *, gsize);

GString *gebr_comm_process_read_stdout_string(GebrCommProcess *, gsize);

GByteArray *gebr_comm_process_read_stdout_all(GebrCommProcess *);

GString *gebr_comm_process_read_stdout_string_all(GebrCommProcess *);

GByteArray *gebr_comm_process_read_stderr(GebrCommProcess *, gsize);

GString *gebr_comm_process_read_stderr_string(GebrCommProcess *, gsize);

GByteArray *gebr_comm_process_read_stderr_all(GebrCommProcess *);

GString *gebr_comm_process_read_stderr_string_all(GebrCommProcess *);

gsize gebr_comm_process_write_stdin(GebrCommProcess *, GByteArray *);

gsize gebr_comm_process_write_stdin_string(GebrCommProcess *, GString *);

G_END_DECLS
#endif				//__GEBR_COMM_PROCESS_H
