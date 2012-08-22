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

struct _GebrUiFlowPriv {
	GebrGeoXmlFlow *flow;
	GebrGeoXmlLineFlow *line_flow;

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
