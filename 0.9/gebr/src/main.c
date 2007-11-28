/*   GÍBR - An environment for seismic processing.
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

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "gebr.h"

int
main(int argc, char ** argv, char ** env)
{
	gtk_init (&argc, &argv);

	/* temporary: necessary for representing fractional numbers only with comma */
	setlocale(LC_NUMERIC, "C");

	assembly_interface ();

	/* read command line */
	gebr_config_load(argc, argv);

	gtk_widget_show_all (W.mainwin);

	gtk_main ();

	return EXIT_SUCCESS;
}
