/*
 * gebr-comm-port-provider.c
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

#include "gebr-comm-port-provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "gebr-comm-ssh.h"
#include <libgebr/utils.h>
#include "gebr-comm-terminalprocess.h"
#include "libgebr-gettext.h"
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "gebr-comm-listensocket.h"
#include "gebr-comm-process.h"
#include "../marshalers.h"
#include "gebr-comm-utils.h"

#define COMMAND_NOT_FOUND "command not found"

struct _GebrCommPortForward {
	GebrCommSsh *ssh;
	gchar *address;
};

typedef enum {
	STATE_INIT,
	STATE_PORT_DEFINED,
	STATE_FORWARD,
	STATE_ERROR
} PortProviderState;

struct _GebrCommPortProviderPriv {
	PortProviderState state;
	GebrCommPortType type;
	gchar *address;
	guint display_port;
	gchar *display_host;
	GebrCommSsh *ssh_forward;
	GebrCommPortForward *forward;

	guint port_timeout;

	guint port;
	guint remote_port;
	const gchar *remote_address;

	gboolean force_init;
	gboolean check_host;

	gchar *gebr_cookie;
};

static gchar *get_local_forward_command(GebrCommPortProvider *self,
					guint *port,
					const gchar *addr,
					guint remote_port);

static gboolean get_port_from_command_output(GebrCommPortProvider *self,
                                             const gchar *buffer,
                                             guint *port,
					     GError **error);

static void create_local_forward(GebrCommPortProvider *self);

GQuark
gebr_comm_port_provider_error_quark(void)
{
	return g_quark_from_static_string("gebr-comm-port-provider-error-quark");
}

/* GObject stuff {{{ */
G_DEFINE_TYPE(GebrCommPortProvider, gebr_comm_port_provider, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_TYPE,
	PROP_ADDRESS,
};

enum {
	PORT_DEFINED,
	ERROR,
	REPASS_PASSWORD,
	QUESTION,
	ACCEPTS_KEY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
gebr_comm_port_provider_get(GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);

	switch (prop_id)
	{
	case PROP_TYPE:
		g_value_set_int(value, self->priv->type);
		break;
	case PROP_ADDRESS:
		g_value_set_string(value, self->priv->address);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_comm_port_provider_set(GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);

	switch (prop_id)
	{
	case PROP_TYPE:
		self->priv->type = g_value_get_int(value);
		break;
	case PROP_ADDRESS:
		if (self->priv->address)
			g_free(self->priv->address);
		self->priv->address = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gebr_comm_port_provider_finalize(GObject *object)
{
	GebrCommPortProvider *self = GEBR_COMM_PORT_PROVIDER(object);
	g_free(self->priv->address);
	g_free(self->priv->display_host);
	G_OBJECT_CLASS(gebr_comm_port_provider_parent_class)->finalize(object);
}

static void
gebr_comm_port_provider_class_init(GebrCommPortProviderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = gebr_comm_port_provider_get;
	object_class->set_property = gebr_comm_port_provider_set;
	object_class->finalize = gebr_comm_port_provider_finalize;

	/**
	 * GebrCommPortProvider::port-define:
	 *
	 * This signal is emitted after gebr_comm_port_provider_start() is
	 * executed and if no error happens in the process. It provides the
	 * port that can be used to communicate with the specified type in the
	 * constructor gebr_comm_port_provider_new().
	 */
	signals[PORT_DEFINED] =
		g_signal_new("port-defined",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, port_defined),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__INT,
			     G_TYPE_NONE, 1,
			     G_TYPE_INT);

	/**
	 * GebrCommPortProvider::error:
	 *
	 * This signal is emitted after gebr_comm_port_provider_start() is
	 * executed and an error has occured during the process. See
	 * #GEBR_COMM_PORT_PROVIDER_ERROR for more information.
	 */
	signals[ERROR] =
		g_signal_new("error",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, error),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__POINTER,
			     G_TYPE_NONE, 1,
			     G_TYPE_POINTER);

	/**
	 * GebrCommPortProvider::password:
	 */
	signals[REPASS_PASSWORD] =
		g_signal_new("repass-password",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, repass_password),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__OBJECT_BOOLEAN,
			     G_TYPE_NONE, 2,
			     G_TYPE_OBJECT, G_TYPE_BOOLEAN);

