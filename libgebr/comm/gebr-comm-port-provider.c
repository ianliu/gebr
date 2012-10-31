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
	PORT_DEFINE,
	ERROR,
	LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

struct _GebrCommPortProviderPriv {
	GebrCommPortType type;
	gchar *address;
	guint display;
};

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
	signals[PORT_DEFINE] =
		g_signal_new("port-define",
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

	g_object_class_install_property(object_class,
					PROP_TYPE,
					g_param_spec_int("type",
							 "Type",
							 "Type",
							 0, 100, 0,
							 G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
					PROP_ADDRESS,
					g_param_spec_string("address",
							    "Address",
							    "Address",
							    NULL,
							    G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(GebrCommPortProviderPriv));
}

static void
gebr_comm_port_provider_init(GebrCommPortProvider *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_COMM_TYPE_PORT_PROVIDER,
						 GebrCommPortProviderPriv);
}
/* }}} */

static gboolean
is_local_address(const gchar *addr)
{
	if (g_strcmp0(addr, "localhost") == 0
	    || g_strcmp0(addr, "127.0.0.1") == 0)
		return TRUE;
	return FALSE;
}

static void
emit_signals(GebrCommPortProvider *self, guint port, GError *error)
{
	if (!error)
		g_signal_emit(self, PORT_DEFINE, 0, port);
	else
		g_signal_emit(self, ERROR, 0, error);
}

/*
 * This structure provides some virtual methods that must be implemented by
 * both local and remote logics. Each method must return a boolean value
 * indicating the success or failure of the method. In case of failure, `error`
 * must be set from GEBR_COMM_PORT_PROVIDER_ERROR domain.
 *
 * See: GebrCommPortProviderError enumeration.
 */
struct PortProviderVirtualMethods {
	gboolean (*get_maestro_port) (GebrCommPortProvider *self, guint *port, GError **error);
	gboolean (*get_daemon_port)  (GebrCommPortProvider *self, guint *port, GError **error);
	gboolean (*get_x11_port)     (GebrCommPortProvider *self, guint *port, GError **error);
	gboolean (*get_sftp_port)    (GebrCommPortProvider *self, guint *port, GError **error);
};

gboolean local_get_maestro_port (GebrCommPortProvider *self, guint *port, GError **error);
gboolean local_get_daemon_port  (GebrCommPortProvider *self, guint *port, GError **error);
gboolean local_get_x11_port     (GebrCommPortProvider *self, guint *port, GError **error);
gboolean local_get_sftp_port    (GebrCommPortProvider *self, guint *port, GError **error);

gboolean remote_get_maestro_port (GebrCommPortProvider *self, guint *port, GError **error);
gboolean remote_get_daemon_port  (GebrCommPortProvider *self, guint *port, GError **error);
gboolean remote_get_x11_port     (GebrCommPortProvider *self, guint *port, GError **error);
gboolean remote_get_sftp_port    (GebrCommPortProvider *self, guint *port, GError **error);

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
	guint port = 0;
	GError *error = NULL;

	switch (self->priv->type)
	{
	case GEBR_COMM_PORT_TYPE_MAESTRO:
		break;
	case GEBR_COMM_PORT_TYPE_DAEMON:
		break;
	case GEBR_COMM_PORT_TYPE_X11:
		port = self->priv->display;
		break;
	case GEBR_COMM_PORT_TYPE_SFTP:
		break;
	default:
		g_set_error(&error, GEBR_COMM_PORT_PROVIDER_ERROR,
			    GEBR_COMM_PORT_PROVIDER_ERROR_NOT_SET,
			    "Port was not set");
		break;
	}

	emit_signals(self, port, error);
}

/* Local port provider implementation {{{ */
gboolean
local_get_maestro_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
local_get_daemon_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
local_get_x11_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
local_get_sftp_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}
/* }}} */

/* Remote port provider implementation {{{ */
gboolean
remote_get_maestro_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
remote_get_daemon_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
remote_get_x11_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}

gboolean
remote_get_sftp_port(GebrCommPortProvider *self, guint *port, GError **error)
{
	return FALSE;
}
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
gebr_comm_port_provider_set_display(GebrCommPortProvider *self,
				    guint display)
{
	self->priv->display = display;
}

void
gebr_comm_port_provider_start(GebrCommPortProvider *self)
{
	struct PortProviderVirtualMethods *vmethods;

	if (is_local_address(self->priv->address))
		vmethods = &local_methods;
	else
		vmethods = &remote_methods;

	start(self, vmethods);
}
/* }}} */
