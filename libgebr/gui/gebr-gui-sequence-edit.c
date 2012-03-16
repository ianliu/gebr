/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "../libgebr-gettext.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>

#include "gebr-gui-sequence-edit.h"
#include "gebr-gui-utils.h"
#include "../marshalers.h"

enum {
	PROP_0,
	VALUE_WIDGET,
	LIST_STORE,
	MAY_RENAME,
	HAS_SCROLL,
};

enum {
	ADD_REQUEST,
	CHANGED,
	RENAMED,
	REMOVED,
	LAST_SIGNAL
};

struct _GebrGuiSequenceEditPriv {
	guint keypress_handler;
	gboolean is_editing;
};

static guint object_signals[LAST_SIGNAL] = { 0 };

//==============================================================================
// PROTOTYPES								       =
//==============================================================================
static GtkMenu *popup_menu (GtkTreeView *tree_view,
			    GebrGuiSequenceEdit *self);

static void on_add_clicked (GtkWidget *button,
			    GebrGuiSequenceEdit *self);

static void on_remove_activated (GtkWidget *button,
				 GebrGuiSequenceEdit *self);

static gboolean on_reorder (GtkTreeView *tree_view,
			    GtkTreeIter *iter,
			    GtkTreeIter *position,
			    GtkTreeViewDropPosition drop_position,
			    GebrGuiSequenceEdit *self);

static void on_move_top_activated (GtkWidget *button,
				   GebrGuiSequenceEdit *self);

static void on_move_bottom_activated (GtkWidget *button,
				      GebrGuiSequenceEdit *self);

static void on_edited (GtkCellRendererText *cell,
		       gchar *path_string,
		       gchar *new_text,
		       GebrGuiSequenceEdit *self);

static void gebr_gui_sequence_edit_remove_real (GebrGuiSequenceEdit *self,
						GtkTreeIter *iter);

static void gebr_gui_sequence_edit_move_real (GebrGuiSequenceEdit *self,
					      GtkTreeIter *iter,
					      GtkTreeIter *position,
					      GtkTreeViewDropPosition drop_position);

static void gebr_gui_sequence_edit_move_top_real (GebrGuiSequenceEdit *self,
						  GtkTreeIter *iter);

static void gebr_gui_sequence_edit_move_bottom_real (GebrGuiSequenceEdit *self,
						     GtkTreeIter *iter);

static void gebr_gui_sequence_edit_rename_real (GebrGuiSequenceEdit *self,
					   GtkTreeIter *iter,
					   const gchar *new_text);

static GtkWidget *gebr_gui_sequence_edit_create_tree_view_real (GebrGuiSequenceEdit *self);

static gboolean gebr_sequence_edit_on_mnemonic_activate (GebrGuiSequenceEdit *self);

static gboolean on_tree_view_key_release (GtkWidget *widget,
					  GdkEventKey *event,
					  GebrGuiSequenceEdit *self);

static void gebr_gui_sequence_edit_add_request (GebrGuiSequenceEdit *self);

static void on_tree_view_size_request(GtkWidget *tree_view, GtkRequisition *requisition);

