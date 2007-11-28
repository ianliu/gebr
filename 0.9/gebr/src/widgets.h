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

#ifndef _GEBR_WS_H_
#define _GEBR_WS_H_

#include <gtk/gtk.h>

#include "cmdline.h"

#define NABAS 4

/* Projects/Lines store fields */
enum {
   PL_NAME = 0,
   PL_FILENAME,
   PL_N_COLUMN
};

/* Flow browser store fields */
enum {
   FB_NAME = 0,
   FB_FILENAME,
   FB_N_COLUMN
};

/* Menubar */
enum {
   MENUBAR_PROJECT,
   MENUBAR_LINE,
   MENUBAR_FLOW,
   MENUBAR_FLOW_COMPONENTS,
   MENUBAR_N
};

/* Flow sequence store fields */
enum {
   FSEQ_STATUS_COLUMN,
   FSEQ_TITLE_COLUMN,
   FSEQ_MENU_FILE_NAME_COLUMN,
   FSEQ_MENU_INDEX,
   FSEQ_N_COLUMN
};

/* Menu store fields */
enum {
   MENU_TITLE_COLUMN,
   MENU_DESC_COLUMN,
   MENU_FILE_NAME_COLUMN,
   MENU_N_COLUMN
};


/* Job control store fields */
enum {
   JC_ICON,
   JC_TITLE,
   JC_STRUCT,
   JC_N_COLUMN
};


/* Server store field */
enum {
   SERVER_ADDRESS,
   SERVER_POINTER,
   SERVER_N_COLUMN
};

typedef struct {
	GtkWidget *hbox;

	GtkWidget *entry;
	GtkWidget *browse_button;
} gebr_save_widget_t;

typedef struct {

   GtkWidget *win;

   GtkWidget *username;
   GtkWidget *email;
   GtkWidget *usermenus;
   GtkWidget *data;
   GtkWidget *editor;
   GtkWidget *browser;

   GString *username_value;
   GString *email_value;
   GString *usermenus_value;
   GString *data_value;
   GString *editor_value;
   GString *browser_value;

} gebrw_pref_t;

typedef struct {
   GtkWidget *win;

   GtkWidget *title;
   GtkWidget *description;
   GtkWidget *help;
   GtkWidget *author;
   GtkWidget *email;

} gebr_flow_prop_t;

typedef struct {
	GtkWidget *		win;

	gebr_save_widget_t	input;
	gebr_save_widget_t	output;
	gebr_save_widget_t	error;
} gebr_flow_io_t;

typedef struct {

   GtkWidget *title;
   GtkWidget *description;

   GtkWidget *input_label;
   GtkWidget *input;
   GtkWidget *output_label;
   GtkWidget *output;
   GtkWidget *error_label;
   GtkWidget *error;

   GtkWidget *help;

   GtkWidget *author;

} gebr_flow_info_t;

typedef struct {
   GdkPixbuf *	configured_icon;
   GdkPixbuf *	unconfigured_icon;
   GdkPixbuf *	disabled_icon;
   GdkPixbuf *	running_icon;
} gebr_pixmaps;

typedef struct {
	GtkWidget *	job_label;
	GtkWidget *	text_view;
	GtkTextBuffer *	text_buffer;
} gebr_job_control_t;

/* Widgets persistentes, globais a todo o cÛdigo */
typedef struct {

	GtkWidget *mainwin;
	GtkWidget *menu[MENUBAR_N];
	GtkWidget *notebook;
	GtkWidget *statusbar;

	/* Trees and Lists */
	GtkTreePath  *proj_line_selection_path;
	GtkTreeStore *proj_line_store;
	GtkListStore *flow_store;
	GtkListStore *fseq_store;
	GtkTreeStore *menu_store;
        GtkListStore *job_store;
        GtkListStore *server_store;

	/* Views */
	GtkWidget *proj_line_view; /* projects and lines             */
	GtkWidget *flow_view;      /* flows of a line                */
	GtkWidget *fseq_view;      /* Sequence of programs of a flow */
	GtkWidget *menu_view;      /* Available menus                */
        GtkWidget *job_view;       /* Running jobs                   */
        GtkWidget *server_view;    /* Server view                    */

	/* config options from gengetopt
	 * loaded in gebr_config_load at callbacks.c
	 */
        struct ggopt		config;

	/* preferences window */
	gebrw_pref_t		pref;
	/* log file */
	FILE *			log_fp;

        /* flow info window */
        gebr_flow_info_t	flow_info;

	/* flow properties window */
	gebr_flow_prop_t	flow_prop;

        /* job control */
        gebr_job_control_t	job_ctrl;

	/* status menu items */
	GtkWidget *		configured_menuitem;
	GtkWidget *		disabled_menuitem;
	GtkWidget *		unconfigured_menuitem;

	/* flow io window. */
	gebr_flow_io_t		flow_io;

	/* Dialogs */
	GtkWidget *filesel;
	GtkWidget *fileselentry;

	GtkWidget *about;

	GtkWidget *	parameters;
	GtkWidget **	parwidgets;
	int		parwidgets_number;
	int		program_index;

        /* Pixmaps */
        gebr_pixmaps pixmaps;

        /* List of temporary file to be deleted */
        GSList * tmpfiles;

} gebrw_t;


#endif //_GEBR_WS_H_
