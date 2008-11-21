/*   DeBR - GeBR Designer
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

#include <gtk/gtk.h>

#include <gui/utils.h>

#include "parametergroup.h"
#include "debr.h"
#include "support.h"
#include "parameter.h"

/*
 * File: parametergroup.c
 * Create dialog for editing a parameter of type group
 */

/*
 * Declarations
 */



/*
 * Section: Public
 */

/*
 * Function: parameter_group_setup_ui
 * Open a dialog to configure a group
 */
void
parameter_group_dialog_setup_ui(void)
{
	GtkWidget *		dialog;
	GtkWidget *		table;

	GtkWidget *		label_label;
	GtkWidget *		label_entry;
	GtkWidget *		instanciable_label;
	GtkWidget *		instanciable_check_button;
	GtkWidget *		instances_label;
	GtkWidget *		instances_spin;
	GtkWidget *		exclusive_label;
	GtkWidget *		exclusive_check_button;
	GtkWidget *		expanded_label;
	GtkWidget *		expanded_check_button;

	GtkTreeIter		iter, parent_iter;

	GeoXmlParameterGroup *	parameter_group;

	if (gtk_tree_store_is_ancestor(debr.ui_parameter.tree_store, &iter, NULL) == FALSE) {
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_parameter.tree_store),
			&iter, &parent_iter);
		iter = parent_iter;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter,
		PARAMETER_XMLPOINTER, &parameter_group,
		-1);

	dialog = gtk_dialog_new_with_buttons(_("Edit group"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 300);

	table = gtk_table_new(10, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/*
	 * Label
	 */
	label_label = gtk_label_new(_("Label:"));
	gtk_widget_show(label_label);
	gtk_table_attach(GTK_TABLE(table), label_label, 0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);

	label_entry = gtk_entry_new();
	gtk_widget_show(label_entry);
	gtk_table_attach(GTK_TABLE(table), label_entry, 0, 1, 1, 2,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	/*
	 * Instanciable
	 */
	instanciable_label = gtk_label_new(_("Instanciable:"));
	gtk_widget_show(instanciable_label);
	gtk_table_attach(GTK_TABLE(table), instanciable_label, 0, 1, 1, 2,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instanciable_label), 0, 0.5);

	instanciable_check_button = gtk_check_button_new();
	gtk_widget_show(instanciable_check_button);
	gtk_table_attach(GTK_TABLE(table), instanciable_check_button, 1, 2, 1, 2,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	/*
	 * Instances
	 */
	instances_label = gtk_label_new(_("Instances:"));
	gtk_widget_show(instances_label);
	gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

	instances_spin = gtk_spin_button_new_with_range(1, 999999999, 1);
	gtk_widget_show(instances_spin);
	gtk_table_attach(GTK_TABLE(table), instances_spin, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	/*
	 * Exclusive
	 */
	exclusive_label = gtk_label_new(_("Exclusive:"));
	gtk_widget_show(exclusive_label);
	gtk_table_attach(GTK_TABLE(table), exclusive_label, 0, 1, 3, 4,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(exclusive_label), 0, 0.5);

	exclusive_check_button = gtk_check_button_new();
	gtk_widget_show(exclusive_check_button);
	gtk_table_attach(GTK_TABLE(table), exclusive_check_button, 1, 2, 3, 4,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	/*
	 * Expanded by default
	 */
	expanded_label = gtk_label_new(_("Expanded by default:"));
	gtk_widget_show(expanded_label);
	gtk_table_attach(GTK_TABLE(table), expanded_label, 0, 1, 5, 6,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(expanded_label), 0, 0.5);

	expanded_check_button = gtk_check_button_new();
	gtk_widget_show(expanded_check_button);
	gtk_table_attach(GTK_TABLE(table), expanded_check_button, 1, 2, 5, 6,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	/* group data -> UI */
	geoxml_parameter_set_label(debr.parameter, gtk_entry_get_text(GTK_ENTRY(label_entry)));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(instanciable_check_button),
		geoxml_parameter_group_get_is_instanciable(parameter_group));
	gtk_widget_set_sensitive(instances_spin, geoxml_parameter_group_get_is_instanciable(parameter_group));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin),
		geoxml_parameter_group_get_instances_number(parameter_group));
// 	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button),
// 		geoxml_parameter_group_get_exclusive(parameter_group) != NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expanded_check_button),
		geoxml_parameter_group_get_expand(parameter_group));

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	parameter_load_selected();
}

/*
 * Section: Private
 */

