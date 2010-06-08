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

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/gui/icons.h>

#include <gebr.h>
#include <../defines.h>
#include <interface.h>

static void
test_gebr_help_ckeditor_confirm_save(void)
{

}

int main(int argc, char *argv[])
{
	int ret;

	/* initialization */
	gtk_test_init(&argc, &argv, NULL);

	gebr_gui_setup_theme();
	gebr_gui_setup_icons();
	gebr_setup_ui();
	gebr_init();

	gtk_main();

	g_test_add_func("/gebr/help/ckeditor/confirm-save", test_gebr_help_ckeditor_confirm_save);
	ret = g_test_run();

	return ret;
}