G_DEFINE_ABSTRACT_TYPE (GebrGuiSequenceEdit, gebr_gui_sequence_edit, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================
static void gebr_gui_sequence_edit_set_property (GObject *object,
						 guint property_id,
						 const GValue *value,
						 GParamSpec *param_spec)
{
	GebrGuiSequenceEdit *self;

	self = GEBR_GUI_SEQUENCE_EDIT (object);

	switch (property_id) {
	case VALUE_WIDGET:
		self->widget = g_value_get_pointer(value);
		gtk_box_pack_start(GTK_BOX(self->widget_hbox), self->widget, TRUE, TRUE, 0);
		break;
	case LIST_STORE: {
		GtkWidget *container;
		GtkWidget *tree_view;

		self->list_store = g_value_get_pointer(value);

		if (self->list_store == NULL)
			self->list_store = gtk_list_store_new(1, G_TYPE_STRING, -1);

		tree_view = GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->create_tree_view (self);

		self->priv->keypress_handler =
				g_signal_connect(tree_view, "key-release-event",
				                 G_CALLBACK (on_tree_view_key_release), self);

		gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(tree_view),
							    (GebrGuiGtkTreeViewReorderCallback)
							    on_reorder, NULL,
							    self);
		self->tree_view = tree_view;

		if (self->has_scroll) {
			container = gtk_scrolled_window_new(NULL, NULL);
			gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (container), GTK_SHADOW_IN);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (container),
							GTK_POLICY_AUTOMATIC,
							GTK_POLICY_AUTOMATIC);
		} else {
			container = gtk_frame_new(NULL);
			gtk_frame_set_shadow_type(GTK_FRAME(container), GTK_SHADOW_IN);
			g_signal_connect(tree_view, "size-request",
					 G_CALLBACK(on_tree_view_size_request), NULL);
		}
		gtk_container_add(GTK_CONTAINER(container), tree_view);
		gtk_box_pack_start(GTK_BOX(self), container, TRUE, TRUE, 0);
		gtk_widget_show_all(container);
		gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(tree_view), (GebrGuiGtkPopupCallback)
							  popup_menu, self);

		break;
	}
	case MAY_RENAME:
		self->may_rename = g_value_get_boolean(value);
		g_object_set(self->renderer, "editable", self->may_rename, NULL);
		break;
	case HAS_SCROLL:
		self->has_scroll = g_value_get_boolean(value);
		break;
	default:
		/*We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, param_spec);
		break;
	}
}

static void gebr_gui_sequence_edit_get_property (GObject *object,
						 guint property_id,
						 GValue *value,
						 GParamSpec *param_spec)
{
	GebrGuiSequenceEdit *self;

	self = GEBR_GUI_SEQUENCE_EDIT (object);

	switch (property_id) {
	case VALUE_WIDGET:
		g_value_set_pointer(value, self->widget);
		break;
	case LIST_STORE:
		g_value_set_pointer(value, self->list_store);
		break;
	case MAY_RENAME:
		g_value_set_boolean(value, self->may_rename);
		break;
	case HAS_SCROLL:
		g_value_set_boolean(value, self->has_scroll);
		break;
	default:
		/*We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, param_spec);
		break;
	}
}

static void gebr_gui_sequence_edit_class_init(GebrGuiSequenceEditClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gebr_gui_sequence_edit_set_property;
	gobject_class->get_property = gebr_gui_sequence_edit_get_property;
	klass->add = NULL;
	klass->add_request = gebr_gui_sequence_edit_add_request;
	klass->remove = gebr_gui_sequence_edit_remove_real;
	klass->move = gebr_gui_sequence_edit_move_real;
	klass->move_top = gebr_gui_sequence_edit_move_top_real;
	klass->move_bottom = gebr_gui_sequence_edit_move_bottom_real;
	klass->rename = gebr_gui_sequence_edit_rename_real;
	klass->create_tree_view = gebr_gui_sequence_edit_create_tree_view_real;

	/**
	 * GebrGuiSequenceEdit::add-request:
	 */
	object_signals[ADD_REQUEST] =
		g_signal_new ("add-request",
			      GEBR_GUI_TYPE_SEQUENCE_EDIT,
			      (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
			      G_STRUCT_OFFSET (GebrGuiSequenceEditClass, add_request),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/**
	 * GebrGuiSequenceEdit::changed:
	 */
	object_signals[CHANGED] =
		g_signal_new ("changed",
			      GEBR_GUI_TYPE_SEQUENCE_EDIT,
			      (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
			      G_STRUCT_OFFSET(GebrGuiSequenceEditClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/**
	 * GebrGuiSequenceEdit::renamed:
	 */
	object_signals[RENAMED] =
		g_signal_new("renamed",
			     GEBR_GUI_TYPE_SEQUENCE_EDIT,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiSequenceEditClass, renamed),
			     NULL, NULL,
			     _gebr_gui_marshal_BOOLEAN__STRING_STRING,
			     G_TYPE_BOOLEAN, 2,
			     G_TYPE_STRING, G_TYPE_STRING);

	/**
	 * GebrGuiSequenceEdit::removed:
	 */
	object_signals[REMOVED] =
		g_signal_new("removed",
			     GEBR_GUI_TYPE_SEQUENCE_EDIT,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiSequenceEditClass, removed),
			     NULL, NULL,
			     _gebr_gui_marshal_VOID__STRING_STRING,
			     G_TYPE_NONE, 2,
			     G_TYPE_STRING, G_TYPE_STRING);


	/**
	 * GebrGuiSequenceEdit:value-widget:
	 */
	g_object_class_install_property (gobject_class,
					 VALUE_WIDGET,
					 g_param_spec_pointer ("value-widget",
							       "Value widget",
							       "Value widget used for adding",
							       (GParamFlags) (G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE)));

	/**
	 * GebrGuiSequenceEdit:list-store:
	 */
	g_object_class_install_property (gobject_class,
					 LIST_STORE,
					 g_param_spec_pointer ("list-store",
							       "List store",
							       "List store, model for view",
							       (GParamFlags) (G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE)));

	/**
	 * GebrGuiSequenceEdit:may-rename:
	 */
	g_object_class_install_property(gobject_class,
					MAY_RENAME,
					g_param_spec_boolean ("may-rename",
							      "Rename enabled",
							      "True if the list is renameable",
							      TRUE,
							      (GParamFlags) (G_PARAM_READWRITE)));

	/**
	 * GebrGuiSequenceEdit:has-scroll:
	 */
	g_object_class_install_property(gobject_class,
					HAS_SCROLL,
					g_param_spec_boolean ("has-scroll",
							      "Has Scroll",
							      "Weather there is a scrolled window, or not",
							      TRUE,
							      (GParamFlags) (G_PARAM_READWRITE)));
	g_type_class_add_private(klass, sizeof (GebrGuiSequenceEditPriv));
}

static void gebr_gui_sequence_edit_init (GebrGuiSequenceEdit *self)
{
	GtkWidget *hbox;
	GtkWidget *button;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GEBR_GUI_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEditPriv);
	self->may_rename = TRUE;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(self), hbox, FALSE, TRUE, 0);

	self->widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(self->widget_hbox);
	gtk_box_pack_start(GTK_BOX(hbox), self->widget_hbox, TRUE, TRUE, 0);

	button = gtk_button_new();
	gtk_container_add (GTK_CONTAINER (button),
			  gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	g_object_set(G_OBJECT(button), "relief", GTK_RELIEF_NONE, NULL);
	gtk_widget_show_all(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);

	g_signal_connect (button, "clicked",
			  G_CALLBACK (on_add_clicked), self);

	g_signal_connect (self, "mnemonic-activate",
			  G_CALLBACK (gebr_sequence_edit_on_mnemonic_activate), NULL);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static GtkMenu *popup_menu(GtkTreeView *tree_view, GebrGuiSequenceEdit *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();

	/*add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_add_clicked), self);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		goto out;

	/*Move top */
	if (gebr_gui_gtk_list_store_can_move_up(self->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
				 G_CALLBACK(on_move_top_activated), self);
	}
	/*Move bottom */
	if (gebr_gui_gtk_list_store_can_move_down(self->list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
				 G_CALLBACK(on_move_bottom_activated), self);
	}
	/*Remove */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_remove_activated), self);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void callback_on_selected(GebrGuiSequenceEdit *self,
						    void (*button_func) (GebrGuiSequenceEdit *self, GtkTreeIter *iter))
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	button_func(self, &iter);
}

