/*
 * gebrm-daemon.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011-2012 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBRM_DAEMON_H__
#define __GEBRM_DAEMON_H__

#include <glib-object.h>
#include <libgebr/comm/gebr-comm.h>

G_BEGIN_DECLS

#define GEBRM_TYPE_DAEMON            (gebrm_daemon_get_type ())
#define GEBRM_DAEMON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRM_TYPE_DAEMON, GebrmDaemon))
#define GEBRM_DAEMON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBRM_TYPE_DAEMON, GebrmDaemonClass))
#define GEBRM_IS_DAEMON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRM_TYPE_DAEMON))
#define GEBRM_IS_DAEMON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBRM_TYPE_DAEMON))
#define GEBRM_DAEMON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBRM_TYPE_DAEMON, GebrmDaemonClass))

typedef struct _GebrmDaemon GebrmDaemon;
typedef struct _GebrmDaemonPriv GebrmDaemonPriv;
typedef struct _GebrmDaemonClass GebrmDaemonClass;

struct _GebrmDaemon {
	GObject parent;
	GebrmDaemonPriv *priv;
};

struct _GebrmDaemonClass {
	GObjectClass parent_class;

	void (*state_change) (GebrmDaemon *daemon,
			      GebrCommServerState new_state);

	void (*task_define) (GebrmDaemon *daemon,
			     GObject *task);

	void (*password_request) (GebrmDaemon *daemon);

	void (*daemon_init) (GebrmDaemon *daemon,
			     const gchar *error_type,
			     const gchar *error_msg);

	void (*port_define) (GebrmDaemon *daemon,
			     const gchar *gid,
			     const gchar *port);
	void (*ret_path) (GebrmDaemon *daemon,
			     const gchar *daemon_addr,
			     const gchar *error);
};

GType gebrm_daemon_get_type(void) G_GNUC_CONST;

GebrmDaemon *gebrm_daemon_new(const gchar *address);

const gchar *gebrm_daemon_get_address(GebrmDaemon *daemon);

/**
 * gebrm_daemon_add_tag:
 *
 * Adds @tag into @daemon's tags list if it does not exists.
 */
void gebrm_daemon_add_tag(GebrmDaemon *daemon,
			  const gchar *tag);

/**
 * gebrm_daemon_update_tag:
 *
 * Update @tag into @daemon's tags list if it does not exists.
 */
gboolean
gebrm_daemon_update_tags(GebrmDaemon *daemon, gchar **tags);

/**
 * gebrm_daemon_has_tag:
 *
 * Returns: %TRUE if @daemon has @tag.
 */
gboolean gebrm_daemon_has_tag(GebrmDaemon *daemon,
			      const gchar *tag);

/**
 * gebrm_daemon_get_tags:
 *
 * Returns: All tags sorted and concatenated with commas. Free with g_free().
 */
gchar *gebrm_daemon_get_tags(GebrmDaemon *daemon);

gboolean gebrm_daemon_has_group(GebrmDaemon *daemon,
                                const gchar *group);

/**
 * gebrm_daemon_connect:
 * @daemon: The daemon to connect.
 * @pass:   The password to connecto to @daemon.
 * @client: The client that made this requisition.
 *
 * Connects to this @daemon sending @pass as password. If @pass is %NULL and
 * @daemon needs a password, then @client will be asked for a password.
 * Otherwise @client and @pass are not used.
 *
 * If @pass and @client are %NULL and @daemon needs a password, then @daemon
 * will wait for future user interaction, provided by
 * gebrm_daemon_continue_stuck_connection().
 */
void gebrm_daemon_connect(GebrmDaemon            *daemon,
			  const gchar            *pass,
			  GebrCommProtocolSocket *client);

void gebrm_daemon_disconnect(GebrmDaemon *daemon);

GebrCommServer *gebrm_daemon_get_server(GebrmDaemon *daemon);

GebrCommServerState gebrm_daemon_get_state(GebrmDaemon *daemon);

void gebrm_daemon_set_autoconnect(GebrmDaemon *daemon, const gchar *ac);

const gchar *gebrm_daemon_get_autoconnect(GebrmDaemon *daemon);

const gchar *gebrm_daemon_get_nfsid(GebrmDaemon *daemon);

gint gebrm_daemon_get_ncores(GebrmDaemon *daemon);

gdouble gebrm_daemon_get_clock(GebrmDaemon *daemon);

void gebrm_daemon_list_tasks_and_forward_x(GebrmDaemon *daemon);

void gebrm_daeamon_answer_question(GebrmDaemon *daemon,
				   const gchar *resp);

/**
 * gebrm_daemon_continue_stuck_connection:
 *
 * If connection on @daemon was already asked with gebrm_daemon_connect()
 * without a client (ie client parameter was %NULL) and @daemon is still not
 * connected, it is probably requesting user interaction; asking for password,
 * for instance. As the client did not exist, @daemon will be stuck there.
 *
 * This method is needed in this situation so @daemon can continue the
 * connection process with the interested client, represented by @socket.
 *
 * Note that @daemon must not have a client already, set by
 * gebrm_daemon_connect() method. Also, @daemon must be in the
 * @SERVER_STATE_RUN state. All these requirements are achieved by calling
 * gebrm_daemon_connect() with the parameter client equal to %NULL.
 */
void gebrm_daemon_continue_stuck_connection(GebrmDaemon *daemon,
					    GebrCommProtocolSocket *socket);

void gebrm_daemon_set_id(GebrmDaemon *daemon,
			 const gchar *id);

const gchar *gebrm_daemon_get_id(GebrmDaemon *daemon);

const gchar *gebrm_daemon_get_error_type(GebrmDaemon *daemon);

const gchar *gebrm_daemon_get_error_msg(GebrmDaemon *daemon);

void gebrm_daemon_set_error_type(GebrmDaemon *daemon,
                                 const gchar *error_type);

void gebrm_daemon_set_error_msg(GebrmDaemon *daemon,
                                const gchar *error_msg);

gint gebrm_daemon_get_uncompleted_tasks(GebrmDaemon *daemon);

GList *gebrm_daemon_get_list_of_jobs(GebrmDaemon *daemon);

void gebrm_daemon_send_client_info(GebrmDaemon *daemon,
				   const gchar *id,
				   const gchar *cookie);

const gchar *gebrm_daemon_get_hostname(GebrmDaemon *daemon);

void gebrm_daemon_send_error_message(GebrmDaemon *daemon,
                                     GebrCommProtocolSocket *socket);

void gebrm_daemon_set_disconnecting(GebrmDaemon *daemon,
				    gboolean setting);

gboolean gebrm_daemon_get_disconnecting(GebrmDaemon *daemon);

const gchar *gebrm_daemon_get_home_dir(GebrmDaemon *daemon);

void gebrm_daemon_set_mpi_flavors(GebrmDaemon *daemon, gchar *flavors);

const gchar *gebrm_daemon_get_mpi_flavors(GebrmDaemon *daemon);

gboolean gebrm_daemon_accepts_mpi(GebrmDaemon *daemon,
                                  const gchar *flavor);

G_END_DECLS

#endif /* __GEBRM_DAEMON_H__ */
