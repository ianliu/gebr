/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include "about.h"
#include "support.h"
#include "pixmaps.h"
#include "../../defines.h"

struct about
about_setup_ui(const gchar * program, const gchar * description)
{
	struct about	about;
	const gchar *	authors[] = {
		_("GêBR Core Team:"),
		_("Developers"),
		"  Bráulio Oliveira <brauliobo@gmail.com>",
		"  Eduardo Filpo <efilpo@gmail.com>",
		"  Fernando Roxo <roxo@roxo.org>",
		"  Ricardo Biloti <biloti@gmail.com>",
		"  Rodrigo Portugal <rosoport@gmail.com>",
		NULL
	};

	about.dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about.dialog), program);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about.dialog), LIBGEBR_VERSION);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about.dialog), pixmaps_gebr_logo());
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about.dialog),
					_("GêBR Core Team"));

	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about.dialog), _(
		"Copyright (C) 2007-2009 GêBR core team (http://sites.google.com/site/gebrproject/)\n"
		"\n"
		"This program is free software: you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation, either version 3 of the License, or "
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License "
		"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
		"\n"
		"In Brazil, this program is under protection against unauthorized usage, "
		"in accordance to brazilian laws #9609, Feb 19, 1998, #2556, "
		"Apr 20, 1998, and #9610, Feb 19, 1998, and is registered in "
		"Instituto Nacional da Propriedade Industrial (INPI) under number 70156."));
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about.dialog), TRUE);

	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about.dialog), "http://sites.google.com/site/gebrproject/");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about.dialog), _("GêBR Home page"));
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about.dialog), authors);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about.dialog),
					description);

        g_signal_connect(GTK_OBJECT(about.dialog), "close",
                         GTK_SIGNAL_FUNC(gtk_widget_hide), GTK_OBJECT(about.dialog));
	g_signal_connect(GTK_OBJECT(about.dialog), "delete-event",
                         GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);
        g_signal_connect_swapped(GTK_OBJECT(about.dialog), "response", 
                                 G_CALLBACK (gtk_widget_hide), GTK_OBJECT(about.dialog));

	return about;
}


