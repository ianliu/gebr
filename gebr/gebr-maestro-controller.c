/*
 * gebr-maestro-controller.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
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

#include "gebr-maestro-controller.h"
#include <config.h>

#include <glib/gi18n.h>
#include "gebr-maestro-server.h"
#include "gebr-marshal.h"

struct _GebrMaestroControllerPriv {
	GList *maestros;
	GtkBuilder *builder;
};

enum {
	JOB_DEFINE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(GebrMaestroController, gebr_maestro_controller, G_TYPE_OBJECT);

static void
gebr_maestro_controller_init(GebrMaestroController *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
						 GEBR_TYPE_MAESTRO_CONTROLLER,
						 GebrMaestroControllerPriv);
}

static void
gebr_maestro_controller_class_init(GebrMaestroControllerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[JOB_DEFINE] =
		g_signal_new("job-define",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrMaestroServerClass, job_define),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__OBJECT_OBJECT,
			     G_TYPE_NONE, 2,
			     GEBR_TYPE_MAESTRO_SERVER, GEBR_TYPE_JOB);

	g_type_class_add_private(klass, sizeof(GebrMaestroControllerPriv));
}

GebrMaestroController *
gebr_maestro_controller_new(void)
{
	return g_object_new(GEBR_TYPE_MAESTRO_CONTROLLER, NULL);
}

GtkDialog *
gebr_maestro_controller_create_dialog(GebrMaestroController *self)
{
	self->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(self->priv->builder,
				  GEBR_GLADE_DIR"/gebr-maestro-dialog.glade",
				  NULL);

	return GTK_DIALOG(gtk_builder_get_object(self->priv->builder, "dialog_maestro"));
}

GList *
gebr_maestro_controller_get_maestro(GebrMaestroController *self)
{
	return self->priv->maestros;
}

static void
on_job_define(GebrMaestroServer *maestro,
	      GebrJob *job,
	      GebrMaestroController *self)
{
	g_signal_emit(self, signals[JOB_DEFINE], 0, maestro, job);
}

static void
on_server_group_changed(GebrMaestroServer *maestro,
			GebrMaestroController *self)
{
	g_warning("Implement me pleaaase %s", __func__);
}

static const gchar *
on_password_request(GebrMaestroServer *maestro,
		    const gchar *address)
{
	gdk_threads_enter();
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Enter password"),
							NULL,
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
							GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gchar *message = g_strdup_printf(_("Server %s is asking for password. Enter it below."), address);
	GtkWidget *label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	gtk_widget_show_all(dialog);
	gboolean confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;
	gchar *password = confirmed ? g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))) : NULL;

	gtk_widget_destroy(dialog);
	gdk_threads_leave();
	return password;
}

void
gebr_maestro_controller_connect(GebrMaestroController *self,
				const gchar *address)
{
	GebrMaestroServer *maestro = gebr_maestro_server_new(address);

	g_signal_connect(maestro, "job-define",
			 G_CALLBACK(on_job_define), self);
	g_signal_connect(maestro, "group-changed",
			 G_CALLBACK(on_server_group_changed), self);
	g_signal_connect(maestro, "password-request",
			 G_CALLBACK(on_password_request), self);

	gebr_maestro_server_connect(maestro);

	self->priv->maestros = g_list_prepend(self->priv->maestros, maestro);
}
