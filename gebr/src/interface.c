/*   G�BR - An environment for seismic processing.
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

/* File: interface.c
 * Assembly the main components of the interface
 *
 * This function assemblies the main window, preference and about
 * dialogs. All other subcomponents of the interface are implemented
 * in the files initiated by "ui_".
 */
#include <string.h>
#include <glib.h>
#include "interface.h"

#include "gebr.h"
#include "ui_menubar.h"
#include "ui_pages.h"
#include "callbacks.h"
#include "cb_proj.h"

/* Pre-defined browser options */
#define NBROWSER 5
const char* browser[] = { "epiphany",
			  "firefox",
			  "galeon",
			  "konqueror",
			  "mozilla" };

/*------------------------------------------------------------------------*
 * Function: assembly_preference_win
 * Assembly preference window.
 *
 */
void
assembly_preference_win (void)
{
	if (W.pref.win != NULL)
		return;

	GtkWidget *table;
	GtkWidget *label;
	GtkTooltips *tips;
	GtkWidget *eventbox;

	W.pref.win = gtk_dialog_new_with_buttons ("Preferences",
					   GTK_WINDOW(W.pref.win),
					   0,
					   GTK_STOCK_OK,     GTK_RESPONSE_OK,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   NULL);
	/* Tooltips */
	tips = gtk_tooltips_new();

	/* Take the apropriate action when a button is pressed */
	g_signal_connect_swapped (W.pref.win, "response",
				  G_CALLBACK (pref_actions),
				  W.pref.win);

	gtk_widget_set_size_request (W.pref.win, 380, 300);

	table = gtk_table_new (6, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (W.pref.win)->vbox), table, TRUE, TRUE, 0);

	/* User name */
	label = gtk_label_new ("User name");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.pref.username = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, W.pref.username, "You should know your name", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.pref.username, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (W.pref.username_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (W.pref.username), W.pref.username_value->str);

	/* User W.pref.email */
	label = gtk_label_new ("Email");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.pref.email = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, W.pref.email, "Your email address", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.pref.email, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	if (W.pref.email_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (W.pref.email), W.pref.email_value->str);

	/* G�BR dir */
	label = gtk_label_new ("User's menus directory");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	/* Browse button for user's menus dir */
	W.pref.usermenus = gtk_file_chooser_button_new ("GêBR dir",
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), W.pref.usermenus);
	gtk_tooltips_set_tip(tips, eventbox, "Path to look for private user's menus", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (W.pref.usermenus_value->len > 0)
	   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (W.pref.usermenus),
						W.pref.usermenus_value->str);

	/* Data dir */
	label = gtk_label_new ("Data directory");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	/* Browse button for W.pref.data */
	W.pref.data = gtk_file_chooser_button_new ("Browser data dir",
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), W.pref.data);
	gtk_tooltips_set_tip(tips, eventbox, "Path to store projects, lines and flows", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (W.pref.data_value->len > 0)
	   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (W.pref.data),
						W.pref.data_value->str);

	/* Editor */
	label = gtk_label_new ("HTML editor");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.pref.editor = gtk_entry_new ();
	gtk_tooltips_set_tip(tips, W.pref.editor, "An HTML capable editor to edit helps and reports", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), W.pref.editor, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
 	if (W.pref.editor_value->len > 0)
	   gtk_entry_set_text (GTK_ENTRY (W.pref.editor), W.pref.editor_value->str);


	/* Browser */
	label = gtk_label_new ("Browser");
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0);
	W.pref.browser = gtk_combo_box_entry_new_text();
	{
	   int ii;
	   int newbrowser = 1;
	   for (ii=0; ii < NBROWSER; ii++){
	      gtk_combo_box_append_text(GTK_COMBO_BOX(W.pref.browser), browser[ii]);
	      if ( (W.pref.browser_value->len > 0) && newbrowser){
		 if (strcmp(browser[ii], W.pref.browser_value->str)==0){
		    newbrowser = 0;
		    gtk_combo_box_set_active (GTK_COMBO_BOX(W.pref.browser), ii );
		 }
	      }
	   }
	   if ((W.pref.browser_value->len > 0) && newbrowser){
	      gtk_combo_box_append_text(GTK_COMBO_BOX(W.pref.browser), W.pref.browser_value->str);
	      gtk_combo_box_set_active (GTK_COMBO_BOX(W.pref.browser), NBROWSER );
	   }
	}

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), W.pref.browser);
	gtk_tooltips_set_tip(tips, eventbox, "An HTML browser to display helps and reports", "");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach (GTK_TABLE (table), eventbox, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

	gtk_widget_show_all(W.pref.win);
}