	/**
	 * GebrCommPortProvider::question:
	 */
	signals[QUESTION] =
		g_signal_new("question",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, question),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__OBJECT_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_OBJECT, G_TYPE_STRING);

	/**
	 * GebrCommPortProvider::accepts-key
	 */
	signals[ACCEPTS_KEY] =
		g_signal_new("accepts-key",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrCommPortProviderClass, accepts_key),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__BOOLEAN,
			     G_TYPE_NONE, 1,
			     G_TYPE_BOOLEAN);

	g_object_class_install_property(object_class,
					PROP_TYPE,
					g_param_spec_int("type",
							 "Type",
							 "Type",
							 0, 100, 0,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Address",
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(GebrCommPortProviderPriv));
}

static void
gebr_comm_port_provider_init(GebrCommPortProvider *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_COMM_TYPE_PORT_PROVIDER,
						 GebrCommPortProviderPriv);
	self->priv->state = STATE_INIT;
	self->priv->check_host = TRUE;
}
/* }}} */

static gboolean
is_local_address(const gchar *addr)
{
	return gebr_comm_is_local_address(addr);
}

/*
 * emit_signals:
 *
 * This method will emit the correct signal given the @port and @error. If
 * @error is non-%NULL, then an GebrCommPortProvider::error signal is emitted,
 * otherwise the GebrCommPortProvider::port-defined signal is emitted.
 *
 * Note: the @error domain must be GEBR_COMM_PORT_PROVIDER_ERROR, see
 * #GebrCommPortProviderError enumeration for a list of possible errors.
 */
static void
emit_signals(GebrCommPortProvider *self, guint port, GError *error)
{
	if (!error) {
		self->priv->state = STATE_PORT_DEFINED;
		g_signal_emit(self, signals[PORT_DEFINED], 0, port);
	} else {
		self->priv->state = STATE_ERROR;
		g_signal_emit(self, signals[ERROR], 0, error);
	}
}

/*
 * PortProviderVirtualMethods:
 *
 * This structure provides some virtual methods that must be implemented by
 * both local and remote logics. Each method must calculate the port and call
 * emit_signals() when done or upon error. See emit_signals() documentation for
 * more info.
 */
struct PortProviderVirtualMethods {
	void (*get_maestro_port) (GebrCommPortProvider *self);
	void (*get_daemon_port)  (GebrCommPortProvider *self);
	void (*get_x11_port)     (GebrCommPortProvider *self);
	void (*get_sftp_port)    (GebrCommPortProvider *self);
};

void local_get_maestro_port (GebrCommPortProvider *self);
void local_get_daemon_port  (GebrCommPortProvider *self);
void local_get_x11_port     (GebrCommPortProvider *self);
void local_get_sftp_port    (GebrCommPortProvider *self);

void remote_get_maestro_port (GebrCommPortProvider *self);
void remote_get_daemon_port  (GebrCommPortProvider *self);
void remote_get_x11_port     (GebrCommPortProvider *self);
void remote_get_sftp_port    (GebrCommPortProvider *self);

struct PortProviderVirtualMethods local_methods = {
	.get_maestro_port = local_get_maestro_port,
	.get_daemon_port  = local_get_daemon_port,
	.get_x11_port     = local_get_x11_port,
	.get_sftp_port    = local_get_sftp_port,
};

struct PortProviderVirtualMethods remote_methods = {
	.get_maestro_port = remote_get_maestro_port,
	.get_daemon_port  = remote_get_daemon_port,
	.get_x11_port     = remote_get_x11_port,
	.get_sftp_port    = remote_get_sftp_port,
};

