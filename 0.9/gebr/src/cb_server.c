#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "gebr.h"
#include "cb_server.h"
#include "server.h"

/*
  Function: server_dialog_actions
  Take the appropriate action when the server dialog emmits
  a response signal.
 */
void
server_dialog_actions                (GtkDialog *dialog,
				      gint       arg1,
				      gpointer   user_data)
{
	switch (arg1){
	case GEBR_SERVER_CLOSE:
	   gtk_widget_destroy(GTK_WIDGET(dialog));
	   break;
	case GEBR_SERVER_REMOVE:
	   server_remove();
	default:
	   break;
	}
	gebr_config_save();
}

/*
  Function: server_add
  Callback to add a server to the server list
 */
void
server_add    (GtkEntry   *entry,
	       gpointer   *data)
{
	GtkTreeIter	iter;
	gboolean	valid;

	/* check if it is already in list */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.server_store), &iter);
	while (valid) {
		gchar *	server;

		gtk_tree_model_get (GTK_TREE_MODEL(W.server_store), &iter,
				SERVER_ADDRESS, &server,
				-1);

		if (!g_ascii_strcasecmp(server, gtk_entry_get_text(entry))) {
			GtkTreeSelection *	selection;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.server_view));
			gtk_tree_selection_select_iter(selection, &iter);

			g_free(server);
			return;
		}

		g_free(server);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(W.server_store), &iter);
	}

	gtk_list_store_append(W.server_store, &iter);
	gtk_list_store_set(W.server_store, &iter,
			SERVER_ADDRESS, gtk_entry_get_text(entry),
			SERVER_POINTER, server_new(gtk_entry_get_text(entry)),
			-1);
	gtk_entry_set_text(entry, "");
}

/*
  Function: server_remove
  Callback to remove the selected server from the server list
 */
void
server_remove    (void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter       	iter;
	GtkTreeModel     *	model;
	struct server *		server;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.server_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL(W.server_store), &iter,
			SERVER_POINTER, &server,
			-1);
	server_free(server);

	gtk_list_store_remove (GTK_LIST_STORE (W.server_store), &iter);
}