/*------------------------------------------------------------------------*
 * Function: assembly_about
 * Assembly the about dialog.
 *
 */
GtkWidget *
assembly_about (void)
{

   const gchar *authors[] = {"GêBR Core Team",
			     "Developers",
                             "  Bráulio Oliveira <brauliobo@gmail.com>",
                             "  Eduardo Filpo <efilpo@gmail.com>",
			     "  Fernando Roxo <roxo@roxo.org>",
			     "  Ricardo Biloti <biloti@gmail.com>",
			     "  Rodrigo Portugal <rosoport@gmail.com>",
			     "",
			     "SU Port Team",
			     "Jesse Costa",
			     "Ellen Costa",
			     "Varios alunos",
			     NULL};

   GtkWidget *about = gtk_about_dialog_new ();
   gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (about), "GêBR");
   gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about), "2.0");
   gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (about),
				   "GêBR Core Team");
   gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (about),
    "Copyright (C) 2007 GêBR Core Team (http://gebr.sourceforge.net/)\n\n"
    "This program is free software; you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation; either version 2, or (at your option) "
    "any later version.\n\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
    "GNU General Public License for more details.\n\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software Foundation, "
    "Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n\n"
    "In Brazil, this program is under protection against unauthorized usage, "
    "in accordance to brazilian laws #9609, Feb 19, 1998, #2556, "
    "Apr 20, 1998, and #9610, Feb 19, 1998, and is registered in "
    "Instituto Nacional da Propriedade Industrial (INPI) under number 70156.");

   gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG (about), TRUE);

   gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (about), "http://sourceforge.net/projects/gebr");
   gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (about), "GêBR Home page");

   gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (about), authors);

   gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (about),
				   "A plug-and-play environment to\nseismic processing tools");

   g_signal_connect (GTK_OBJECT (about), "delete_event",
		     GTK_SIGNAL_FUNC (gtk_widget_hide), NULL );


   return about;
}


/*------------------------------------------------------------------------*
 * Function: assembly_interface
 * Assembly the whole interface.
 *
 */