static void on_add_clicked(GtkWidget *button, GebrGuiSequenceEdit *self)
{
	if (GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->add != NULL)
		GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->add(self);
	else
		g_signal_emit(self, object_signals[ADD_REQUEST], 0);
}

static void on_remove_activated(GtkWidget *button, GebrGuiSequenceEdit *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *old_text;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(self->list_store), &iter, 0, &old_text, -1);

	g_signal_emit(self, object_signals[REMOVED], 0, old_text, "");
	callback_on_selected(self, GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->remove);

	g_free(old_text);
}

static gboolean on_reorder (GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreeIter *position,
			    GtkTreeViewDropPosition drop_position, GebrGuiSequenceEdit *self)
{
	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->move (self, iter, position, drop_position);

	return TRUE;
}

static void on_move_top_activated(GtkWidget *button, GebrGuiSequenceEdit *self)
{
	callback_on_selected(self, GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->move_top);
}

static void on_move_bottom_activated(GtkWidget *button, GebrGuiSequenceEdit *self)
{
	callback_on_selected(self, GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->move_bottom);
}

static void on_edited(GtkCellRendererText *cell, gchar *path_string,
		      gchar *new_text, GebrGuiSequenceEdit *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *old_text;
	GValue *params;
	GValue retval = {0,};

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(self->list_store), &iter, 0, &old_text, -1);
	
	/*false rename */
	if (!strcmp(old_text, new_text)) {
		g_free(old_text);
		return;
	}

	params = g_new0 (GValue, 3);
	g_value_init (params, G_TYPE_OBJECT);
	g_value_set_object (params, self);

	g_value_init (params+1, G_TYPE_STRING);
	g_value_set_string (params+1, old_text);

	g_value_init (params+2, G_TYPE_STRING);
	g_value_set_string (params+2, new_text);

	g_value_init (&retval, G_TYPE_BOOLEAN);
	g_value_set_boolean (&retval, TRUE);

	g_signal_emitv (params, object_signals[RENAMED], 0, &retval);

	g_free (params);

	if (g_value_get_boolean (&retval))
		GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(self)->rename(self, &iter, new_text);

	g_free(old_text);
}

