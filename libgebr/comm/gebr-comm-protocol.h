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

#ifndef __GEBR_COMM_PROTOCOL_H__
#define __GEBR_COMM_PROTOCOL_H__

#include <glib.h>

#include "gebr-comm-streamsocket.h"

G_BEGIN_DECLS

extern struct gebr_comm_protocol_defs gebr_comm_protocol_defs;

struct gebr_comm_message_def {
	guint		code_hash;
	const gchar *	code;
	gboolean	returns; // does this message send return (RET) command?
	gint		arg_number;
};

enum {
	GEBR_COMM_PROTOCOL_PATH_CREATE,
	GEBR_COMM_PROTOCOL_PATH_RENAME,
	GEBR_COMM_PROTOCOL_PATH_DELETE,
	GEBR_COMM_PROTOCOL_N_COLUMN
};  

typedef enum {
	GEBR_COMM_PROTOCOL_STATUS_PATH_OK,
	GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS,
	GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR,
} GebrCommProtocolStatusPath;

struct gebr_comm_protocol_defs {
	GHashTable *hash_table;

	/* messages identifiers hashes */
	struct gebr_comm_message_def ret_def;
	struct gebr_comm_message_def ini_def;
	struct gebr_comm_message_def err_def;
	struct gebr_comm_message_def qut_def;
	struct gebr_comm_message_def lst_def;
	struct gebr_comm_message_def rnq_def;
	struct gebr_comm_message_def flw_def;
	struct gebr_comm_message_def clr_def;
	struct gebr_comm_message_def end_def;
	struct gebr_comm_message_def kil_def;
	struct gebr_comm_message_def out_def;
	struct gebr_comm_message_def sta_def;

	struct gebr_comm_message_def gid_def;   // Gebr id              Maestro -> Daemon

	struct gebr_comm_message_def cfrm_def;
	struct gebr_comm_message_def path_def;
	struct gebr_comm_message_def home_def;
	struct gebr_comm_message_def mpi_def;
	struct gebr_comm_message_def nfsid_def;

	struct gebr_comm_message_def prt_def;   // Maestro port         Maestro -> GeBR

	struct gebr_comm_message_def ac_def;    // Autoconnect option of the daemons
	struct gebr_comm_message_def agrp_def;  // Add group on GeBR
	struct gebr_comm_message_def dgrp_def;  // Delete group on GeBR

	struct gebr_comm_message_def run_def;   // Run request          Maestro -> Daemon
	struct gebr_comm_message_def tsk_def;   // Task definition      Daemon  -> Maestro

	struct gebr_comm_message_def jcl_def;   // Job Close 		Maestro -> GeBR
	struct gebr_comm_message_def job_def;   // Job definition       Maestro -> GeBR
	struct gebr_comm_message_def ssta_def;  // Server status change Maestro -> GeBR
	struct gebr_comm_message_def srm_def;   // Server remove	Maestro -> GeBR
	struct gebr_comm_message_def cmd_def;   // Command line         Maestro -> GeBR
	struct gebr_comm_message_def iss_def;   // Issues               Maestro -> GeBR

	struct gebr_comm_message_def qst_def;   // Question request     Maestro -> GeBR
	struct gebr_comm_message_def pss_def;   // Password request     Maestro -> GeBR

	struct gebr_comm_message_def harakiri_def;// Asks daemon to die Maestro -> Daemon
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
	GQueue *waiting_ret_hashs;
	/* logged in with INI and RET? */
	gboolean logged;
	/* if we are logged, we received a host name from the peer */
	GString *hostname;
};

void gebr_comm_protocol_reset(struct gebr_comm_protocol *protocol);

void gebr_comm_message_free(struct gebr_comm_message *message);

const gchar *gebr_comm_protocol_get_version(void);

const gchar *gebr_comm_protocol_path_enum_to_str(gint option);

gint gebr_comm_protocol_path_str_to_enum(const gchar *option);
G_END_DECLS

#endif /* __GEBR_COMM_PROTOCOL_H__ */
