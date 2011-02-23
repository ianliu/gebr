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

#ifndef __GEBR_COMM_PROTOCOL_H
#define __GEBR_COMM_PROTOCOL_H

#include <glib.h>

#include "gebr-comm-streamsocket.h"

G_BEGIN_DECLS

/* DEPRECATED */

extern struct gebr_comm_protocol_defs gebr_comm_protocol_defs;
#define PROTOCOL_VERSION "1.0.6"

struct gebr_comm_message_def {
	guint		code_hash;
	const gchar *	code;
	gboolean	returns; // does this message send return (RET) command?
	gint		arg_number;
};

struct gebr_comm_protocol_defs {
	GHashTable *hash_table;

	/* messages identifiers hashes */
	struct gebr_comm_message_def ret_def;
	struct gebr_comm_message_def ini_def;
	struct gebr_comm_message_def err_def;
	struct gebr_comm_message_def qut_def;
	struct gebr_comm_message_def lst_def;
	struct gebr_comm_message_def job_def;
	struct gebr_comm_message_def run_def;
	struct gebr_comm_message_def rnq_def;
	struct gebr_comm_message_def flw_def;
	struct gebr_comm_message_def clr_def;
	struct gebr_comm_message_def end_def;
	struct gebr_comm_message_def kil_def;
	struct gebr_comm_message_def out_def;
	struct gebr_comm_message_def sta_def;
};

struct gebr_comm_message {
	/* message identifier */
	guint hash;
	/* argument size */
	gsize argument_size;
	/* message argument (can be an empty string) */
	GString *argument;
};

struct gebr_comm_protocol {
	/* the data being received is forming message(s) */
	GString *data;
	struct gebr_comm_message *message;
	/* received messages to be parsed */
	GList *messages;
	/* waiting for return of message with "hash"; 0 if it is not waiting responses */
	guint waiting_ret_hash;
	/* logged in with INI and RET? */
	gboolean logged;
	/* if we are logged, we received a host name from the peer */
	GString *hostname;
};

void gebr_comm_protocol_reset(struct gebr_comm_protocol *protocol);
void gebr_comm_message_free(struct gebr_comm_message *message);

G_END_DECLS
#endif				//__GEBR_COMM_PROTOCOL_H
