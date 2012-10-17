/*
 * gebr-menu-view.h
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

#ifndef __GEBR_MENU_VIEW_H__
#define __GEBR_MENU_VIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEBR_TYPE_MENU_VIEW            (gebr_menu_view_get_type ())
#define GEBR_MENU_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_MENU_VIEW, GebrMenuView))
#define GEBR_MENU_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_TYPE_MENU_VIEW, GebrMenuViewClass))
#define GEBR_IS_MENU_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_MENU_VIEW))
#define GEBR_IS_MENU_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_TYPE_MENU_VIEW))
#define GEBR_MENU_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_TYPE_MENU_VIEW, GebrMenuViewClass))

typedef struct _GebrMenuView GebrMenuView;
typedef struct _GebrMenuViewPriv GebrMenuViewPriv;
typedef struct _GebrMenuViewClass GebrMenuViewClass;

struct _GebrMenuView {
	GObject parent;
	GebrMenuViewPriv *priv;
};

struct _GebrMenuViewClass {
	GObjectClass parent_class;

	/* Signals */
	void (*add_menu) (GebrMenuView *menuView,
			  const gchar *menuPath);
};

enum {
	MENU_TITLE_COLUMN = 0,
	MENU_DESCRIPTION_COLUMN,
	MENU_FILEPATH_COLUMN,
	MENU_VISIBLE_COLUMN,
	MENU_N_COLUMN
};

GType gebr_menu_view_get_type(void) G_GNUC_CONST;

GebrMenuView *gebr_menu_view_new(void);

/**
 * gebr_menu_view_clear_model:
 * @view:
 *
 */
void gebr_menu_view_clear_model(GebrMenuView *view);

/**
 * gebr_menu_view_find_or_add_category:
 * @title:
 * @categories_hash:
 * @view:
 *
 * Returns: A iter of category with @title
 */
GtkTreeIter gebr_menu_view_find_or_add_category(const gchar *title,
                                                GHashTable *categories_hash,
                                                GebrMenuView *view);

/**
 * gebr_menu_view_add_menu:
 * @parent:
 * @title:
 * @desc:
 * @file:
 * @view:
 *
 */
void gebr_menu_view_add_menu(GtkTreeIter *parent,
                             const gchar *title,
                             const gchar *desc,
                             const gchar *file,
                             GebrMenuView *view);
/**
 * gebr_menu_view_get_widget:
 *
 * Returns a widget of menu list
 */
GtkWidget *gebr_menu_view_get_widget(GebrMenuView *view);

/**
 * gebr_menu_view_set_focus_on_entry:
 *
 * Set focus on search entry
 */
void gebr_menu_view_set_focus_on_entry(GebrMenuView *view);

/**
 * gebr_menu_view_multiple_flow_selected:
 *
 * Verify if there are multiple flows selected
 */
gboolean gebr_menu_view_multiple_flow_selected();

/**
 * gebr_menu_view_set_sensitive_add:
 *
 * Set sensitivity of add button
 */
void gebr_menu_view_set_sensitive_add(GebrMenuView *view,  gboolean sensitive);

G_END_DECLS

#endif /* end of include guard: __GEBR_MENU_VIEW_H__ */