static void
start(GebrCommPortProvider *self, struct PortProviderVirtualMethods *vmethods)
{
	GError *error = NULL;

	switch (self->priv->type)
	{
	case GEBR_COMM_PORT_TYPE_MAESTRO:
		vmethods->get_maestro_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_DAEMON:
		vmethods->get_daemon_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_X11:
		vmethods->get_x11_port(self);
		break;
	case GEBR_COMM_PORT_TYPE_SFTP:
		vmethods->get_sftp_port(self);
		break;
	default:
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_UNKNOWN_TYPE,
			    "Unknown port type");
		emit_signals(self, 0, error);
		break;
	}
}

/* Set our own error*/
static void
transform_spawn_sync_error(GebrCommPortProvider *self, GError *error, GError **local_error)
{
	if (!error)
		return;

	gboolean is_maestro = self->priv->type == GEBR_COMM_PORT_TYPE_MAESTRO;

	switch (error->code) {
	case G_SPAWN_ERROR_NOENT:
		g_set_error(local_error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_NOT_FOUND,
			    _("%s not found at %s"),
			    is_maestro ? "Maestro" : "Daemon",
			    self->priv->address);
		break;
	default:
		g_set_error(local_error,
			    GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN,
			    "%s", error->message);
		break;
	}
}

static void
set_forward(GebrCommPortProvider *self, GebrCommSsh *ssh)
{
	g_return_if_fail(self->priv->forward == NULL);
	self->priv->forward = g_new0(GebrCommPortForward, 1);
	self->priv->forward->ssh = ssh;
	self->priv->forward->address = g_strdup(self->priv->address);
}

static void
clear_forward(GebrCommPortProvider *self)
{
	gebr_comm_port_forward_close(self->priv->forward);
	gebr_comm_port_forward_free(self->priv->forward);
	self->priv->forward = NULL;
}

static guint
get_port(GebrCommPortProvider *self)
{
	static guint p1 = 6000;
	static guint p2 = 3000;

	if (self->priv->port != 0)
		return self->priv->port;

	if (self->priv->type == GEBR_COMM_PORT_TYPE_X11)
		self->priv->port = p1++;
	else
		self->priv->port = p2++;

	return self->priv->port;
}

/* Local port provider implementation {{{ */
static GPid
start_program(GebrCommPortProvider *self,
	      gboolean maestro,
	      gint *fdin,
	      gint *fdout,
	      GError **error)
{
	GPid pid = 0;
	const gchar *binary = maestro ? "gebrm":"gebrd";
	const gchar *argv[] = {binary, NULL, NULL};
	if (self->priv->force_init && maestro)
		argv[1] = "-f";

	g_spawn_async_with_pipes(g_get_home_dir(),
				 (gchar**)argv, NULL,
				 G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				 NULL, NULL,
				 &pid,
				 fdin,
				 fdout,
				 NULL,
				 error);
	return pid;
}

static gboolean
communicate_with_program(GebrCommPortProvider *self,
			 GPid pid,
			 gint fdin,
			 gint fdout,
			 gint *status,
			 gchar **output,
			 GError **error)
{
	GString *bufout = g_string_new(NULL);
	GByteArray *bufin = g_byte_array_new();
	g_byte_array_append(bufin, (guint8*)self->priv->gebr_cookie,
			    strlen(self->priv->gebr_cookie));
	g_byte_array_append(bufin, (guint8*)"\n", 1);
	while (1) {
		fd_set readfd, writefd;
		gint nfds = MAX(fdin, fdout);

		FD_ZERO(&readfd);
		FD_ZERO(&writefd);
		FD_SET(fdin, &writefd);
		FD_SET(fdout, &readfd);

		gint n = select(nfds + 1, &readfd, &writefd, NULL, NULL);

		if (n == -1) {
			perror("select");
			g_set_error(error, GEBR_COMM_PORT_PROVIDER_ERROR,
				    GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN,
				    "Error running select function");
			return FALSE;
		}

		if (bufin->len && FD_ISSET(fdin, &writefd)) {
			gssize len = write(fdin, bufin->data, bufin->len);
			if (len > 0)
				g_byte_array_remove_range(bufin, 0, len);
		}

		if (FD_ISSET(fdout, &readfd)) {
			gchar buf[1024];
			gssize len = read(fdout, buf, sizeof(buf));
			if (len > 0)
				g_string_append_len(bufout, buf, len);
		}

		if (bufout->len > 0) {
			pid_t ret = waitpid(pid, status, WNOHANG);
			if (ret != 0)
				break;
		}
	}

	g_byte_array_unref(bufin);

	if (output)
		*output = g_string_free(bufout, FALSE);

	return TRUE;
}

