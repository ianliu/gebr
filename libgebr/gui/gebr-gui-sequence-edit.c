/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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
#include <config.h>
#include <glib/gi18n-lib.h>

#include "gebr-gui-sequence-edit.h"
#include "gebr-gui-utils.h"
#include "marshalers.h"

/*
 * Prototypes
 */

static GtkMenu *__gebr_gui_sequence_edit_popup_menu(GtkTreeView * tree_view, GebrGuiSequenceEdit * sequence_edit);
static void __gebr_gui_sequence_edit_on_add_clicked(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit);
static void __gebr_gui_sequence_edit_on_remove_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit);
static gboolean __gebr_gui_sequence_edit_on_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
					       GtkTreeViewDropPosition drop_position, GebrGuiSequenceEdit * sequence_edit);
static void __gebr_gui_sequence_edit_on_move_top_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit);
static void __gebr_gui_sequence_edit_on_move_bottom_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit);
static void __gebr_gui_sequence_edit_on_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
					  GebrGuiSequenceEdit * sequence_edit);

static void __gebr_gui_sequence_edit_remove(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gebr_gui_sequence_edit_move(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter, GtkTreeIter * position,
				     GtkTreeViewDropPosition drop_position);
static void __gebr_gui_sequence_edit_move_top(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gebr_gui_sequence_edit_move_bottom(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __gebr_gui_sequence_edit_rename(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text);
static GtkWidget *__gebr_gui_sequence_edit_create_tree_view(GebrGuiSequenceEdit * sequence_edit);
static gboolean gebr_sequence_edit_on_mnemonic_activate(GebrGuiSequenceEdit * self);

/*
 * gobject stuff
 */

enum {
	VALUE_WIDGET = 1,
	LIST_STORE,
	MAY_RENAME,
};

enum {
	ADD_REQUEST = 0,
	CHANGED,
	RENAMED,
	REMOVED,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gebr_gui_sequence_edit_set_property(GebrGuiSequenceEdit * sequence_edit, guint property_id, const GValue * value,
			       GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_WIDGET:
		sequence_edit->widget = g_value_get_pointer(value);
		gtk_box_pack_start(GTK_BOX(sequence_edit->widget_hbox), sequence_edit->widget, TRUE, TRUE, 0);
		break;
	case LIST_STORE:{
			GtkWidget *scrolled_window;
			GtkWidget *tree_view;

			sequence_edit->list_store = g_value_get_pointer(value);

			if (sequence_edit->list_store == NULL)
				sequence_edit->list_store = gtk_list_store_new(1, G_TYPE_STRING, -1);

			scrolled_window = gtk_scrolled_window_new(NULL, NULL);
			gtk_widget_show(scrolled_window);
			gtk_box_pack_start(GTK_BOX(sequence_edit), scrolled_window, TRUE, TRUE, 0);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
						       GTK_POLICY_AUTOMATIC);

			tree_view =
			    GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->create_tree_view(sequence_edit);
			gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(tree_view),
								    (GebrGuiGtkTreeViewReorderCallback)
								    __gebr_gui_sequence_edit_on_reorder, NULL,
								    sequence_edit);
			sequence_edit->tree_view = tree_view;
			gtk_widget_show(tree_view);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), tree_view);
			gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view), (GebrGuiGtkPopupCallback)
								  __gebr_gui_sequence_edit_popup_menu, sequence_edit);

			break;
	case MAY_RENAME:
			sequence_edit->may_rename = g_value_get_boolean(value);
			g_object_set(sequence_edit->renderer, "editable", sequence_edit->may_rename, NULL);
			break;
		}
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
		break;
	}
}

static void
gebr_gui_sequence_edit_get_property(GebrGuiSequenceEdit * sequence_edit, guint property_id, GValue * value,
			       GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_WIDGET:
		g_value_set_pointer(value, sequence_edit->widget);
		break;
	case LIST_STORE:
		g_value_set_pointer(value, sequence_edit->list_store);
		break;
	case MAY_RENAME:
		g_value_set_boolean(value, sequence_edit->may_rename);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
		break;
	}
}

