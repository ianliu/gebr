/*
 * ui_flows_io.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Team
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

#ifndef __GEBR_UI_FLOWS_IO_H__
#define  __GEBR_UI_FLOWS_IO_H__

#include <glib-object.h>
#include <libgebr/geoxml/geoxml.h>

#define GEBR_TYPE_UI_FLOWS_IO            (gebr_ui_flows_io_get_type())
#define GEBR_UI_FLOWS_IO(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_TYPE_UI_FLOWS_IO, GebrUiFlowsIo))
#define GEBR_UI_FLOWS_IO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GEBR_TYPE_UI_FLOWS_IO, GebrUiFlowsIoClass))
#define GEBR_IS_UI_FLOWS_IO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_TYPE_UI_FLOWS_IO))
#define GEBR_UI_FLOWS_IO_IS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GEBR_TYPE_UI_FLOWS_IO))
#define GEBR_UI_FLOWS_IO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GEBR_TYPE_UI_FLOWS_IO, GebrUiFlowsIoClass))

G_BEGIN_DECLS

typedef struct _GebrUiFlowsIo GebrUiFlowsIo;
typedef struct _GebrUiFlowsIoPriv GebrUiFlowsIoPriv;
typedef struct _GebrUiFlowsIoClass GebrUiFlowsIoClass;

struct _GebrUiFlowsIo {
	GObject parent;
	GebrUiFlowsIoPriv *priv;
};

struct _GebrUiFlowsIoClass {
	GObjectClass parent_class;
};

typedef enum {
	GEBR_IO_TYPE_NONE = 0,
	GEBR_IO_TYPE_INPUT,
	GEBR_IO_TYPE_OUTPUT,
	GEBR_IO_TYPE_ERROR
} GebrFlowsIoType;

/* GebrUiFlowsIoSingleton methods {{{1 */
/**
 * GebrUiFlowsIoFactory:
 *
 * A function that constructs a #GebrUiFlowsIo. See
 * gebr_ui_flows_io_singleton_set_factory().
 */
typedef GebrUiFlowsIo* (*GebrUiFlowsIoFactory) (gpointer data);

/**
 * gebr_ui_flows_io_singleton_set_factory:
 *
 * Changes the method that is used to create a #GebrUiFlowsIo in
 * gebr_ui_flows_io_singleton_get(). The default method creates a #GebrUiFlowsIo per
 * program execution. You should use this method in unit tests to prevent one
 * test from interfering with another. See the example below.
 *
 * Note that this method changes global behavior and calling it on different
 * threads is probably an error.
 *
 */

void gebr_ui_flows_io_singleton_set_factory(GebrUiFlowsIoFactory factory,
				     gpointer data);

/**
 * gebr_ui_flows_io_singleton_get:
 *
 * Gets the #GebrUiFlowsIo singleton instance. The instance returned is calculated
 * by the method set with gebr_ui_flows_io_singleton_set_factory(). If no method was
 * set, then a default one is used, which creates one #GebrUiFlowsIo for
 * application.
 */
GebrUiFlowsIo *gebr_ui_flows_io_singleton_get(void);
/* }}} GebrUiFlowsIoSingleton methods */

/* GebrUiFlowsIo methods {{{ */
GType gebr_ui_flows_io_get_type(void) G_GNUC_CONST;

/**
 * gebr_ui_flows_io_new:
 *
 * Creates a new #GebrUiFlowsIo with reference count of one. Free with
 * g_object_unref().
 */
GebrUiFlowsIo *gebr_ui_flows_io_new(void);

/*
 * Setters and getters
 */
void gebr_ui_flows_io_set_id(GebrUiFlowsIo *io,
			     const gchar *id);

const gchar *gebr_ui_flows_io_get_id(GebrUiFlowsIo *io);

void gebr_ui_flows_io_set_io_type(GebrUiFlowsIo *io,
		                  GebrFlowsIoType type);

GebrFlowsIoType gebr_ui_flows_io_get_io_type(GebrUiFlowsIo *io);

void gebr_ui_flows_io_set_value(GebrUiFlowsIo *io,
		                const gchar *value);

const gchar* gebr_ui_flows_io_get_value(GebrUiFlowsIo *io);

void gebr_ui_flows_io_set_overwrite(GebrUiFlowsIo *io,
				gboolean overwrite);

gboolean gebr_ui_flows_io_get_overwrite(GebrUiFlowsIo *io);

void gebr_ui_flows_io_set_active(GebrUiFlowsIo *io,
				gboolean active);

gboolean gebr_ui_flows_io_get_active(GebrUiFlowsIo *io);

gboolean gebr_ui_flows_io_set_value_from_flow(GebrUiFlowsIo *io,
				GebrGeoXmlDocument *flow);

G_END_DECLS

#endif /* __GEBR_UI_FLOWS_IO_H__ */