static void gebr_gui_sequence_edit_remove_real (GebrGuiSequenceEdit *self,
						GtkTreeIter *iter)
{
	gtk_list_store_remove(self->list_store, iter);
	g_signal_emit(self, object_signals[CHANGED], 0);
}

static void gebr_gui_sequence_edit_move_real (GebrGuiSequenceEdit *self,
					      GtkTreeIter *iter,
					      GtkTreeIter *position,
					      GtkTreeViewDropPosition drop_position)
{
	if (drop_position == GTK_TREE_VIEW_DROP_AFTER)
		gtk_list_store_move_after(self->list_store, iter, position);
	else
		gtk_list_store_move_before(self->list_store, iter, position);
	g_signal_emit(self, object_signals[CHANGED], 0);
}

static void gebr_gui_sequence_edit_move_top_real(GebrGuiSequenceEdit *self, GtkTreeIter *iter)
{
	gtk_list_store_move_after(self->list_store, iter, NULL);
	g_signal_emit(self, object_signals[CHANGED], 0);
}

static void gebr_gui_sequence_edit_move_bottom_real(GebrGuiSequenceEdit *self, GtkTreeIter *iter)
{
	gtk_list_store_move_before(self->list_store, iter, NULL);
	g_signal_emit(self, object_signals[CHANGED], 0);
}

static void gebr_gui_sequence_edit_rename_real(GebrGuiSequenceEdit *self, GtkTreeIter *iter, const gchar *new_text)
{
	gtk_list_store_set(self->list_store, iter, 0, new_text, -1);
	g_signal_emit(self, object_signals[CHANGED], 0);
}

void on_editing_started(GtkCellRenderer *renderer,
                        GtkCellEditable *editable,
                        gchar           *path,
                        GebrGuiSequenceEdit *self)
{
	self->priv->is_editing = TRUE;
}

void on_editing_canceled(GebrGuiSequenceEdit *self)
{
	self->priv->is_editing = FALSE;
}


static GtkWidget *gebr_gui_sequence_edit_create_tree_view_real(GebrGuiSequenceEdit *self)
{
	GtkWidget *tree_view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(self->list_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

	self->renderer = renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", self->may_rename, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_edited), self);
	g_signal_connect_swapped(renderer, "edited", G_CALLBACK(on_editing_canceled), self);
	g_signal_connect_swapped(renderer, "editing-canceled", G_CALLBACK(on_editing_canceled), self);
	g_signal_connect(renderer, "editing-started", G_CALLBACK(on_editing_started), self);
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

static gboolean gebr_sequence_edit_on_mnemonic_activate(GebrGuiSequenceEdit *self)
{
	gtk_widget_mnemonic_activate(self->widget, TRUE);
	return TRUE;
}

static gboolean on_tree_view_key_release (GtkWidget *widget,
					  GdkEventKey *event,
					  GebrGuiSequenceEdit *self)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter selected;
	GtkTreeIter prev_iter;
	GtkTreeIter next_iter;
	GtkTreeSelection *tree_sel;
	gboolean has_prev = FALSE;
	gboolean has_next = FALSE;

	tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tree_view));
	if (!gtk_tree_selection_get_selected (tree_sel, &model, &selected))
		return FALSE;

	path = gtk_tree_model_get_path (model, &selected);
	if (gtk_tree_path_prev (path))
		 has_prev = gtk_tree_model_get_iter (model, &prev_iter, path);
	gtk_tree_path_free (path);

	path = gtk_tree_model_get_path (model, &selected);
	gtk_tree_path_next (path);
	has_next = gtk_tree_model_get_iter (model, &next_iter, path);
	gtk_tree_path_free (path);

	switch (event->keyval) {
	case GDK_Delete:
		if (!self->priv->is_editing)
			gebr_gui_sequence_edit_remove (self, &selected);
		else
			return FALSE;
		break;
	case GDK_Up:
		if (!(event->state & GDK_CONTROL_MASK) || !has_prev)
			return FALSE;
		gebr_gui_sequence_edit_move (self, &selected, &prev_iter,
					     GTK_TREE_VIEW_DROP_BEFORE);
		break;
	case GDK_Down:
		if (!(event->state & GDK_CONTROL_MASK) || !has_next)
			return FALSE;
		gebr_gui_sequence_edit_move (self, &selected, &next_iter,
					     GTK_TREE_VIEW_DROP_AFTER);
		break;
	case GDK_Home: {
		if (!(event->state & GDK_CONTROL_MASK) || !has_prev)
			return FALSE;
		gebr_gui_sequence_edit_move_top (self, &selected);
		break;
	}
	case GDK_End:
		if (!(event->state & GDK_CONTROL_MASK) || !has_next)
			return FALSE;
		gebr_gui_sequence_edit_move_bottom (self, &selected);
		break;
	default:
		return FALSE;
	}

	if (!gtk_tree_selection_get_selected (tree_sel, &model, &selected))
		return FALSE;
	path = gtk_tree_model_get_path (model, &selected);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->tree_view), path, NULL, FALSE);
	gtk_tree_path_free (path);

	return TRUE;
}