static void
local_get_port(GebrCommPortProvider *self, gboolean maestro)
{
	GPid pid;
	gint fdin, fdout;
	GError *error = NULL;

	pid = start_program(self, maestro, &fdin, &fdout, &error);
	if (error) {
		GError *tmp = NULL;
		transform_spawn_sync_error(self, error, &tmp);
		emit_signals(self, 0, tmp);
		g_clear_error(&error);
		g_clear_error(&tmp);
		return;
	}

	gint status;
	gchar *output;
	communicate_with_program(self, pid, fdin, fdout,
				 &status, &output, &error);
	if (error) {
		emit_signals(self, 0, error);
		g_clear_error(&error);
		return;
	}

	guint port;
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN,
			    "Program exited with error");
	} else if (output) {
		get_port_from_command_output(self, output, &port, &error);
	} else {
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN,
			    "Unknown error");
		g_warn_if_reached();
	}

	emit_signals(self, port, error);
	if (error)
		g_clear_error(&error);
	g_free(output);
}

void
local_get_maestro_port(GebrCommPortProvider *self)
{
	local_get_port(self, TRUE);
}

void
local_get_daemon_port(GebrCommPortProvider *self)
{
	local_get_port(self, FALSE);
}

void
local_get_x11_port(GebrCommPortProvider *self)
{
	g_return_if_fail(self->priv->display_port != 0);
	emit_signals(self, self->priv->display_port, NULL);
}

void
local_get_sftp_port(GebrCommPortProvider *self)
{
	GError *error = NULL;
	g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
		    GEBR_COMM_PORT_PROVIDER_ERROR_SFTP_NOT_REQUIRED,
		    "SFTP not required in local configuration");
	emit_signals(self, 0, error);
}
/* }}} */

/* Remote port provider implementation {{{ */
static void
on_ssh_password(GebrCommSsh *ssh, gboolean retry, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[REPASS_PASSWORD], 0, ssh, retry);
}

static void
on_ssh_question(GebrCommSsh *ssh, const gchar *question, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[QUESTION], 0, ssh, question);
}

static void
on_ssh_error(GebrCommSsh *ssh, const gchar *msg, GebrCommPortProvider *self)
{
	GError *error = NULL;
	g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
		    GEBR_COMM_PORT_PROVIDER_ERROR_SSH,
		    "%s", msg);
	emit_signals(self, 0, error);
}

static void
on_local_forward_ssh_error(GebrCommSsh *ssh, const gchar *msg, GebrCommPortProvider *self)
{
	if (self->priv->port_timeout)
		g_source_remove(self->priv->port_timeout);

	if (g_strcmp0(msg, GEBR_COMM_SSH_ERROR_LOCAL_FORWARD) == 0) {
		clear_forward(self);
		self->priv->port = 0;
		create_local_forward(self);
	} else {
		on_ssh_error(ssh, msg, self);
	}
}

static void
on_ssh_key(GebrCommSsh *ssh, gboolean accepts_key, GebrCommPortProvider *self)
{
	g_signal_emit(self, signals[ACCEPTS_KEY], 0, accepts_key);
}

