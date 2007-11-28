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

/* File: ui_pages.c
 * Assembly each page of the main notebook widget of the interface.
 */
#include "ui_pages.h"

#include "gebr.h"
#include "callbacks.h"
#include "cb_proj.h"
#include "cb_line.h"
#include "cb_flow.h"
#include "cb_flowcomp.h"
#include "cb_job.h"
#include "menus.h"
#include "ui_prop.h"

#define _(CONTENT)  CONTENT

/*------------------------------------------------------------------------*
 * Function: add_project
 * Assembly the project page.
 *
 * Input:
 * notebook - Pointer to the notebook to which the page will be added to.
 *
 * TODO:
 * * Add an info summary about the project/line.
 */
void
add_project   (GtkNotebook    *notebook)
{
	GtkWidget *page;
	GtkWidget *pagetitle;
	GtkWidget *scrolledwin;

	GtkTreeSelection *selection;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* Create projects/lines page */
	page = gtk_vbox_new (FALSE, 0);
	pagetitle = gtk_label_new ("Projects and Lines");

	/* Project and line tree */
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (page), scrolledwin);

	W.proj_line_store = gtk_tree_store_new (PL_N_COLUMN,
						G_TYPE_STRING,  /* Name (title for libgeoxml) */
						G_TYPE_STRING); /* Filename */

	W.proj_line_view = gtk_tree_view_new_with_model
	(GTK_TREE_MODEL (W.proj_line_store));

	gtk_container_add (GTK_CONTAINER (scrolledwin), W.proj_line_view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new ();

	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (GTK_OBJECT (renderer), "edited",
			GTK_SIGNAL_FUNC (proj_line_rename), NULL );

	col = gtk_tree_view_column_new_with_attributes ("Projects/lines index", renderer, NULL);
	gtk_tree_view_column_set_sort_column_id (col, PL_NAME);
	gtk_tree_view_column_set_sort_indicator (col, TRUE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (W.proj_line_view), col);
	gtk_tree_view_column_add_attribute (col, renderer, "text", PL_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (GTK_OBJECT (W.proj_line_view), "cursor-changed",
			GTK_SIGNAL_FUNC (line_load_flows), NULL );

	W.proj_line_selection_path = NULL;

	/* Add this page to the notebook */
	gtk_notebook_append_page (notebook, page, pagetitle);
}

/*------------------------------------------------------------------------*
 * Function: add_flow_browse
 * Assembly the flow browse page.
 *
 * Input:
 * notebook - Pointer to the notebook to which the page will be added to.
 *
 * TODO:
 * * Add an info summary about the selected flow.
 */
void
add_flow_browse      (GtkNotebook    *notebook)
{
   GtkWidget         *page;
   GtkWidget         *pagetitle;
   GtkWidget         *hpanel;
   GtkWidget         *scrolledwin;
   GtkTreeSelection  *selection;
   GtkTreeViewColumn *col;
   GtkCellRenderer   *renderer;
   static const char *label = "Flows";

   /* Create flow browse page */
   page = gtk_vbox_new (FALSE, 0);
   pagetitle = gtk_label_new (label);

   hpanel = gtk_hpaned_new ();
   gtk_container_add (GTK_CONTAINER (page), hpanel);

   /* Left side */
   /* Flow list */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_paned_pack1 (GTK_PANED (hpanel), scrolledwin, FALSE, FALSE);
   gtk_widget_set_size_request (scrolledwin, 180, -1);

   W.flow_store = gtk_list_store_new (FB_N_COLUMN,
				      G_TYPE_STRING,  /* Name (title for libgeoxml) */
				      G_TYPE_STRING); /* Filename */

   W.flow_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (W.flow_store));

   renderer = gtk_cell_renderer_text_new ();

   g_object_set (renderer, "editable", TRUE, NULL);
   g_signal_connect (GTK_OBJECT (renderer), "edited",
		     GTK_SIGNAL_FUNC  (flow_rename), NULL );

   g_signal_connect (GTK_OBJECT (renderer), "edited",
		     GTK_SIGNAL_FUNC  (flow_info_update), NULL );

   col = gtk_tree_view_column_new_with_attributes  (label, renderer, NULL);
   gtk_tree_view_column_set_sort_column_id  (col, FB_NAME);
   gtk_tree_view_column_set_sort_indicator  (col, TRUE);
   gtk_tree_view_append_column  (GTK_TREE_VIEW (W.flow_view), col);
   gtk_tree_view_column_add_attribute  (col, renderer, "text", FB_NAME);

   selection = gtk_tree_view_get_selection  (GTK_TREE_VIEW (W.flow_view));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

   gtk_container_add (GTK_CONTAINER (scrolledwin), W.flow_view);

   g_signal_connect (GTK_OBJECT (W.flow_view), "cursor-changed",
		     GTK_SIGNAL_FUNC (flow_load), NULL );

   g_signal_connect (GTK_OBJECT (W.flow_view), "cursor-changed",
		     GTK_SIGNAL_FUNC (flow_info_update), NULL );

   /* Right side */
   /* Flow info  */
   {
      GtkWidget *frame;
      GtkWidget *infopage;

      frame = gtk_frame_new ("Details");
      gtk_paned_pack2 (GTK_PANED (hpanel), frame, TRUE, FALSE);

      infopage = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (frame), infopage);

      /* Title */
      W.flow_info.title = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.title), 0, 0);
      gtk_box_pack_start (GTK_BOX (infopage), W.flow_info.title, FALSE, TRUE, 0);

      /* Description */
      W.flow_info.description = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.description), 0, 0);
      gtk_box_pack_start (GTK_BOX (infopage), W.flow_info.description, FALSE, TRUE, 10);

      /* I/O */
      GtkWidget *table;
      table = gtk_table_new (3, 2, FALSE);
      gtk_box_pack_start (GTK_BOX (infopage), table, FALSE, TRUE, 0);

      W.flow_info.input_label = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.input_label), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.input_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

      W.flow_info.input = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.input), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.input, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

      W.flow_info.output_label = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.output_label), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.output_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

      W.flow_info.output = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.output), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

      W.flow_info.error_label = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.error_label), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.error_label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

      W.flow_info.error = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.error), 0, 0);
      gtk_table_attach (GTK_TABLE (table), W.flow_info.error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

      /* Help */
      W.flow_info.help = gtk_button_new_from_stock ( GTK_STOCK_INFO );
      gtk_box_pack_end (GTK_BOX (infopage), W.flow_info.help, FALSE, TRUE, 0);
      g_signal_connect (GTK_OBJECT (W.flow_info.help), "clicked",
			GTK_SIGNAL_FUNC (flow_show_help), flow );

      /* Author */
      W.flow_info.author = gtk_label_new("");
      gtk_misc_set_alignment ( GTK_MISC(W.flow_info.author), 0, 0);
      gtk_box_pack_end (GTK_BOX (infopage), W.flow_info.author, FALSE, TRUE, 0);

   }

   /* Add this page to the notebook */
   gtk_notebook_append_page (notebook, page, pagetitle);
}

