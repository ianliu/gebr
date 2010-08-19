/*   libgebr - GêBR Library
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <unistd.h>

#include <libgebr/intl.h>
#include <libgebr/gui.h>

#include <gebr.h>
#include <../defines.h>
#include <interface.h>

#if GTK_CHECK_VERSION(2,14,0)
void _gtk_loop()
{
	while (gtk_events_pending()) {
		gtk_main_iteration();
	}
}
void gtk_loop()
{
	_gtk_loop();
	sleep(0);
}


static void test_gebr_help_about(void)
{
	GtkWidget *about = gebr_gui_about_setup_ui("gebr", NULL).dialog;
	gtk_widget_show(about);
	gtk_loop();
	GtkWidget *ok = gtk_test_find_widget (about, "Close", GTK_TYPE_BUTTON);
	gboolean hit = gtk_test_widget_send_key (ok, GDK_Return, 0);
	g_assert (hit == TRUE);
	gtk_loop();
	gtk_widget_show(about);
	gtk_loop();
	gboolean click = gtk_test_widget_click(ok, 1, 0);
	g_assert (click == TRUE);
	gtk_loop();
}

int main(int argc, char *argv[])
{
	/* initialization */
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebr/help/about", test_gebr_help_about);
	/* FIXME: this test is not passing anymore
	g_test_add_func("/gebr/help/ckeditor/confirm-save", test_gebr_help_ckeditor_confirm_save);
	*/
	return g_test_run();

}
#else
int main(int argc, char *argv[])
{
	return 0;
}
#endif
