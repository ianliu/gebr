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

#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "log.h"

/*
 * Internal functions
 */

void
log_messages_free_each(struct log_message * message)
{
	g_string_free(message->string, TRUE);
	g_free(message);
}

/*
 * Library functions
 */

struct log *
log_open(const gchar * path)
{
	struct log *	log;
	GIOStatus	io_status;

	GString *	line;
	GError *	error;

	/* does it exist? */
	if (g_access(path, F_OK)) {
		FILE *	fp;

		fp = fopen(path, "w");
		fclose(fp);
	}

	log = g_malloc(sizeof(log));
	line = g_string_new(NULL);
	error = NULL;
	*log = (struct log) {
		.io_channel = g_io_channel_new_file(path, "r+", &error),
		.messages = NULL
	};

// 	while (1) {
// 		GString *	tmp;
// 		GError *	error;
//
// 		error = NULL;
// 		tmp = g_string_new(NULL);
// 		io_status = g_io_channel_read_line_string(log->io_channel, line, NULL, &error);
// 		if (io_status != G_IO_STATUS_EOF) {
// 			struct log_message * log
// 		} else {
// 			break;
// 		}
// 	}
	/* FIXME: */
	g_io_channel_seek_position(log->io_channel, 0, G_SEEK_END, &error);

	g_string_free(line, TRUE);

	return log;
}

void
log_close(struct log * log)
{
	g_io_channel_unref(log->io_channel);

	g_list_foreach(log->messages, (GFunc)log_messages_free_each, NULL);
	g_list_free(log->messages);

	g_free(log);
}

void
log_add_message(struct log * log, enum log_message_type type, const gchar * message)
{
	GString *	line;
	gchar *		ident_str;
	gchar		time_str[30];
	time_t		t;
	struct tm *	lt;
	gsize		bytes_written;
	GError *	error;

	/* initialization */
	line = g_string_new(NULL);
	error = NULL;

	switch (type) {
	case START:
		ident_str = "[STR]";
		break;
	case END:
		ident_str = "[END]";
		break;
	case INFO:
		ident_str = "[INFO]";
		break;
	case ERROR:
		ident_str = "[ERR]";
		break;
	case WARNING:
		ident_str = "[WARN]";
		break;
	case DEBUG:
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

	/* TODO: use own date library */
	time(&t);
	lt = localtime(&t);
	strftime(time_str, 30, "%F %T", lt);

	/* assembly log line and write to file */
	g_string_printf(line, "%s %s %s\n", ident_str, time_str, message);
	g_io_channel_write_chars(log->io_channel, line->str, line->len, &bytes_written, &error);
	g_io_channel_flush(log->io_channel, &error);

	/* frees */
	g_string_free(line, TRUE);
}
