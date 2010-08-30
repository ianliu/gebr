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

#include <locale.h>

#include <gtk/gtk.h>

#include <libgebr/gui/icons.h>
#include <libgebr/libgebr.h>

#include "interface.h"
#include "debr.h"

int main(int argc, char *argv[])
{
	gebr_libinit(GETTEXT_PACKAGE, argv[0]);

	g_thread_init(NULL);
	gtk_init(&argc, &argv);
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	gebr_gui_setup_icons();
	gebr_gui_setup_theme();
	debr_setup_ui();
	gtk_main();

	return 0;
}
