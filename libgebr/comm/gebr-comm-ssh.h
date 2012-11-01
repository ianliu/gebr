/*
 * gebr-comm-ssh.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

#ifndef __GEBR_COMM_SSH_H__
#define __GEBR_COMM_SSH_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEBR_COMM_TYPE_SSH            (gebr_comm_ssh_get_type ())
#define GEBR_COMM_SSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_TYPE_SSH, GebrCommSsh))
#define GEBR_COMM_SSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_COMM_TYPE_SSH, GebrCommSshClass))
#define GEBR_COMM_IS_SSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_TYPE_SSH))
#define GEBR_COMM_IS_SSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_COMM_TYPE_SSH))
#define GEBR_COMM_SSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_COMM_TYPE_SSH, GebrCommSshClass))

typedef struct _GebrCommSsh GebrCommSsh;
typedef struct _GebrCommSshPriv GebrCommSshPriv;
typedef struct _GebrCommSshClass GebrCommSshClass;

struct _GebrCommSsh {
	GObject parent;
	GebrCommSshPriv *priv;
};

struct _GebrCommSshClass {
	GObjectClass parent;

	/**
	 * GebrCommSsh::ssh-password:
	 *
	 * This signal is emitted when a password is needed for this ssh
	 * connection to continue. The password must be provided by means of
	 * gebr_comm_ssh_set_password() method.
	 *
	 * If @retry is %TRUE, then a password was already requested but it was
	 * not accepted.
	 */
	void (*ssh_password) (GebrCommSsh *self, gboolean retry);

	/**
	 * GebrCommSsh::ssh-question:
	 *
	 * This signal is emitted when a yes/no question is asked by this ssh
	 * connection. It must be answered by means of
	 * gebr_comm_ssh_answer_question() method.
	 */
	void (*ssh_question) (GebrCommSsh *self, const gchar *question);

	/**
	 * GebrCommSsh::ssh-error:
	 *
	 * This signal is emitted when there was an error in this ssh
	 * connection and it cannot be continued. The error message is provided
	 * by @msg parameter and is an exact copy of the ssh error.
	 */
	void (*ssh_error) (GebrCommSsh *self, const gchar *msg);

	/**
	 * GebrCommSsh::ssh-stdout:
	 *
	 * This signal is emitted when the command executed by ssh generates
	 * output.
	 */
	void (*ssh_stdout) (GebrCommSsh *self, const GString *buffer);
};

GType gebr_comm_ssh_get_type(void) G_GNUC_CONST;

GebrCommSsh *gebr_comm_ssh_new(void);

void gebr_comm_ssh_set_command(GebrCommSsh *self, const gchar *command);

void gebr_comm_ssh_run(GebrCommSsh *self);

void gebr_comm_ssh_set_password(GebrCommSsh *self, const gchar *password);

void gebr_comm_ssh_answer_question(GebrCommSsh *self, gboolean response);

void gebr_comm_ssh_kill(GebrCommSsh *self);

G_END_DECLS

#endif /* end of include guard: __GEBR_COMM_SSH_H__ */

