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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "gebr-comm-protocol.h"
#include "gebr-comm-protocol_p.h"
#include "gebr-version.h"

/*
 * Internal variables and variables
 */

struct gebr_comm_protocol_defs gebr_comm_protocol_defs;

void gebr_comm_protocol_split_free_each(GString * string)
{
	g_string_free(string, TRUE);
}

/*
 * Public functions
 */

void gebr_comm_protocol_init(void)
{
	/* create messages definitions */
	gebr_comm_protocol_defs.ret_def  = gebr_comm_message_def_create("RET", FALSE, -1);
	gebr_comm_protocol_defs.err_def  = gebr_comm_message_def_create("ERR", FALSE,  1);
	gebr_comm_protocol_defs.ini_def  = gebr_comm_message_def_create("INI", TRUE,   6);
	gebr_comm_protocol_defs.qut_def  = gebr_comm_message_def_create("QUT", FALSE,  0);
	gebr_comm_protocol_defs.lst_def  = gebr_comm_message_def_create("LST", FALSE,  0); /* return JOBs, not RET */
	gebr_comm_protocol_defs.job_def  = gebr_comm_message_def_create("JOB", FALSE, 18);
	gebr_comm_protocol_defs.run_def  = gebr_comm_message_def_create("RUN", FALSE,   5);
	gebr_comm_protocol_defs.rnq_def  = gebr_comm_message_def_create("RNQ", FALSE,  2);
	gebr_comm_protocol_defs.clr_def  = gebr_comm_message_def_create("CLR", FALSE,  1);
	gebr_comm_protocol_defs.end_def  = gebr_comm_message_def_create("END", FALSE,  1);
	gebr_comm_protocol_defs.jcl_def  = gebr_comm_message_def_create("JCL", FALSE, 1);
	gebr_comm_protocol_defs.kil_def  = gebr_comm_message_def_create("KIL", FALSE,  1);
	gebr_comm_protocol_defs.out_def  = gebr_comm_message_def_create("OUT", FALSE,  4);
	gebr_comm_protocol_defs.sta_def  = gebr_comm_message_def_create("STA", FALSE,  5);
	gebr_comm_protocol_defs.prt_def  = gebr_comm_message_def_create("PRT", FALSE,  1);
	gebr_comm_protocol_defs.cfrm_def = gebr_comm_message_def_create("CFRM", FALSE, 1);
	gebr_comm_protocol_defs.path_def = gebr_comm_message_def_create("PATH", TRUE, 3);
	gebr_comm_protocol_defs.home_def = gebr_comm_message_def_create("HOME", FALSE, 1);
	gebr_comm_protocol_defs.nfsid_def = gebr_comm_message_def_create("NFS", FALSE, 1);
	gebr_comm_protocol_defs.mpi_def = gebr_comm_message_def_create("MPI", FALSE, 2);

	gebr_comm_protocol_defs.gid_def  = gebr_comm_message_def_create("GID", FALSE,  1);

	gebr_comm_protocol_defs.ssta_def = gebr_comm_message_def_create("SST", FALSE,  5);
	gebr_comm_protocol_defs.srm_def = gebr_comm_message_def_create("SRM", FALSE,  1);

	gebr_comm_protocol_defs.flw_def  = gebr_comm_message_def_create("FLW", TRUE,   1); /* UNUSED */

	gebr_comm_protocol_defs.ac_def    = gebr_comm_message_def_create("AC", FALSE, 2);
	gebr_comm_protocol_defs.agrp_def  = gebr_comm_message_def_create("AGRP", FALSE, 1);
	gebr_comm_protocol_defs.dgrp_def  = gebr_comm_message_def_create("GGRP", FALSE, 1);
	gebr_comm_protocol_defs.tsk_def   = gebr_comm_message_def_create("TSK", FALSE, 1);
	gebr_comm_protocol_defs.iss_def   = gebr_comm_message_def_create("ISS", FALSE, 1);
	gebr_comm_protocol_defs.cmd_def   = gebr_comm_message_def_create("CMD", FALSE, 1);
	gebr_comm_protocol_defs.pss_def   = gebr_comm_message_def_create("PSS", FALSE, 3);
	gebr_comm_protocol_defs.qst_def   = gebr_comm_message_def_create("QST", FALSE, 3);
	gebr_comm_protocol_defs.harakiri_def = gebr_comm_message_def_create("HRK", FALSE, 0);
	gebr_comm_protocol_defs.dsp_def = gebr_comm_message_def_create("DSP", FALSE, 0);

	/* hashes them */
	gebr_comm_protocol_defs.hash_table = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.ret_def.code,  &gebr_comm_protocol_defs.ret_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.err_def.code,  &gebr_comm_protocol_defs.err_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.ini_def.code,  &gebr_comm_protocol_defs.ini_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.qut_def.code,  &gebr_comm_protocol_defs.qut_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.lst_def.code,  &gebr_comm_protocol_defs.lst_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.job_def.code,  &gebr_comm_protocol_defs.job_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.run_def.code,  &gebr_comm_protocol_defs.run_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.rnq_def.code,  &gebr_comm_protocol_defs.rnq_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.flw_def.code,  &gebr_comm_protocol_defs.flw_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.clr_def.code,  &gebr_comm_protocol_defs.clr_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.end_def.code,  &gebr_comm_protocol_defs.end_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.jcl_def.code,  &gebr_comm_protocol_defs.jcl_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.kil_def.code,  &gebr_comm_protocol_defs.kil_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.out_def.code,  &gebr_comm_protocol_defs.out_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.sta_def.code,  &gebr_comm_protocol_defs.sta_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.gid_def.code,  &gebr_comm_protocol_defs.gid_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.prt_def.code,  &gebr_comm_protocol_defs.prt_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.ssta_def.code, &gebr_comm_protocol_defs.ssta_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.srm_def.code, &gebr_comm_protocol_defs.srm_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.cfrm_def.code, &gebr_comm_protocol_defs.cfrm_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.path_def.code, &gebr_comm_protocol_defs.path_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.home_def.code, &gebr_comm_protocol_defs.home_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.nfsid_def.code, &gebr_comm_protocol_defs.nfsid_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.mpi_def.code, &gebr_comm_protocol_defs.mpi_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.ac_def.code, &gebr_comm_protocol_defs.ac_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.agrp_def.code, &gebr_comm_protocol_defs.agrp_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.dgrp_def.code, &gebr_comm_protocol_defs.dgrp_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.tsk_def.code, &gebr_comm_protocol_defs.tsk_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.iss_def.code, &gebr_comm_protocol_defs.iss_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.cmd_def.code, &gebr_comm_protocol_defs.cmd_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.pss_def.code, &gebr_comm_protocol_defs.pss_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.qst_def.code, &gebr_comm_protocol_defs.qst_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.harakiri_def.code, &gebr_comm_protocol_defs.harakiri_def);
	g_hash_table_insert(gebr_comm_protocol_defs.hash_table, (gpointer)gebr_comm_protocol_defs.dsp_def.code, &gebr_comm_protocol_defs.dsp_def);

	gebr_comm_protocol_defs.code_hash_table = g_hash_table_new(NULL, NULL);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.ret_def.code_hash),  &gebr_comm_protocol_defs.ret_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.err_def.code_hash),  &gebr_comm_protocol_defs.err_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.ini_def.code_hash),  &gebr_comm_protocol_defs.ini_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.qut_def.code_hash),  &gebr_comm_protocol_defs.qut_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.lst_def.code_hash),  &gebr_comm_protocol_defs.lst_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.job_def.code_hash),  &gebr_comm_protocol_defs.job_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.run_def.code_hash),  &gebr_comm_protocol_defs.run_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.rnq_def.code_hash),  &gebr_comm_protocol_defs.rnq_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.flw_def.code_hash),  &gebr_comm_protocol_defs.flw_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.clr_def.code_hash),  &gebr_comm_protocol_defs.clr_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.end_def.code_hash),  &gebr_comm_protocol_defs.end_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.jcl_def.code_hash),  &gebr_comm_protocol_defs.jcl_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.kil_def.code_hash),  &gebr_comm_protocol_defs.kil_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.out_def.code_hash),  &gebr_comm_protocol_defs.out_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.sta_def.code_hash),  &gebr_comm_protocol_defs.sta_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.gid_def.code_hash),  &gebr_comm_protocol_defs.gid_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.prt_def.code_hash),  &gebr_comm_protocol_defs.prt_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.ssta_def.code_hash), &gebr_comm_protocol_defs.ssta_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.srm_def.code_hash), &gebr_comm_protocol_defs.srm_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.cfrm_def.code_hash), &gebr_comm_protocol_defs.cfrm_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.path_def.code_hash), &gebr_comm_protocol_defs.path_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.home_def.code_hash), &gebr_comm_protocol_defs.home_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.nfsid_def.code_hash), &gebr_comm_protocol_defs.nfsid_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.mpi_def.code_hash), &gebr_comm_protocol_defs.mpi_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.ac_def.code_hash), &gebr_comm_protocol_defs.ac_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.agrp_def.code_hash), &gebr_comm_protocol_defs.agrp_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.dgrp_def.code_hash), &gebr_comm_protocol_defs.dgrp_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.tsk_def.code_hash), &gebr_comm_protocol_defs.tsk_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.iss_def.code_hash), &gebr_comm_protocol_defs.iss_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.cmd_def.code_hash), &gebr_comm_protocol_defs.cmd_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.pss_def.code_hash), &gebr_comm_protocol_defs.pss_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.qst_def.code_hash), &gebr_comm_protocol_defs.qst_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.harakiri_def.code_hash), &gebr_comm_protocol_defs.harakiri_def);
	g_hash_table_insert(gebr_comm_protocol_defs.code_hash_table, GUINT_TO_POINTER(gebr_comm_protocol_defs.dsp_def.code_hash), &gebr_comm_protocol_defs.dsp_def);
}

