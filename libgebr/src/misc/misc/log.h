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
 */

#ifndef __LIBGEBR_MISC_LOG_H
#define __LIBGEBR_MISC_LOG_H

#include <stdio.h>

#include <glib.h>

enum log_message_type {
	START, END, INFO, ERROR, WARNING, DEBUG
};

struct log_message {
	enum log_message_type	type;
	GString *		string;
};

struct log {
	GIOChannel *	io_channel;
	GList *		messages;
};

struct log *
log_open(const gchar * path);

void
log_close(struct log * log);

void
log_add_message(struct log * log, enum log_message_type type, const gchar * message);

#endif //__LIBGEBR_MISC_LOG_H