/*------------------------------------------------------------------------*
 * Function: add_flow_edit
 * Assembly the flow edit page.
 *
 * Input:
 * notebook - Pointer to the notebook to which the page will be added to.
 *
 */
void
add_flow_edit   (GtkNotebook    *notebook)
{
   GtkWidget *page;
   GtkWidget *pagetitle;
   GtkWidget *hpanel;
   GtkWidget *scrolledwin;
   GtkWidget *frame;

   GtkTreeViewColumn *col;
   GtkCellRenderer *renderer;

   /* Create flow edit page */
   page = gtk_vbox_new (FALSE, 0);
   pagetitle = gtk_label_new ("Flow edition");

   hpanel = gtk_hpaned_new ();
   gtk_container_add (GTK_CONTAINER (page), hpanel);

   /* Left side */
   frame = gtk_frame_new ("Flow sequence");
   gtk_paned_pack1 (GTK_PANED (hpanel), frame, FALSE, FALSE);

   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (frame), scrolledwin);

   W.fseq_store = gtk_list_store_new (FSEQ_N_COLUMN,
				      GDK_TYPE_PIXBUF,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_ULONG);

   W.fseq_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (W.fseq_store));
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(W.fseq_view), FALSE);

   renderer = gtk_cell_renderer_pixbuf_new ();
   col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (W.fseq_view), col);
   gtk_tree_view_column_add_attribute (col, renderer, "pixbuf", FSEQ_STATUS_COLUMN);

   renderer = gtk_cell_renderer_text_new ();
   col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (W.fseq_view), col);
   gtk_tree_view_column_add_attribute (col, renderer, "text", FSEQ_TITLE_COLUMN);

   /* Double click on flow component open its parameter window */
   g_signal_connect (W.fseq_view, "row-activated",
		     (GCallback) progpar_config_window, NULL );
   g_signal_connect (GTK_OBJECT (W.fseq_view), "cursor-changed",
			GTK_SIGNAL_FUNC (flow_component_selected), NULL );

   gtk_container_add (GTK_CONTAINER (scrolledwin), W.fseq_view);
   gtk_widget_set_size_request (GTK_WIDGET (scrolledwin), 180, 30);

   /* Right side */
   {
      GtkWidget *hbox;

      frame = gtk_frame_new ("Flow components");
      gtk_paned_pack2 (GTK_PANED (hpanel), frame, TRUE, TRUE);

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (frame), hbox);

      /* Up, Down, Right and Left buttons */
      {
	 GtkWidget *button;
	 GtkWidget *vbox;

	 vbox = gtk_vbox_new (FALSE, 4);
	 gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	 button = gtk_button_new ();
	 gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_ADD, 1));
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	 g_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (program_add_to_flow), NULL );

	 button = gtk_button_new ();
	 gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_REMOVE, 1));
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	 g_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (program_remove_from_flow), NULL );

	 button = gtk_button_new ();
	 gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_HELP, 1));
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	 g_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (menu_show_help), NULL );

	 button = gtk_button_new ();
	 gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, 1));
	 gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	 g_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (program_move_down), NULL );

	 button = gtk_button_new ();
	 gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_stock (GTK_STOCK_GO_UP, 1));
	 gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	 g_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (program_move_up), NULL );

      }

      /* Menu list */
      {
	 GtkWidget *vbox;

	 vbox = gtk_vbox_new (FALSE, 3);
	 gtk_box_pack_end (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	 scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	 gtk_container_add (GTK_CONTAINER (vbox), scrolledwin);

	 W.menu_store = gtk_tree_store_new (MENU_N_COLUMN,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING);

	 W.menu_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (W.menu_store));

	 g_signal_connect (GTK_OBJECT (W.menu_view), "row-activated",
			   GTK_SIGNAL_FUNC (program_add_to_flow), NULL );

	 renderer = gtk_cell_renderer_text_new ();
	 col = gtk_tree_view_column_new_with_attributes ("Flow", renderer, NULL);
	 gtk_tree_view_append_column (GTK_TREE_VIEW (W.menu_view), col);
	 gtk_tree_view_column_add_attribute (col, renderer, "markup", MENU_TITLE_COLUMN);
	 gtk_tree_view_column_set_sort_column_id (col, MENU_TITLE_COLUMN);
	 gtk_tree_view_column_set_sort_indicator (col, TRUE);

	 renderer = gtk_cell_renderer_text_new ();
	 col = gtk_tree_view_column_new_with_attributes ("Description", renderer, NULL);
	 gtk_tree_view_append_column (GTK_TREE_VIEW (W.menu_view), col);
	 gtk_tree_view_column_add_attribute (col, renderer, "text", MENU_DESC_COLUMN);

	 gtk_container_add (GTK_CONTAINER (scrolledwin), W.menu_view);
      }
   }
   /* Add this page to the notebook */
   gtk_notebook_append_page (notebook, page, pagetitle);
}

