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

#ifndef __CATEGORY_EDIT_H
#define __CATEGORY_EDIT_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>
#include <libgebr/gui/gtksequenceedit.h>

G_BEGIN_DECLS

GType category_edit_get_type(void);

#define TYPE_CATEGORY_EDIT		(category_edit_get_type())
#define CATEGORY_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CATEGORY_EDIT, CategoryEdit))
#define CATEGORY_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CATEGORY_EDIT, CategoryEditClass))
#define IS_CATEGORY_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CATEGORY_EDIT))
#define IS_CATEGORY_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CATEGORY_EDIT))
#define CATEGORY_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CATEGORY_EDIT, CategoryEditClass))

typedef struct _CategoryEdit CategoryEdit;
typedef struct _CategoryEditClass CategoryEditClass;

struct _CategoryEdit {
	GtkSequenceEdit parent;

	GtkWidget *validate_image;
	GebrGeoXmlCategory *category;
	GebrGeoXmlFlow *menu;
};
struct _CategoryEditClass {
	GtkSequenceEditClass parent;
};

/**
 * Creates a new CategoryEdit widget and returns it.
 */
GtkWidget *category_edit_new(GebrGeoXmlFlow * menu, gboolean new_menu);

G_END_DECLS
#endif //__CATEGORY_EDIT_H
