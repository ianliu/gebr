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

#include "gtksequenceedit.h"
#include "support.h"
#include "utils.h"

/*
 * Prototypes
 */

static GtkMenu * __gtk_sequence_edit_popup_menu(GtkTreeView * tree_view, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_add_clicked(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_remove_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_move_up_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_move_down_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);

static void __gtk_sequence_edit_add(GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_remove(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gtk_sequence_edit_move_up(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gtk_sequence_edit_move_down(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);

/*
 * gobject stuff
 */

static void
gtk_sequence_edit_class_init(GtkSequenceEditClass * class)
{
	class->add = __gtk_sequence_edit_add;
	class->remove = __gtk_sequence_edit_remove;
	class->move_up = __gtk_sequence_edit_move_up;
	class->move_down = __gtk_sequence_edit_move_down;
}

static void
gtk_sequence_edit_init(GtkSequenceEdit * sequence_edit)
{
}

G_DEFINE_TYPE(GtkSequenceEdit, gtk_sequence_edit, GTK_TYPE_VBOX);

/*
 * Internal functions
 */

void
__gtk_sequence_edit_init(GtkSequenceEdit * sequence_edit)
{
	GtkWidget *		vbox;
	GtkWidget *		hbox;
	GtkWidget *		button;

	GtkWidget *		tree_view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(sequence_edit), vbox, TRUE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), sequence_edit->widget, TRUE, TRUE, 5);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_sequence_edit_on_add_clicked), sequence_edit);

	/*
	 * store/view
	 */
	sequence_edit->list_store = gtk_list_store_new(1, G_TYPE_STRING);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sequence_edit->list_store));
	sequence_edit->tree_view = tree_view;
	gtk_widget_show(tree_view);
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view),
		(GtkPopupCallback)__gtk_sequence_edit_popup_menu, sequence_edit);
	gtk_box_pack_start(GTK_BOX(sequence_edit), tree_view, TRUE, TRUE, 0);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
}

static GtkMenu *
__gtk_sequence_edit_popup_menu(GtkTreeView * tree_view, GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		menu;
	GtkWidget *		menu_item;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	menu = gtk_menu_new();

	/* Move up */
	if (gtk_list_store_can_move_up(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_UP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)__gtk_sequence_edit_on_move_up_activated, sequence_edit);
	}
	/* Move down */
	if (gtk_list_store_can_move_down(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_DOWN, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)__gtk_sequence_edit_on_move_down_activated, NULL);
	}
	/* Remove */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate",
		(GCallback)__gtk_sequence_edit_on_remove_activated, NULL);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void
__gtk_sequence_edit_button_clicked(GtkSequenceEdit * sequence_edit, void (*button_func)(GtkSequenceEdit * self, GtkTreeIter * iter))
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	button_func(sequence_edit, &iter);
}

static void
__gtk_sequence_edit_on_add_clicked(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->add(sequence_edit);
}

static void
__gtk_sequence_edit_on_remove_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->remove);
}

static void
__gtk_sequence_edit_on_move_up_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_up);
}

static void
__gtk_sequence_edit_on_move_down_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_down);
}

static void
__gtk_sequence_edit_add(GtkSequenceEdit * sequence_edit)
{
	/* pure virtual */
}

static void
__gtk_sequence_edit_remove(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_remove(sequence_edit->list_store, iter);
}

static void
__gtk_sequence_edit_move_up(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_up(sequence_edit->list_store, iter);
}

static void
__gtk_sequence_edit_move_down(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_down(sequence_edit->list_store, iter);
}

/*
 * Library functions
 */

GtkWidget *
gtk_sequence_edit_new(GtkWidget * widget)
{
	GtkSequenceEdit *	sequence_edit;

	sequence_edit = g_object_new(GTK_TYPE_SEQUENCE_EDIT, NULL);
	sequence_edit->widget = widget;
	__gtk_sequence_edit_init(sequence_edit);

	return GTK_WIDGET(sequence_edit);
}

GtkWidget *
gtk_sequence_edit_new_from_store(GtkWidget * widget, GtkListStore * list_store)
{
	GtkSequenceEdit *	sequence_edit;

	sequence_edit = g_object_new(GTK_TYPE_SEQUENCE_EDIT, NULL);
	sequence_edit->widget = widget;
	sequence_edit->list_store = list_store;
	__gtk_sequence_edit_init(sequence_edit);

	return GTK_WIDGET(sequence_edit);
}

GtkListStore *
gtk_sequence_edit_get_store(GtkSequenceEdit * sequence_edit)
{
	return sequence_edit->list_store;
}
