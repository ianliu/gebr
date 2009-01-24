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

#include "valuesequenceedit.h"
#include "support.h"
#include "utils.h"

/*
 * Prototypes
 */

static void __value_sequence_edit_remove(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_move_before(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter, GtkTreeIter * before);
static void __value_sequence_edit_move_top(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_move_bottom(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_rename(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter, const gchar * new_text);

/*
 * gobject stuff
 */

enum {
	VALUE_SEQUENCE = 1,
};

static void
value_sequence_edit_set_property(ValueSequenceEdit * value_sequence_edit, guint property_id, const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_SEQUENCE:
		value_sequence_edit->value_sequence = g_value_get_pointer(value);
		gtk_list_store_clear(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(value_sequence_edit, property_id, param_spec);
		break;
	}
}

static void
value_sequence_edit_get_property(ValueSequenceEdit * value_sequence_edit, guint property_id, GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_SEQUENCE:
		g_value_set_pointer(value, value_sequence_edit->value_sequence);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(value_sequence_edit, property_id, param_spec);
		break;
	}
}

static void
value_sequence_edit_class_init(ValueSequenceEditClass * class)
{
	GtkSequenceEditClass *	sequence_edit_class;
	GObjectClass *		gobject_class;
	GParamSpec *		param_spec;

	/* virtual */
	sequence_edit_class = GTK_SEQUENCE_EDIT_CLASS(class);
	sequence_edit_class->remove = (typeof(sequence_edit_class->remove))__value_sequence_edit_remove;
	sequence_edit_class->move_before = (typeof(sequence_edit_class->move_before))__value_sequence_edit_move_before;
	sequence_edit_class->move_top = (typeof(sequence_edit_class->move_top))__value_sequence_edit_move_top;
	sequence_edit_class->move_bottom = (typeof(sequence_edit_class->move_bottom))__value_sequence_edit_move_bottom;
	sequence_edit_class->rename = (typeof(sequence_edit_class->rename))__value_sequence_edit_rename;

	gobject_class = G_OBJECT_CLASS(class);
	gobject_class->set_property = (typeof(gobject_class->set_property))value_sequence_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property))value_sequence_edit_get_property;

	param_spec = g_param_spec_pointer("value-sequence",
		"Value sequence", "Value sequence data basis",
		G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, VALUE_SEQUENCE, param_spec);
}

static void
value_sequence_edit_init(ValueSequenceEdit * value_sequence_edit)
{
}

G_DEFINE_TYPE(ValueSequenceEdit, value_sequence_edit, GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void
__value_sequence_edit_remove(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_remove(sequence);
	gtk_list_store_remove(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, iter);

	g_signal_emit_by_name(value_sequence_edit, "changed");
}

static void
__value_sequence_edit_move_before(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter, GtkTreeIter * before)
{
	GeoXmlSequence *	sequence;
	GeoXmlSequence *	before_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), iter,
		1, &sequence,
		-1);
	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), before,
		1, &before_sequence,
		-1);

	geoxml_sequence_move_before(sequence, before_sequence);
	gtk_list_store_move_before(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, iter, before);

	g_signal_emit_by_name(value_sequence_edit, "changed");
}

static void
__value_sequence_edit_move_top(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_move_after(sequence, NULL);
	gtk_list_store_move_after(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(value_sequence_edit, "changed");
}

static void
__value_sequence_edit_move_bottom(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_move_before(sequence, NULL);
	gtk_list_store_move_before(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(value_sequence_edit, "changed");
}

static void
__value_sequence_edit_rename(ValueSequenceEdit * value_sequence_edit, GtkTreeIter * iter, const gchar * new_text)
{
	GeoXmlValueSequence *	value_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store), iter,
		1, &value_sequence,
		-1);

	gtk_list_store_set(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, iter,
		0, new_text,
		-1);
	geoxml_value_sequence_set(value_sequence, new_text);

	g_signal_emit_by_name(value_sequence_edit, "changed");
}

/*
 * Library functions
 */

GtkWidget *
value_sequence_edit_new(GtkWidget * widget)
{
	return value_sequence_edit_new_with_sequence(widget, NULL);
}

GtkWidget *
value_sequence_edit_new_with_sequence(GtkWidget * widget, GeoXmlValueSequence * value_sequence)
{
	GtkListStore *	list_store;

	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER, -1);

	return g_object_new(TYPE_VALUE_SEQUENCE_EDIT,
		"value-widget", widget,
		"value-sequence", value_sequence,
		"list-store", list_store,
		NULL);
}

void
value_sequence_edit_add(ValueSequenceEdit * value_sequence_edit, GeoXmlValueSequence * value_sequence)
{
	GtkTreeIter	iter;

	iter = gtk_sequence_edit_add(GTK_SEQUENCE_EDIT(value_sequence_edit),
		geoxml_value_sequence_get(value_sequence), TRUE);
	gtk_list_store_set(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store, &iter,
		1, value_sequence,
		-1);
}

void
value_sequence_edit_load(ValueSequenceEdit * value_sequence_edit)
{
	GeoXmlSequence *	sequence;

	gtk_list_store_clear(GTK_SEQUENCE_EDIT(value_sequence_edit)->list_store);

	sequence = GEOXML_SEQUENCE(value_sequence_edit->value_sequence);
	for (; sequence != NULL; geoxml_sequence_next(&sequence))
		value_sequence_edit_add(value_sequence_edit, GEOXML_VALUE_SEQUENCE(sequence));
}
