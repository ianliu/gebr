/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include <string.h>

#include "gtksequenceedit.h"
#include "support.h"
#include "utils.h"

/*
 * Prototypes
 */

static GtkMenu * __gtk_sequence_edit_popup_menu(GtkTreeView * tree_view, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_add_clicked(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_remove_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static gboolean __gtk_sequence_edit_on_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_move_top_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_move_bottom_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit);
static void __gtk_sequence_edit_on_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
	GtkSequenceEdit * sequence_edit);

static void __gtk_sequence_edit_remove(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gtk_sequence_edit_move_before(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter, GtkTreeIter * before);
static void __gtk_sequence_edit_move_top(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gtk_sequence_edit_move_bottom(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gtk_sequence_edit_rename(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text);
static GtkWidget * __gtk_sequence_edit_create_tree_view(GtkSequenceEdit * sequence_edit);

/*
 * gobject stuff
 */

enum {
	VALUE_WIDGET = 1,
	LIST_STORE,
	MINIMUM_ONE
};

enum {
	ADD_REQUEST = 0,
	CHANGED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gtk_sequence_edit_set_property(GtkSequenceEdit * sequence_edit, guint property_id, const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_WIDGET:
		sequence_edit->widget = g_value_get_pointer(value);
		gtk_box_pack_start(GTK_BOX(sequence_edit->widget_hbox), sequence_edit->widget, TRUE, TRUE, 0);
		break;
	case LIST_STORE: {
		GtkWidget *		scrolled_window;
		GtkWidget *		tree_view;

		sequence_edit->list_store = g_value_get_pointer(value);

		if (sequence_edit->list_store == NULL)
			sequence_edit->list_store = gtk_list_store_new(1, G_TYPE_STRING, -1);

		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_show(scrolled_window);
		gtk_box_pack_start(GTK_BOX(sequence_edit), scrolled_window, TRUE, TRUE, 0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		tree_view = GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->create_tree_view(sequence_edit);
		gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(tree_view),
			(GtkTreeViewReorderCallback)__gtk_sequence_edit_on_reorder, NULL, sequence_edit);
		sequence_edit->tree_view = tree_view;
		gtk_widget_show(tree_view);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), tree_view);
		gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view),
			(GtkPopupCallback)__gtk_sequence_edit_popup_menu, sequence_edit);

		break;
	case MINIMUM_ONE:
		sequence_edit->minimum_one = g_value_get_boolean(value);
		break;
	} default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
		break;
	}
}

static void
gtk_sequence_edit_get_property(GtkSequenceEdit * sequence_edit, guint property_id, GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_WIDGET:
		g_value_set_pointer(value, sequence_edit->widget);
		break;
	case LIST_STORE:
		g_value_set_pointer(value, sequence_edit->list_store);
		break;
	case MINIMUM_ONE:
		g_value_set_boolean(value, sequence_edit->minimum_one);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
		break;
	}
}

static void
gtk_sequence_edit_class_init(GtkSequenceEditClass * class)
{
	GObjectClass *	gobject_class;
	GParamSpec *	param_spec;

	/* signals */
	object_signals[ADD_REQUEST] = g_signal_new("add-request",
		GTK_TYPE_SEQUENCE_EDIT,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET(GtkSequenceEditClass, add_request),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	object_signals[CHANGED] = g_signal_new("changed",
		GTK_TYPE_SEQUENCE_EDIT,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET(GtkSequenceEditClass, changed),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	/* virtual definition */
	class->add = NULL;
	class->remove = __gtk_sequence_edit_remove;
	class->move_before = __gtk_sequence_edit_move_before;
	class->move_top = __gtk_sequence_edit_move_top;
	class->move_bottom = __gtk_sequence_edit_move_bottom;
	class->rename = __gtk_sequence_edit_rename;
	class->create_tree_view = __gtk_sequence_edit_create_tree_view;

	gobject_class = G_OBJECT_CLASS(class);
	gobject_class->set_property = (typeof(gobject_class->set_property))gtk_sequence_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property))gtk_sequence_edit_get_property;

	param_spec = g_param_spec_pointer("value-widget",
		"Value widget", "Value widget used for adding",
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, VALUE_WIDGET, param_spec);

	param_spec = g_param_spec_pointer("list-store",
		"List store", "List store, model for view",
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, LIST_STORE, param_spec);

	param_spec = g_param_spec_boolean("minimum-one",
		"Minimum one", "True if the list keep at least one item",
		FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, MINIMUM_ONE, param_spec);
}

