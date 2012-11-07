/*
 * gebr-comm-ssh.c
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

#include "gebr-comm-ssh.h"

#include "gebr-comm-terminalprocess.h"
#include <string.h>
#include <libgebr.h>

#define GEBR_PORT_PREFIX "gebr-port="

G_DEFINE_TYPE(GebrCommSsh, gebr_comm_ssh, G_TYPE_OBJECT);

static void ssh_process_read(GebrCommTerminalProcess *process,
			     GebrCommSsh *self);

static void ssh_process_finished(GebrCommTerminalProcess *process,
				 GebrCommSsh *self);

static gboolean gebr_comm_ssh_parse_output(GebrCommSsh *self,
					   GebrCommTerminalProcess *process,
					   GString * output);

enum {
	SSH_PASSWORD,
	SSH_QUESTION,
	SSH_ERROR,
	SSH_STDOUT,
	SSH_KEY,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };

typedef enum {
	GEBR_COMM_SSH_STATE_INIT,
	GEBR_COMM_SSH_STATE_PASSWORD,
	GEBR_COMM_SSH_STATE_QUESTION,
	GEBR_COMM_SSH_STATE_ERROR,
	GEBR_COMM_SSH_STATE_FINISHED,
} GebrCommSshState;

struct _GebrCommSshPriv {
	GebrCommSshState state;
	GebrCommTerminalProcess *process;
	gchar *command;
	gboolean missed_password;
};

static void
gebr_comm_ssh_finalize(GObject *object)
{
	GebrCommSsh *self = GEBR_COMM_SSH(object);
	g_free(self->priv->command);
	gebr_comm_terminal_process_free(self->priv->process);
	G_OBJECT_CLASS(gebr_comm_ssh_parent_class)->finalize(object);
}

static void
gebr_comm_ssh_class_init(GebrCommSshClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gebr_comm_ssh_finalize;

	signals[SSH_PASSWORD] =
		g_signal_new("ssh-password",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_password),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__BOOLEAN,
			     G_TYPE_NONE, 1,
			     G_TYPE_BOOLEAN);

	signals[SSH_QUESTION] =
		g_signal_new("ssh-question",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_question),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1,
			     G_TYPE_STRING);

	signals[SSH_ERROR] =
		g_signal_new("ssh-error",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_error),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1,
			     G_TYPE_STRING);

	signals[SSH_STDOUT] =
		g_signal_new("ssh-stdout",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_stdout),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__POINTER,
			     G_TYPE_NONE, 1,
			     G_TYPE_GSTRING);

	signals[SSH_KEY] =
		g_signal_new("ssh-key",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_key),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__BOOLEAN,
			     G_TYPE_NONE, 1,
			     G_TYPE_BOOLEAN);

	g_type_class_add_private(klass, sizeof(GebrCommSshPriv));
}

static void
gebr_comm_ssh_init(GebrCommSsh *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_COMM_TYPE_SSH,
						 GebrCommSshPriv);

	self->priv->state = GEBR_COMM_SSH_STATE_INIT;
	self->priv->missed_password = FALSE;

	self->priv->process = gebr_comm_terminal_process_new();

	g_signal_connect(self->priv->process, "ready-read",
			 G_CALLBACK(ssh_process_read), self);
	g_signal_connect(self->priv->process, "finished",
			 G_CALLBACK(ssh_process_finished), self);
}

GebrCommSsh *
gebr_comm_ssh_new(void)
{
	return g_object_new(GEBR_COMM_TYPE_SSH, NULL);
}

void
gebr_comm_ssh_set_command(GebrCommSsh *self,
			  const gchar *command)
{
	if (self->priv->command)
		g_free(self->priv->command);
	self->priv->command = g_strdup(command);
}

void
gebr_comm_ssh_run(GebrCommSsh *self)
{
	GString *tmp = g_string_new(self->priv->command);
	gebr_comm_terminal_process_start(self->priv->process, tmp);
	g_string_free(tmp, TRUE);
}

static void
write_pass_in_process(GebrCommTerminalProcess *process,
		      const gchar *pass)
{
	gsize len = strlen(pass);
	GString p = {(gchar*)pass, len, len};
	gebr_comm_terminal_process_write_string(process, &p);
	p.str = "\n";
	p.len = p.allocated_len = 1;
	gebr_comm_terminal_process_write_string(process, &p);
}

void
gebr_comm_ssh_set_password(GebrCommSsh *self, const gchar *password)
{
	if (self->priv->state == GEBR_COMM_SSH_STATE_PASSWORD)
		write_pass_in_process(self->priv->process, password);
}

void
gebr_comm_ssh_answer_question(GebrCommSsh *self, gboolean response)
{
	if (self->priv->state == GEBR_COMM_SSH_STATE_QUESTION) {
		GString *answer = g_string_new(response ? "yes\n" : "no\n");
		gebr_comm_terminal_process_write_string(self->priv->process, answer);
	}
}

void
gebr_comm_ssh_kill(GebrCommSsh *self)
{
	gebr_comm_terminal_process_kill(self->priv->process);
}

static void
ssh_process_read(GebrCommTerminalProcess *process,
		 GebrCommSsh *self)
{
	GString *output;
	output = gebr_comm_terminal_process_read_string_all(process);

	if (!gebr_comm_ssh_parse_output(self, process, output))
		g_signal_emit(self, signals[SSH_STDOUT], 0, output);
}

static void
ssh_process_finished(GebrCommTerminalProcess *process,
		     GebrCommSsh *self)
{
	self->priv->state = GEBR_COMM_SSH_STATE_FINISHED;
	gebr_comm_terminal_process_free(process);
}

/*
 * Returns: %TRUE if an ssh message was parsed. Otherwise, the program's output
 * is being read.
 */
