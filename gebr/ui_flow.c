/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2012 GeBR core team (http://www.gebrproject.com/)
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

#include "ui_flow.h"

#include "gebr.h"
#include "line.h"

struct _GebrUiFlowPriv {
	GebrGeoXmlFlow *flow;
	GebrGeoXmlLineFlow *line_flow;
	gboolean selected;

	gchar *filename;
	gchar *last_modified;
};

G_DEFINE_TYPE(GebrUiFlow, gebr_ui_flow, G_TYPE_OBJECT);


static void
gebr_ui_flow_finalize(GObject *object)
{
	GebrUiFlow *ui_flow = GEBR_UI_FLOW(object);
	G_OBJECT_CLASS(gebr_ui_flow_parent_class)->finalize(object);

	g_free(ui_flow->priv->filename);
	g_free(ui_flow->priv);
}

static void
gebr_ui_flow_init(GebrUiFlow *ui_flow)
{
	ui_flow->priv = G_TYPE_INSTANCE_GET_PRIVATE(ui_flow,
	                                            GEBR_TYPE_UI_FLOW,
	                                            GebrUiFlowPriv);
}

static void
gebr_ui_flow_class_init(GebrUiFlowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_ui_flow_finalize;

	g_type_class_add_private(klass, sizeof(GebrUiFlowPriv));
}

GebrUiFlow *
gebr_ui_flow_new(GebrGeoXmlFlow *flow,
                 GebrGeoXmlLineFlow *line_flow)
{
	GebrUiFlow *ui_flow = g_object_new(GEBR_TYPE_UI_FLOW, NULL);

	ui_flow->priv->flow = flow;
	ui_flow->priv->line_flow = line_flow;
	ui_flow->priv->filename = g_strdup(gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow)));
	ui_flow->priv->selected = FALSE;

	return ui_flow;
}

GebrGeoXmlFlow *
gebr_ui_flow_get_flow(GebrUiFlow *ui_flow)
{
	return ui_flow->priv->flow;
}

GebrGeoXmlLineFlow *
gebr_ui_flow_get_line_flow(GebrUiFlow *ui_flow)
{
	return ui_flow->priv->line_flow;
}

const gchar *
gebr_ui_flow_get_filename(GebrUiFlow *ui_flow)
{
	return ui_flow->priv->filename;
}

const gchar *
gebr_ui_flow_get_last_modified(GebrUiFlow *ui_flow)
{
	return ui_flow->priv->last_modified;
}

void
gebr_ui_flow_set_last_modified(GebrUiFlow *ui_flow,
                               const gchar *last_modified)
{
	if (ui_flow->priv->last_modified)
		g_free(ui_flow->priv->last_modified);

	ui_flow->priv->last_modified = g_strdup(last_modified);
}

gboolean
gebr_ui_flow_get_is_selected(GebrUiFlow *ui_flow)
{
	return ui_flow->priv->selected;
}

void
gebr_ui_flow_set_is_selected(GebrUiFlow *ui_flow,
                             gboolean is_selected)
{
	ui_flow->priv->selected = is_selected;
}


GtkMenu *
gebr_ui_flow_popup_menu(GebrUiFlow *ui_flow, gboolean multiple)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	GtkTreeIter iter;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
		return NULL;

	menu = gtk_menu_new();

	if (!flow_browse_get_selected(&iter, FALSE)) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_new")));
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_paste")));
		goto out;
	}

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_new_program")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_delete")));

	if (!multiple) {
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_properties")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_view")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_edit")));

		menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"));
		gtk_container_add(GTK_CONTAINER(menu), menu_item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		gtk_container_add(GTK_CONTAINER(menu),
					  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute")));
	} else {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute"));
		gtk_container_add(GTK_CONTAINER(menu), menu_item);

		menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute_parallel"));
		gtk_container_add(GTK_CONTAINER(menu), menu_item);
	}




 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}