static void
on_ssh_finished(GebrCommSsh *ssh,
		GebrCommPortProvider *self)
{
	if (self->priv->state == STATE_INIT) {
		GError *error = NULL;
		const gchar *out = gebr_comm_ssh_get_ssh_output(ssh);
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_SSH,
			    "%s", out);
		emit_signals(self, 0, error);
		g_clear_error(&error);
	}
}

struct TunnelPollData {
	GebrCommPortProvider *self;
	guint port;
};

static gboolean
tunnel_poll_port(gpointer user_data)
{
	struct TunnelPollData *data = user_data;

	if (gebr_comm_listen_socket_is_local_port_available(data->port))
		return TRUE;

	emit_signals(data->self, data->port, NULL);
	g_free(data);
	return FALSE;
}

static gboolean
get_port_from_command_output(GebrCommPortProvider *self,
                             const gchar *buffer,
                             guint *port,
			     GError **error)
{
	gboolean is_maestro = self->priv->type == GEBR_COMM_PORT_TYPE_MAESTRO;
	GError *err = NULL;

	gchar *redirect_addr = g_strrstr(buffer, GEBR_ADDR_PREFIX);
	gchar *port_str = g_strrstr(buffer, GEBR_PORT_PREFIX);

	if (strstr(buffer, COMMAND_NOT_FOUND)) {
		g_set_error(&err, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_NOT_FOUND,
			    _("%s not found at %s"),
			    is_maestro ? "Maestro" : "Daemon",
			    self->priv->address);
	} else if (!port_str) {
		g_set_error(&err, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_PORT,
			    _("Unexpected output from %s at %s: %s"),
			    is_maestro ? "maestro" : "daemon",
			    self->priv->address,
			    buffer);
	} else if (redirect_addr) {
		redirect_addr += strlen(GEBR_ADDR_PREFIX);
		gchar *nl = strchr(redirect_addr, '\n');
		gchar *addr;
		if (nl)
			addr = g_strndup(redirect_addr, (nl - redirect_addr)/sizeof(gchar));
		else
			addr = g_strdup(redirect_addr);
		g_strstrip(addr);

		if (gebr_comm_is_address_equal(self->priv->address, addr)) {
			*port = atoi(port_str + strlen(GEBR_PORT_PREFIX));
			g_free(addr);
		} else {
			g_set_error(&err, GEBR_COMM_PORT_PROVIDER_ERROR,
				    GEBR_COMM_PORT_PROVIDER_ERROR_REDIRECT,
				    "%s", addr);
			g_free(addr);
		}
	} else
		*port = atoi(port_str + strlen(GEBR_PORT_PREFIX));

	if (err) {
		if (error)
			*error = g_error_copy(err);
		g_clear_error(&err);
		return FALSE;
	} else {
		self->priv->remote_port = *port;
	}

	return TRUE;
}

static void
create_local_forward(GebrCommPortProvider *self)
{
	guint port;
	gchar *command = get_local_forward_command(self, &port, "127.0.0.1", self->priv->remote_port);

	self->priv->state = STATE_FORWARD;

	g_debug("Got port %d for daemon %s", port, self->priv->address);

	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_local_forward_ssh_error), self);
	gebr_comm_ssh_set_command(ssh, command);
	set_forward(self, ssh);
	gebr_comm_ssh_run(ssh);
	g_free(command);

	struct TunnelPollData *data = g_new(struct TunnelPollData, 1);
	data->self = self;
	data->port = port;
	self->priv->port_timeout = g_timeout_add(200, tunnel_poll_port, data);
}

static void
on_ssh_stdout(GebrCommSsh *_ssh, const GString *buffer, GebrCommPortProvider *self)
{
	GError *error = NULL;
	guint remote_port;

	if (!get_port_from_command_output(self, buffer->str, &remote_port, &error)) {
		emit_signals(self, 0, error);
		return;
	}

	self->priv->remote_address = "127.0.0.1";
	create_local_forward(self);
}

