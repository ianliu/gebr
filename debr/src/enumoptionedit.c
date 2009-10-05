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

#include <libgebr/gui/gtkenhancedentry.h>
#include <libgebr/gui/utils.h>

#include <libgebr/intl.h>

#include "enumoptionedit.h"

/*
 * Prototypes
 */

static void __enum_option_edit_add(EnumOptionEdit * enum_option_edit, GeoXmlEnumOption * enum_option);
static void __enum_option_edit_remove(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter);
static void __enum_option_edit_move(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter, GtkTreeIter * position, 	GtkTreeViewDropPosition drop_position);
static void __enum_option_edit_move_top(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter);
static void __enum_option_edit_move_bottom(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter);
static GtkWidget * __enum_option_edit_create_tree_view(EnumOptionEdit * enum_option_edit);

/*
 * gobject stuff
 */

enum {
	ENUM_OPTION = 1
};

static void
enum_option_edit_set_property(EnumOptionEdit * enum_option_edit, guint property_id, const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case ENUM_OPTION: {
		GeoXmlSequence *	enum_option;

		gtk_list_store_clear(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store);
		enum_option_edit->enum_option = g_value_get_pointer(value);
		enum_option = (GeoXmlSequence*)enum_option_edit->enum_option;
		for (; enum_option != NULL; geoxml_sequence_next(&enum_option))
			__enum_option_edit_add(enum_option_edit, GEOXML_ENUM_OPTION(enum_option));

		break;
	} default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(enum_option_edit, property_id, param_spec);
		break;
	}
}

static void
enum_option_edit_get_property(EnumOptionEdit * enum_option_edit, guint property_id, GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case ENUM_OPTION:
		g_value_set_pointer(value, enum_option_edit->enum_option);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(enum_option_edit, property_id, param_spec);
		break;
	}
}

static void
enum_option_edit_class_init(EnumOptionEditClass * class)
{
	GtkSequenceEditClass *	enum_option_edit_class;
	GObjectClass *		gobject_class;
	GParamSpec *		param_spec;

	/* virtual */
	enum_option_edit_class = GTK_SEQUENCE_EDIT_CLASS(class);
	enum_option_edit_class->remove = (typeof(enum_option_edit_class->remove))__enum_option_edit_remove;
	enum_option_edit_class->move = (typeof(enum_option_edit_class->move))__enum_option_edit_move;
	enum_option_edit_class->move_top = (typeof(enum_option_edit_class->move_top))__enum_option_edit_move_top;
	enum_option_edit_class->move_bottom = (typeof(enum_option_edit_class->move_bottom))__enum_option_edit_move_bottom;
	enum_option_edit_class->create_tree_view =
		(typeof(enum_option_edit_class->create_tree_view))__enum_option_edit_create_tree_view;

	gobject_class = G_OBJECT_CLASS(class);
	gobject_class->set_property = (typeof(gobject_class->set_property))enum_option_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property))enum_option_edit_get_property;

	param_spec = g_param_spec_pointer("enum-option",
		"Enum Option", "GeoXml's enum option source of data",
		G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, ENUM_OPTION, param_spec);
}

static void
enum_option_edit_init(EnumOptionEdit * enum_option_edit)
{
}

