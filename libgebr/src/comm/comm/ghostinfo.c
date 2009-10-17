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

#include <unistd.h>
#include <netdb.h>
extern int h_errno;
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ghostinfo.h"

// PORT TO getaddrinfo

/*
 * Internal functions
 */

// struct thread_data {
// 	GString *	hostname;
// 	GebrCommHostInfoFunc	callback;
// 	gpointer	user_data;
// };
// 
// void
// __g_host_info_init(GebrCommHostInfo * host_info)
// {
// 	host_info->error = GEBR_COMM_HOST_INFO_ERROR_NONE;
// 	host_info->addresses = NULL;
// }
// 
// GebrCommHostInfo *
// __g_host_info_new(void)
// {
// 	GebrCommHostInfo *	new;
// 
// 	new = g_malloc(sizeof(GebrCommHostInfo));
// 	if (new == NULL)
// 		return NULL;
// 	__g_host_info_init(new);
// 
// 	return new;
// }
// 
// GebrCommHostInfo *
// __g_host_info_lookup_thread(struct thread_data * thread_data)
// {
// 	GebrCommHostInfo *		new;
// 	struct hostent *	hostent;
// 	gchar *			address;
// 	gint			i;
// 
// 	new = __g_host_info_new();
// 	if ((hostent = gethostbyname(thread_data->hostname->str)) == NULL) {
// 		switch (h_errno) {
// 		case HOST_NOT_FOUND:
// 			new->error = GEBR_COMM_HOST_INFO_ERROR_NOT_FOUND;
// 			break;
// 		case NO_ADDRESS:
// 			new->error = GEBR_COMM_HOST_INFO_ERROR_NO_ADDRESS;
// 			break;
// 		case TRY_AGAIN:
// 			new->error = GEBR_COMM_HOST_INFO_ERROR_TRY_AGAIN;
// 			break;
// // 		case NO_RECOVERY:
// 		default:
// 			new->error = GEBR_COMM_HOST_INFO_ERROR_UNKNOWN;
// 			break;
// 		}
// 		goto out;
// 	}
// 	new->error = GEBR_COMM_HOST_INFO_ERROR_NONE;
// 
// 	/* create list of GebrCommSocketAddress */
// 	i = 0;
// 	while (1) {
// 		address = hostent->h_addr_list[i++];
// 		if (address == NULL)
// 			break;
// 
// 		GebrCommSocketAddress *	socket_address;
// 
// 		/* TODO: check _address_ family */
// 		socket_address = g_socket_address_new("", GEBR_COMM_SOCKET_ADDRESS_TYPE_IPV4);
// 		g_socket_address_set_ipv4(socket_address, address.s_addr);
// 		new->addresses = g_list_append(new->addresses, socket_address);
// 	}
// 
// out:	if (thread_data->callback != NULL)
// 		thread_data->callback(new, thread_data->user_data);
// 
// 	g_string_free(thread_data->hostname, TRUE);
// 	g_free(thread_data);
// 	return new;
// }
// 
// GebrCommHostInfo *
// __g_host_info_lookup(GString * hostname, GebrCommHostInfoFunc callback, gpointer user_data)
// {
// 	struct thread_data *	thread_data;
// 	GThread *		thread;
// 	GError *		error;
// 	gboolean		joinable;
// 
// 	thread_data = g_malloc(sizeof(struct thread_data));
// 	if (thread_data == NULL)
// 		return NULL;
// 	*thread_data = (struct thread_data) {
// 		.hostname = g_string_new(hostname->str),
// 		.callback = callback,
// 		.user_data = user_data
// 	};
// 
// 	if (!g_thread_supported())
// 		 g_thread_init(NULL);
// 	error = NULL;
// 	joinable = (callback == NULL) ? TRUE : FALSE;
// 	thread = g_thread_create((GThreadFunc)__g_host_info_lookup_thread,
// 		thread_data, joinable, &error);
// 	if (joinable)
// 		return (GebrCommHostInfo*)g_thread_join(thread);
// 
// 	return NULL;
// }
// 
// /*
//  * API functions
//  */
// 
// void
// gebr_comm_host_info_lookup(GString * hostname, GebrCommHostInfoFunc callback, gpointer user_data)
// {
// 	__g_host_info_lookup(hostname, callback, user_data);
// }
// 
// void
// gebr_comm_host_info_free(GebrCommHostInfo * host_info)
// {
// 	g_list_foreach(host_info->addresses, (GFunc)g_socket_address_free, NULL);
// 	g_list_free(host_info->addresses);
// 	g_free(host_info);
// }
// 
// enum GebrCommHostInfoError
// gebr_comm_host_info_error(GebrCommHostInfo * host_info)
// {
// 	return host_info->error;
// }
// 
// GList *
// gebr_comm_host_info_addesses(GebrCommHostInfo * host_info)
// {
// 	return host_info->addresses;
// }
// 
// GebrCommSocketAddress *
// gebr_comm_host_info_first_address(GebrCommHostInfo * host_info)
// {
// 	GList *	link;
// 
// 	link = g_list_first(host_info->addresses);
// 	if (link == NULL)
// 		return NULL;
// 
// 	return (GebrCommSocketAddress*)link->data;
// }
// 
// GebrCommHostInfo *
// gebr_comm_host_info_lookup_blocking(GString * hostname)
// {
// 	return __g_host_info_lookup(hostname, NULL, NULL);
// }
// 
// GebrCommHostInfo *
// gebr_comm_host_info_lookup_local(void)
// {
// 	GebrCommHostInfo *	host_info;
// 	GString *	hostname;
// 	char		name[512];
// 
// 	gethostname(name, 512);
// 	hostname = g_string_new(name);
// 	host_info = __g_host_info_lookup(hostname, NULL, NULL);
// 
// 	g_string_free(hostname, TRUE);
// 	return host_info;
// }