static void
on_ssh_stdin(GebrCommSsh *ssh, GebrCommPortProvider *self)
{
	gchar *tmp = g_strconcat(self->priv->gebr_cookie, "\n", NULL);
	gebr_comm_ssh_write_chars(ssh, tmp);
	g_free(tmp);
}

static gchar *
get_launch_command(GebrCommPortProvider *self, gboolean is_maestro)
{
	const gchar *binary = is_maestro ? "gebrm" : "gebrd";
	gboolean force_init = FALSE;

	if (is_maestro && self->priv->force_init)
		force_init = TRUE;

	gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key(self->priv->check_host);

	GString *cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s -v -x %s \"bash -l -c '%s%s'\"",
	                ssh_cmd, self->priv->address, binary, force_init? " -f" : "");
	gchar *cmd = g_shell_quote(cmd_line->str);

	g_string_printf(cmd_line, "bash -c %s", cmd);

	g_free(cmd);
	g_free(ssh_cmd);

	return g_string_free(cmd_line, FALSE);
}

void
remote_get_port(GebrCommPortProvider *self, gboolean is_maestro)
{
	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	g_signal_connect(ssh, "ssh-stdout", G_CALLBACK(on_ssh_stdout), self);
	g_signal_connect(ssh, "ssh-stdin", G_CALLBACK(on_ssh_stdin), self);
	g_signal_connect(ssh, "ssh-key", G_CALLBACK(on_ssh_key), self);
	g_signal_connect(ssh, "ssh-finished", G_CALLBACK(on_ssh_finished), self);
	gchar *command = get_launch_command(self, is_maestro);
	gebr_comm_ssh_set_command(ssh, command);
	gebr_comm_ssh_run(ssh);
	g_free(command);
}

void
remote_get_maestro_port(GebrCommPortProvider *self)
{
	remote_get_port(self, TRUE);
}

void
remote_get_daemon_port(GebrCommPortProvider *self)
{
	remote_get_port(self, FALSE);
}

static gchar *
get_x11_command(GebrCommPortProvider *self)
{
	gchar *ssh_cmd;
	GString *cmd_line;

	ssh_cmd = gebr_comm_get_ssh_command_with_key(self->priv->check_host);
	cmd_line = g_string_new(NULL);
	g_string_printf(cmd_line, "%s -v -x -R 0:127.0.0.1:%d %s -N",
			ssh_cmd, self->priv->display_port, self->priv->address);

	g_free(ssh_cmd);

	return g_string_free(cmd_line, FALSE);
}

static void
on_ssh_x11_stdout(GebrCommSsh *ssh,
		  const GString *buffer,
		  GebrCommPortProvider *self)
{
	gchar **parts = g_strsplit(buffer->str, " ", -1);

	if (g_strv_length(parts) < 3)
		g_return_if_reached();

	guint forward_port = atoi(parts[2]);

	if (forward_port == 0)
		g_return_if_reached();

	emit_signals(self, forward_port, NULL);

	g_strfreev(parts);
}

void
remote_get_x11_port(GebrCommPortProvider *self)
{
	GebrCommSsh *ssh = gebr_comm_ssh_new();
	g_signal_connect(ssh, "ssh-password", G_CALLBACK(on_ssh_password), self);
	g_signal_connect(ssh, "ssh-question", G_CALLBACK(on_ssh_question), self);
	g_signal_connect(ssh, "ssh-stdout", G_CALLBACK(on_ssh_x11_stdout), self);
	g_signal_connect(ssh, "ssh-error", G_CALLBACK(on_ssh_error), self);
	gchar *command = get_x11_command(self);
	gebr_comm_ssh_set_command(ssh, command);
	set_forward(self, ssh);
	gebr_comm_ssh_run(ssh);
	g_free(command);
}

