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

#ifndef __GEBR_GUI_ENHANCED_ENTRY_H
#define __GEBR_GUI_ENHANCED_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType gebr_gui_enhanced_entry_get_type(void);

#define GEBR_GUI_TYPE_ENHANCED_ENTRY		(gebr_gui_enhanced_entry_get_type())
#define GEBR_GUI_ENHANCED_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_ENHANCED_ENTRY, GebrGuiEnhancedEntry))
#define GEBR_GUI_ENHANCED_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_ENHANCED_ENTRY, GebrGuiEnhancedEntryClass))
#define GEBR_GUI_IS_ENHANCED_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_ENHANCED_ENTRY))
#define GEBR_GUI_IS_ENHANCED_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_ENHANCED_ENTRY))
#define GEBR_GUI_ENHANCED_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_ENHANCED_ENTRY, GebrGuiEnhancedEntryClass))

typedef struct _GebrGuiEnhancedEntry GebrGuiEnhancedEntry;
typedef struct _GebrGuiEnhancedEntryClass GebrGuiEnhancedEntryClass;

struct _GebrGuiEnhancedEntry {
	GtkEntry parent;

	gboolean empty;
	gchar *empty_text;
};
struct _GebrGuiEnhancedEntryClass {
	GtkEntryClass parent;
};

GtkWidget *gebr_gui_enhanced_entry_new();

GtkWidget *gebr_gui_enhanced_entry_new_with_empty_text(const gchar * empty_text);

void gebr_gui_enhanced_entry_set_text(GebrGuiEnhancedEntry * enhanced_entry, const gchar * text);

const gchar *gebr_gui_enhanced_entry_get_text(GebrGuiEnhancedEntry * enhanced_entry);

void gebr_gui_enhanced_entry_set_empty_text(GebrGuiEnhancedEntry * enhanced_entry, const gchar * empty_text);

const gchar *gebr_gui_enhanced_entry_get_empty_text(GebrGuiEnhancedEntry * enhanced_entry);

G_END_DECLS
#endif				//__GEBR_GUI_ENHANCED_ENTRY_H