void gebr_comm_protocol_destroy(void)
{
	g_hash_table_unref(gebr_comm_protocol_defs.hash_table);
} 

struct gebr_comm_message *gebr_comm_message_new(void)
{
	struct gebr_comm_message *message;

	message = g_new(struct gebr_comm_message, 1);
	message->hash = 0;
	message->argument_size = 0;
	message->argument = g_string_new(NULL);

	return message;
}

void gebr_comm_message_free(struct gebr_comm_message *message)
{
	if (message == NULL)
		return;
	g_string_free(message->argument, TRUE);
	g_free(message);
}

struct gebr_comm_protocol *gebr_comm_protocol_new(void)
{
	struct gebr_comm_protocol *protocol;

	protocol = g_new(struct gebr_comm_protocol, 1);
	protocol->data = g_string_new(NULL);
	protocol->message = NULL;
	protocol->messages = NULL;
	protocol->hostname = g_string_new(NULL);
	protocol->waiting_ret_hashs = g_queue_new();

	gebr_comm_protocol_reset(protocol);

	return protocol;
}

void gebr_comm_protocol_reset(struct gebr_comm_protocol *protocol)
{
	g_string_assign(protocol->data, "");
	protocol->logged = FALSE;

	gebr_comm_message_free(protocol->message);
	protocol->message = gebr_comm_message_new();

	g_list_foreach(protocol->messages, (GFunc)gebr_comm_message_free, NULL);
	g_list_free(protocol->messages);
	protocol->messages = NULL;
}

