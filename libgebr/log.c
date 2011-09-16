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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "log.h"
#include "date.h"

/*
 * Internal functions
 */

/*
 * Library functions
 */

struct _GebrLogMessage {
	GebrLogMessageType type;
	GString *date;
	GString *message;
};

struct _GebrLog {
	GIOChannel *io_channel;
};

GebrLog *gebr_log_open(const gchar * path)
{
	GebrLog *log;
	GError *error;

	/* does it exist? if not, create it */
	if (g_access(path, F_OK)) {
		FILE *fp;

		fp = fopen(path, "w");
		fclose(fp);
	}

	error = NULL;
	log = g_new(GebrLog, 1);
	log->io_channel = g_io_channel_new_file(path, "r+", &error);
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_END, &error);

	return log;
}

GebrLogMessage *gebr_log_message_new(GebrLogMessageType type, const gchar * date,
					      const gchar * message)
{
	GebrLogMessage *log_message;

	log_message = g_new(GebrLogMessage, 1);
	log_message->type = type;
	log_message->date = g_string_new(date);
	log_message->message = g_string_new(message);

	return log_message;
}

void gebr_log_message_free(GebrLogMessage *message)
{
	g_string_free(message->date, TRUE);
	g_string_free(message->message, TRUE);
	g_free(message);
}

void gebr_log_close(GebrLog *log)
{
	g_io_channel_unref(log->io_channel);

	g_free(log);
}

GList *gebr_log_messages_read(GebrLog *log)
{
	GList *messages;
	GString *line;
	GError *error;

	error = NULL;
	messages = NULL;
	line = g_string_new(NULL);
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_SET, &error);
	while (g_io_channel_read_line_string(log->io_channel, line, NULL, &error) == G_IO_STATUS_NORMAL) {
		GebrLogMessage *message;
		GebrLogMessageType type;
		gchar **splits;

		splits = g_strsplit(line->str, " ", 3);
		if (splits[0][0] != '[') {
			g_strfreev(splits);
			continue;
		}

		if (!strcmp(splits[0], "[ERR]"))
			type = GEBR_LOG_ERROR;
		else if (!strcmp(splits[0], "[WARN]"))
			type = GEBR_LOG_WARNING;
		else if (!strcmp(splits[0], "[INFO]"))
			type = GEBR_LOG_INFO;
		else if (!strcmp(splits[0], "[STR]"))
			type = GEBR_LOG_START;
		else if (!strcmp(splits[0], "[END]"))
			type = GEBR_LOG_END;
		else {
			g_strfreev(splits);
			continue;
		}
		/* remove end of line */
		splits[2][strlen(splits[2]) - 1] = '\0';
		message = gebr_log_message_new(type, splits[1], splits[2]);
		messages = g_list_prepend(messages, message);

		g_strfreev(splits);
	}
	messages = g_list_reverse(messages);
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_END, &error);

	g_string_free(line, TRUE);

	return messages;
}

void gebr_log_messages_free(GList * messages)
{
	g_list_foreach(messages, (GFunc) gebr_log_message_free, NULL);
	g_list_free(messages);
}

void gebr_log_add_message(GebrLog *log, GebrLogMessageType type, const gchar * message)
{
	GString *line;
	gchar *ident_str;
	gsize bytes_written;
	GError *error;

	/* initialization */
	line = g_string_new(NULL);
	error = NULL;

	switch (type) {
	case GEBR_LOG_START:
		ident_str = "[STR]";
		break;
	case GEBR_LOG_END:
		ident_str = "[END]";
		break;
	case GEBR_LOG_INFO:
		ident_str = "[INFO]";
		break;
	case GEBR_LOG_ERROR:
		ident_str = "[ERR]";
		break;
	case GEBR_LOG_WARNING:
		ident_str = "[WARN]";
		break;
	case GEBR_LOG_DEBUG:
#ifdef DEBUG
		ident_str = "[DEB]";
		break;
#else
		return;
#endif
	default:
		ident_str = "[UNK]";
		break;
	}

	/* assembly log line and write to file */
	g_string_printf(line, "%s %s %s\n", ident_str, gebr_iso_date(), message);
	g_io_channel_write_chars(log->io_channel, line->str, line->len, &bytes_written, &error);
	error = NULL;
	g_io_channel_flush(log->io_channel, &error);

	g_string_free(line, TRUE);
}

const gchar *
gebr_log_message_get_date(GebrLogMessage *message)
{
	return message->date->str;
}

const gchar *
gebr_log_message_get_message(GebrLogMessage *message)
{
	return message->message->str;
}

GebrLogMessageType
gebr_log_message_get_type(GebrLogMessage *message)
{
	return message->type;
}