static void
gtk_sequence_edit_init(GtkSequenceEdit * sequence_edit)
{
	GtkWidget *		hbox;
	GtkWidget *		button;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(sequence_edit), hbox, FALSE, TRUE, 0);

	sequence_edit->widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(sequence_edit->widget_hbox);
	gtk_box_pack_start(GTK_BOX(hbox), sequence_edit->widget_hbox, TRUE, TRUE, 0);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_ADD, 1));
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show_all(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
	g_signal_connect(button, "clicked",
		GTK_SIGNAL_FUNC(__gtk_sequence_edit_on_add_clicked), sequence_edit);
}

G_DEFINE_TYPE(GtkSequenceEdit, gtk_sequence_edit, GTK_TYPE_VBOX);

/*
 * Internal functions
 */

static GtkMenu *
__gtk_sequence_edit_popup_menu(GtkTreeView * tree_view, GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		menu;
	GtkWidget *		menu_item;

	menu = gtk_menu_new();

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate",
		(GCallback)__gtk_sequence_edit_on_add_clicked, sequence_edit);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		goto out;
	if (sequence_edit->minimum_one
	&& gtk_tree_model_iter_n_children(GTK_TREE_MODEL(sequence_edit->list_store), NULL) == 1)
		goto out;

	/* Move top */
	if (gtk_list_store_can_move_up(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)__gtk_sequence_edit_on_move_top_activated, sequence_edit);
	}
	/* Move bottom */
	if (gtk_list_store_can_move_down(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)__gtk_sequence_edit_on_move_bottom_activated, sequence_edit);
	}
	/* Remove */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate",
		(GCallback)__gtk_sequence_edit_on_remove_activated, sequence_edit);

out:	gtk_widget_show_all(menu);

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
	if (GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->add != NULL)
		GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->add(sequence_edit);
	else
		g_signal_emit(sequence_edit, object_signals[ADD_REQUEST], 0);
}

static void
__gtk_sequence_edit_on_remove_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->remove);
}

static gboolean
__gtk_sequence_edit_on_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before,
	GtkSequenceEdit * sequence_edit)
{
	GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_before(sequence_edit, iter, before);
	return TRUE;
}

static void
__gtk_sequence_edit_on_move_top_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_top);
}

static void
__gtk_sequence_edit_on_move_bottom_activated(GtkWidget * button, GtkSequenceEdit * sequence_edit)
{
	__gtk_sequence_edit_button_clicked(sequence_edit, GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_bottom);
}

static void
__gtk_sequence_edit_on_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
	GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	GTK_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->rename(sequence_edit, &iter, new_text);
}

static void
__gtk_sequence_edit_remove(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_remove(sequence_edit->list_store, iter);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void
__gtk_sequence_edit_move_before(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter, GtkTreeIter * before)
{
	gtk_list_store_move_before(sequence_edit->list_store, iter, before);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void
__gtk_sequence_edit_move_top(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_before(sequence_edit->list_store, iter, NULL);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void
__gtk_sequence_edit_move_bottom(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_after(sequence_edit->list_store, iter, NULL);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void
__gtk_sequence_edit_rename(GtkSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text)
{
	gtk_list_store_set(sequence_edit->list_store, iter,
		0, new_text,
		-1);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static GtkWidget *
__gtk_sequence_edit_create_tree_view(GtkSequenceEdit * sequence_edit)
{
	GtkWidget *		tree_view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sequence_edit->list_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		(GCallback)__gtk_sequence_edit_on_edited, sequence_edit);
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

/*
 * Library functions
 */

GtkWidget *
gtk_sequence_edit_new(GtkWidget * widget)
{
	return g_object_new(GTK_TYPE_SEQUENCE_EDIT,
		"value-widget", widget,
		"list-store", NULL,
		NULL);
}

GtkWidget *
gtk_sequence_edit_new_from_store(GtkWidget * widget, GtkListStore * list_store)
{
	return g_object_new(GTK_TYPE_SEQUENCE_EDIT,
		"value-widget", widget,
		"list-store", list_store,
		NULL);
}

GtkTreeIter
gtk_sequence_edit_add(GtkSequenceEdit * sequence_edit, const gchar * text, gboolean show_empty_value_text)
{
	GtkTreeIter	iter;

	gtk_list_store_append(sequence_edit->list_store, &iter);
	gtk_list_store_set(sequence_edit->list_store, &iter,
		0, (show_empty_value_text && !strlen(text)) ? _("<empty value>") : text,
		-1);

	return iter;
}

void
gtk_sequence_edit_clear(GtkSequenceEdit * sequence_edit)
{
	gtk_list_store_clear(sequence_edit->list_store);
}