void gebr_comm_protocol_free(struct gebr_comm_protocol *protocol)
{
	g_string_free(protocol->data, TRUE);
	gebr_comm_message_free(protocol->message);
	g_list_foreach(protocol->messages, (GFunc)gebr_comm_message_free, NULL);
	g_list_free(protocol->messages);
	g_string_free(protocol->hostname, TRUE);
	g_queue_free(protocol->waiting_ret_hashs);
	g_free(protocol);
}

gboolean gebr_comm_protocol_receive_data(struct gebr_comm_protocol *protocol, GString * data)
{
	gchar **splits;
	gboolean reached_end = FALSE;

	while (!reached_end && data->len) {
		gsize missing;

		/* if so, this is a new message; otherwise, another part
		 * of the argument of protocol->message
		 */
		if (!protocol->message->hash) {
			gint erase_len;
			gchar *strtol_endptr;

			splits = g_strsplit(data->str, " ", 3);
			gint n_tokens = g_strv_length(splits);
			if (n_tokens < 2)
				goto err;

			/* code */
			if (g_hash_table_lookup(gebr_comm_protocol_defs.hash_table, splits[0]) == NULL) {
				g_string_assign(data, "");
				goto err;
			}
			protocol->message->hash = g_str_hash(splits[0]);
			/* argument size */
			strtol_endptr = NULL;
			protocol->message->argument_size = strtol(splits[1], &strtol_endptr, 10);
			if (errno == ERANGE)
				goto err;

			/* the missing bytes to complete the read of argument */
			missing = protocol->message->argument_size;
			/* erase code and size from data */
			erase_len = strlen(splits[0]) + 1 + strlen(splits[1]) + ((n_tokens == 2) ? 0 : 1);
			g_string_erase(data, 0, erase_len);

			g_strfreev(splits);
		} else {
			missing = protocol->message->argument_size - protocol->message->argument->len;
		}

		/* if so, protocol->message has now received its
		 * argument entirely using > because there must be a line feed.
		 */
		reached_end = data->len > missing; 
		if (reached_end) {
			struct gebr_comm_message *queued;

			g_string_append_len(protocol->message->argument, data->str, missing);
			g_string_erase(data, 0, missing + 1);

			/* add to the list of messages */
			queued = protocol->message;
			protocol->messages = g_list_prepend(protocol->messages, queued);
			protocol->message = gebr_comm_message_new();
		} else {
			g_string_append(protocol->message->argument, data->str);
			g_string_erase(data, 0, -1);
		}
	}

	return TRUE;

 err:	g_strfreev(splits);
	return FALSE;
}

