/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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

#include "../intl.h"

#include "gebr-gui-enhanced-entry.h"

/*
 * Prototypes
 */

static void __gebr_gui_gtk_enhanced_entry_text_changed(GtkEntry * entry, GebrGuiGtkEnhancedEntry * enhanced_entry);

static gboolean
__gebr_gui_gtk_enhanced_entry_focus_in(GtkEntry * entry, GdkEventFocus * event,
				       GebrGuiGtkEnhancedEntry * enhanced_entry);

static gboolean
__gebr_gui_gtk_enhanced_entry_focus_out(GtkEntry * widget, GdkEventFocus * event,
					GebrGuiGtkEnhancedEntry * enhanced_entry);

/*
 * gobject stuff
 */

enum {
	EMPTY_TEXT = 1,
	LAST_PROPERTY
};

static void
gebr_gui_gtk_enhanced_entry_set_property(GebrGuiGtkEnhancedEntry * enhanced_entry, guint property_id,
					 const GValue * value, GParamSpec * param_spec)
{
	switch (property_id) {
	case EMPTY_TEXT:
		gebr_gui_gtk_enhanced_entry_set_empty_text(enhanced_entry, g_value_get_pointer(value));
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(enhanced_entry, property_id, param_spec);
		break;
	}
}

static void
gebr_gui_gtk_enhanced_entry_get_property(GebrGuiGtkEnhancedEntry * enhanced_entry, guint property_id, GValue * value,
					 GParamSpec * param_spec)
{
	switch (property_id) {
	case EMPTY_TEXT:
		g_value_set_pointer(value, (gchar *) gebr_gui_gtk_enhanced_entry_get_empty_text(enhanced_entry));
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(enhanced_entry, property_id, param_spec);
		break;
	}
}

static void gebr_gui_gtk_enhanced_entry_destroy(GtkObject *object)
{
	GebrGuiGtkEnhancedEntry *enhanced_entry = GEBR_GUI_GTK_ENHANCED_ENTRY(object);
	if (enhanced_entry->empty_text != NULL) {
		g_free(enhanced_entry->empty_text);
		enhanced_entry->empty_text = NULL;
	}
}

static void gebr_gui_gtk_enhanced_entry_class_init(GebrGuiGtkEnhancedEntryClass * klass)
{
	GParamSpec *param_spec;

	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) gebr_gui_gtk_enhanced_entry_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) gebr_gui_gtk_enhanced_entry_get_property;
	GtkObjectClass *gtkobject_class;
	gtkobject_class = GTK_OBJECT_CLASS(klass);
	gtkobject_class->destroy = gebr_gui_gtk_enhanced_entry_destroy;

	param_spec = g_param_spec_pointer("empty-text",
					  "Empty text", "Text to show when there is no user text in it",
					  (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, EMPTY_TEXT, param_spec);
}

static void gebr_gui_gtk_enhanced_entry_init(GebrGuiGtkEnhancedEntry * enhanced_entry)
{
//      gdk_window_set_events(GTK_WIDGET(enhanced_entry)->window,
//              GDK_FOCUS_CHANGE_MASK | gdk_window_get_events(GTK_WIDGET(enhanced_entry)->window));
	enhanced_entry->empty = TRUE;

	g_signal_connect(GTK_ENTRY(enhanced_entry), "changed",
			 G_CALLBACK(__gebr_gui_gtk_enhanced_entry_text_changed), enhanced_entry);
	g_signal_connect(GTK_ENTRY(enhanced_entry), "focus-in-event",
			 G_CALLBACK(__gebr_gui_gtk_enhanced_entry_focus_in), enhanced_entry);
	g_signal_connect(GTK_ENTRY(enhanced_entry), "focus-out-event",
			 G_CALLBACK(__gebr_gui_gtk_enhanced_entry_focus_out), enhanced_entry);
}

G_DEFINE_TYPE(GebrGuiGtkEnhancedEntry, gebr_gui_gtk_enhanced_entry, GTK_TYPE_ENTRY);

/*
 * Internal functions
 */

static void __gebr_gui_gtk_enhanced_entry_text_changed(GtkEntry * entry, GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	enhanced_entry->empty = (gboolean) ! strlen(gtk_entry_get_text(entry));
}

