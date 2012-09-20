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
#include "gebr.h"

#include <libgebr/utils.h>
#include <glib/gi18n.h>
#include <glib-object.h>

G_DEFINE_TYPE(GebrMenuView, gebr_menu_view, G_TYPE_OBJECT);

struct _GebrMenuViewPriv {
	GtkTreeStore *tree_store;
	GtkTreeView *tree_view;

	GtkWidget *vbox;
};

enum {
	ADD_MENU = 0,
	LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

/*
 * Private methods
 */
static gboolean
gebr_menu_view_search_func(GtkTreeModel *model,
                           gint column,
                           const gchar *key,
                           GtkTreeIter *iter,
                           gpointer data)
{
	gchar *title, *desc, *text;
	gchar *lt, *ld, *lk; // Lower case strings
	gboolean match;

	if (!key)
		return FALSE;

	gtk_tree_model_get(model, iter,
			   MENU_TITLE_COLUMN, &text,
			   -1);

	gchar **parts = g_strsplit(text, "\n", -1);
	title = g_strdup(parts[0]);
	desc = g_strdup(parts[1]);

	lt = title ? g_utf8_strdown(title, -1) : g_strdup("");
	ld = desc ?  g_utf8_strdown(desc, -1)  : g_strdup("");
	lk = g_utf8_strdown(key, -1);

	match = gebr_utf8_strstr(lt, lk) || gebr_utf8_strstr(ld, lk);

	g_free(title);
	g_free(desc);
	g_free(text);
	g_free(lt);
	g_free(ld);
	g_free(lk);
	g_strfreev(parts);

	return !match;
}

static gboolean
gebr_menu_view_get_selected_menu(GtkTreeIter * iter,
                                 gboolean warn_unselected,
                                 GebrMenuView *view)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view->priv->tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(view->priv->tree_store), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

static void
gebr_menu_view_add(GtkTreeView *tree_view,
                   GtkTreePath *path,
                   GtkTreeViewColumn *column,
                   GebrMenuView *view)
{
	GtkTreeIter iter;
	gchar *name;
	gchar *filename;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!gebr_menu_view_get_selected_menu(&iter, FALSE, view)) {
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(tree_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), path, FALSE);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(view->priv->tree_store), &iter,
	                   MENU_TITLE_COLUMN, &name,
	                   MENU_FILEPATH_COLUMN, &filename,
	                   -1);

	g_signal_emit(view, signals[ADD_MENU], 0, filename);
}

static GtkMenu *
gebr_menu_view_popup_menu(GtkWidget * widget,
                          GebrMenuView *view)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!gebr_menu_view_get_selected_menu(&iter, FALSE, view)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), view->priv->tree_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), view->priv->tree_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(gebr_menu_view_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), view->priv->tree_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), view->priv->tree_view);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

//static void
//gebr_menu_view_show_help(void)
//{
//	GtkTreeIter iter;
//	gchar *menu_filename;
//	GebrGeoXmlFlow *menu;
//
//	if (!flow_browse_get_selected_menu(&iter, TRUE))
//		return;
//
//	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
//			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
//
//	menu = menu_load_path(menu_filename);
//	if (menu == NULL)
//		goto out;
//	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);
//
//out:	g_free(menu_filename);
//}

static void
on_search_entry_press(GtkEntry *entry,
                      GtkEntryIconPosition pos,
                      GdkEvent *event,
                      GebrMenuView *view)
{
	if (pos == GTK_ENTRY_ICON_SECONDARY)
		gtk_entry_set_text(entry, "");
}

static void
gebr_menu_view_init(GebrMenuView *view)
{
	view->priv = G_TYPE_INSTANCE_GET_PRIVATE(view,
			GEBR_TYPE_MENU_VIEW,
			GebrMenuViewPriv);

	view->priv->tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
	view->priv->tree_store = gtk_tree_store_new(MENU_N_COLUMN,
	                                            G_TYPE_STRING,
	                                            G_TYPE_STRING);
	view->priv->vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_set_size_request(GTK_WIDGET(view->priv->vbox), 500, 450);

	gtk_tree_view_set_model(view->priv->tree_view,
	                        GTK_TREE_MODEL(view->priv->tree_store));

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(view->priv->tree_view));

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view->priv->tree_view),
	                                          (GebrGuiGtkPopupCallback) gebr_menu_view_popup_menu,
	                                          view);

	g_signal_connect(GTK_OBJECT(view->priv->tree_view), "row-activated",
	                 G_CALLBACK(gebr_menu_view_add), view);

	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(view->priv->tree_view), MENU_TITLE_COLUMN);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view->priv->tree_view),
	                                    gebr_menu_view_search_func, NULL, NULL);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;

	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view->priv->tree_view), col);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	g_object_set(renderer, "ypad", 2, NULL);
	g_object_set(renderer, "wrap-mode", PANGO_WRAP_WORD_CHAR, "wrap-width", 490, NULL);


	/*
	 * Create search entry
	 */
	GtkWidget *entry = gtk_entry_new();
	gtk_widget_set_size_request(entry, 200, -1);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry),
	                              GTK_ENTRY_ICON_PRIMARY,
	                              GTK_STOCK_FIND);

	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry),
	                              GTK_ENTRY_ICON_SECONDARY,
	                              GTK_STOCK_CLOSE);

	g_signal_connect(entry, "icon-press", G_CALLBACK(on_search_entry_press), view);

	// Add Search entry
	gtk_box_pack_start(GTK_BOX(view->priv->vbox), entry, FALSE, FALSE, 5);

	// Add menu list
	gtk_box_pack_start(GTK_BOX(view->priv->vbox), scrolled_window, TRUE, TRUE, 0);
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

/*
 * Public Methods
 */

GebrMenuView *
gebr_menu_view_new(void)
{
	return g_object_new(GEBR_TYPE_MENU_VIEW, NULL);
}

GtkTreeStore *
gebr_menu_view_get_model(GebrMenuView *view)
{
	return view->priv->tree_store;
}

GtkWidget *
gebr_menu_view_get_widget(GebrMenuView *view)
{
	return view->priv->vbox;
}
