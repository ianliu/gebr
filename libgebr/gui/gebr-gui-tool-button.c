/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include "gebr-gui-tool-button.h"

#include <gdk/gdkkeysyms.h>

struct _GebrGuiToolButtonPriv {
	GtkWidget *popup;
};

G_DEFINE_TYPE(GebrGuiToolButton, gebr_gui_tool_button, GTK_TYPE_TOGGLE_BUTTON);

static void
gebr_gui_tool_button_toggled(GtkToggleButton *toggle)
{
	GebrGuiToolButton *button = GEBR_GUI_TOOL_BUTTON(toggle);
	gboolean active = gtk_toggle_button_get_active(toggle);

	if (!active) {
		if (gtk_widget_get_visible(button->priv->popup)) {
			gtk_widget_hide(button->priv->popup);
			gtk_grab_remove(button->priv->popup);
		}
		return;
	}

	gint x, y;
	GtkAllocation a;
	GtkWidget *widget = GTK_WIDGET(button);

	gtk_widget_get_allocation(widget, &a);

	GdkWindow *window = gtk_widget_get_window(widget);
	gdk_window_get_origin(window, &x, &y);

	gtk_window_move(GTK_WINDOW(button->priv->popup),
			x + a.x,
			y + a.y + a.height);
	gtk_widget_show(button->priv->popup);

	window = gtk_widget_get_window(button->priv->popup);

	gtk_grab_add(button->priv->popup);
	gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME);
	gdk_pointer_grab(window, TRUE, GDK_BUTTON_PRESS_MASK,
			 NULL, NULL, GDK_CURRENT_TIME);
}

static gboolean
popup_button_press(GtkWidget         *popup,
		   GdkEventButton    *event,
		   GebrGuiToolButton *button)
{
	GdkWindow *window = gtk_widget_get_window(popup);
	gint xroot, yroot, width, height;
	gdk_window_get_position(window, &xroot, &yroot);
	gdk_drawable_get_size(GDK_DRAWABLE(window), &width, &height);

	if (event->x_root < xroot || event->x_root > xroot + width
	    || event->y_root < yroot || event->y_root > yroot + height)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

	return TRUE;
}

static gboolean
popup_key_press(GtkWidget         *popup,
		GdkEventKey       *event,
		GebrGuiToolButton *button)
{
	if (event->keyval == GDK_Escape)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		return TRUE;
	}

	return FALSE;
}

static void
popup_grab_notify(GtkWidget *popup,
		  gboolean was_grabbed)
{
	if (was_grabbed) {
		GdkWindow *window = gtk_widget_get_window(popup);
		gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME);
		gdk_pointer_grab(window, TRUE, GDK_BUTTON_PRESS_MASK,
				 NULL, NULL, GDK_CURRENT_TIME);
	}
}

/* Class & Instance initialization {{{1 */
static void
gebr_gui_tool_button_init(GebrGuiToolButton *button)
{
	button->priv = G_TYPE_INSTANCE_GET_PRIVATE(button,
						   GEBR_GUI_TYPE_TOOL_BUTTON,
						   GebrGuiToolButtonPriv);

	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_widget_show(frame);

	button->priv->popup = gtk_window_new(GTK_WINDOW_POPUP);
	g_signal_connect(button->priv->popup, "button-press-event",
			 G_CALLBACK(popup_button_press), button);
	g_signal_connect(button->priv->popup, "key-press-event",
			 G_CALLBACK(popup_key_press), button);
	g_signal_connect(button->priv->popup, "grab-notify",
			 G_CALLBACK(popup_grab_notify), button);
	gtk_window_set_resizable(GTK_WINDOW(button->priv->popup), FALSE);
	gtk_container_add(GTK_CONTAINER(button->priv->popup), frame);
}

static void
gebr_gui_tool_button_class_init(GebrGuiToolButtonClass *klass)
{
	GtkToggleButtonClass *toggle_class = GTK_TOGGLE_BUTTON_CLASS(klass);
	toggle_class->toggled = gebr_gui_tool_button_toggled;
	g_type_class_add_private(klass, sizeof(GebrGuiToolButtonPriv));
}

/* Public methods {{{1 */
GtkWidget *
gebr_gui_tool_button_new(void)
{
	return g_object_new(GEBR_GUI_TYPE_TOOL_BUTTON, NULL);
}

void
gebr_gui_tool_button_add(GebrGuiToolButton *button,
			 GtkWidget *widget)
{
	GtkWidget *child;

	g_return_if_fail(GEBR_GUI_IS_TOOL_BUTTON(button));

	child = gtk_bin_get_child(GTK_BIN(button->priv->popup));
	gtk_container_add(GTK_CONTAINER(child), widget);
}
