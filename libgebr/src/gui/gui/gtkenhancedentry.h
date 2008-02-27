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

#ifndef __LIBGEBR_GUI_GTK_ENHANCED_ENTRY_H
#define __LIBGEBR_GUI_GTK_ENHANCED_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType
gtk_enhanced_entry_get_type(void);

#define GTK_TYPE_ENHANCED_ENTRY		(gtk_enhanced_entry_get_type())
#define GTK_ENHANCED_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ENHANCED_ENTRY, GtkEnhancedEntry))
#define GTK_ENHANCED_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ENHANCED_ENTRY, GtkEnhancedEntryClass))
#define GTK_IS_ENHANCED_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ENHANCED_ENTRY))
#define GTK_IS_ENHANCED_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ENHANCED_ENTRY))
#define GTK_ENHANCED_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ENHANCED_ENTRY, GtkEnhancedEntryClass))

typedef struct _GtkEnhancedEntry		GtkEnhancedEntry;
typedef struct _GtkEnhancedEntryClass	GtkEnhancedEntryClass;

struct _GtkEnhancedEntry {
	GtkEntry		parent;

	gchar *			empty_text;
};
struct _GtkEnhancedEntryClass {
	GtkEntryClass		parent;
};

GtkWidget *
gtk_enhanced_entry_new();

GtkWidget *
gtk_enhanced_entry_new_with_empty_text(const gchar * empty_text);

const gchar *
gtk_enhanced_entry_get_text(GtkEnhancedEntry * enhanced_entry);

void
gtk_enhanced_entry_set_empty_text(GtkEnhancedEntry * enhanced_entry, const gchar * empty_text);

const gchar *
gtk_enhanced_entry_get_empty_text(GtkEnhancedEntry * enhanced_entry);

G_END_DECLS

#endif //__LIBGEBR_GUI_GTK_ENHANCED_ENTRY_H