static void gebr_gui_sequence_edit_class_init(GebrGuiSequenceEditClass * klass)
{
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	/* signals */
	object_signals[ADD_REQUEST] = g_signal_new("add-request", GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrGuiSequenceEditClass, add_request), NULL, NULL,	/* acumulators */
						   g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[CHANGED] = g_signal_new("changed", GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrGuiSequenceEditClass, changed), NULL, NULL,	/* acumulators */
					       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	object_signals[RENAMED] = g_signal_new(("renamed"),
					       GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT,
					       G_SIGNAL_RUN_LAST,
					       G_STRUCT_OFFSET(GebrGuiSequenceEditClass, renamed),
					       NULL, NULL,
					       _gebr_gui_marshal_BOOLEAN__STRING_STRING,
					       G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);
	object_signals[REMOVED] = g_signal_new(("removed"),
					       GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT,
					       G_SIGNAL_RUN_LAST,
					       G_STRUCT_OFFSET(GebrGuiSequenceEditClass, removed),
					       NULL, NULL,
					       _gebr_gui_marshal_VOID__STRING_STRING,
					       G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

	/* virtual definition */
	klass->add = NULL;
	klass->remove = __gebr_gui_sequence_edit_remove;
	klass->move = __gebr_gui_sequence_edit_move;
	klass->move_top = __gebr_gui_sequence_edit_move_top;
	klass->move_bottom = __gebr_gui_sequence_edit_move_bottom;
	klass->rename = __gebr_gui_sequence_edit_rename;
	klass->create_tree_view = __gebr_gui_sequence_edit_create_tree_view;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) gebr_gui_sequence_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) gebr_gui_sequence_edit_get_property;

	param_spec = g_param_spec_pointer("value-widget",
					  "Value widget", "Value widget used for adding",
					  (GParamFlags)(G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, VALUE_WIDGET, param_spec);

	param_spec = g_param_spec_pointer("list-store",
					  "List store", "List store, model for view",
					  (GParamFlags)(G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, LIST_STORE, param_spec);

	param_spec = g_param_spec_boolean("may-rename",
					  "Rename enabled", "True if the list is renameable", TRUE,
					  (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, MAY_RENAME, param_spec);
}

static void gebr_gui_sequence_edit_init(GebrGuiSequenceEdit * sequence_edit)
{
	GtkWidget *hbox;
	GtkWidget *button;

	sequence_edit->may_rename = TRUE;

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
	g_signal_connect(button, "clicked", G_CALLBACK(__gebr_gui_sequence_edit_on_add_clicked), sequence_edit);
	g_signal_connect(sequence_edit, "mnemonic-activate", G_CALLBACK(gebr_sequence_edit_on_mnemonic_activate), NULL);
}

G_DEFINE_TYPE(GebrGuiSequenceEdit, gebr_gui_sequence_edit, GTK_TYPE_VBOX);

/*
 * Internal functions
 */

static GtkMenu *__gebr_gui_sequence_edit_popup_menu(GtkTreeView * tree_view, GebrGuiSequenceEdit * sequence_edit)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(__gebr_gui_sequence_edit_on_add_clicked), sequence_edit);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		goto out;

	/* Move top */
	if (gebr_gui_gtk_list_store_can_move_up(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
				 G_CALLBACK(__gebr_gui_sequence_edit_on_move_top_activated), sequence_edit);
	}
	/* Move bottom */
	if (gebr_gui_gtk_list_store_can_move_down(sequence_edit->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
				 G_CALLBACK(__gebr_gui_sequence_edit_on_move_bottom_activated), sequence_edit);
	}
	/* Remove */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(__gebr_gui_sequence_edit_on_remove_activated), sequence_edit);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void
__gebr_gui_sequence_edit_button_clicked(GebrGuiSequenceEdit * sequence_edit,
				   void (*button_func) (GebrGuiSequenceEdit * self, GtkTreeIter * iter))
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	button_func(sequence_edit, &iter);
}

static void __gebr_gui_sequence_edit_on_add_clicked(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit)
{
	if (GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->add != NULL)
		GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->add(sequence_edit);
	else
		g_signal_emit(sequence_edit, object_signals[ADD_REQUEST], 0);
}

