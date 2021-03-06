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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "debr-gettext.h"

#include <locale.h>
#include <gtk/gtk.h>
#include <libgebr/gui/gebr-gui-icons.h>
#include <libgebr/libgebr.h>

#include "debr-interface.h"
#include "debr.h"

int main(int argc, char *argv[])
{
	g_type_init();
	g_thread_init(NULL);

	gebr_libinit(GETTEXT_PACKAGE);
	gebr_geoxml_init();

	gtk_init(&argc, &argv);

	gebr_gui_setup_icons();
	debr_setup_ui();
	gtk_main();

	gebr_geoxml_finalize();

	return 0;
}
