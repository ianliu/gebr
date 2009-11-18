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
 */

#ifndef __MISC_LOG_H
#define __MISC_LOG_H

#include <stdio.h>

#include <glib.h>

enum log_message_type {
	LOG_START, LOG_END, LOG_INFO, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_MSG
};

struct log_message {
	enum log_message_type	type;
	GString *		date;
	GString *		message;
};

struct log {
	GIOChannel *	io_channel;
};

struct log *
log_open(const gchar * path);

void
log_close(struct log * log);

struct log_message *
log_gebr_comm_message_new(enum log_message_type type, const gchar * date, const gchar * message);

void
log_gebr_comm_message_free(struct log_message * message);

GList *
log_messages_read(struct log * log);

void
log_messages_free(GList * messages);

void
log_add_message(struct log * log, enum log_message_type type, const gchar * message);

#endif //__MISC_LOG_H
