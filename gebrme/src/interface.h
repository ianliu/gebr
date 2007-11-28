/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#ifndef __INTERFACE_H
#define __INTERFACE_H

GtkWidget *
create_gebrme_window (void);

typedef struct {
	GtkWidget *	hbox;

	GtkWidget *	entry;
	GtkWidget *	browse_button;

	gpointer 	user_data;
} gebr_file_selector_t;

gebr_file_selector_t
create_file_selector_widget(void);

#define gtk_expander_hacked_define(expander, label_widget)			\
	g_signal_connect_after ((gpointer) label_widget, "expose-event",	\
			G_CALLBACK (gtk_expander_hacked_idle),			\
			expander);						\
	g_signal_connect((gpointer) expander, "unmap",				\
			G_CALLBACK (gtk_expander_hacked_visible),		\
			label_widget)

void
gtk_expander_hacked_visible(GtkWidget * parent_expander, GtkWidget * hbox);

gboolean
gtk_expander_hacked_idle(GtkWidget * hbox, GdkEventExpose *event, GtkWidget * expander);

#endif //__INTERFACE_H
