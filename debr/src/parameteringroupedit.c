/*   libgebr - GÍBR Library
 *   Copyright (C) 2007-2008 GÍBR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>

#include <gui/utils.h>

#include "parameteringroupedit.h"
#include "support.h"

/*
 * Prototypes
 */

static void
on_parameter_in_group_tree_view_selected(GtkWidget * tree_view, ParameterInGroupEdit * parameter_in_group_edit);
static GeoXmlParameter *
__parameter_in_group_edit_get_parameter_selected(ParameterInGroupEdit * parameter_in_group_edit);
static GeoXmlSequence *
__parameter_in_group_edit_get_master_parameter_from_iter(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter);
static void
__on_parameter_widget_changed(struct parameter_widget * parameter_widget, ParameterInGroupEdit * parameter_in_group_edit); 
static void __parameter_in_group_edit_add(ParameterInGroupEdit * parameter_in_group_edit, GeoXmlParameter * parameter);
static void __parameter_in_group_edit_add_request(ParameterInGroupEdit * parameter_in_group_edit);
static void __parameter_in_group_edit_remove(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter);
static void __parameter_in_group_edit_move_up(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter);
static void __parameter_in_group_edit_move_down(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter);
static void __parameter_in_group_edit_rename(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter, const gchar * new_text);

/*
 * gobject stuff
 */

enum {
	INSTANCES_CHANGED = 0,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
parameter_in_group_edit_class_init(ParameterInGroupEditClass * class)
{
	GtkSequenceEditClass *	gtk_sequence_edit_class;

	/* virtual */
	gtk_sequence_edit_class = GTK_SEQUENCE_EDIT_CLASS(class);
	gtk_sequence_edit_class->add =
		(typeof(gtk_sequence_edit_class->add))__parameter_in_group_edit_add_request;
	gtk_sequence_edit_class->remove =
		(typeof(gtk_sequence_edit_class->remove))__parameter_in_group_edit_remove;
	gtk_sequence_edit_class->move_up =
		(typeof(gtk_sequence_edit_class->move_up))__parameter_in_group_edit_move_up;
	gtk_sequence_edit_class->move_down =
		(typeof(gtk_sequence_edit_class->move_down))__parameter_in_group_edit_move_down;
	gtk_sequence_edit_class->rename =
		(typeof(gtk_sequence_edit_class->rename))__parameter_in_group_edit_rename;

	/* signals */
	object_signals[INSTANCES_CHANGED] = g_signal_new("instances-changed",
		TYPE_PARAMETER_IN_GROUP_EDIT,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET(ParameterInGroupEditClass, instances_changed),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
parameter_in_group_edit_init(ParameterInGroupEdit * parameter_in_group_edit)
{
	parameter_in_group_edit->parameter_widget = NULL;
}

G_DEFINE_TYPE(ParameterInGroupEdit, parameter_in_group_edit, GTK_TYPE_SEQUENCE_EDIT);

/*
 * Internal functions
 */

static void
on_parameter_in_group_tree_view_selected(GtkWidget * tree_view, ParameterInGroupEdit * parameter_in_group_edit)
{
	GtkWidget *		hbox;
	GeoXmlParameter *	parameter;

	parameter = __parameter_in_group_edit_get_parameter_selected(parameter_in_group_edit);
	if (parameter == NULL)
		return;

	if (parameter_in_group_edit->parameter_widget != NULL)
		gtk_widget_destroy(parameter_in_group_edit->parameter_widget->widget);

	g_object_get(parameter_in_group_edit, "value-widget", &hbox, NULL);
	parameter_in_group_edit->parameter_widget = parameter_widget_new(parameter, TRUE, NULL);
	gtk_widget_show(parameter_in_group_edit->parameter_widget->widget);
	gtk_box_pack_start(GTK_BOX(hbox), parameter_in_group_edit->parameter_widget->widget, TRUE, TRUE, 0);
	parameter_widget_set_auto_submit_callback(parameter_in_group_edit->parameter_widget,
		(changed_callback)__on_parameter_widget_changed, parameter_in_group_edit);
}

static GeoXmlParameter *
__parameter_in_group_edit_get_parameter_selected(ParameterInGroupEdit * parameter_in_group_edit)
{
	GtkWidget *		tree_view;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlParameter *	parameter;

	g_object_get(parameter_in_group_edit, "list-store", &model, NULL);
	tree_view = GTK_SEQUENCE_EDIT(parameter_in_group_edit)->tree_view;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;
	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(parameter_in_group_edit)->list_store), &iter,
		1, &parameter,
		-1);

	return parameter;
}

static GeoXmlSequence *
__parameter_in_group_edit_get_master_parameter_from_iter(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter)
{
	GtkTreeModel *		tree_model;
	GtkTreePath *		tree_path;
	guint			parameter_index;

	GeoXmlSequence *	instance;
	GeoXmlSequence *	parameter;

	g_object_get(parameter_in_group_edit, "list-store", &tree_model, NULL);
	tree_path = gtk_tree_model_get_path(tree_model, iter);
	parameter_index = (guint)atoi(gtk_tree_path_to_string(tree_path));
	gtk_tree_path_free(tree_path);

	geoxml_parameter_group_get_instance(parameter_in_group_edit->parameter_group, &instance, 0);
	geoxml_parameters_get_parameter(GEOXML_PARAMETERS(instance), &parameter, parameter_index);

	return parameter;
}

static void
__on_parameter_widget_changed(struct parameter_widget * parameter_widget, ParameterInGroupEdit * parameter_in_group_edit)
{
	GeoXmlParameter *	parameter;

	parameter = __parameter_in_group_edit_get_parameter_selected(parameter_in_group_edit);
	if (parameter == NULL)
		return;

	GtkListStore *		list_store;
	GtkTreeIter		iter;
	GString *		value;
	gboolean		has_next;

	value = geoxml_program_parameter_get_string_value(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter), TRUE);
	g_object_get(parameter_in_group_edit, "list-store", &list_store, NULL);
	has_next = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	while (has_next) {
		GeoXmlParameter *	i;

		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter,
			1, &i,
			-1);
		if (i == parameter_widget->parameter) {
			gtk_list_store_set(list_store, &iter,
				0, value->str,
				-1);
			break;
		}