static void gebr_gui_sequence_edit_add_request (GebrGuiSequenceEdit *self)
{
	gint n;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	tree = GTK_TREE_VIEW (self->tree_view);
	model = GTK_TREE_MODEL (self->list_store);
	selection = gtk_tree_view_get_selection (tree);
	n = gtk_tree_model_iter_n_children (model, NULL);

	if (!n)
		return;

	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, n - 1)) {
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0, 0);
		gtk_tree_path_free (path);
	}
}

static void on_tree_view_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	g_signal_handlers_block_by_func(widget, on_tree_view_size_request, NULL);
	gtk_widget_size_request(widget, requisition);
	g_signal_handlers_unblock_by_func(widget, on_tree_view_size_request, NULL);

	requisition->height = MAX(requisition->height, 100);
	requisition->width = widget->allocation.width;
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GtkTreeIter gebr_gui_sequence_edit_add (GebrGuiSequenceEdit *self,
					const gchar *text,
					gboolean show_empty_value_text)
{
	GtkTreeIter iter = { 0, };

	g_return_val_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self), iter);

	gtk_list_store_append(self->list_store, &iter);
	gtk_list_store_set(self->list_store, &iter,
			   0, (show_empty_value_text && !strlen(text)) ? _("<empty value>") : text,
			   -1);

	return iter;
}

void gebr_gui_sequence_edit_clear (GebrGuiSequenceEdit *self)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	gtk_list_store_clear (self->list_store);
}

void gebr_gui_sequence_edit_remove (GebrGuiSequenceEdit *self,
				    GtkTreeIter *iter)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->remove (self, iter);
}

void gebr_gui_sequence_edit_move (GebrGuiSequenceEdit *self,
				  GtkTreeIter *iter,
				  GtkTreeIter *position,
				  GtkTreeViewDropPosition pos)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->move (self, iter, position, pos);
}

void gebr_gui_sequence_edit_move_top (GebrGuiSequenceEdit *self,
				      GtkTreeIter *iter)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->move_top (self, iter);
}

void gebr_gui_sequence_edit_move_bottom (GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->move_bottom (self, iter);
}

void gebr_gui_sequence_edit_rename (GebrGuiSequenceEdit *self,
                                    GtkTreeIter *iter,
                                    const gchar *new_text)
{
	g_return_if_fail (GEBR_GUI_IS_SEQUENCE_EDIT (self));

	GEBR_GUI_SEQUENCE_EDIT_GET_CLASS (self)->rename (self, iter, new_text);
}

void
gebr_gui_sequence_edit_set_keypresses(GebrGuiSequenceEdit *self,
                                      gboolean setting)
{
	if (!setting)
		g_signal_handler_block(self->tree_view, self->priv->keypress_handler);
	else
		g_signal_handler_unblock(self->tree_view, self->priv->keypress_handler);
}
