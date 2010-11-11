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

#ifndef __GEBR_COMM_TERMINAL_PROCESS_H
#define __GEBR_COMM_TERMINAL_PROCESS_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

G_BEGIN_DECLS

typedef struct _GebrCommTerminalProcess GebrCommTerminalProcess;
typedef struct _GebrCommTerminalProcessClass GebrCommTerminalProcessClass;

GType gebr_comm_terminal_process_get_type(void);

#define GEBR_COMM_TERMINAL_PROCESS_TYPE			(gebr_comm_terminal_process_get_type())
#define GEBR_COMM_TERMINAL_PROCESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_TERMINAL_PROCESS_TYPE, GebrCommTerminalProcess))
#define GEBR_COMM_TERMINAL_PROCESS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_TERMINAL_PROCESS_TYPE, GebrCommTerminalProcessClass))
#define GEBR_COMM_IS_TERMINAL_PROCESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_TERMINAL_PROCESS_TYPE))
#define GEBR_COMM_IS_TERMINAL_PROCESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_TERMINAL_PROCESS_TYPE))
#define GEBR_COMM_TERMINAL_PROCESS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_TERMINAL_PROCESS_TYPE, GebrCommTerminalProcessClass))

enum GebrCommTerminalProcessExitStatus {
	GEBR_COMM_TERMINAL_PROCESS_NORMAL_EXIT,
	GEBR_COMM_TERMINAL_PROCESS_CRASH_EXIT
};

struct _GebrCommTerminalProcess {
	GObject parent;

	GPid pid;
	gboolean is_running;
	gint exit_code;
	enum GebrCommTerminalProcessExitStatus exit_status;

	GIOChannel *ptm_io_channel;
	guint ptm_watch_id;
	guint finish_watch_id;
};
struct _GebrCommTerminalProcessClass {
	GObjectClass parent;

	/* signals */
	void (*ready_read) (GebrCommTerminalProcess * self);
	void (*finished) (GebrCommTerminalProcess * self);
};

/*
 * user functions
 */

GebrCommTerminalProcess *gebr_comm_terminal_process_new(void);

void gebr_comm_terminal_process_free(GebrCommTerminalProcess *);

gboolean gebr_comm_terminal_process_is_running(GebrCommTerminalProcess *);

gboolean gebr_comm_terminal_process_start(GebrCommTerminalProcess *, GString *);

GPid gebr_comm_terminal_process_get_pid(GebrCommTerminalProcess *);

void gebr_comm_terminal_process_kill(GebrCommTerminalProcess *);

void gebr_comm_terminal_process_terminate(GebrCommTerminalProcess *);

gulong gebr_comm_terminal_process_bytes_available(GebrCommTerminalProcess *);

GByteArray *gebr_comm_terminal_process_read(GebrCommTerminalProcess *, gsize);

GString *gebr_comm_terminal_process_read_string(GebrCommTerminalProcess *, gsize);

GByteArray *gebr_comm_terminal_process_read_all(GebrCommTerminalProcess *);

GString *gebr_comm_terminal_process_read_string_all(GebrCommTerminalProcess *);

gsize gebr_comm_terminal_process_write(GebrCommTerminalProcess *, GByteArray *);

gsize gebr_comm_terminal_process_write_string(GebrCommTerminalProcess *, GString *);

G_END_DECLS
#endif				//__GEBR_COMM_TERMINAL_PROCESS_H
