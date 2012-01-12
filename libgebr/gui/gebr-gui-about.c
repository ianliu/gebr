/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#include "../libgebr-gettext.h"

#include <glib/gi18n-lib.h>
#include <libgebr/gebr-version.h>

#include "gebr-gui-about.h"
#include "gebr-gui-pixmaps.h"

#if !GTK_CHECK_VERSION(2,20,0)
void __dummy__(GtkAboutDialog * about, const gchar * link, gpointer data)
{
	// Esta funcao eh vazia pois o GtkLinkButton interno
	// do GtkAboutDialog ja chama o link. Entretanto, ele soh
	// eh configurado quando esta funcao eh "instalada"
	// atravez do gtk_about_dialog_set_url_hook. Veja abaixo.
}
#endif

struct about gebr_gui_about_setup_ui(const gchar * program, const gchar * description)
{
	struct about about;
	const gchar *authors[] = {
		_("GêBR Core Team:"),
		" ",
		_("Coordinator"),
		"  Ricardo Biloti <biloti@gebrproject.com>",
                "  Ph.D. Applied Mathematics",
                "  State University of Campinas, Brazil",
		" ",
                _("Development manager"),
		"  Fabrício Matheus Gonçalves <fmatheus@gebrproject.com>",
                "  B.Sc. Computer Engineering",
                " ",
		_("Developers"),
		"  Eric Keiji <keiji.eric@gebrproject.com>",
		"  Giuliano Roberto Pinheiro <giuliano@gebrproject.com>",
		"  Ian Liu Rodrigues <ian.liu@gebrproject.com>",
		"  Igor Henrique Soares Nunes <igor.snunes@gebrproject.com>",
		"  Jorge Pizzolatto <jorge.pzt@gebrproject.com>",
		"  Júlia Koury Marques <julia@gebrproject.com>",
		"  Vinícius Oliveira Querência <querencia@gebrproject.com>",
		" ",
		_("Consultants"),
		"  Bráulio Oliveira",
		"  Eduardo Filpo, Petrobras",
		"  Fernando Roxo, Prosismo",
		"  Luis Alberto D'Afonseca, CGG",
		"  Rodrigo Portugal, Schlumberger",
                " ",
                _("Former members"),
		"  Alexandre Baaklini",
		"  Davi Clemente",
		"  Fábio Azevedo",
		"  Gabriel Sobral",
		"  Priscila Moraes",
		"  Renan William Giarola Miranda",               
		"  Rodrigo Morelatto",
		NULL
	};

#if !GTK_CHECK_VERSION(2,20,0)
	// Instala a funcao que nada faz. Este bug ja foi
	// corrigido no trunk do Gtk+, eu acredito.
	gtk_about_dialog_set_url_hook(__dummy__, NULL, NULL);
#endif

	about.dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about.dialog), program);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about.dialog), gebr_version());
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about.dialog), gebr_gui_pixmaps_gebr_logo());
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about.dialog), _("GêBR Core Team"));

	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about.dialog),
				     _("Copyright (C) 2007-2011 GêBR core team (http://www.gebrproject.com/)\n" "\n"
				       "This program is free software: you can redistribute it and/or modify "
				       "it under the terms of the GNU General Public License as published by "
				       "the Free Software Foundation, either version 3 of the License, or "
				       "(at your option) any later version.\n" "\n"
				       "This program is distributed in the hope that it will be useful, "
				       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
				       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
				       "GNU General Public License for more details.\n" "\n"
				       "You should have received a copy of the GNU General Public License "
				       "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n" "\n"
				       "In Brazil, this program is under protection against unauthorized usage, "
				       "in accordance to brazilian laws #9609, Feb 19, 1998, #2556, "
				       "Apr 20, 1998, and #9610, Feb 19, 1998, and is registered in "
				       "Instituto Nacional da Propriedade Industrial (INPI) under number 70156."));
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about.dialog), TRUE);

	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about.dialog), "http://www.gebrproject.com/");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about.dialog), _("GêBR Home page"));
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about.dialog), authors);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about.dialog), description);

	g_signal_connect (about.dialog, "close",
			  G_CALLBACK (gtk_widget_hide), about.dialog);

	g_signal_connect (about.dialog, "response",
			  G_CALLBACK (gtk_widget_hide), about.dialog);

	g_signal_connect(about.dialog, "delete-event",
			 G_CALLBACK (gtk_widget_hide_on_delete), NULL);

	return about;
}
