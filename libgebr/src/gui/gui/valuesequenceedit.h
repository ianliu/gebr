/*   libgebr - GÍBR Library
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

#ifndef __LIBGEBR_GUI_VALUE_SEQUENCE_EDIT_H
#define __LIBGEBR_GUI_VALUE_SEQUENCE_EDIT_H

#include <gtk/gtk.h>

#include <geoxml.h>

#include "gtksequenceedit.h"

G_BEGIN_DECLS

GType
value_sequence_edit_get_type(void);

#define TYPE_VALUE_SEQUENCE_EDIT		(value_sequence_edit_get_type())
#define VALUE_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_VALUE_SEQUENCE_EDIT, ValueSequenceEdit))
#define VALUE_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_VALUE_SEQUENCE_EDIT, ValueSequenceEditClass))
#define IS_VALUE_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_VALUE_SEQUENCE_EDIT))
#define IS_VALUE_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_VALUE_SEQUENCE_EDIT))
#define VALUE_SEQUENCE_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_VALUE_SEQUENCE_EDIT, ValueSequenceEditClass))

typedef struct _ValueSequenceEdit	ValueSequenceEdit;
typedef struct _ValueSequenceEditClass	ValueSequenceEditClass;

struct _ValueSequenceEdit {
	GtkSequenceEdit		parent;

	GeoXmlValueSequence *	value_sequence;
};
struct _ValueSequenceEditClass {
	GtkSequenceEditClass	parent;
};

GtkWidget *
value_sequence_edit_new(GtkWidget * widget);

GtkWidget *
value_sequence_edit_new_with_sequence(GtkWidget * widget, GeoXmlValueSequence * value_sequence);

void
value_sequence_edit_add(ValueSequenceEdit * value_sequence_edit, GeoXmlValueSequence * value_sequence);

void
value_sequence_edit_load(ValueSequenceEdit * value_sequence_edit);

G_END_DECLS

#endif //__LIBGEBR_GUI_VALUE_SEQUENCE_EDIT_H
