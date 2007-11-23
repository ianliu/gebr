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

#ifndef __LIBGEBR_MISC_PROTOCOL_H
#define __LIBGEBR_MISC_PROTOCOL_H

#include <glib.h>

#include "gtcpsocket.h"

extern struct protocol_defs protocol_defs;

struct message_def {
	guint		hash;
	gchar *		string;
	/* does this message send return (RET) command? */
	gboolean	returns;
};

struct protocol_defs {
	/* messages identifiers hashes */
	struct message_def	ret_def;
	struct message_def	ini_def;
	struct message_def	qut_def;
	struct message_def	lst_def;
	struct message_def	job_def;
	struct message_def	run_def;
	struct message_def	flw_def;
	struct message_def	clr_def;
	struct message_def	end_def;
	struct message_def	kil_def;
	struct message_def	out_def;
	struct message_def	fin_def;
};

struct message {
	/* message identifier */
	guint			hash;
	/* argument size */
	gsize			argument_size;
	/* message argument (can be an empty string) */
	GString *		argument;
};

struct protocol {
	/* the data being received is forming message(s) */
	GString *		data;
	struct message *	message;
	/* received messages to be parsed */
	GList *			messages;
	/* waiting for return of message with "hash"; 0 if it is not waiting responses */
	guint			waiting_ret_hash;
	/* logged in with INI and RET? */
	gboolean		logged;
	/* if we are logged, we received a host name from the client */
	GString *		hostname;
};

struct message *
message_new(void);

void
message_free(struct message * message);

void
protocol_init(void);

struct protocol *
protocol_new(void);

void
protocol_free(struct protocol * protocol);

gboolean
protocol_receive_data(struct protocol * protocol, GString * data);

void
protocol_send_data(struct protocol * protocol, GTcpSocket * tcp_socket,
		struct message_def message_def, guint n_params, ...);

GList *
protocol_split_new(GString * arguments, guint parts);

void
protocol_split_free(GList * split);

#endif //__LIBGEBR_MISC_PROTOCOL_H
