/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtklistedit.h"
#include "support.h"
#include "utils.h"

/*
 * Prototypes
 */

static void __gtk_list_edit_on_add_clicked(GtkWidget * button, GtkListEdit * list_edit);
static void __gtk_list_edit_on_remove_clicked(GtkWidget * button, GtkListEdit * list_edit);
static void __gtk_list_edit_on_move_up_clicked(GtkWidget * button, GtkListEdit * list_edit);
static void __gtk_list_edit_on_move_down_clicked(GtkWidget * button, GtkListEdit * list_edit);
static void __gtk_list_edit_on_selection_changed(GtkTreeSelection * selection, GtkListEdit * list_edit);

static void __gtk_list_edit_create_store(GtkListEdit * list_edit);
static void __gtk_list_edit_add(GtkListEdit * list_edit, GtkTreeIter * iter);
static void __gtk_list_edit_remove(GtkListEdit * list_edit, GtkTreeIter * iter);
static void __gtk_list_edit_move_up(GtkListEdit * list_edit, GtkTreeIter * iter);
static void __gtk_list_edit_move_down(GtkListEdit * list_edit, GtkTreeIter * iter);

/*
 * gobject stuff
 */

static void
gtk_list_edit_class_init(GtkListEditClass * class)
{
	/* default virtual methods definition */
	class->create_store = __gtk_list_edit_create_store;
	class->add = __gtk_list_edit_add;
	class->remove = __gtk_list_edit_remove;
	class->move_up = __gtk_list_edit_move_up;
	class->move_down = __gtk_list_edit_move_down;
}

static void
gtk_list_edit_init(GtkListEdit * list_edit)
{
	GtkListEditClass *	class;

	GtkWidget *		button_vbox;
	GtkWidget *		button;

	GtkWidget *		tree_view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	class = GTK_LIST_EDIT_GET_CLASS(list_edit);

	/*
	 * buttons
	 */
	button_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(button_vbox);
	gtk_box_pack_start(GTK_BOX(list_edit), button_vbox, TRUE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	list_edit->add_button = button;
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(button_vbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_list_edit_on_add_clicked), list_edit);

	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	list_edit->remove_button = button;
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(button_vbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_list_edit_on_remove_clicked), list_edit);

	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	list_edit->move_up_button = button;
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(button_vbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_list_edit_on_move_up_clicked), list_edit);

	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	list_edit->move_down_button = button;
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(button_vbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_list_edit_on_move_down_clicked), list_edit);

	/*
	 * store/view
	 */
	class->create_store(list_edit);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_edit->list_store));
	list_edit->tree_view = tree_view;
	gtk_widget_show(tree_view);
	gtk_box_pack_start(GTK_BOX(list_edit), tree_view, TRUE, TRUE, 0);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)), "changed",
		GTK_SIGNAL_FUNC(__gtk_list_edit_on_selection_changed), list_edit);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
}

G_DEFINE_TYPE(GtkListEdit, gtk_list_edit, GTK_TYPE_HBOX);

/*
 * Internal functions
 */

static void
__gtk_list_edit_button_clicked(GtkListEdit * list_edit, void (*button_func)(GtkListEdit * self, GtkTreeIter * iter))
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	button_func(list_edit, &iter);
}

static void
__gtk_list_edit_on_add_clicked(GtkWidget * button, GtkListEdit * list_edit)
{
	GtkTreeIter		iter;

	gtk_list_store_append(list_edit->list_store, &iter);
	GTK_LIST_EDIT_GET_CLASS(list_edit)->add(list_edit, &iter);
}

static void
__gtk_list_edit_on_remove_clicked(GtkWidget * button, GtkListEdit * list_edit)
{
	__gtk_list_edit_button_clicked(list_edit, GTK_LIST_EDIT_GET_CLASS(list_edit)->remove);
}

static void
__gtk_list_edit_on_move_up_clicked(GtkWidget * button, GtkListEdit * list_edit)
{
	__gtk_list_edit_button_clicked(list_edit, GTK_LIST_EDIT_GET_CLASS(list_edit)->move_up);
}

static void
__gtk_list_edit_on_move_down_clicked(GtkWidget * button, GtkListEdit * list_edit)
{
	__gtk_list_edit_button_clicked(list_edit, GTK_LIST_EDIT_GET_CLASS(list_edit)->move_down);
}

static void
__gtk_list_edit_on_selection_changed(GtkTreeSelection * selection, GtkListEdit * list_edit)
{
	GtkTreeModel *		model;
	GtkTreeIter		iter;
	gboolean		has_selection;

	has_selection = gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_widget_set_sensitive(list_edit->remove_button, has_selection);
	if (has_selection == FALSE) {
		gtk_widget_set_sensitive(list_edit->move_up_button, FALSE);
		gtk_widget_set_sensitive(list_edit->move_down_button, FALSE);
	} else {
		gtk_widget_set_sensitive(list_edit->move_up_button,
			gtk_list_store_can_move_up(list_edit->list_store, &iter));
		gtk_widget_set_sensitive(list_edit->move_down_button,
			gtk_list_store_can_move_down(list_edit->list_store, &iter));
	}
}

static void
__gtk_list_edit_create_store(GtkListEdit * list_edit)
{
	list_edit->list_store = gtk_list_store_new(1, G_TYPE_STRING);
}

static void
__gtk_list_edit_add(GtkListEdit * list_edit, GtkTreeIter * iter)
{

}

static void
__gtk_list_edit_remove(GtkListEdit * list_edit, GtkTreeIter * iter)
{

}

static void
__gtk_list_edit_move_up(GtkListEdit * list_edit, GtkTreeIter * iter)
{

}

static void
__gtk_list_edit_move_down(GtkListEdit * list_edit, GtkTreeIter * iter)
{

}

/*
 * Library functions
 */

GtkWidget *
gtk_list_edit_new()
{
	return g_object_new(GTK_TYPE_LIST_EDIT, NULL);
}

GtkListStore *
gtk_list_edit_get_store(GtkListEdit * list_edit)
{
	return list_edit->list_store;
}
