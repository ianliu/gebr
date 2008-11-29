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

#ifndef __LIBGEBR_GUI_PARAMETER_IN_GROUP_EDIT_H
#define __LIBGEBR_GUI_PARAMETER_IN_GROUP_EDIT_H

#include <gtk/gtk.h>

#include <geoxml.h>

#include <gui/gtksequenceedit.h>
#include <gui/parameter.h>

G_BEGIN_DECLS

GType
parameter_in_group_edit_get_type(void);

#define TYPE_PARAMETER_IN_GROUP_EDIT		(parameter_in_group_edit_get_type())
#define PARAMETER_IN_GROUP_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PARAMETER_IN_GROUP_EDIT, ParameterInGroupEdit))
#define PARAMETER_IN_GROUP_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PARAMETER_IN_GROUP_EDIT, ParameterInGroupEditClass))
#define IS_PARAMETER_IN_GROUP_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PARAMETER_IN_GROUP_EDIT))
#define IS_PARAMETER_IN_GROUP_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PARAMETER_IN_GROUP_EDIT))
#define PARAMETER_IN_GROUP_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PARAMETER_IN_GROUP_EDIT, ParameterInGroupEditClass))

typedef struct _ParameterInGroupEdit		ParameterInGroupEdit;
typedef struct _ParameterInGroupEditClass	ParameterInGroupEditClass;

struct _ParameterInGroupEdit {
	GtkSequenceEdit			parent;

	GeoXmlParameterGroup *		parameter_group;
	guint				parameter_index;

	struct parameter_widget *	parameter_widget;
};
struct _ParameterInGroupEditClass {
	GtkSequenceEditClass		parent;

	/* signals */
	void				(*instances_changed)(GtkSequenceEdit * self);
};

GtkWidget *
parameter_in_group_edit_new(GeoXmlParameterGroup * parameter_group, guint parameter_index);

void
parameter_in_group_edit_load(ParameterInGroupEdit * parameter_in_group_edit);

G_END_DECLS

#endif //__LIBGEBR_GUI_PARAMETER_IN_GROUP_EDIT_H