static gchar *
get_local_forward_command(GebrCommPortProvider *self,
			  guint *port,
			  const gchar *addr,
			  guint remote_port)
{
	gchar *ssh_cmd = gebr_comm_get_ssh_command_with_key(self->priv->check_host);
	GString *string = g_string_new(NULL);

	*port = get_port(self);

	g_string_printf(string, "%s -v -x -L %d:%s:%d %s -N", ssh_cmd, *port,
			addr, remote_port, self->priv->address);

	g_free(ssh_cmd);

	return g_string_free(string, FALSE);
}

void
remote_get_sftp_port(GebrCommPortProvider *self)
{
	self->priv->remote_port = 22;
	self->priv->remote_address = "127.0.0.1";

	create_local_forward(self);
}

/* Authentication keys methods*/
/* }}} */

/* Public API {{{ */
GebrCommPortProvider *
gebr_comm_port_provider_new(GebrCommPortType type,
			    const gchar *address)
{
	return g_object_new(GEBR_COMM_TYPE_PORT_PROVIDER,
			    "type", type,
			    "address", address,
			    NULL);
}

void
gebr_comm_port_provider_set_force_init(GebrCommPortProvider *self,
                                       gboolean force_init)
{
	self->priv->force_init = force_init;
}

void
gebr_comm_port_provider_set_display(GebrCommPortProvider *self,
				    guint display,
				    const gchar *host)
{
	// Set dsplay port
	self->priv->display_port = display;

	// Set dsplay host
	if (self->priv->display_host)
		g_free(self->priv->display_host);
	self->priv->display_host = g_strdup(host);
}

const gchar *
gebr_comm_port_provider_get_display_host(GebrCommPortProvider *self)
{
	if (is_local_address(self->priv->address))
		return self->priv->display_host;
	else
		return "127.0.0.1";
}

void
gebr_comm_port_provider_start(GebrCommPortProvider *self)
{
	gebr_comm_port_provider_start_with_port(self, 0);
}

void
gebr_comm_port_provider_start_with_port(GebrCommPortProvider *self, guint port)
{
	self->priv->port = port;

	struct PortProviderVirtualMethods *vmethods;

	if (is_local_address(self->priv->address))
		vmethods = &local_methods;
	else
		vmethods = &remote_methods;

	start(self, vmethods);
}

GebrCommPortForward *
gebr_comm_port_provider_get_forward(GebrCommPortProvider *self)
{
	return self->priv->forward;
}

/* GebrCommPortForward methods */

const gchar *
gebr_comm_port_forward_get_address(GebrCommPortForward *port_forward)
{
	if (!port_forward)
		return NULL;

	return port_forward->address;
}

static gboolean
release_ssh_object(gpointer data)
{
	GebrCommSsh *ssh = data;
	g_object_unref(ssh);
	return FALSE;
}

void
gebr_comm_port_forward_close(GebrCommPortForward *port_forward)
{
	if (!port_forward)
		return;

	if (GEBR_COMM_IS_SSH(port_forward->ssh)) {
		gebr_comm_ssh_kill(port_forward->ssh);
		g_idle_add(release_ssh_object, port_forward->ssh);
		port_forward->ssh = NULL;
	}
}

void
gebr_comm_port_forward_free(GebrCommPortForward *port_forward)
{
	if (!port_forward)
		return;

	g_free(port_forward->address);
	g_free(port_forward);
}

void
gebr_comm_port_provider_set_sftp_port(GebrCommPortProvider *port_provider,
				      guint remote_port)
{
	port_provider->priv->remote_port = remote_port;
}

void
gebr_comm_port_provider_set_check_host(GebrCommPortProvider *port_provider,
                                       gboolean check_host)
{
	port_provider->priv->check_host = check_host;
}

void
gebr_comm_port_provider_set_cookie(GebrCommPortProvider *port_provider,
				   const gchar *cookie)
{
	if (port_provider->priv->gebr_cookie)
		g_free(port_provider->priv->gebr_cookie);
	port_provider->priv->gebr_cookie = g_strdup(cookie);
}

/* }}} */
