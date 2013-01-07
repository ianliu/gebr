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
#include "libgebr-gettext.h"

#include "gebr-comm-terminalprocess.h"
#include <string.h>
#include <libgebr.h>
#include <glib/gi18n-lib.h>

#define GEBR_PORT_PREFIX "gebr-port="
#define FINGERPRINT_MSG "fingerprint is "
#define CERTIFICATE_QUESTION "Are you sure"
#define CERTIFICATE_ERROR "@@@@@@@@@@@@@@@@@@@@@@@@@@@"
#define QUESTION_SUFFIX "(yes/no)?"
#define PASSWORD_SUFFIX "password:"
#define ACCEPTS_PUBLIC_KEY "Authentications that can continue:"
#define SSH_ERROR_PREFIX "ssh: "
#define SENDING_COMMAND "Sending command: "
#define REMOTE_FORWARD "remote forward success for:"
#define LIMITED_WRONG_PASSWORD "No more authentication methods to try"
#define LOCAL_FORWARD_ERROR "Could not request local forwarding."
#define HOST_VERIFICATION_ERROR "Host key verification failed."
#define DEBUG_LINE "debug1:"


G_DEFINE_TYPE(GebrCommSsh, gebr_comm_ssh, G_TYPE_OBJECT);

static void ssh_process_read(GebrCommTerminalProcess *process,
			     GebrCommSsh *self);

static void ssh_process_finished(GebrCommTerminalProcess *process,
				 GebrCommSsh *self);

static void gebr_comm_ssh_parse_output(GebrCommSsh *self,
				       GebrCommTerminalProcess *process,
				       GString * output);

enum {
	SSH_PASSWORD,
	SSH_QUESTION,
	SSH_ERROR,
	SSH_STDOUT,
	SSH_KEY,
	SSH_FINISHED,
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

typedef enum {
	SSH_OUT_STATE_INIT,
	SSH_OUT_STATE_COMMAND_OUTPUT,
} SshOutState;

struct _GebrCommSshPriv {
	GebrCommSshState state;
	GebrCommTerminalProcess *process;
	gchar *command;
	gint attempts;

	GString *buffer;
	GString *ssh_output;
	GString *out_buffer;
	SshOutState out_state;
	gchar *fingerprint;
};

static void
gebr_comm_ssh_finalize(GObject *object)
{
	GebrCommSsh *self = GEBR_COMM_SSH(object);
	g_free(self->priv->command);
	g_string_free(self->priv->ssh_output, TRUE);
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

	signals[SSH_FINISHED] =
		g_signal_new("ssh-finished",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommSshClass, ssh_finished),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_type_class_add_private(klass, sizeof(GebrCommSshPriv));
}

static void
gebr_comm_ssh_init(GebrCommSsh *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_COMM_TYPE_SSH,
						 GebrCommSshPriv);

	self->priv->state = GEBR_COMM_SSH_STATE_INIT;

	self->priv->process = gebr_comm_terminal_process_new();
	self->priv->ssh_output = g_string_new("");

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
	g_return_if_fail(GEBR_COMM_IS_SSH(self));
	g_return_if_fail(self->priv->buffer == NULL);

	GString *tmp = g_string_new(self->priv->command);
	gebr_comm_terminal_process_start(self->priv->process, tmp);
	g_string_free(tmp, TRUE);

