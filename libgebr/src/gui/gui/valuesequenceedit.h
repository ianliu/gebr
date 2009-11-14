/*   libgebr - GeBR Library
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

#ifndef __GEBR_GUI_VALUE_SEQUENCE_EDIT_H
#define __GEBR_GUI_VALUE_SEQUENCE_EDIT_H

#include <gtk/gtk.h>

#include <geoxml.h>

#include "gtksequenceedit.h"

G_BEGIN_DECLS

GType
gebr_gui_value_sequence_edit_get_type(void);

#define TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT		(gebr_gui_value_sequence_edit_get_type())
#define GEBR_GUI_VALUE_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT, GebrGuiValueSequenceEdit))
#define GEBR_GUI_VALUE_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT, GebrGuiValueSequenceEditClass))
#define IS_GEBR_GUI_VALUE_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT))
#define IS_GEBR_GUI_VALUE_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT))
#define GEBR_GUI_VALUE_SEQUENCE_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GEBR_GUI_VALUE_SEQUENCE_EDIT, GebrGuiValueSequenceEditClass))

typedef struct _GebrGuiValueSequenceEdit	GebrGuiValueSequenceEdit;
typedef struct _GebrGuiValueSequenceEditClass	GebrGuiValueSequenceEditClass;

typedef void (*ValueSequenceSetFunction)(GebrGeoXmlSequence *, const gchar *, gpointer);
typedef const gchar * (*ValueSequenceGetFunction)(GebrGeoXmlSequence *, gpointer);

struct _GebrGuiValueSequenceEdit {
	GtkSequenceEdit			parent;

	gboolean			minimum_one;
	ValueSequenceSetFunction	set_function;
	ValueSequenceGetFunction	get_function;
	gpointer			user_data;
};
struct _GebrGuiValueSequenceEditClass {
	GtkSequenceEditClass		parent;
};

GtkWidget *
gebr_gui_value_sequence_edit_new(GtkWidget * widget);

void
gebr_gui_value_sequence_edit_add(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GebrGeoXmlSequence * sequence);

void
gebr_gui_value_sequence_edit_load(GebrGuiValueSequenceEdit * gebr_gui_value_sequence_edit, GebrGeoXmlSequence * sequence,
	ValueSequenceSetFunction set_function, ValueSequenceGetFunction get_function, gpointer user_data);

G_END_DECLS

#endif //__GEBR_GUI_VALUE_SEQUENCE_EDIT_H
