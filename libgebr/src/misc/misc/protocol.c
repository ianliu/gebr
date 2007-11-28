/*   GêBR Daemon - Process and control execution of flows
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "protocol.h"

/*
 * Internal variables and variables
 */

struct protocol_defs protocol_defs;

void
protocol_split_free_each(GString * string)
{
	g_string_free(string, TRUE);
}

/*
 * Public functions
 */

struct message *
message_new(void)
{
	struct message * message;

	message = g_malloc(sizeof(struct message));
	*message = (struct message) {
		.hash = 0,
		.argument_size = 0,
		.argument = g_string_new(NULL)
	};

	return message;
}

void
message_free(struct message * message)
{
	g_string_free(message->argument, TRUE);
	g_free(message);
}

#define create_message_def(str, resp) ((struct message_def){g_str_hash(str), str, resp})

void
protocol_init(void)
{
	/* create messages hashes */
	protocol_defs = (struct protocol_defs) {
		.ret_def = create_message_def("RET", FALSE),
		.ini_def = create_message_def("INI", TRUE),
		.qut_def = create_message_def("QUT", FALSE),
		.lst_def = create_message_def("LST", FALSE), /* return, but it is not a ret */
		.job_def = create_message_def("JOB", FALSE),
		.run_def = create_message_def("RUN", TRUE),
		.flw_def = create_message_def("FLW", TRUE),
		.clr_def = create_message_def("CLR", FALSE),
		.end_def = create_message_def("END", FALSE),
		.kil_def = create_message_def("KIL", FALSE),
		.out_def = create_message_def("OUT", FALSE),
		.fin_def = create_message_def("FIN", FALSE)
	};
}

struct protocol *
protocol_new(void)
{
	struct protocol *	new;

	new = g_malloc(sizeof(struct protocol));
	if (new == NULL)
		return NULL;

	*new = (struct protocol) {
		.data = g_string_new(NULL),
		.message = message_new(),
		.messages = NULL,
		.logged = FALSE,
		.hostname = g_string_new(NULL)
	};

	return new;
}

void
protocol_free(struct protocol * protocol)
{
	message_free(protocol->message);
	g_list_foreach(protocol->messages, (GFunc)message_free, NULL);
	g_list_free(protocol->messages);
	g_string_free(protocol->hostname, TRUE);
	g_free(protocol);
}

gboolean
protocol_receive_data(struct protocol * protocol, GString * data)
{
	gchar **	splits;
	gint		n_tokens;

	g_string_append(protocol->data, data->str);

	/* minimum message size: 6 */
	while (protocol->data->len >= 6) {
		gsize		missing;

		/* if so, this is a new message; otherwise, another part
		 * of the argument of protocol->message
		 */
		if (!protocol->message->hash) {
			gint	erase_len;
			gchar *	strtol_endptr;

			splits = g_strsplit(protocol->data->str, " ", 3);
			for (n_tokens = 0; splits[n_tokens] != NULL; n_tokens++);
			if (n_tokens < 2)
				goto err;

			/* code */
			protocol->message->hash = g_str_hash(splits[0]);
			/* argument size */
			strtol_endptr = NULL;
			protocol->message->argument_size = strtol(splits[1], &strtol_endptr, 10);
			if (errno == ERANGE)
				goto err;

			/* the missing bytes to complete the read of argument */
			missing = protocol->message->argument_size;
			/* erase code and size from protocol->data */
			erase_len = strlen(splits[0]) + 1 + strlen(splits[1]) + ((n_tokens == 2) ? 0 : 1);
			g_string_erase(protocol->data, 0, erase_len);

			g_strfreev(splits);
		} else {
			missing = protocol->message->argument_size - protocol->message->argument->len;
		}

		/* if so, protocol->message has now received its
		 * argument entirely
		 * using > because there must be a line break.
		 */
		if (protocol->data->len > missing) {
			struct message *	queued;

			g_string_append_len(protocol->message->argument, protocol->data->str, missing);
			g_string_erase(protocol->data, 0, missing+1);

			/* add to the list of messages */
			queued = protocol->message;
			protocol->messages = g_list_prepend(protocol->messages, queued);
			protocol->message = message_new();
		} else {
			g_string_append(protocol->message->argument, protocol->data->str);
			g_string_erase(protocol->data, 0, -1);
		}
	}

	return TRUE;

err:	g_strfreev(splits);
	return FALSE;
}

void
protocol_send_data(struct protocol * protocol, GTcpSocket * tcp_socket,
		struct message_def message_def, guint n_params, ...)
{
	GString *	message;
	va_list		ap;
	GString *	data;
	guint		i;

	/* very little code, but very interesting functionality ;) */
	data = g_string_new(NULL);
	va_start(ap, n_params);
	for (i = 1; i <= n_params; ++i) {
		gchar *	param;

		param = va_arg(ap, char *);
		g_string_append_printf(data, "%lu|%s", strlen(param), param);
		if (i != n_params)
			g_string_append(data, " ");
	}
	va_end(ap);

	/* does this message need return? */
	protocol->waiting_ret_hash = (message_def.returns == TRUE) ? message_def.hash : 0;
	/* assembly message */
	message = g_string_new(NULL);
	g_string_printf(message, "%s %lu %s\n", message_def.string, data->len, data->str);
	/* send it */
	g_socket_write_string(G_SOCKET(tcp_socket), message);

	g_string_free(data, TRUE);
	g_string_free(message, TRUE);
}

GList *
protocol_split_new(GString * arguments, guint parts)
{
	guint		i;
	gchar *		iarg;
	GList *		split;

	iarg = arguments->str;
	split = NULL;
	for (i = 1; i <= parts; ++i) {
		gchar *		sep;
		GString *	arg;
		gssize		arg_size;

		/* get the argument size */
		sscanf(iarg, "%lu|", &arg_size);

		/* discover the position of separator and get arg, data */
		sep = strchr(iarg, '|');
		arg = g_string_new("");
		if (arg_size)
			g_string_append_len(arg, sep+1, arg_size);

		/* add to list */
		split = g_list_append(split, arg);

		/* go to the next */
		iarg = sep + arg_size*sizeof(gchar);
		if (i != parts) {
			/* jump space between args */
			++iarg;
		}
	}

	return split;
}

void
protocol_split_free(GList * split)
{
	g_list_foreach(split, (GFunc)protocol_split_free_each, NULL);
	g_list_free(split);
}
