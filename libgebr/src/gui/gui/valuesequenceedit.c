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

#include "valuesequenceedit.h"
#include "support.h"
#include "utils.h"

/*
 * Prototypes
 */

static void __value_sequence_edit_remove(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_move_up(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_move_down(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter);
static void __value_sequence_edit_rename(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text);

/*
 * gobject stuff
 */

enum {
	VALUE_SEQUENCE = 1
};

static void
value_sequence_edit_set_property(ValueSequenceEdit * sequence_edit, guint property_id, const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_SEQUENCE:
		sequence_edit->value_sequence = g_value_get_pointer(value);
		gtk_list_store_clear(GTK_SEQUENCE_EDIT(sequence_edit)->list_store);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
		break;
	}
}

static void
value_sequence_edit_get_property(ValueSequenceEdit * sequence_edit, guint property_id, GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case VALUE_SEQUENCE:
		g_value_set_pointer(value, sequence_edit->value_sequence);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(sequence_edit, property_id, param_spec);
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
	sequence_edit_class->move_up = (typeof(sequence_edit_class->move_up))__value_sequence_edit_move_up;
	sequence_edit_class->move_down = (typeof(sequence_edit_class->move_down))__value_sequence_edit_move_down;
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
value_sequence_edit_init(ValueSequenceEdit * sequence_edit)
{
}

G_DEFINE_TYPE(ValueSequenceEdit, value_sequence_edit, GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void
__value_sequence_edit_remove(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_remove(sequence);
	gtk_list_store_remove(GTK_SEQUENCE_EDIT(sequence_edit)->list_store, iter);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__value_sequence_edit_move_up(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_move_up(sequence);
	gtk_list_store_move_up(GTK_SEQUENCE_EDIT(sequence_edit)->list_store, iter);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__value_sequence_edit_move_down(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(sequence_edit)->list_store), iter,
		1, &sequence,
		-1);

	geoxml_sequence_move_down(sequence);
	gtk_list_store_move_down(GTK_SEQUENCE_EDIT(sequence_edit)->list_store, iter);

	g_signal_emit_by_name(sequence_edit, "changed");
}

static void
__value_sequence_edit_rename(ValueSequenceEdit * sequence_edit, GtkTreeIter * iter, const gchar * new_text)
{
	GeoXmlValueSequence *	value_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(sequence_edit)->list_store), iter,
		1, &value_sequence,
		-1);

	gtk_list_store_set(GTK_SEQUENCE_EDIT(sequence_edit)->list_store, iter,
		0, new_text,
		-1);
	geoxml_value_sequence_set(value_sequence, new_text);

	g_signal_emit_by_name(sequence_edit, "changed");
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
value_sequence_edit_add(ValueSequenceEdit * sequence_edit, GeoXmlValueSequence * value_sequence)
{
	GtkTreeIter	iter;

	iter = gtk_sequence_edit_add(GTK_SEQUENCE_EDIT(sequence_edit), geoxml_value_sequence_get(value_sequence));
	gtk_list_store_set(GTK_SEQUENCE_EDIT(sequence_edit)->list_store, &iter,
		1, value_sequence,
		-1);
}

void
value_sequence_edit_load(ValueSequenceEdit * sequence_edit)
{
	GeoXmlSequence *	sequence;

	gtk_list_store_clear(GTK_SEQUENCE_EDIT(sequence_edit)->list_store);

	sequence = GEOXML_SEQUENCE(sequence_edit->value_sequence);
	while (sequence != NULL) {
		value_sequence_edit_add(sequence_edit, GEOXML_VALUE_SEQUENCE(sequence));

		geoxml_sequence_next(&sequence);
	}
}
