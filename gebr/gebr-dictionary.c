/*
 * gebr-dictionary.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-dictionary.h"

struct _GebrDictCompletePriv
{
	GebrGeoXmlProject *proj;
	GebrGeoXmlLine *line;
	GebrGeoXmlFlow *flow;
	GtkListStore *store;
};

static gpointer
_gebr_dict_complete_copy(gpointer boxed)
{
	return NULL;
}

static void
_gebr_dict_complete_free(gpointer boxed)
{
	return;
}

GType
gebr_dict_complete_get_type(void)
{
	static GType type_id = 0;

	if (type_id == 0)
		type_id = g_boxed_type_register_static("GebrDictComplete",
				_gebr_dict_complete_copy,
				_gebr_dict_complete_free);
	return type_id;
}

static void
gebr_dict_complete_update_model(GebrDictComplete *self)
{
	gtk_list_store_clear(self->priv->store);
}

GebrDictComplete *
gebr_dict_complete_new(GebrGeoXmlProject *proj, GebrGeoXmlLine *line, GebrGeoXmlFlow *flow)
{
	GebrDictComplete *dict = g_new0(GebrDictComplete, 1);
	dict->priv = g_new0(GebrDictCompletePriv, 1);
	dict->priv->store = gtk_list_store_new(4,
					       G_TYPE_STRING,  /* Keyword */
					       G_TYPE_INT,     /* Completion type */
					       G_TYPE_INT,     /* Variable type */
					       G_TYPE_STRING); /* Result */

	gebr_dict_complete_set_documents(dict, proj, line, flow);
	return dict;
}

GebrDictComplete *
gebr_dict_complete_set_documents(GebrDictComplete *self,
				 GebrGeoXmlProject *proj,
				 GebrGeoXmlLine *line,
				 GebrGeoXmlFlow *flow)
{
	if (self->priv->proj)
		gebr_geoxml_document_unref(self->priv->proj);
	self->priv->proj = gebr_geoxml_document_ref(proj);

	if (self->priv->line)
		gebr_geoxml_document_unref(self->priv->line);
	self->priv->line = gebr_geoxml_document_ref(line);

	if (self->priv->flow)
		gebr_geoxml_document_unref(self->priv->flow);
	self->priv->flow = gebr_geoxml_document_ref(flow);

	gebr_dict_complete_update_model(self);
}

