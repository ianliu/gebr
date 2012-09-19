/*
 * gebr-menu-view.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR core team (www.gebrproject.com)
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

#include "gebr-menu-view.h"

#include <glib-object.h>

G_DEFINE_TYPE(GebrMenuView, gebr_menu_view, GTK_TYPE_VBOX);

struct _GebrMenuViewPriv {
	GtkTreeStore *tree_store;
	GtkTreeView *tree_view;
};

enum {
	ADD_MENU = 0,
	LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

static void
gebr_menu_view_init(GebrMenuView *view)
{
	view->priv = G_TYPE_INSTANCE_GET_PRIVATE(view,
			GEBR_TYPE_MENU_VIEW,
			GebrMenuViewPriv);

	view->priv->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	view->priv->tree_store = gtk_tree_store_new(3,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	gtk_tree_view_set_model(view->priv->tree_view,
			GTK_TREE_MODEL(view->priv->tree_store));
}

static void
gebr_menu_view_class_init(GebrMenuViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	signals[ADD_MENU] =
		g_signal_new("add-menu",
			     G_OBJECT_CLASS_TYPE(object_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrMenuViewClass, add_menu),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1,
			     G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrMenuViewPriv));
}

GtkWidget *
gebr_menu_view_new(void)
{
	return g_object_new(GEBR_TYPE_MENU_VIEW, NULL);
}