G_DEFINE_TYPE(EnumOptionEdit, enum_option_edit, GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void
enum_option_edit_add_request(EnumOptionEdit * enum_option_edit)
{
	GeoXmlEnumOption *		enum_option;

	enum_option = geoxml_program_parameter_append_enum_option(enum_option_edit->program_parameter,
		gtk_enhanced_entry_get_text(GTK_ENHANCED_ENTRY(enum_option_edit->label_entry)),
		gtk_enhanced_entry_get_text(GTK_ENHANCED_ENTRY(enum_option_edit->value_entry)));
	__enum_option_edit_add(enum_option_edit, enum_option);

	gtk_enhanced_entry_set_text(GTK_ENHANCED_ENTRY(enum_option_edit->label_entry), "");
	gtk_enhanced_entry_set_text(GTK_ENHANCED_ENTRY(enum_option_edit->value_entry), "");

	g_signal_emit_by_name(enum_option_edit, "changed");
}

static void
__enum_option_edit_on_label_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
	GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlEnumOption *	enum_option;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter,
		2, &enum_option,
		-1);

	gtk_list_store_set(sequence_edit->list_store, &iter,
		1, new_text,
		-1);
	geoxml_enum_option_set_label(enum_option, new_text);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__enum_option_edit_on_value_edited(GtkCellRendererText * cell, gchar * path_string, gchar * new_text,
	GtkSequenceEdit * sequence_edit)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlEnumOption *	enum_option;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sequence_edit->tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(sequence_edit->list_store), &iter,
		2, &enum_option,
		-1);

	gtk_list_store_set(sequence_edit->list_store, &iter,
		0, new_text,
		-1);
	geoxml_enum_option_set_value(enum_option, new_text);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__enum_option_edit_add(EnumOptionEdit * enum_option_edit, GeoXmlEnumOption * enum_option)
{
	GtkTreeIter	iter;

	gtk_list_store_append(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, &iter);
	gtk_list_store_set(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, &iter,
		0, geoxml_enum_option_get_value(enum_option),
		1, geoxml_enum_option_get_label(enum_option),
		2, enum_option,
		-1);
}

static void
__enum_option_edit_remove(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store), iter,
		2, &sequence,
		-1);

	geoxml_sequence_remove(sequence);
	gtk_list_store_remove(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, iter);

	g_signal_emit_by_name(enum_option_edit, "changed");
}

static void
__enum_option_edit_move(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position)
{
	GeoXmlSequence *	sequence;
	GeoXmlSequence *	position_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store), iter,
		2, &sequence, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store), position,
		2, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, iter, position);
	} else {
		geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, iter, position);
	}

	g_signal_emit_by_name(enum_option_edit, "changed");
}

static void
__enum_option_edit_move_top(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store), iter,
		2, &sequence,
		-1);

	geoxml_sequence_move_after(sequence, NULL);
	gtk_list_store_move_after(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(enum_option_edit, "changed");
}

static void
__enum_option_edit_move_bottom(EnumOptionEdit * enum_option_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store), iter,
		2, &sequence,
		-1);

	geoxml_sequence_move_before(sequence, NULL);
	gtk_list_store_move_before(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(enum_option_edit, "changed");
}

static GtkWidget *
__enum_option_edit_create_tree_view(EnumOptionEdit * enum_option_edit)
{
	GtkWidget *		tree_view;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(enum_option_edit)->list_store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(__enum_option_edit_on_value_edited), enum_option_edit);
	col = gtk_tree_view_column_new_with_attributes(_("value"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
		G_CALLBACK(__enum_option_edit_on_label_edited), enum_option_edit);
	col = gtk_tree_view_column_new_with_attributes(_("label"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

/*
 * Library functions
 */

GtkWidget *
enum_option_edit_new(GeoXmlEnumOption * enum_option, GeoXmlProgramParameter * program_parameter)
{
	EnumOptionEdit *	enum_option_edit;
	GtkListStore *		list_store;
	GtkWidget *		hbox;
	GtkWidget *		label_entry;
	GtkWidget *		value_entry;

	list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, -1);
	hbox = gtk_hbox_new(TRUE, 0);
	label_entry = gtk_enhanced_entry_new_with_empty_text(_("label"));
	value_entry = gtk_enhanced_entry_new_with_empty_text(_("value"));
	gtk_widget_set_size_request(value_entry, 20, -1);
	gtk_box_pack_start(GTK_BOX(hbox), value_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label_entry, TRUE, TRUE, 2);
	gtk_widget_show_all(hbox);

	enum_option_edit = g_object_new(TYPE_ENUM_OPTION_EDIT,
		"value-widget", hbox,
		"list-store", list_store,
		"enum-option", enum_option,
		NULL);

	enum_option_edit->program_parameter = program_parameter;
	enum_option_edit->label_entry = label_entry;
	enum_option_edit->value_entry = value_entry;
	g_signal_connect(GTK_OBJECT(enum_option_edit), "add-request",
		GTK_SIGNAL_FUNC(enum_option_edit_add_request), NULL);

	gtk_widget_set_size_request(GTK_WIDGET(enum_option_edit), -1, 150);

	return (GtkWidget*)enum_option_edit;
}
