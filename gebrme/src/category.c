/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include <string.h>

#include "category.h"
#include "gebrme.h"
#include "support.h"
#include "menu.h"

void
category_add(void)
{
	GtkTreeIter		iter;
	gchar *			name;

	name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gebrme.categories_combo));
	if (!strlen(name))
		name = _("New category");

	gtk_list_store_append (gebrme.categories_liststore, &iter);
	gtk_list_store_set (gebrme.categories_liststore, &iter,
		CATEGORY_NAME, name,
		CATEGORY_XMLPOINTER, geoxml_flow_append_category(gebrme.current, name),
		-1);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
category_remove(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	category;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebrme.categories_treeview));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gebrme_message(ERROR, TRUE, FALSE, _("Please select a category"));
		return;
	}

	gtk_tree_model_get (GTK_TREE_MODEL(gebrme.categories_liststore), &iter,
		CATEGORY_XMLPOINTER, &category,
		-1);
	geoxml_sequence_remove(category);
	gtk_list_store_remove(gebrme.categories_liststore, &iter);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
category_renamed(	GtkCellRendererText *	cell,
			gchar *			path_string,
			gchar *			new_text,
			gpointer		user_data)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlCategory *	category;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebrme.categories_treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);

	/* get xml pointer */
	gtk_tree_model_get (GTK_TREE_MODEL(gebrme.categories_liststore), &iter,
		CATEGORY_XMLPOINTER, &category,
		-1);

	/* rename it */
	geoxml_category_set_name(category, new_text);
	gtk_list_store_set (gebrme.categories_liststore, &iter,
		CATEGORY_NAME, new_text,
		-1);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
