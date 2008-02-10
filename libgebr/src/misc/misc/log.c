/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

struct log *
log_open(const gchar * path)
{
	struct log *	log;
	GError *	error;

	/* does it exist? if not, create it */
	if (g_access(path, F_OK)) {
		FILE *	fp;

		fp = fopen(path, "w");
		fclose(fp);
	}

	error = NULL;
	log = g_malloc(sizeof(struct log));
	*log = (struct log) {
		.io_channel = g_io_channel_new_file(path, "r+", &error),
	};
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_END, &error);

	return log;
}

struct log_message *
log_message_new(enum log_message_type type, const gchar * date, const gchar * message)
{
	struct log_message *	log_message;

	log_message = g_malloc(sizeof(struct log_message));
	*log_message = (struct log_message) {
		.type = type,
		.date = g_string_new(date),
		.message = g_string_new(message)
	};

	return log_message;
}

void
log_message_free(struct log_message * message)
{
	g_string_free(message->date, TRUE);
	g_string_free(message->message, TRUE);
	g_free(message);
}

void
log_close(struct log * log)
{
	g_io_channel_unref(log->io_channel);

	g_free(log);
}

GList *
log_messages_read(struct log * log)
{
	GList *		messages;
	GString *	line;
	GError *	error;

	error = NULL;
	messages = NULL;
	line = g_string_new(NULL);
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_SET, &error);
	while (g_io_channel_read_line_string(log->io_channel, line, NULL, &error) == G_IO_STATUS_NORMAL) {
		struct log_message *	message;
		enum log_message_type	type;
		gchar **		splits;

		splits = g_strsplit_set(line->str, " \n", 4);
		if (splits[0][0] != '[') {
			g_strfreev(splits);
			continue;
		}

		if (!g_ascii_strcasecmp(splits[0], "[ERR]"))
			type = LOG_ERROR;
		else if (!g_ascii_strcasecmp(splits[0], "[WARN]"))
			type = LOG_WARNING;
		else if (!g_ascii_strcasecmp(splits[0], "[INFO]"))
			type = LOG_INFO;
		else if (!g_ascii_strcasecmp(splits[0], "[STR]"))
			type = LOG_START;
		else if (!g_ascii_strcasecmp(splits[0], "[END]"))
			type = LOG_END;
		else {
			g_strfreev(splits);
			continue;
		}
		message = log_message_new(type, splits[1], splits[2]);
		messages = g_list_prepend(messages, message);

		g_strfreev(splits);
	}
	messages = g_list_reverse(messages);
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_END, &error);

	g_string_free(line, TRUE);

	return messages;
}

void
log_messages_free(GList * messages)
{
	g_list_foreach(messages, (GFunc)log_message_free, NULL);
	g_list_free(messages);
}

void
log_add_message(struct log * log, enum log_message_type type, const gchar * message)
{
	GString *	line;
	gchar *		ident_str;
	gsize		bytes_written;
	GError *	error;

	/* initialization */
	line = g_string_new(NULL);
	error = NULL;

	switch (type) {
	case LOG_START:
		ident_str = "[STR]";
		break;
	case LOG_END:
		ident_str = "[END]";
		break;
	case LOG_INFO:
		ident_str = "[INFO]";
		break;
	case LOG_ERROR:
		ident_str = "[ERR]";
		break;
	case LOG_WARNING:
		ident_str = "[WARN]";
		break;
	case LOG_DEBUG:
#ifdef LOGDEBUG
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
	g_string_printf(line, "%s %s %s\n", ident_str, iso_date(), message);
	g_io_channel_write_chars(log->io_channel, line->str, line->len, &bytes_written, &error);
	g_io_channel_flush(log->io_channel, &error);

	/* frees */
	g_string_free(line, TRUE);
}