GString * gebr_comm_protocol_build_messagev(struct gebr_comm_message_def msg_def, guint n_params, va_list ap)
{
	GString * data;
	GString * message;

	/* very little code, but very interesting functionality ;) */
	data = g_string_new(NULL);
	for (guint i = 1; i <= n_params; ++i) {
		gchar *param;
		param = va_arg(ap, char *);
		if (!param)
			param = "";
		g_string_append_printf(data, (i != n_params) ? "%zu|%s " : "%zu|%s", strlen(param), param);
	}
	va_end(ap);

	/* assembly message */
	message = g_string_new(NULL);
	g_string_printf(message, "%s %zu %s\n", msg_def.code, data->len, data->str);
	g_string_free(data, TRUE);

	return message;
}

GString * gebr_comm_protocol_build_message(struct gebr_comm_message_def msg_def, guint n_params, ...)
{
	va_list ap;
	va_start(ap, n_params);
	return gebr_comm_protocol_build_messagev(msg_def, n_params, ap);
}

GList *gebr_comm_protocol_split_new(GString * arguments, guint parts)
{
	gchar *iarg;
	GList *split;

	/* TODO: use static array instead of a GList */

	iarg = arguments->str;
	split = NULL;
	for (guint i = 0; i < parts; ++i) {
		GString *arg;
		gchar *sep;
		gsize arg_size;

		/* get the argument size */
		if (sscanf(iarg, "%zu|", &arg_size) == EOF)
			goto err;

		/* discover the position of separator and get arg, data */
		sep = strchr(iarg, '|') + sizeof(gchar);
		if (strlen(sep) < arg_size)
			goto err;
		arg = g_string_new("");
		g_string_append_len(arg, sep, arg_size);

		/* add to list */
		split = g_list_append(split, arg);

		/* go to the next */
		iarg = sep + arg_size;
		if (i != parts-1) {
			/* jump space between args */
			if (!strlen(iarg))
				goto err;

			++iarg;

			if (!strlen(iarg))
				goto err;
		}
	}

	return split;

err:	gebr_comm_protocol_split_free(split);
	return NULL;
}

void gebr_comm_protocol_split_free(GList * split)
{
	g_list_foreach(split, (GFunc)gebr_comm_protocol_split_free_each, NULL);
	g_list_free(split);
}

const gchar *
gebr_comm_protocol_get_version(void)
{
	return gebr_version();
}

const gchar *
gebr_comm_protocol_path_enum_to_str (gint option){
	switch(option){
	case GEBR_COMM_PROTOCOL_PATH_CREATE:
		return ("create");
	case GEBR_COMM_PROTOCOL_PATH_RENAME:
		return ("rename");
	case GEBR_COMM_PROTOCOL_PATH_DELETE:
		return ("delete");
	default:
		return NULL;
	}
}

gint gebr_comm_protocol_path_str_to_enum(const gchar *option){
	if (!g_strcmp0(option, "create"))
		return GEBR_COMM_PROTOCOL_PATH_CREATE;
	else if (!g_strcmp0(option, "rename"))
		return GEBR_COMM_PROTOCOL_PATH_RENAME;
	else if (!g_strcmp0(option, "delete"))
		return GEBR_COMM_PROTOCOL_PATH_DELETE;
	else
		return 0;
}
