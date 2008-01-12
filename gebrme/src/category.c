/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>
#include <gui/valuesequenceedit.h>

#include "category.h"
#include "gebrme.h"
#include "support.h"
#include "menu.h"

void
category_add(ValueSequenceEdit * sequence_edit)
{
	gchar *	name;

	name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gebrme.categories_combo));
	if (!strlen(name))
		name = _("New category");
	value_sequence_edit_add(VALUE_SEQUENCE_EDIT(sequence_edit),
		GEOXML_VALUE_SEQUENCE(geoxml_flow_append_category(gebrme.current, name)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
category_changed(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
