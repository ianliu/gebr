/*
 * gebr-maestro-server.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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

#ifndef __GEBR_MAESTRO_H__
#define __GEBR_MAESTRO_H__

#include <gtk/gtk.h>

#include "gebr-daemon-server.h"
#include "gebr-job.h"
#include <libgebr/gebr-maestro-info.h>

G_BEGIN_DECLS

#define GEBR_TYPE_MAESTRO_SERVER            (gebr_maestro_server_get_type ())
#define GEBR_MAESTRO_SERVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServer))
#define GEBR_MAESTRO_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServerClass))
#define GEBR_IS_MAESTRO_SERVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_MAESTRO_SERVER))
#define GEBR_IS_MAESTRO_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_MAESTRO_SERVER))
#define GEBR_MAESTRO_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_MAESTRO_SERVER, GebrMaestroServerClass))

typedef struct _GebrMaestroServer GebrMaestroServer;
typedef struct _GebrMaestroServerPriv GebrMaestroServerPriv;
typedef struct _GebrMaestroServerClass GebrMaestroServerClass;

typedef struct _passwordKeys {
	gchar *password;
	gboolean use_public_key;
} PasswordKeys;

struct _GebrMaestroServerClass {
	GObjectClass parent_class;

	void (*job_define) (GebrMaestroServer *maestro,
			    GebrJob           *job);

	void (*group_changed) (GebrMaestroServer *maestro);

	gboolean (*question_request) (GebrMaestroServer *maestro,
				      const gchar       *address,
				      const gchar       *title,
				      const gchar       *question);

	PasswordKeys * (*password_request) (GebrMaestroServer *maestro,
					    const gchar       *address,
					    gboolean	      accepts_key);

	void (*daemons_changed) (GebrMaestroServer *maestro);

	void    (*state_change) (GebrMaestroServer *maestro);

	void    (*ac_change) (GebrMaestroServer *maestro,
			      gboolean	         is_ac);

	void    (*mpi_changed) (GebrMaestroServer *maestro,
				GebrDaemonServer *daemon,
				const gchar *mpi_flavors);

	void    (*daemon_error) (GebrMaestroServer *maestro,
				 const gchar       *addr,
				 const gchar       *error_type,
				 const gchar       *error_msg);

	void    (*maestro_error) (GebrMaestroServer *maestro,
				  const gchar       *addr,
				  const gchar       *error_type,
				  const gchar       *error_msg);

	void (*confirm) (GebrMaestroServer *maestro,
			 const gchar *addr,
			 const gchar *type);

	void (*path_error) (GebrMaestroServer         *maestro,
			    GebrCommProtocolStatusPath error_id);
};

struct _GebrMaestroServer {
	GObject parent;
	GebrMaestroServerPriv *priv;
};

typedef enum {
	MAESTRO_SERVER_TYPE,
	MAESTRO_SERVER_NAME,
	MAESTRO_SERVER_HOST,
	MAESTRO_SERVER_N
} GebrMaestroServerGroupColumns;

/*
 * ** IMPORTANT **
 *
 * Keep the order of this enum equal the string list in the source file!
 */
typedef enum {
	MAESTRO_SERVER_TYPE_GROUP,
	MAESTRO_SERVER_TYPE_DAEMON,
} GebrMaestroServerGroupType;

GType gebr_maestro_server_get_type(void) G_GNUC_CONST;

GebrMaestroServer *gebr_maestro_server_new(const gchar *addr);

void gebr_maestro_server_add_daemon(GebrMaestroServer *maestro,
				    GebrDaemonServer *daemon);

GebrCommServer *gebr_maestro_server_get_server(GebrMaestroServer *maestro);

GtkTreeModel *gebr_maestro_server_get_model(GebrMaestroServer *maestro,
					    gboolean include_autochoose,
					    const gchar *group);

const gchar *gebr_maestro_server_get_address(GebrMaestroServer *maestro);

gchar *gebr_maestro_server_get_display_address(GebrMaestroServer *maestro);

GList *gebr_maestro_server_get_all_tags(GebrMaestroServer *maestro);

GtkTreeModel *gebr_maestro_server_get_queues_model(GebrMaestroServer *maestro);

void gebr_maestro_server_disconnect(GebrMaestroServer *maestro, gboolean quit);

void gebr_maestro_server_connect(GebrMaestroServer *maestro);

void gebr_maestro_server_add_temporary_job(GebrMaestroServer *maestro, GebrJob *job);

void gebr_maestro_server_add_tag_to(GebrMaestroServer *maestro,
				    GebrDaemonServer *daemon,
				    const gchar *tag);

void gebr_maestro_server_remove_tag_from(GebrMaestroServer *maestro,
                                         GebrDaemonServer *daemon,
                                         const gchar *tag);

void gebr_maestro_server_set_autoconnect(GebrMaestroServer *maestro,
                                         GebrDaemonServer *daemon,
                                         gboolean ac);

GebrMaestroServerGroupType gebr_maestro_server_group_str_to_enum(const gchar *str);

const gchar *gebr_maestro_server_group_enum_to_str(GebrMaestroServerGroupType type);

GtkTreeModel *gebr_maestro_server_get_groups_model(GebrMaestroServer *maestro);

GebrDaemonServer *gebr_maestro_server_get_daemon(GebrMaestroServer *server,
						 const gchar *address);

GebrCommServerState gebr_maestro_server_get_state(GebrMaestroServer *maestro);

void gebr_maestro_server_set_window(GebrMaestroServer *maestro, GtkWindow *window);

void gebr_maestro_server_set_error(GebrMaestroServer *maestro,
                                   const gchar *error_type,
                                   const gchar *error_msg);

void gebr_maestro_server_get_error(GebrMaestroServer *maestro,
				   const gchar **error_type,
				   const gchar **error_msg);

gchar *gebr_maestro_server_get_sftp_prefix(GebrMaestroServer *maestro);

const gchar *gebr_maestro_server_get_home_dir(GebrMaestroServer *maestro);

gchar *gebr_maestro_server_get_sftp_root(GebrMaestroServer *maestro);

void gebr_maestro_server_set_clocks_diff(GebrMaestroServer *maestro, gint secs);

gint gebr_maestro_server_get_clocks_diff(GebrMaestroServer *maestro);

GebrMaestroInfo *gebr_maestro_server_get_info(GebrMaestroServer *maestro);

gint gebr_maestro_server_get_ncores_for_group(GebrMaestroServer *maestro,
                                              const gchar *mpi_flavor,
                                              const gchar *group,
                                              GebrMaestroServerGroupType type);

gboolean gebr_maestro_server_has_servers(GebrMaestroServer *maestro,
                                         gboolean connected_servers);

gboolean gebr_maestro_server_has_connected_daemon(GebrMaestroServer *maestro);

G_END_DECLS

#endif /* __GEBR_MAESTRO_H__ */
