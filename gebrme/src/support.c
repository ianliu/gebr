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

#include "support.h"

GtkWidget *
create_depth(GtkWidget * container)
{
	GtkWidget *	depth_hbox;
	GtkWidget *	depth_widget;

	depth_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), depth_hbox);
	gtk_widget_show(depth_hbox);
	depth_widget = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(depth_hbox), depth_widget, FALSE, TRUE, 0);
	gtk_widget_set_size_request(depth_widget, 25, -1);
	gtk_widget_show(depth_widget);

	return depth_hbox;
}

void
gtk_expander_hacked_visible(GtkWidget * expander, GtkWidget * label_widget)
{
	g_signal_handlers_unblock_matched(G_OBJECT(label_widget),
					G_SIGNAL_MATCH_FUNC,
					0, 0, NULL,
					(GCallback)gtk_expander_hacked_idle,
					NULL);
}

gboolean
gtk_expander_hacked_idle(GtkWidget * label_widget, GdkEventExpose *event, GtkWidget * expander)
{
	g_signal_handlers_block_matched(G_OBJECT(label_widget),
					G_SIGNAL_MATCH_FUNC,
					0, 0, NULL,
					(GCallback)gtk_expander_hacked_idle,
					NULL);
	g_object_ref (G_OBJECT (label_widget));
	gtk_expander_set_label_widget (GTK_EXPANDER (expander), NULL);
	gtk_expander_set_label_widget (GTK_EXPANDER (expander), label_widget);
	g_object_unref (G_OBJECT (label_widget));

	return TRUE;
}