	self->priv->attempts = 0;
	self->priv->buffer = g_string_new(NULL);
	self->priv->out_buffer = g_string_new(NULL);
	self->priv->fingerprint = NULL;
	self->priv->out_state = SSH_OUT_STATE_INIT;
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
gebr_comm_ssh_password_error(GebrCommSsh *self,
                             gboolean wrong_pass)
{
	self->priv->attempts = 0;
	self->priv->state = GEBR_COMM_SSH_STATE_ERROR;

	const gchar *err;
	if (wrong_pass)
		err = "Wrong password. Please, try again.";
	else
		err = "Please, type the password to connect.";

	g_signal_emit(self, signals[SSH_ERROR], 0, err);
}

void
gebr_comm_ssh_set_password(GebrCommSsh *self, const gchar *password)
{
	if (!password) {
		gebr_comm_ssh_password_error(self, FALSE);
		return;
	}

	self->priv->attempts++;
	if (self->priv->state == GEBR_COMM_SSH_STATE_PASSWORD)
		write_pass_in_process(self->priv->process, password);
}

void
gebr_comm_ssh_connect_finished_callback(GebrCommSsh *self,
					void *finished_callback,
					gpointer user_data)
{
	g_signal_connect(self->priv->process, "finished",
			 G_CALLBACK(finished_callback), user_data);

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

	gebr_comm_ssh_parse_output(self, process, output);
}

static void
ssh_process_finished(GebrCommTerminalProcess *process,
		     GebrCommSsh *self)
{
	self->priv->state = GEBR_COMM_SSH_STATE_FINISHED;
	gebr_comm_terminal_process_free(process);
	g_signal_emit(self, signals[SSH_FINISHED], 0);
}

static gchar *
get_next_line(GebrCommSsh *self)
{
	gchar *buf = self->priv->buffer->str;
	gchar *nl = strchr(buf, '\n');
	gchar *line;

	if (!nl) {
		if (!g_strrstr(buf, QUESTION_SUFFIX)
		    && !g_strrstr(buf, PASSWORD_SUFFIX))
			return NULL;

		line = g_strdup(buf);
		g_string_assign(self->priv->buffer, "");
	} else {
		gssize size = (nl - buf) / sizeof(gchar);
		line = g_strndup(buf, size);
		g_string_erase(self->priv->buffer, 0, size + 1);
	}

	return g_strstrip(line);
}

static void
process_ssh_line(GebrCommSsh *self,
		 const gchar *line)
{
	if (self->priv->out_state == SSH_OUT_STATE_INIT) {
		gchar *finger = strstr(line, FINGERPRINT_MSG);
		if (finger) {
			GString *tmp = g_string_new(finger + strlen(FINGERPRINT_MSG));
			if (tmp->str[tmp->len-1] == '.')
				g_string_erase(tmp, tmp->len - 1, -1);

			if (self->priv->fingerprint)
				g_free(self->priv->fingerprint);
			self->priv->fingerprint = g_string_free(tmp, FALSE);
		}
		else if (g_strrstr(line, CERTIFICATE_QUESTION)) {
			self->priv->state = GEBR_COMM_SSH_STATE_QUESTION;

			gchar *question;
			question = g_markup_printf_escaped(_("This host has never been authenticated and"
							     " has the fingerprint %s.\n"),
							     self->priv->fingerprint);
			g_signal_emit(self, signals[SSH_QUESTION], 0, question);
			g_free(question);
		}
		else if (strstr(line, HOST_VERIFICATION_ERROR)) {
			self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
			g_signal_emit(self, signals[SSH_ERROR], 0, _("Host key verification failed."));
		}
		else if (g_strrstr(line, PASSWORD_SUFFIX)) {
			self->priv->state = GEBR_COMM_SSH_STATE_PASSWORD;
			g_signal_emit(self, signals[SSH_PASSWORD], 0, self->priv->attempts > 0);
		}
		else if (strstr(line, CERTIFICATE_ERROR)) {
			self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
			g_signal_emit(self, signals[SSH_ERROR], 0, _("The known_hosts file is probably corrupted."
								     " Contact your system administrator for help."));
		}
		else if (strstr(line, ACCEPTS_PUBLIC_KEY)) {
			gboolean accepts_key;
			if (g_strrstr(line, "publickey"))
				accepts_key = TRUE;
			else
				accepts_key = FALSE;
			g_signal_emit(self, signals[SSH_KEY], 0, accepts_key);
		}
		else if (g_str_has_prefix(line, SSH_ERROR_PREFIX)) {
			self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
			g_signal_emit(self, signals[SSH_ERROR], 0, line + strlen(SSH_ERROR_PREFIX));
		}
		else if (strstr(line, LIMITED_WRONG_PASSWORD)) {
			gebr_comm_ssh_password_error(self, TRUE);
		}
		else if (strstr(line, LOCAL_FORWARD_ERROR)) {
			self->priv->state = GEBR_COMM_SSH_STATE_ERROR;
			g_signal_emit(self, signals[SSH_ERROR], 0, GEBR_COMM_SSH_ERROR_LOCAL_FORWARD);
		}
		else if (strstr(line, SENDING_COMMAND) || strstr(line, REMOTE_FORWARD)) {
			self->priv->out_state = SSH_OUT_STATE_COMMAND_OUTPUT;
		}
	} else if (self->priv->out_state == SSH_OUT_STATE_COMMAND_OUTPUT) {
		if (g_str_has_prefix(line, "debug1: ")) {
			g_signal_emit(self, signals[SSH_STDOUT], 0, self->priv->out_buffer);
			self->priv->out_state = SSH_OUT_STATE_INIT;
		} else {
			g_string_append(self->priv->out_buffer, line);
			g_string_append_c(self->priv->out_buffer, '\n');
		}
	}
}

static void
fill_ssh_output(GebrCommSsh *self, const gchar *line)
{
	if (self->priv->out_state != SSH_OUT_STATE_COMMAND_OUTPUT &&
	    g_str_has_prefix(line, DEBUG_LINE) == FALSE) {
		g_string_append(self->priv->ssh_output, line);
		g_string_append_c(self->priv->ssh_output, '\n');
	}
}

static void
gebr_comm_ssh_parse_output(GebrCommSsh *self,
			   GebrCommTerminalProcess *process,
			   GString *output)
{
	g_string_append(self->priv->buffer, output->str);
	gchar *line = get_next_line(self);

	while (line != NULL) {
		process_ssh_line(self, line);
		fill_ssh_output(self, line);
		line = get_next_line(self);
	}
}

const gchar *
gebr_comm_ssh_get_ssh_output(GebrCommSsh *ssh)
{
	return ssh->priv->ssh_output->str;
}

void
gebr_comm_ssh_write_chars(GebrCommSsh *ssh, const gchar *data)
{
	GString *tmp = g_string_new(data);
	gebr_comm_terminal_process_write_string(ssh->priv->process, tmp);
	g_string_free(tmp, TRUE);
}
