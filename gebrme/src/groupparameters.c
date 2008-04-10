/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>

#include "groupparameters.h"
#include "support.h"
#include "parameter.h"
#include "menu.h"

/*
 * Internal stuff
 */

static void
group_parameters_data_free(GtkObject * expander, struct group_parameters_data * data);

/*
 * Public functions
 */
GtkWidget *
group_parameters_create_ui(struct parameter_data * parameter_data, gboolean expanded)
{
	GtkWidget *			group_parameters_expander;
	GtkWidget *			group_parameters_label_widget;
	GtkWidget *			group_parameters_label;
	GtkWidget *			group_parameters_vbox;
	GtkWidget *			instanciate_button;
	GtkWidget *			widget;
	GtkWidget *			depth_hbox;

	GeoXmlSequence *		i;
	GeoXmlParameterGroup *		parameter_group;
	struct group_parameters_data *	data;

	parameter_group = GEOXML_PARAMETER_GROUP(parameter_data->parameter);
	data = g_malloc(sizeof(struct group_parameters_data));
	data->parameters.is_group = TRUE;
	data->parameters.parameters = geoxml_parameter_group_get_parameters(parameter_group);

	group_parameters_expander = gtk_expander_new("");
	gtk_expander_set_expanded(GTK_EXPANDER(group_parameters_expander), expanded);
	g_signal_connect(group_parameters_expander, "destroy",
		GTK_SIGNAL_FUNC(group_parameters_data_free), data);
	gtk_widget_show(group_parameters_expander);
	depth_hbox = gtk_container_add_depth_hbox(group_parameters_expander);

	group_parameters_vbox = gtk_vbox_new(FALSE, 0);
	data->parameters.vbox = group_parameters_vbox;
	gtk_box_pack_start(GTK_BOX(depth_hbox), group_parameters_vbox, TRUE, TRUE, 0);
	gtk_widget_show(group_parameters_vbox);

	group_parameters_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(group_parameters_expander), group_parameters_label_widget);
	gtk_widget_show(group_parameters_label_widget);
	gtk_expander_hacked_define(group_parameters_expander, group_parameters_label_widget);
	group_parameters_label = gtk_label_new(_("Parameters"));
	gtk_widget_show(group_parameters_label);
	gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), group_parameters_label, FALSE, TRUE, 0);

	if (geoxml_parameter_group_get_can_instanciate(parameter_group)) {
		instanciate_button = gtk_button_new();
		gtk_widget_show(instanciate_button);
		g_signal_connect(instanciate_button, "clicked",
			GTK_SIGNAL_FUNC(group_parameters_instanciate), data);
		g_object_set(G_OBJECT(instanciate_button),
			"label", _("Instanciate"),
			"image", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR),
			"user-data", parameter_data,
			"relief", GTK_RELIEF_NONE,
			NULL);
	} else
		instanciate_button = NULL;
	if (geoxml_parameter_group_get_instances(parameter_group) == 1) {
		widget = gtk_button_new_from_stock(GTK_STOCK_ADD);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), widget, FALSE, TRUE, 5);
		g_signal_connect(widget, "clicked",
			GTK_SIGNAL_FUNC(parameters_add), data);
		g_object_set(G_OBJECT(widget),
			"user-data", group_parameters_vbox,
			"relief", GTK_RELIEF_NONE,
			NULL);

		if (instanciate_button != NULL)
			gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), instanciate_button, FALSE, TRUE, 5);
	} else {
		gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), instanciate_button, FALSE, TRUE, 5);

		widget = gtk_button_new();
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(group_parameters_label_widget), widget, FALSE, TRUE, 5);
		g_signal_connect(widget, "clicked",
			GTK_SIGNAL_FUNC(group_parameters_deinstanciate), data);
		g_object_set(G_OBJECT(widget),
			"label", _("Deinstanciate"),
			"image", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR),
			"user-data", parameter_data,
			"relief", GTK_RELIEF_NONE,
			NULL);
	}

	data->group = parameter_group;
	data->radio_group = NULL;
	i = geoxml_parameters_get_first_parameter(data->parameters.parameters);
	while (i != NULL) {
		gtk_box_pack_start(GTK_BOX(group_parameters_vbox),
			parameter_create_ui(GEOXML_PARAMETER(i), (struct parameters_data *)data, expanded), FALSE, TRUE, 0);

		geoxml_sequence_next(&i);
	}

	return group_parameters_expander;
}

void
group_parameters_instanciate(GtkButton * button, struct parameters_data * parameters_data)
{
	struct parameter_data *	parameter_data;

	g_object_get(G_OBJECT(button), "user-data", &parameter_data, NULL);

	geoxml_parameter_group_instanciate(GEOXML_PARAMETER_GROUP(parameter_data->parameter));
	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(parameter_data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(parameter_data->specific_table, parameter_data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
group_parameters_deinstanciate(GtkButton * button, struct parameters_data * parameters_data)
{
	struct parameter_data *	parameter_data;

	g_object_get(G_OBJECT(button), "user-data", &parameter_data, NULL);

	geoxml_parameter_group_deinstanciate(GEOXML_PARAMETER_GROUP(parameter_data->parameter));
	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(parameter_data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(parameter_data->specific_table, parameter_data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Section: Private
 */

static void
group_parameters_data_free(GtkObject * expander, struct group_parameters_data * data)
{
	g_free(data);
}
