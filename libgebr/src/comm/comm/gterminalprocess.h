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

#ifndef __LIBGEBR_COMM_GTERMINAL_PROCESS_H
#define __LIBGEBR_COMM_GTERMINAL_PROCESS_H

#include <glib.h>
#include <glib-object.h>
#include <netinet/in.h>

typedef struct _GTerminalProcess	GTerminalProcess;
typedef struct _GTerminalProcessClass	GTerminalProcessClass;

G_BEGIN_DECLS

GType
g_terminal_process_get_type(void);

#define G_TERMINAL_PROCESS_TYPE			(g_terminal_process_get_type())
#define G_TERMINAL_PROCESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TERMINAL_PROCESS_TYPE, GTerminalProcess))
#define G_TERMINAL_PROCESS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), G_TERMINAL_PROCESS_TYPE, GTerminalProcessClass))
#define G_IS_TERMINAL_PROCESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TERMINAL_PROCESS_TYPE))
#define G_IS_TERMINAL_PROCESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_TERMINAL_PROCESS_TYPE))
#define G_TERMINAL_PROCESS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TERMINAL_PROCESS_TYPE, GTerminalProcessClass))

enum GTerminalProcessExitStatus {
	G_TERMINAL_PROCESS_NORMAL_EXIT,
	G_TERMINAL_PROCESS_CRASH_EXIT
};

struct _GTerminalProcess {
	GObject				parent;

	GPid				pid;
	gboolean			is_running;
	gint				exit_code;
	enum GTerminalProcessExitStatus	exit_status;

	GIOChannel *			ptm_io_channel;
	guint				watch_id;
};
struct _GTerminalProcessClass {
	GObjectClass			parent;

	/* signals */
	void				(*ready_read)(GTerminalProcess * self);
	void				(*finished)(GTerminalProcess * self);
};

/*
 * user functions
 */

GTerminalProcess *
g_terminal_process_new(void);

void
g_terminal_process_free(GTerminalProcess *);

gboolean
g_terminal_process_is_running(GTerminalProcess *);

gboolean
g_terminal_process_start(GTerminalProcess *, GString *);

GPid
g_terminal_process_get_pid(GTerminalProcess *);

void
g_terminal_process_kill(GTerminalProcess *);

void
g_terminal_process_terminate(GTerminalProcess *);

gulong
g_terminal_process_bytes_available(GTerminalProcess *);

GByteArray *
g_terminal_process_read(GTerminalProcess *, gsize);

GString *
g_terminal_process_read_string(GTerminalProcess *, gsize);

GByteArray *
g_terminal_process_read_all(GTerminalProcess *);

GString *
g_terminal_process_read_string_all(GTerminalProcess *);

gsize
g_terminal_process_write(GTerminalProcess *, GByteArray *);

gsize
g_terminal_process_write_string(GTerminalProcess *, GString *);

G_END_DECLS

#endif //__LIBGEBR_COMM_GTERMINAL_PROCESS_H
