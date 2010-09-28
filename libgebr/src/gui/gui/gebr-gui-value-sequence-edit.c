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

#include "../../intl.h"

#include "gebr-gui-value-sequence-edit.h"
#include "gebr-gui-utils.h"

/*
 * Prototypes
 */

static void __gebr_gui_value_sequence_edit_remove(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
						  GtkTreeIter * iter);
static void __gebr_gui_value_sequence_edit_move(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
						GtkTreeIter * iter, GtkTreeIter * position,
						GtkTreeViewDropPosition drop_position);
static void __gebr_gui_value_sequence_edit_move_top(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
						    GtkTreeIter * iter);
static void __gebr_gui_value_sequence_edit_move_bottom(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
						       GtkTreeIter * iter);
static void __gebr_gui_value_sequence_edit_rename(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
						  GtkTreeIter * iter, const gchar * new_text);

/*
 * gobject stuff
 */

enum {
	MINIMUM_ONE = 1,
};

static void
gebr_gui_value_sequence_edit_set_property(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, guint property_id,
					  const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case MINIMUM_ONE:
		gebr_gui_value_sequence_edit->minimum_one = g_value_get_boolean(value);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gebr_gui_value_sequence_edit, property_id, param_spec);
		break;
	}
}

static void
gebr_gui_value_sequence_edit_get_property(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, guint property_id,
					  GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case MINIMUM_ONE:
		g_value_set_boolean(value, gebr_gui_value_sequence_edit->minimum_one);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gebr_gui_value_sequence_edit, property_id, param_spec);
		break;
	}
}

static void gebr_gui_value_sequence_edit_class_init(GebrGuiValueSequenceEditClass * klass)
{
	GebrGuiSequenceEditClass *sequence_edit_class;
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	/* virtual */
	sequence_edit_class = GEBR_GUI_gebr_gui_sequence_edit_CLASS(klass);
	sequence_edit_class->remove = (typeof(sequence_edit_class->remove)) __gebr_gui_value_sequence_edit_remove;
	sequence_edit_class->move = (typeof(sequence_edit_class->move)) __gebr_gui_value_sequence_edit_move;
	sequence_edit_class->move_top = (typeof(sequence_edit_class->move_top)) __gebr_gui_value_sequence_edit_move_top;
	sequence_edit_class->move_bottom =
	    (typeof(sequence_edit_class->move_bottom)) __gebr_gui_value_sequence_edit_move_bottom;
	sequence_edit_class->rename = (typeof(sequence_edit_class->rename)) __gebr_gui_value_sequence_edit_rename;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) gebr_gui_value_sequence_edit_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) gebr_gui_value_sequence_edit_get_property;

	param_spec = g_param_spec_boolean("minimum-one",
					  "Minimum one", "True if the list keep at least one item",
					  FALSE, (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, MINIMUM_ONE, param_spec);
}

static void gebr_gui_value_sequence_edit_init(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit)
{
}

G_DEFINE_TYPE(GebrGuiValueSequenceEdit, gebr_gui_value_sequence_edit, GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void
__gebr_gui_value_sequence_edit_remove(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store), iter,
			   1, &sequence, -1);

	if (gebr_gui_value_sequence_edit->minimum_one &&
	    gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr_gui_value_sequence_edit->parent.list_store), NULL) == 1)
		gebr_gui_value_sequence_edit->set_function(sequence, "", gebr_gui_value_sequence_edit->user_data);
	else
		gebr_geoxml_sequence_remove(sequence);

	gtk_list_store_remove(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter);

	g_signal_emit_by_name(gebr_gui_value_sequence_edit, "changed");
}

static void
__gebr_gui_value_sequence_edit_move(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GtkTreeIter * iter,
				    GtkTreeIter * position, GtkTreeViewDropPosition drop_position)
{
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlSequence *position_sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store), iter,
			   1, &sequence, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store),
			   position, 1, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter,
					  position);
	} else {
		gebr_geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter,
					   position);
	}

	g_signal_emit_by_name(gebr_gui_value_sequence_edit, "changed");
}

static void
__gebr_gui_value_sequence_edit_move_top(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store), iter,
			   1, &sequence, -1);

	gebr_geoxml_sequence_move_after(sequence, NULL);
	gtk_list_store_move_after(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(gebr_gui_value_sequence_edit, "changed");
}

static void
__gebr_gui_value_sequence_edit_move_bottom(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GtkTreeIter * iter)
{
	GebrGeoXmlSequence *sequence;

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store), iter,
			   1, &sequence, -1);

	gebr_geoxml_sequence_move_before(sequence, NULL);
	gtk_list_store_move_before(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter, NULL);

	g_signal_emit_by_name(gebr_gui_value_sequence_edit, "changed");
}

static void
__gebr_gui_value_sequence_edit_rename(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GtkTreeIter * iter,
				      const gchar * new_text)
{
	GebrGeoXmlSequence *sequence;
	if(!strlen(new_text)){
		return; 
	}

	gtk_tree_model_get(GTK_TREE_MODEL(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store), iter,
			   1, &sequence, -1);

	gtk_list_store_set(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, iter, 0, new_text, -1);
	gebr_gui_value_sequence_edit->set_function(sequence, new_text, gebr_gui_value_sequence_edit->user_data);

	g_signal_emit_by_name(gebr_gui_value_sequence_edit, "changed");
}

/*
 * Library functions
 */

GtkWidget *gebr_gui_value_sequence_edit_new(GtkWidget * widget)
{
	GtkListStore *list_store;
	GebrGuiValueSequenceEdit *gebr_gui_value_sequence_edit;

	list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER, -1);

	gebr_gui_value_sequence_edit = g_object_new(TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT,
						    "value-widget", widget, "list-store", list_store, NULL);
	gebr_gui_value_sequence_edit->set_function = NULL;
	gebr_gui_value_sequence_edit->get_function = NULL;

	return GTK_WIDGET(gebr_gui_value_sequence_edit);
}

void
gebr_gui_value_sequence_edit_add(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GebrGeoXmlSequence * sequence)
{
	GtkTreeIter iter;

	iter = gebr_gui_sequence_edit_add(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit),
				     gebr_gui_value_sequence_edit->get_function(sequence,
										gebr_gui_value_sequence_edit->
										user_data), FALSE);
	gtk_list_store_set(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store, &iter, 1, sequence,
			   -1);
}

void
gebr_gui_value_sequence_edit_load(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit,
				  GebrGeoXmlSequence * sequence, ValueSequenceSetFunction set_function,
				  ValueSequenceGetFunction get_function, gpointer user_data)
{
	if (set_function == NULL || get_function == NULL)
		return;

	gebr_gui_value_sequence_edit->set_function = set_function;
	gebr_gui_value_sequence_edit->get_function = get_function;
	gebr_gui_value_sequence_edit->user_data = user_data;
	gtk_list_store_clear(GEBR_GUI_gebr_gui_sequence_edit(gebr_gui_value_sequence_edit)->list_store);

	if (gebr_gui_value_sequence_edit->minimum_one) {
		GebrGeoXmlSequence *next;

		next = sequence;
		gebr_geoxml_sequence_next(&next);
		if (next == NULL &&
		    !strlen(gebr_gui_value_sequence_edit->get_function
			    (sequence, gebr_gui_value_sequence_edit->user_data)))
			return;
	}

	for (; sequence != NULL; gebr_geoxml_sequence_next(&sequence))
		gebr_gui_value_sequence_edit_add(gebr_gui_value_sequence_edit, sequence);
}
