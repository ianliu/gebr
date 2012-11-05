/*
 * gebr-comm-port-provider.h
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

#ifndef __GEBR_COMM_PORT_PROVIDER_H__
#define __GEBR_COMM_PORT_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEBR_PORT_PREFIX "gebr-port="

/**
 * GebrCommPortType:
 *
 * This enumeration defines values to be used by gebr_comm_port_provider_new()
 * method. Depending on this enumeration, the GebrCommPortProvider structure
 * behaves differently: the port returned by the signal
 * GebrCommPortProvider::port_defined correspond to the type defined in the
 * constructor.
 */
typedef enum {
	GEBR_COMM_PORT_TYPE_MAESTRO,
	GEBR_COMM_PORT_TYPE_DAEMON,
	GEBR_COMM_PORT_TYPE_X11,
	GEBR_COMM_PORT_TYPE_SFTP,
} GebrCommPortType;

/**
 * GebrCommPortProviderError:
 * @GEBR_COMM_PORT_PROVIDER_ERROR_UNKNOWN_TYPE: The type set in constructor is unknown, see #GebrCommPortType.
 */
typedef enum {
	GEBR_COMM_PORT_PROVIDER_ERROR_UNKNOWN_TYPE,
	GEBR_COMM_PORT_PROVIDER_ERROR_SFTP_NOT_REQUIRED,
	GEBR_COMM_PORT_PROVIDER_ERROR_SPAWN,
	GEBR_COMM_PORT_PROVIDER_ERROR_SSH,
} GebrCommPortProviderError;

/**
 * GEBR_COMM_PORT_PROVIDER_ERROR:
 *
 * Error domain that can occur when working with GebrCommPortProvider.
 */
#define GEBR_COMM_PORT_PROVIDER_ERROR (gebr_comm_port_provider_error_quark())
GQuark gebr_comm_port_provider_error_quark(void);

#define GEBR_COMM_TYPE_PORT_PROVIDER            (gebr_comm_port_provider_get_type ())
#define GEBR_COMM_PORT_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_TYPE_PORT_PROVIDER, GebrCommPortProvider))
#define GEBR_COMM_PORT_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_COMM_TYPE_PORT_PROVIDER, GebrCommPortProviderClass))
#define GEBR_COMM_IS_PORT_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_TYPE_PORT_PROVIDER))
#define GEBR_COMM_IS_PORT_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_COMM_TYPE_PORT_PROVIDER))
#define GEBR_COMM_PORT_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_COMM_TYPE_PORT_PROVIDER, GebrCommPortProviderClass))

typedef struct _GebrCommPortProvider GebrCommPortProvider;
typedef struct _GebrCommPortProviderPriv GebrCommPortProviderPriv;
typedef struct _GebrCommPortProviderClass GebrCommPortProviderClass;

struct _GebrCommPortProvider {
	GObject parent;
	GebrCommPortProviderPriv *priv;
};

struct _GebrCommPortProviderClass {
	GObjectClass parent;

	void (*port_defined) (GebrCommPortProvider *self, guint port);
	void (*error) (GebrCommPortProvider *self, GError *error);
	void (*password) (GebrCommPortProvider *self, gboolean retry);
	void (*question) (GebrCommPortProvider *self, const gchar *question);
};

GType gebr_comm_port_provider_get_type(void) G_GNUC_CONST;

/**
 * gebr_comm_port_provider_new:
 *
 * Creates a GebrCommPortProvider of @type and @address to communicate through
 * the ports.
 */
GebrCommPortProvider *gebr_comm_port_provider_new(GebrCommPortType type,
						  const gchar *address);

/**
 * gebr_comm_port_provider_set_display:
 *
 * Sets the X11 display to use when @self was constructed with
 * GEBR_COMM_PORT_TYPE_X11. This display is used to forward the connection in
 * case where the address is remote. If address is local, the port returned by
 * the signal is the display itself.
 */
void gebr_comm_port_provider_set_display(GebrCommPortProvider *self,
					 guint display);

/**
 * gebr_comm_port_provider_set_sftp_address:
 * Sets the address to be used in a remote nfs case.
 */
void
gebr_comm_port_provider_set_sftp_address(GebrCommPortProvider *self,
					 const gchar *address);
/**
 * gebr_comm_port_provider_start:
 *
 * After calling this method, the signal GebrCommPortProvider::port_defined may
 * be emitted, where the port can be fetched.
 */
void gebr_comm_port_provider_start(GebrCommPortProvider *self);

G_END_DECLS

#endif /* end of include guard: __GEBR_COMM_PORT_PROVIDER_H__ */