		has_next = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}

	g_string_free(value, TRUE);
}

static void
__parameter_in_group_edit_add(ParameterInGroupEdit * parameter_in_group_edit, GeoXmlParameter * parameter)
{
	GtkTreeIter	iter;
	GString *	value;

	value = geoxml_program_parameter_get_string_value(GEOXML_PROGRAM_PARAMETER(parameter), TRUE);
	iter = gtk_sequence_edit_add(GTK_SEQUENCE_EDIT(parameter_in_group_edit), value->str, TRUE);
	gtk_list_store_set(GTK_SEQUENCE_EDIT(parameter_in_group_edit)->list_store, &iter,
		1, parameter,
		-1);

	g_string_free(value, TRUE);
}

static void
__parameter_in_group_edit_add_request(ParameterInGroupEdit * parameter_in_group_edit)
{
	if (parameter_in_group_edit->parameter_widget == NULL)
		return;

	GeoXmlParameter *	parameter;
	GeoXmlParameters *	instance;
	GeoXmlSequence *	new_parameter;
	GString *		value;

	parameter = __parameter_in_group_edit_get_parameter_selected(parameter_in_group_edit);
	if (parameter == NULL)
		return;

	instance = geoxml_parameter_group_instanciate(parameter_in_group_edit->parameter_group);
	geoxml_parameters_get_parameter(instance, &new_parameter, parameter_in_group_edit->parameter_index);
	value = parameter_widget_get_widget_value(parameter_in_group_edit->parameter_widget);
	geoxml_program_parameter_set_string_value(GEOXML_PROGRAM_PARAMETER(new_parameter), TRUE, value->str);
	__parameter_in_group_edit_add(parameter_in_group_edit, GEOXML_PARAMETER(new_parameter));

	g_signal_emit(parameter_in_group_edit, object_signals[INSTANCES_CHANGED], 0);
	g_signal_emit_by_name(parameter_in_group_edit, "changed");

	g_string_free(value, TRUE);
}

static void
__parameter_in_group_edit_remove(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter)
{
	GtkListStore *		list_store;
	GtkTreePath *		tree_path;
	guint			instance_index;

	GeoXmlSequence *	instance;

	g_object_get(parameter_in_group_edit, "list-store", &list_store, NULL);
	tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(list_store), iter);
	instance_index = (guint)atoi(gtk_tree_path_to_string(tree_path));
	gtk_tree_path_free(tree_path);

	geoxml_parameter_group_get_instance(parameter_in_group_edit->parameter_group, &instance, instance_index);

	geoxml_sequence_remove(instance);
	gtk_list_store_remove(list_store, iter);

	g_signal_emit(parameter_in_group_edit, object_signals[INSTANCES_CHANGED], 0);
	g_signal_emit_by_name(parameter_in_group_edit, "changed");
}

