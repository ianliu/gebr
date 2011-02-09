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
 *
 *   Inspired on Qt 4.3 version of QHostInfo, by Trolltech
 */

#ifndef __GEBR_COMM_HOST_INFO_H
#define __GEBR_COMM_HOST_INFO_H

#include <glib.h>

#include "gebr-comm-socketaddress.h"

G_BEGIN_DECLS

typedef struct _GebrCommHostInfo GebrCommHostInfo;

enum GebrCommHostInfoError {
	GEBR_COMM_HOST_INFO_ERROR_NONE = 0,
	GEBR_COMM_HOST_INFO_ERROR_NOT_FOUND,
	GEBR_COMM_HOST_INFO_ERROR_NO_ADDRESS,
	GEBR_COMM_HOST_INFO_ERROR_TRY_AGAIN,
	GEBR_COMM_HOST_INFO_ERROR_UNKNOWN,
};

struct _GebrCommHostInfo {
	enum GebrCommHostInfoError error;
	GList *addresses;
};

typedef void (*GebrCommHostInfoFunc) (GebrCommHostInfo * host_info, gpointer user_data);

void gebr_comm_host_info_lookup(GString * hostname, GebrCommHostInfoFunc callback, gpointer user_data);

void gebr_comm_host_info_free(GebrCommHostInfo * host_info);

enum GebrCommHostInfoError gebr_comm_host_info_error(GebrCommHostInfo * host_info);

GList *gebr_comm_host_info_addesses(GebrCommHostInfo * host_info);

GebrCommSocketAddress *gebr_comm_host_info_first_address(GebrCommHostInfo * host_info);

GebrCommHostInfo *gebr_comm_host_info_lookup_blocking(GString * hostname);

GebrCommHostInfo *gebr_comm_host_info_lookup_local(void);

G_END_DECLS
#endif				//__GEBR_COMM_HOST_INFO_H
