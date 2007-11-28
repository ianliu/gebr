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

/* File: ui_menubar.c
 * Callbacks for the lines manipulation
 */
#include "ui_menubar.h"

#include <stdlib.h>
#include "gebr.h"
#include "interface.h"
#include "callbacks.h"
#include "cb_proj.h"
#include "cb_line.h"
#include "cb_flow.h"
#include "cb_flowcomp.h"
#include "ui_server.h"
#include "menus.h"

/*------------------------------------------------------------------------*
 * Function: assembly_configmenu
 * Assembly the config menu
 *
 */
GtkWidget *
assembly_configmenu (void)
{

   GtkWidget *menuitem;
   GtkWidget *menu;
   GtkWidget *submenu;

   menu = gtk_menu_new ();
   gtk_menu_set_title (GTK_MENU (menu), "Config menu");

   /* Pref entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     G_CALLBACK (assembly_preference_win), NULL);

   /* Server entry */
   submenu =  gtk_image_menu_item_new_with_mnemonic("_Servers");

   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (assembly_server_win),
		     NULL);

   menuitem = gtk_menu_item_new_with_label ("Configure");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

   return menuitem;
}

/*------------------------------------------------------------------------*
 * Function: assembly_helpmenu
 * Assembly the help menu
 *
 */
GtkWidget *
assembly_helpmenu (void)
{

   GtkWidget *menuitem;
   GtkWidget *menu;
   GtkWidget *submenu;

   menu = gtk_menu_new ();
   gtk_menu_set_title (GTK_MENU (menu), "Help menu");

   /* About entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (show_widget),
		     GTK_WIDGET (W.about));

   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);


   menuitem = gtk_menu_item_new_with_label ("Help");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

   gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

   return menuitem;
}

/*------------------------------------------------------------------------*
 * Function: assembly_projectmenu
 * Assembly the menu to the project page
 *
 */
GtkWidget *
assembly_projectmenu (void)
{

   GtkWidget *menuitem;
   GtkWidget *menu;
   GtkWidget *submenu;

   menu = gtk_menu_new ();
   gtk_menu_set_title (GTK_MENU (menu), "Project menu");

   /* New entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (project_new), NULL );

   /* Delete entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (project_delete), NULL );

   /* Refresh entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_REFRESH, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (projects_refresh), NULL );

   menuitem = gtk_menu_item_new_with_label ("Project");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

   gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

   return menuitem;

}

/*------------------------------------------------------------------------*
 * Function: assembly_linemenu
 * Assembly the menu to the line page
 *
 */
GtkWidget *
assembly_linemenu (void)
{

   GtkWidget *menuitem;
   GtkWidget *menu;
   GtkWidget *submenu;

   menu = gtk_menu_new ();
   gtk_menu_set_title (GTK_MENU (menu), "Line menu");

   /* New entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (line_new), NULL );

   /*
    * Add entry
    *
    * In the future, an "add entry" could pop-up a dialog to browse
    * through lines. The user could then import to the current project
    * an existent line. Would that be useful?
    */
   /*
     submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
     gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
   */

   /* Delete entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);

   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (line_delete), NULL );

   /*
    * Properties entry
    *
    * Infos about a line, like title, description, etc.
    */
   /*
     submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
     gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
   */

   menuitem = gtk_menu_item_new_with_label ("Line");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

   gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

   return menuitem;

}

/*------------------------------------------------------------------------*
 * Function: assembly_flowmenu
 * Assembly the menu to the flow page
 *
 */
GtkWidget *
assembly_flowmenu (void)
{

   GtkWidget *menuitem;
   GtkWidget *menu;
   GtkWidget *submenu;

   menu = gtk_menu_new ();
   gtk_menu_set_title (GTK_MENU (menu), "Flow menu");

   /* New entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (flow_new), NULL );

   /* Export entry */
   submenu = gtk_image_menu_item_new_with_label ("Export");
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (flow_export), NULL );


   /* Delete entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
   g_signal_connect (GTK_OBJECT (submenu), "activate",
		     GTK_SIGNAL_FUNC (flow_delete), NULL );

   /* Separation line */
   submenu = gtk_menu_item_new();
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);


   /* Properties entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
   g_signal_connect (GTK_OBJECT (submenu), "activate",
 		     GTK_SIGNAL_FUNC (flow_properties), NULL );

   /* Input/Output entry */
   submenu = gtk_image_menu_item_new_with_label ("Input/Output");
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
   g_signal_connect (GTK_OBJECT (submenu), "activate",
 		     GTK_SIGNAL_FUNC (flow_io), NULL );

   /* Execute entry */
   submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
   gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu );
   g_signal_connect (GTK_OBJECT (submenu), "activate",
 		     GTK_SIGNAL_FUNC (flow_run), NULL );

   menuitem = gtk_menu_item_new_with_label ("Flow");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

   gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

   return menuitem;

}


/*------------------------------------------------------------------------*
 * Function: assembly_flow_component_menu
 * Assembly the menu to the flow edit page associated to flow_components
 *
 */
GtkWidget *
assembly_flowcomponentsmenu (void)
{
	GtkWidget *	menuitem;
	GtkWidget *	menu;
	GtkWidget *	submenu;
	GSList *	radio_slist;

	menu = gtk_menu_new ();
	gtk_menu_set_title (GTK_MENU (menu), "Flow component menu");

	/* Properties entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (flow_component_properties_set), NULL );

	/* Refresh entry */
	submenu = gtk_image_menu_item_new_from_stock (GTK_STOCK_REFRESH, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (menus_create_index), NULL);
	g_signal_connect (GTK_OBJECT (submenu), "activate",
			GTK_SIGNAL_FUNC (menus_populate), NULL );

	/* separator */
	submenu = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), submenu);
	/* component status items */
	/* configured */
	radio_slist = NULL;
	W.configured_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Configured");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.configured_menuitem);
	g_signal_connect (	GTK_OBJECT (W.configured_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );
	/* disabled */
	radio_slist = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (W.configured_menuitem));
	W.disabled_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Disabled");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.disabled_menuitem);
	g_signal_connect (	GTK_OBJECT (W.disabled_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );
	/* unconfigured */
	radio_slist = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (W.disabled_menuitem));
	W.unconfigured_menuitem = gtk_radio_menu_item_new_with_label (radio_slist, "Unconfigured");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), W.unconfigured_menuitem);
	g_signal_connect (	GTK_OBJECT (W.unconfigured_menuitem), "activate",
				GTK_SIGNAL_FUNC (flow_component_set_status), NULL );

	menuitem = gtk_menu_item_new_with_label ("Flow component");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
	gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));

	return menuitem;
}
