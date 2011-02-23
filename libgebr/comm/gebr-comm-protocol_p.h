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

#ifndef __GEBR_COMM_PROTOCOL_P_H
#define __GEBR_COMM_PROTOCOL_P_H

#include <glib.h>

#include "gebr-comm-streamsocket.h"
#include "gebr-comm-protocol.h"

G_BEGIN_DECLS

#define gebr_comm_message_def_create(code, resp, arg_number) ((struct gebr_comm_message_def){g_str_hash(code), code, resp, arg_number})

void gebr_comm_protocol_init(void);
void gebr_comm_protocol_destroy(void);

struct gebr_comm_message *gebr_comm_message_new(void);

struct gebr_comm_protocol *gebr_comm_protocol_new(void);

void gebr_comm_protocol_free(struct gebr_comm_protocol *protocol);

gboolean gebr_comm_protocol_receive_data(struct gebr_comm_protocol *protocol, GString * data);

GString * gebr_comm_protocol_build_messagev(struct gebr_comm_message_def msg_def, guint n_params, va_list ap);
GString * gebr_comm_protocol_build_message(struct gebr_comm_message_def msg_def, guint n_params, ...);

GList *gebr_comm_protocol_split_new(GString * arguments, guint parts);

void gebr_comm_protocol_split_free(GList * split);

G_END_DECLS
#endif				//__GEBR_COMM_PROTOCOL_P_H
