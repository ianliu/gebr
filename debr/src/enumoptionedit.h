/*   DeBR - GeBR Designer
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

#ifndef __ENUM_OPTION_EDIT_H
#define __ENUM_OPTION_EDIT_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>
#include <libgebr/gui/gtksequenceedit.h>

G_BEGIN_DECLS GType enum_option_edit_get_type(void);

#define TYPE_ENUM_OPTION_EDIT			(enum_option_edit_get_type())
#define ENUM_OPTION_EDIT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ENUM_OPTION_EDIT, EnumOptionEdit))
#define ENUM_OPTION_EDIT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_ENUM_OPTION_EDIT, EnumOptionEditClass))
#define IS_ENUM_OPTION_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ENUM_OPTION_EDIT))
#define IS_ENUM_OPTION_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_ENUM_OPTION_EDIT))
#define ENUM_OPTION_EDIT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ENUM_OPTION_EDIT, EnumOptionEditClass))

typedef struct _EnumOptionEdit EnumOptionEdit;
typedef struct _EnumOptionEditClass EnumOptionEditClass;

struct _EnumOptionEdit {
	GtkSequenceEdit parent;

	GebrGeoXmlEnumOption *enum_option;
	GebrGeoXmlProgramParameter *program_parameter;
	GtkWidget *label_entry;
	GtkWidget *value_entry;
};
struct _EnumOptionEditClass {
	GtkSequenceEditClass parent;
};

GtkWidget *enum_option_edit_new(GebrGeoXmlEnumOption * enum_option, GebrGeoXmlProgramParameter * program_parameter);

#endif				//__ENUM_OPTION_EDIT_H