static gboolean
gebr_comm_ssh_parse_output(GebrCommSsh *self,
			   GebrCommTerminalProcess *process,
			   GString * output)
{
	if (output->len <= 2)
		return TRUE;

	gchar *start = g_strrstr(output->str, GEBR_PORT_PREFIX);
	if (start)
		return FALSE;

	for (gint i = 0; output->str[i]; i++) {
		if (!g_ascii_isdigit(output->str[i]))
			break;
		return FALSE;
	}

	gchar *point;

	if (output->str[output->len - 2] == ':') { 		/*Password*/
		self->priv->state = GEBR_COMM_SSH_STATE_PASSWORD;
		g_signal_emit(self, signals[SSH_PASSWORD], 0, self->priv->missed_password);
	}
	else if (output->str[output->len - 2] == '?') { 	/*Question*/
		self->priv->state = GEBR_COMM_SSH_STATE_QUESTION;
		g_signal_emit(self, signals[SSH_QUESTION], 0, output->str);
	}
	else if (g_str_has_prefix(output->str, "@@@")) { 	/*Error*/
		self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
		g_signal_emit(self, signals[SSH_ERROR], 0, output->str);
	}
	else if (!strcmp(output->str, "yes\r\n")) { 		/*User's positive feedback*/
		g_debug("On '%s', function: '%s', USER ANSWERED YES, output:'%s'", __FILE__, __func__, output->str);
	}
	else if (g_str_has_prefix(output->str, "ssh:") || g_str_has_prefix(output->str, "channel ")) { 	/*Known errors*/
		self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
		g_signal_emit(self, signals[SSH_ERROR], 0, output->str);
	}
	else if ((point = g_strrstr(output->str, "Authentications that can continue:")) != NULL) { /* SSH Public Key */
		if (point) {
			gboolean accepts_key = FALSE;
			gchar *last_point = g_strstr_len(point, strlen(point), "\n");
			if (last_point) {
				gchar *key_point = g_strstr_len(point, (last_point - point), "publickey");
				if (key_point)
					accepts_key = TRUE;
			}
			g_signal_emit(self, signals[SSH_KEY], 0, accepts_key);
		}
	}

	return TRUE;
}
