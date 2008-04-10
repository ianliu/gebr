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

#include "parameters.h"
#include "support.h"
#include "parameter.h"
#include "menu.h"

/*
 * Internal stuff
 */
void
parameters_data_free(GtkObject * expander, struct parameters_data * data)
{
	g_free(data);
}

/*
 * Public functions
 */
GtkWidget *
parameters_create_ui(GeoXmlParameters * parameters, gboolean expanded)
{
	GtkWidget *			parameters_expander;
	GtkWidget *			parameters_label_widget;
	GtkWidget *			parameters_label;
	GtkWidget *			parameters_vbox;
	GtkWidget *			widget;
	GtkWidget *			depth_hbox;

	GeoXmlSequence *		i;
	struct parameters_data *	data;

	data = g_malloc(sizeof(struct parameters_data));
	*data = (struct parameters_data) {
		.parameters = parameters,
		.is_group = FALSE,
	};

	parameters_expander = gtk_expander_new("");
	gtk_expander_set_expanded(GTK_EXPANDER(parameters_expander), expanded);
	g_signal_connect(parameters_expander, "destroy",
		GTK_SIGNAL_FUNC(parameters_data_free), data);
	gtk_widget_show(parameters_expander);
	depth_hbox = gtk_container_add_depth_hbox(parameters_expander);

	parameters_vbox = gtk_vbox_new(FALSE, 0);
	data->vbox = parameters_vbox;
	gtk_box_pack_start(GTK_BOX(depth_hbox), parameters_vbox, TRUE, TRUE, 0);
	gtk_widget_show(parameters_vbox);

	parameters_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(parameters_expander), parameters_label_widget);
	gtk_widget_show(parameters_label_widget);
	gtk_expander_hacked_define(parameters_expander, parameters_label_widget);
	parameters_label = gtk_label_new(_("Parameters"));
	gtk_widget_show(parameters_label);
	gtk_box_pack_start(GTK_BOX(parameters_label_widget), parameters_label, FALSE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(parameters_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect(widget, "clicked",
		GTK_SIGNAL_FUNC(parameters_add), data);
	g_object_set(G_OBJECT(widget), "relief", GTK_RELIEF_NONE, NULL);

	i = geoxml_parameters_get_first_parameter(parameters);
	while (i != NULL) {
		gtk_box_pack_start(GTK_BOX(parameters_vbox),
			parameter_create_ui(GEOXML_PARAMETER(i), data, expanded), FALSE, TRUE, 0);

		geoxml_sequence_next(&i);
	}

	return parameters_expander;
}

void
parameters_add(GtkButton * button, struct parameters_data * parameters_data)
{
	GtkWidget *		parameter_widget;

	GeoXmlParameter *	parameter;

	parameter = geoxml_parameters_append_parameter(parameters_data->parameters, GEOXML_PARAMETERTYPE_STRING);
	parameter_widget = parameter_create_ui(parameter, parameters_data, FALSE);
	gtk_box_pack_start(GTK_BOX(parameters_data->vbox), parameter_widget, FALSE, TRUE, 0);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
