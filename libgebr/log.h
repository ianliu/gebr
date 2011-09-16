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

/**
 * SECTION:gebr-log log.h Logging API
 * @short_description: GeBR's logging class
 * @title: GeBR Log Class
 * @include <libgebr/log.h>
 *
 * The GeBR logging class provides functions for logging various types of
 * messages, such as informative messages and error messages.
 */

#ifndef __GEBR_GEBR_LOG_H
#define __GEBR_GEBR_LOG_H

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

/**
 * gebr_log_message_new:
 * @type: The type of this message.
 * @date: When this message occured.
 * @message: The message itself.
 *
 * Returns: A newly allocated #GebrLogMessage structure. Free with
 * gebr_log_message_free().
 */
GebrLogMessage *gebr_log_message_new(GebrLogMessageType type,
				     const gchar       *date,
				     const gchar       *message);

/**
 * gebr_log_message_get_date:
 * @message: The #GebrLogMessage.
 *
 * Returns: The time in which this message was sent; do not free it.
 */
const gchar *gebr_log_message_get_date(GebrLogMessage *message);

/**
 * gebr_log_message_get_message:
 * @message: The #GebrLogMessage.
 *
 * Returns: The message that was sent; do not free it.
 */
const gchar *gebr_log_message_get_message(GebrLogMessage *message);

/**
 * gebr_log_message_get_type:
 * @message: The #GebrLogMessage.
 *
 * Returns: The type of this message.
 */
GebrLogMessageType gebr_log_message_get_type(GebrLogMessage *message);

/**
 * gebr_log_message_free:
 * @message: The #GebrLogMessage.
 *
 * Free the message structure.
 */
void gebr_log_message_free(GebrLogMessage *message);

G_END_DECLS

#endif				//__GEBR_GEBR_LOG_H