static void __gebr_gui_sequence_edit_on_remove_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *old_text;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter, 0, &old_text, -1);

	g_signal_emit(sequence_edit, object_signals[REMOVED], 0, old_text, "");
	__gebr_gui_sequence_edit_button_clicked(sequence_edit, GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->remove);

	g_free(old_text);
}

static gboolean
__gebr_gui_sequence_edit_on_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			       GtkTreeViewDropPosition drop_position, GebrGuiSequenceEdit * sequence_edit)
{
	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move(sequence_edit, iter, position, drop_position);
	return TRUE;
}

static void __gebr_gui_sequence_edit_on_move_top_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit)
{
	__gebr_gui_sequence_edit_button_clicked(sequence_edit,
					   GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_top);
}

static void __gebr_gui_sequence_edit_on_move_bottom_activated(GtkWidget * button, GebrGuiSequenceEdit * sequence_edit)
{
	__gebr_gui_sequence_edit_button_clicked(sequence_edit,
					   GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->move_bottom);
}

static void
__gebr_gui_sequence_edit_on_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
			      GebrGuiSequenceEdit * sequence_edit)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *old_text;
	gboolean retval;


	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter, 0, &old_text, -1);
	
	/* false rename */
	if (!strcmp(old_text, new_text)) {
		g_free(old_text);
		return;
	}
	g_signal_emit(sequence_edit, object_signals[RENAMED], 0, old_text, new_text, &retval);

	if (retval)
		GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(sequence_edit)->rename(sequence_edit, &iter, new_text);

	g_free(old_text);
}

static void __gebr_gui_sequence_edit_remove(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_remove(sequence_edit->list_store, iter);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void
__gebr_gui_sequence_edit_move(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position)
{
	if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
		gtk_list_store_move_after(sequence_edit->list_store, iter, position);
	else
		gtk_list_store_move_before(sequence_edit->list_store, iter, position);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void __gebr_gui_sequence_edit_move_top(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_after(sequence_edit->list_store, iter, NULL);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void __gebr_gui_sequence_edit_move_bottom(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	gtk_list_store_move_before(sequence_edit->list_store, iter, NULL);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static void __gebr_gui_sequence_edit_rename(GebrGuiSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text)
{
	gtk_list_store_set(sequence_edit->list_store, iter, 0, new_text, -1);
	g_signal_emit(sequence_edit, object_signals[CHANGED], 0);
}

static GtkWidget *__gebr_gui_sequence_edit_create_tree_view(GebrGuiSequenceEdit * sequence_edit)
{
	GtkWidget *tree_view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sequence_edit->list_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

	sequence_edit->renderer = renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", sequence_edit->may_rename, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(__gebr_gui_sequence_edit_on_edited), sequence_edit);
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

static gboolean gebr_sequence_edit_on_mnemonic_activate(GebrGuiSequenceEdit * self)
{
	gtk_widget_mnemonic_activate(self->widget, TRUE);
	return TRUE;
}

/*
 * Library functions
 */

GtkWidget *gebr_gui_sequence_edit_new(GtkWidget * widget)
{
	return g_object_new(GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, "value-widget", widget, "list-store", NULL, NULL);
}

GtkWidget *gebr_gui_sequence_edit_new_from_store(GtkWidget * widget, GtkListStore * list_store)
{
	return g_object_new(GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, "value-widget", widget, "list-store", list_store, NULL);
}

GtkTreeIter gebr_gui_sequence_edit_add(GebrGuiSequenceEdit * sequence_edit, const gchar * text, gboolean show_empty_value_text)
{
	GtkTreeIter iter;

	gtk_list_store_append(sequence_edit->list_store, &iter);
	gtk_list_store_set(sequence_edit->list_store, &iter,
			   0, (show_empty_value_text && !strlen(text)) ? _("<empty value>") : text, -1);

	return iter;
}

void gebr_gui_sequence_edit_clear(GebrGuiSequenceEdit * sequence_edit)
{
	gtk_list_store_clear(sequence_edit->list_store);
}