static void __gebr_gui_gtk_enhanced_entry_check_empty(GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	if (enhanced_entry->empty && !GTK_WIDGET_HAS_FOCUS(enhanced_entry) && enhanced_entry->empty_text != NULL) {
		GtkEntry *entry = GTK_ENTRY(enhanced_entry);

		gtk_widget_modify_text(GTK_WIDGET(entry), GTK_STATE_NORMAL, &(GdkColor) {
				       0xFFFF, 200, 200, 200}
		);
		gtk_widget_modify_text(GTK_WIDGET(entry), GTK_STATE_ACTIVE, &(GdkColor) {
				       0xFFFF, 200, 200, 200}
		);

		g_signal_handlers_block_matched(G_OBJECT(enhanced_entry),
						G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
						G_CALLBACK(__gebr_gui_gtk_enhanced_entry_text_changed), NULL);
		gtk_entry_set_text(entry, enhanced_entry->empty_text);
		g_signal_handlers_unblock_matched(G_OBJECT(enhanced_entry),
						  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
						  G_CALLBACK(__gebr_gui_gtk_enhanced_entry_text_changed), NULL);
	}
}

static gboolean
__gebr_gui_gtk_enhanced_entry_focus_in(GtkEntry * entry, GdkEventFocus * event,
				       GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	gtk_widget_modify_text(GTK_WIDGET(entry), GTK_STATE_NORMAL, &(GdkColor) {
			       0xFFFF, 255, 255, 255});
	gtk_widget_modify_text(GTK_WIDGET(entry), GTK_STATE_ACTIVE, &(GdkColor) {
			       0xFFFF, 255, 255, 255});

	if (enhanced_entry->empty) {
		g_signal_handlers_block_matched(G_OBJECT(entry),
						G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
						G_CALLBACK(__gebr_gui_gtk_enhanced_entry_text_changed), NULL);
		gtk_entry_set_text(entry, "");
		g_signal_handlers_unblock_matched(G_OBJECT(entry),
						  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
						  G_CALLBACK(__gebr_gui_gtk_enhanced_entry_text_changed), NULL);
	}

	return FALSE;
}

static gboolean
__gebr_gui_gtk_enhanced_entry_focus_out(GtkEntry * entry, GdkEventFocus * event,
					GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	__gebr_gui_gtk_enhanced_entry_check_empty(enhanced_entry);

	return FALSE;
}

/*
 * Library functions
 */

GtkWidget *gebr_gui_gtk_enhanced_entry_new()
{
	return g_object_new(GEBR_GUI_GTK_TYPE_ENHANCED_ENTRY, "empty-text", NULL, NULL);
}

GtkWidget *gebr_gui_gtk_enhanced_entry_new_with_empty_text(const gchar * empty_text)
{
	return g_object_new(GEBR_GUI_GTK_TYPE_ENHANCED_ENTRY, "empty-text", empty_text, NULL);
}

void gebr_gui_gtk_enhanced_entry_set_text(GebrGuiGtkEnhancedEntry * enhanced_entry, const gchar * text)
{
	gtk_entry_set_text(GTK_ENTRY(enhanced_entry), text);
	__gebr_gui_gtk_enhanced_entry_check_empty(enhanced_entry);
}

const gchar *gebr_gui_gtk_enhanced_entry_get_text(GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	return enhanced_entry->empty == TRUE ? "" : gtk_entry_get_text(GTK_ENTRY(enhanced_entry));
}

void gebr_gui_gtk_enhanced_entry_set_empty_text(GebrGuiGtkEnhancedEntry * enhanced_entry, const gchar * empty_text)
{
	if (enhanced_entry->empty_text != NULL)
		g_free(enhanced_entry->empty_text);
	enhanced_entry->empty_text = (empty_text != NULL) ? strdup(empty_text) : NULL;
	__gebr_gui_gtk_enhanced_entry_focus_out(GTK_ENTRY(enhanced_entry), NULL, enhanced_entry);
}

const gchar *gebr_gui_gtk_enhanced_entry_get_empty_text(GebrGuiGtkEnhancedEntry * enhanced_entry)
{
	return enhanced_entry->empty_text;
}
