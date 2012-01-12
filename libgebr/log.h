/*
 * log.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2012 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_GEBR_LOG_H__
#define __GEBR_GEBR_LOG_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrLogMessage GebrLogMessage;
typedef struct _GebrLog GebrLog;

typedef enum {
	GEBR_LOG_START,
	GEBR_LOG_END,
	GEBR_LOG_INFO,
	GEBR_LOG_ERROR,
	GEBR_LOG_WARNING,
	GEBR_LOG_DEBUG,
	GEBR_LOG_MSG
} GebrLogMessageType;

GebrLog *gebr_log_open(const gchar * path);

GList *gebr_log_messages_read(GebrLog *log);

void gebr_log_messages_free(GList * messages);

void gebr_log_add_message(GebrLog *log, GebrLogMessageType type, const gchar * message);

void gebr_log_close(GebrLog *log);

GebrLogMessage *gebr_log_message_new(GebrLogMessageType type,
				     const gchar       *date,
				     const gchar       *message);

const gchar *gebr_log_message_get_date(GebrLogMessage *message);

const gchar *gebr_log_message_get_message(GebrLogMessage *message);

GebrLogMessageType gebr_log_message_get_type(GebrLogMessage *message);

void gebr_log_message_free(GebrLogMessage *message);

/**
 * gebr_log_set_default:
 *
 * Sets the default @path for logging.
 */
void gebr_log_set_default(const gchar *path);

/**
 * gebr_log:
 *
 * Logs into the default path. See gebr_log_set_default().
 */
void gebr_log(GebrLogMessageType type, const gchar *msg, ...);

G_END_DECLS

#endif /* __GEBR_GEBR_LOG_H__ */