static void
__parameter_in_group_edit_move_up(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	parameter;
	GtkListStore *		list_store;

	parameter = __parameter_in_group_edit_get_master_parameter_from_iter(parameter_in_group_edit, iter);
	g_object_get(parameter_in_group_edit, "list-store", &list_store, NULL);

	geoxml_sequence_move_up(parameter);
	gtk_list_store_move_up(list_store, iter);

	g_signal_emit(parameter_in_group_edit, object_signals[INSTANCES_CHANGED], 0);
	g_signal_emit_by_name(parameter_in_group_edit, "changed");
}

static void
__parameter_in_group_edit_move_down(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter)
{
	GeoXmlSequence *	parameter;
	GtkListStore *		list_store;

	parameter = __parameter_in_group_edit_get_master_parameter_from_iter(parameter_in_group_edit, iter);
	g_object_get(parameter_in_group_edit, "list-store", &list_store, NULL);

	geoxml_sequence_move_down(parameter);
	gtk_list_store_move_down(list_store, iter);

	g_signal_emit(parameter_in_group_edit, object_signals[INSTANCES_CHANGED], 0);
	g_signal_emit_by_name(parameter_in_group_edit, "changed");
}

static void
__parameter_in_group_edit_rename(ParameterInGroupEdit * parameter_in_group_edit, GtkTreeIter * iter, const gchar * new_text)
{
	GeoXmlProgramParameter *	program_parameter;

	gtk_tree_model_get(GTK_TREE_MODEL(GTK_SEQUENCE_EDIT(parameter_in_group_edit)->list_store), iter,
		1, &program_parameter,
		-1);
	gtk_list_store_set(GTK_SEQUENCE_EDIT(parameter_in_group_edit)->list_store, iter,
		0, new_text,
		-1);
	geoxml_program_parameter_set_string_value(program_parameter, TRUE, new_text);
	parameter_widget_update(parameter_in_group_edit->parameter_widget);

	g_signal_emit_by_name(parameter_in_group_edit, "changed");
}

/*
 * Library functions
 */

GtkWidget *
parameter_in_group_edit_new(GeoXmlParameterGroup * parameter_group, guint parameter_index)
{
	ParameterInGroupEdit *	parameter_in_group_edit;
	GtkWidget *		hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

	parameter_in_group_edit = PARAMETER_IN_GROUP_EDIT(g_object_new(TYPE_PARAMETER_IN_GROUP_EDIT,
		"value-widget", hbox,
		"list-store", gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER, -1),
		"minimum-one", TRUE,
		NULL));

	g_signal_connect(GTK_SEQUENCE_EDIT(parameter_in_group_edit)->tree_view, "cursor-changed",
		(GCallback)on_parameter_in_group_tree_view_selected, parameter_in_group_edit);

	parameter_in_group_edit->parameter_group = parameter_group;
	parameter_in_group_edit->parameter_index = parameter_index;

	return GTK_WIDGET(parameter_in_group_edit);
}

void
parameter_in_group_edit_load(ParameterInGroupEdit * parameter_in_group_edit)
{
	GtkListStore *		list_store;
	GtkWidget *		tree_view;
	GtkTreeSelection *	tree_selection;
	GtkTreeIter		iter;

	GSList *		referencee_list, * i;

	g_object_get(parameter_in_group_edit, "list-store", &list_store, NULL);
	tree_view = GTK_SEQUENCE_EDIT(parameter_in_group_edit)->tree_view;
	gtk_list_store_clear(list_store);

	referencee_list = geoxml_parameter_group_get_parameter_in_all_instances(
		parameter_in_group_edit->parameter_group, parameter_in_group_edit->parameter_index);
	for (i = referencee_list; i != NULL; i = g_slist_next(i))
		__parameter_in_group_edit_add(parameter_in_group_edit, GEOXML_PARAMETER(i->data));

	/* select first */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_select_iter(tree_selection, &iter);
	on_parameter_in_group_tree_view_selected(tree_view, parameter_in_group_edit);

	/* frees */
	g_slist_free(referencee_list);
}
