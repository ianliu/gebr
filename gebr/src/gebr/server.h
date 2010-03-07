/*   GeBR - An environment for seismic processing.
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

#ifndef __SERVER_H
#define __SERVER_H

#include <gtk/gtk.h>

#include <libgebr/comm/protocol.h>
#include <libgebr/comm/server.h>
#include <libgebr/geoxml.h>

/**
 * @p address: The server to be found.
 * @p iter: A #GtkTreeIter that will hold the corresponding iterator.
 *
 * Searches for the @p address server and fill @p iter with the correct
 * iterator for the gebr.ui_server_list->common.store model.
 *
 * Returns: TRUE if the server was found, FALSE otherwise.
 */
gboolean server_find_address(const gchar * address, GtkTreeIter * iter);

/**
 * Return the server name from \p address. Except for \p address 127.0.0.1,
 * returns \p address.
 */
const gchar *server_get_name_from_address(const gchar * address);

struct server {
	struct gebr_comm_server *comm;
	GtkTreeIter iter;

	/* last error showed on tooltip */
	GString *last_error;

	GebrCommServerType type;
	GtkListStore * queues_model;
	GtkListStore * accounts_model;

	GString *ran_jid;
};

/**
 * @p address: The server address.
 * @p autoconnect: The server will connect whenever GêBR starts.
 *
 * Creates a new server.
 *
 * Returns: A server structure.
 */
struct server *server_new(const gchar * address, gboolean autoconnect);

/** 
 * Free \p server structure
 */
void server_free(struct server *server);

/** 
 * Find \p server iterator and put on \p iter
 */
gboolean server_find(struct server *server, GtkTreeIter * iter);

/**
 * Find the queue named \p queue_name.
 * If found, returns TRUE and set \p iter corresponding (from the list of queues). 
 */
gboolean server_queue_find(struct server * server, const gchar * queue_name, GtkTreeIter * iter);

/**
 * Find the queue named \p queue_name and set \p iter corresponding (from the job control UI).
 * If not found, the queue is added.
 */
void server_queue_find_at_job_control(struct server * server, const gchar * queue_name, GtkTreeIter * _iter);

#endif				//__SERVER_H