/*------------------------------------------------------------------------*
 * Function: add_job_control
 * Assembly the job control page.
 *
 * Input:
 * notebook - Pointer to the notebook to which the page will be added to.
 *
 */
void
add_job_control   (GtkNotebook    *notebook)
{

   GtkWidget *page;
   GtkWidget *pagetitle;
   GtkWidget *vbox;

   GtkWidget *toolbar;
   GtkIconSize tmp_toolbar_icon_size;
   GtkWidget *toolitem;
   GtkWidget *button;

   GtkWidget *hpanel;
   GtkWidget *scrolledwin;
   GtkWidget *frame;

   GtkTreeViewColumn *col;
   GtkCellRenderer *renderer;

   GtkWidget *text_view;

   /* Create flow edit page */
   page = gtk_vbox_new (FALSE, 0);
   pagetitle = gtk_label_new ("Job control");

   /* Vbox to hold toolbar and main content */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (page), vbox);

   /* Toolbar */
   toolbar = gtk_toolbar_new ();
   gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
   /* FIXME ! */
   /* g_object_set_property(G_OBJECT(toolbar), "shadow-type", GTK_SHADOW_NONE); */

   tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));

   /* Cancel button = END */
   toolitem = (GtkWidget*) gtk_tool_item_new ();
   gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

   button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
   gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
   gtk_container_add (GTK_CONTAINER (toolitem), button);

   g_signal_connect (GTK_BUTTON(button), "clicked",
		     GTK_SIGNAL_FUNC(job_cancel), NULL);

   /* Close button */
   toolitem = (GtkWidget*) gtk_tool_item_new ();
   gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

   button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
   gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
   gtk_container_add (GTK_CONTAINER (toolitem), button);

   g_signal_connect (GTK_BUTTON(button), "clicked",
		     GTK_SIGNAL_FUNC(job_close), NULL);

   /* Clear button */
   toolitem = (GtkWidget*) gtk_tool_item_new ();
   gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

   button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
   gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
   gtk_container_add (GTK_CONTAINER (toolitem), button);

   g_signal_connect (GTK_BUTTON(button), "clicked",
		     GTK_SIGNAL_FUNC(job_clear), NULL);

   /* Stop button = KILL */
   toolitem = (GtkWidget*) gtk_tool_item_new ();
   gtk_container_add (GTK_CONTAINER (toolbar), toolitem);

   button = gtk_button_new_from_stock (GTK_STOCK_STOP);
   gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
   gtk_container_add (GTK_CONTAINER (toolitem), button);

   g_signal_connect (GTK_BUTTON(button), "clicked",
		     GTK_SIGNAL_FUNC(job_stop), NULL);

   hpanel = gtk_hpaned_new ();
   gtk_box_pack_start (GTK_BOX (vbox), hpanel, TRUE, TRUE, 0);

   /* Left side */
   frame = gtk_frame_new ("Jobs");
   gtk_paned_pack1 (GTK_PANED (hpanel), frame, FALSE, FALSE);

   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (frame), scrolledwin);

   W.job_store = gtk_list_store_new (JC_N_COLUMN,
				     GDK_TYPE_PIXBUF,	/* Icon		*/
				     G_TYPE_STRING,	/* Title	*/
				     G_TYPE_POINTER);	/* struct job	*/

   W.job_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (W.job_store));
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(W.job_view), FALSE);

   g_signal_connect (GTK_OBJECT (W.job_view), "cursor-changed",
		GTK_SIGNAL_FUNC (job_clicked), NULL);

   renderer = gtk_cell_renderer_pixbuf_new ();
   col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (W.job_view), col);
   gtk_tree_view_column_add_attribute (col, renderer, "pixbuf", JC_ICON);

   renderer = gtk_cell_renderer_text_new ();
   col = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (W.job_view), col);
   gtk_tree_view_column_add_attribute (col, renderer, "text", JC_TITLE);

   gtk_container_add (GTK_CONTAINER (scrolledwin), W.job_view);
   gtk_widget_set_size_request (GTK_WIDGET (scrolledwin), 180, 30);

   /* Right side */
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_paned_pack2 (GTK_PANED (hpanel), vbox, TRUE, TRUE);

   W.job_ctrl.job_label = gtk_label_new("");
   gtk_box_pack_start (GTK_BOX (vbox), W.job_ctrl.job_label, FALSE, TRUE, 0);

   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_end (GTK_BOX (vbox), scrolledwin, TRUE, TRUE, 0);

   W.job_ctrl.text_buffer = gtk_text_buffer_new(NULL);
   text_view = gtk_text_view_new_with_buffer(W.job_ctrl.text_buffer);
   g_object_set(G_OBJECT(text_view),
		"editable", FALSE,
		"cursor-visible", FALSE,
		NULL);
   W.job_ctrl.text_view = text_view;
   gtk_container_add (GTK_CONTAINER (scrolledwin), text_view);


   /* Add this page to the notebook */
   gtk_notebook_append_page (notebook, page, pagetitle);
}
