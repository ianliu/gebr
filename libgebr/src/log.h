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

#ifndef __GEBR_LOG_H
#define __GEBR_LOG_H

#include <stdio.h>

#include <glib.h>

enum gebr_log_message_type {
	LOG_START, LOG_END, LOG_INFO, LOG_ERROR, LOG_WARNING, LOG_DEBUG, LOG_MSG
};

struct gebr_log_message {
	enum gebr_log_message_type	type;
	GString *		date;
	GString *		message;
};

struct gebr_log {
	GIOChannel *	io_channel;
};

struct gebr_log *
gebr_log_open(const gchar * path);

void
gebr_log_close(struct gebr_log * log);

struct gebr_log_message *
gebr_log_message_new(enum gebr_log_message_type type, const gchar * date, const gchar * message);

void
gebr_log_message_free(struct gebr_log_message * message);

GList *
gebr_log_messages_read(struct gebr_log * log);

void
gebr_log_messages_free(GList * messages);

void
gebr_log_add_message(struct gebr_log * log, enum gebr_log_message_type type, const gchar * message);

#endif //__GEBR_LOG_H
