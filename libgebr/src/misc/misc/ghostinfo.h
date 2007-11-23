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
 *
 *   Inspired on Qt 4.3 version of QHostInfo, by Trolltech
 */

#ifndef __G_HOST_INFO
#define __G_HOST_INFO

#include <glib.h>

#include "ghostaddress.h"

G_BEGIN_DECLS

typedef struct _GHostInfo	GHostInfo;

enum GHostInfoError {
	G_HOST_INFO_ERROR_NONE,
	G_HOST_INFO_ERROR_NOT_FOUND,
	G_HOST_INFO_ERROR_NO_ADDRESS,
	G_HOST_INFO_ERROR_TRY_AGAIN,
	G_HOST_INFO_ERROR_UNKNOWN,
};

struct _GHostInfo {
	enum GHostInfoError	error;
	GList *			addresses;
};

typedef void (*GHostInfoFunc)	(GHostInfo * host_info, gpointer user_data);

GHostInfo *
g_host_info_new_from_name(GString * hostname);

void
g_host_info_lookup(GString * hostname, GHostInfoFunc callback, gpointer user_data);

void
g_host_info_free(GHostInfo * host_info);

enum GHostInfoError
g_host_info_error(GHostInfo * host_info);

gboolean
g_host_info_ok(GHostInfo * host_info);

GList *
g_host_info_addesses(GHostInfo * host_info);

GHostAddress *
g_host_info_first_address(GHostInfo * host_info);

G_END_DECLS

#endif //__G_HOST_INFO
