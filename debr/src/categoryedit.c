/*   DeBR - GeBR Designer
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

#include <libgebr/intl.h>
#include <libgebr/validate.h>
#include <libgebr/gui/gtkenhancedentry.h>
#include <libgebr/gui/utils.h>

#include "categoryedit.h"
#include "debr.h"

/*
 * Prototypes
 */

static void __category_edit_add(CategoryEdit * category_edit, GebrGeoXmlSequence * category);
static void __category_edit_remove(CategoryEdit * category_edit, GtkTreeIter * iter);
static void __category_edit_move(CategoryEdit * category_edit, GtkTreeIter * iter, GtkTreeIter * position,
				    GtkTreeViewDropPosition drop_position);
static void __category_edit_move_top(CategoryEdit * category_edit, GtkTreeIter * iter);
static void __category_edit_move_bottom(CategoryEdit * category_edit, GtkTreeIter * iter);
static GtkWidget *__category_edit_create_tree_view(CategoryEdit * category_edit);

/*
 * gobject stuff
 */

/**
 * \internal
 * Properties enumeration
 */
enum {
	CATEGORY = 1
};

/**
 * \internal
 */
static void
category_edit_set_property(CategoryEdit * category_edit, guint property_id, const GValue * value,
			      GParamSpec * param_spec)
{
	switch (property_id) {
	case CATEGORY:{
		GebrGeoXmlSequence *category;

		gtk_list_store_clear(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store);
		category_edit->category = g_value_get_pointer(value);
		category = (GebrGeoXmlSequence *) category_edit->category;
		for (; category != NULL; gebr_geoxml_sequence_next(&category))
			__category_edit_add(category_edit, category);

		break;
	} default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(category_edit, property_id, param_spec);
		break;
	}
}

/**
 * \internal
 */
static void
category_edit_get_property(CategoryEdit * category_edit, guint property_id, GValue * value,
			      GParamSpec * param_spec)
{
	switch (property_id) {
	case CATEGORY:
		g_value_set_pointer(value, category_edit->category);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(category_edit, property_id, param_spec);
		break;
	}
}

/**
 * \internal
 */
static void category_edit_class_init(CategoryEditClass * class)
{
	GtkSequenceEditClass *category_edit_class;
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	/* virtual */
	category_edit_class = GEBR_GUI_GTK_SEQUENCE_EDIT_CLASS(class);
	category_edit_class->remove = (typeof(category_edit_class->remove)) __category_edit_remove;
	category_edit_class->move = (typeof(category_edit_class->move)) __category_edit_move;
	category_edit_class->move_top = (typeof(category_edit_class->move_top)) __category_edit_move_top;
	category_edit_class->move_bottom =
	    (typeof(category_edit_class->move_bottom)) __category_edit_move_bottom;
	category_edit_class->create_tree_view =
	    (typeof(category_edit_class->create_tree_view)) __category_edit_create_tree_view;

	gobject_class = G_OBJECT_CLASS(class);
	gobject_class->set_property = (typeof(gobject_class->set_property)) category_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) category_edit_get_property;

	param_spec = g_param_spec_pointer("category",
					  "", "", G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, CATEGORY, param_spec);
}

/**
 * \internal
 */
static void category_edit_init(CategoryEdit * category_edit)
{
}

G_DEFINE_TYPE(CategoryEdit, category_edit, GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

/**
 * \internal
 */
static void category_edit_add_request(CategoryEdit * category_edit)
{
	gchar *name;
	GtkWidget *combo;

	g_object_get(G_OBJECT(category_edit), "value-widget", &combo, NULL);
	name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
	if (!strlen(name))
		name = g_strdup(_("New category"));
	else
		debr_has_category(name, TRUE);
	__category_edit_add(category_edit, GEBR_GEOXML_SEQUENCE(gebr_geoxml_flow_append_category(category_edit->menu, name)));

	g_signal_emit_by_name(category_edit, "changed");

	g_free(name);
}

/**
 * \internal
 */
static void
__category_edit_on_value_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
				   GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	GebrGeoXmlCategory *category;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter, 2, &category, -1);

	gtk_list_store_set(sequence_edit->list_store, &iter, 0, new_text, -1);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(category), new_text);

	g_signal_emit_by_name(sequence_edit, "changed");
}

/**
 * \internal
 */
static void __category_edit_add(CategoryEdit * category_edit, GebrGeoXmlSequence * category)
{
	GtkTreeIter iter;

	GebrValidateCase *validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY);

	gtk_list_store_append(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, &iter);
	gtk_list_store_set(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, &iter,
			   0, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)),
			   1, NULL, 2, category, -1);
}

/**
 * \internal
 */
static void __category_edit_remove(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_remove(sequence);
	gtk_list_store_remove(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, iter);

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static void
__category_edit_move(CategoryEdit * category_edit, GtkTreeIter * iter, GtkTreeIter * position,
			GtkTreeViewDropPosition drop_position)
{
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlSequence *position_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store), position,
			   2, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, iter, position);
	} else {
		gebr_geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, iter, position);
	}

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static void __category_edit_move_top(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_move_after(sequence, NULL);
	gtk_list_store_move_after(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static void __category_edit_move_bottom(CategoryEdit * category_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store), iter,
			   2, &sequence, -1);

	gebr_geoxml_sequence_move_before(sequence, NULL);
	gtk_list_store_move_before(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(category_edit, "changed");
}

/**
 * \internal
 */
static GtkWidget *__category_edit_create_tree_view(CategoryEdit * category_edit)
{
	GtkWidget *tree_view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	tree_view =
	    gtk_tree_view_new_with_model(GTK_TREE_MODEL(GEBR_GUI_GTK_SEQUENCE_EDIT(category_edit)->list_store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(__category_edit_on_value_edited), category_edit);
	col = gtk_tree_view_column_new_with_attributes(_("value"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes(_("label"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

GtkWidget *category_edit_new(GebrGeoXmlFlow * menu)
{
	CategoryEdit *category_edit;
	GtkListStore *list_store;
	GtkWidget *hbox;
	GtkWidget *categories_combo;
	GtkWidget *validate_image;

	list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, -1);
	hbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(hbox);
	categories_combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(debr.categories_model), CATEGORY_NAME);
	gtk_box_pack_start(GTK_BOX(hbox), categories_combo, TRUE, TRUE, 0);
	validate_image = validate_image_warning_new();
	gtk_box_pack_start(GTK_BOX(hbox), categories_combo, TRUE, TRUE, 0);
	gtk_widget_show(categories_combo);

	GebrGeoXmlSequence *category;
	gebr_geoxml_flow_get_category(menu, &category, 0);
	category_edit = g_object_new(TYPE_CATEGORY_EDIT,
				     "value-widget", hbox,
				     "list-store", list_store, "category", category, NULL);

	category_edit->validate_image = validate_image;
	category_edit->menu = menu;
	g_signal_connect(GTK_OBJECT(category_edit), "add-request", G_CALLBACK(category_edit_add_request), NULL);

	gtk_widget_set_size_request(GTK_WIDGET(category_edit), -1, 150);

	return (GtkWidget *) category_edit;
}