void
assembly_interface (void)
{
   GtkWidget *	vboxmain;

   /* File Chooser dialog */
   W.filesel = gtk_file_selection_new ("File browse");

   g_signal_connect (GTK_OBJECT (W.filesel), "delete_event",
		     GTK_SIGNAL_FUNC (gtk_widget_hide),
		     NULL);

   g_signal_connect (GTK_FILE_SELECTION (W.filesel)->ok_button, "clicked",
		     GTK_SIGNAL_FUNC (file_browse_ok), NULL);

   g_signal_connect (GTK_FILE_SELECTION (W.filesel)->ok_button, "clicked",
		     GTK_SIGNAL_FUNC (hide_widget), W.filesel);

   g_signal_connect (GTK_FILE_SELECTION (W.filesel)->cancel_button, "clicked",
		     GTK_SIGNAL_FUNC (hide_widget), W.filesel);

   /* Preferences dialog*/
   W.pref.win = NULL;

   /* About dialog */
   W.about = assembly_about ();

   /*
    * Parameters dialog
    * (to be created on the fly)
    */
   W.parameters = NULL;

   /* Create the main window */
   W.mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title (GTK_WINDOW (W.mainwin), "GêBR" );
   gtk_widget_set_size_request (GTK_WIDGET (W.mainwin), 700, 400);

   /* Define the function to be called when the main loops is finished */
   /* (Not implemented yet) */
   /* gtk_quit_add (0, save_and_quit, NULL); */

   /* Create the main vbox to hold menu, notebook and status bar */
   vboxmain = gtk_vbox_new (FALSE, 1);
   gtk_container_add (GTK_CONTAINER (W.mainwin), vboxmain );

   /* Create the main menu */
   {
      GtkWidget *mainmenu;

      mainmenu = gtk_menu_bar_new ();
      gtk_box_pack_start (GTK_BOX (vboxmain), mainmenu, FALSE, FALSE, 0);


      W.menu[MENUBAR_PROJECT] = assembly_projectmenu ();
      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), W.menu[MENUBAR_PROJECT]);

      W.menu[MENUBAR_LINE] = assembly_linemenu ();
      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), W.menu[MENUBAR_LINE]);

      W.menu[MENUBAR_FLOW] = assembly_flowmenu ();
      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), W.menu[MENUBAR_FLOW]);

      W.menu[MENUBAR_FLOW_COMPONENTS] = assembly_flowcomponentsmenu ();
      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), W.menu[MENUBAR_FLOW_COMPONENTS]);

      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), assembly_configmenu ());
      gtk_menu_bar_append (GTK_MENU_BAR (mainmenu), assembly_helpmenu ());
   }

   /* Create a notebook to hold several pages */
   {
      W.notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (vboxmain), W.notebook, TRUE, TRUE, 0);

      g_signal_connect (GTK_OBJECT (W.notebook), "switch-page",
			 GTK_SIGNAL_FUNC (switch_menu), NULL);

      /* Fill in the notebook */
      add_project (GTK_NOTEBOOK (W.notebook));
      add_flow_browse (GTK_NOTEBOOK (W.notebook));
      add_flow_edit (GTK_NOTEBOOK (W.notebook));
      add_job_control (GTK_NOTEBOOK (W.notebook));
   }

   /* Create a status bar */
   W.statusbar = gtk_statusbar_new ();
   gtk_box_pack_end (GTK_BOX (vboxmain), W.statusbar, FALSE, FALSE, 0);


   /* Signals */
   g_signal_connect (GTK_OBJECT (W.mainwin), "delete_event",
		     GTK_SIGNAL_FUNC (gebr_quit),
		     NULL);
   g_signal_connect (GTK_OBJECT (W.mainwin), "show",
		     GTK_SIGNAL_FUNC (gebr_init), NULL);
}

void
save_widget_browse_button_clicked(GtkWidget * button, GtkWidget * entry)
{
	GtkWidget *	chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(	"Choose file", NULL,
							GTK_FILE_CHOOSER_ACTION_SAVE,
							GTK_STOCK_OK, GTK_RESPONSE_OK,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							NULL);

	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
	case GTK_RESPONSE_OK:
		gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

gebr_save_widget_t
save_widget_create(void)
{
	gebr_save_widget_t save_widget;

	save_widget.hbox = gtk_hbox_new(FALSE, 10);

	/* entry */
	save_widget.entry = gtk_entry_new();
// 	gtk_entry_set_editable(GTK_ENTRY(save_widget.entry), FALSE);
	gtk_box_pack_start(GTK_BOX (save_widget.hbox), save_widget.entry, TRUE, TRUE, 0);

	/* browse button */
	save_widget.browse_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX (save_widget.hbox), save_widget.browse_button, FALSE, TRUE, 0);
	g_signal_connect (GTK_OBJECT (save_widget.browse_button), "clicked",
			  G_CALLBACK (save_widget_browse_button_clicked), save_widget.entry);

	return save_widget;
}
