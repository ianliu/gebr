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
#include <glib-object.h>

#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/comm/gebr-comm-server.h>
#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

#define GEBR_TYPE_SERVER                 (gebr_server_get_type ())
#define GEBR_SERVER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_SERVER, GebrServer))
#define GEBR_SERVER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_SERVER, GebrServerClass))
#define GEBR_IS_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_SERVER))
#define GEBR_IS_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_SERVER))
#define GEBR_SERVER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_SERVER, GebrServerClass))

typedef struct _GebrServer GebrServer;
typedef struct _GebrServerClass GebrServerClass;

struct _GebrServer {
	/*< private >*/
	GObject parent;
	gint ncores;
	gdouble clock_cpu;

	/*< public >*/
	struct gebr_comm_server *comm;
	GtkTreeIter iter;
	GString *nfsid;
	GString *last_error; /* last error showed on tooltip */
	GebrCommServerType type;
	GtkListStore * queues_model;
	GtkListStore * accounts_model;
};

struct _GebrServerClass {
	GObjectClass parent_class;

	void (*initialized) (GebrServer *self);
};

GType gebr_server_get_type (void) G_GNUC_CONST;

/**
 * server_find_address:
 * @address: The server to be found.
 * @iter: A #GtkTreeIter that will hold the corresponding iterator.
 * @group: The group to find server in, or %NULL
 *
 * Searches for the @address server and fill @iter with the correct
 * iterator for the gebr.ui_server_list->common.store model.
 *
 * Returns: %TRUE if the server was found, %FALSE otherwise.
 */
gboolean server_find_address (const gchar *address,
			      GtkTreeIter *iter,
			      const gchar *group,
			      gboolean is_fs);

/**
 * Return the server name from \p address. Except for \p address 127.0.0.1,
 * returns \p address.
 */
const gchar *server_get_name_from_address(const gchar * address);

enum {
	SERVER_QUEUE_TITLE = 0, 
	SERVER_QUEUE_ID, 
	SERVER_QUEUE_LAST_RUNNING_JOB, 
	SERVER_QUEUE_N_COLUMNS, 
};

/**
 * @p address: The server address.
 * @p autoconnect: The server will connect whenever GêBR starts.
 *
 * Creates a new server.
 *
 * Returns: A server structure.
 */
GebrServer *gebr_server_new (const gchar * address, gboolean autoconnect, const gchar* tags);

/** 
 * Free \p server structure
 */
void server_free(GebrServer *server);

/**
 */
const gchar *server_get_name(GebrServer * server);

/** 
 * Find \p server iterator and put on \p iter
 */
gboolean server_find(GebrServer *server, GtkTreeIter * iter);

/**
 * Find the queue named \p queue_name.
 * If found, returns TRUE and set \p iter corresponding (from the list of queues). 
 */
gboolean server_queue_find(GebrServer * server, const gchar * queue_name, GtkTreeIter * iter);

/**
 * gebr_server_emit_initialized:
 * @self:
 *
 * <note>
 *   <para>Private function, do not use it!</para>
 * </note>
 *
 * Emits the GebrServer::initialized signal. This should be called
 * only when the server returns from the ini message, ie, in client.c.
 */
void gebr_server_emit_initialized (GebrServer *self);

/**
 * gebr_server_get_autoconnect:
 * @self:
 */
gboolean gebr_server_get_autoconnect (GebrServer *self);

/**
 * gebr_server_set_autoconnect:
 * @self:
 * @setting:
 */
void gebr_server_set_autoconnect (GebrServer *self, gboolean setting);

/**
 * gebr_server_connect:
 */
void gebr_server_connect (GebrServer *self);

/**
 * gebr_server_get_ncores:
 * @self: A #GebrServer
 *
 * Returns: the number of cores for server @self, as shown in /proc/cpuinfo.
 */
gint gebr_server_get_ncores (GebrServer *self);

/**
 * gebr_server_set_ncores:
 * @self: A #GebrServer
 * @cores: The number of cores for this server.
 */
void gebr_server_set_ncores (GebrServer *self, gint cores);

G_END_DECLS

#endif				//__SERVER_H
