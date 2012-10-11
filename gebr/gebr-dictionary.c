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

#include <libgebr/utils.h>
#include <libgebr/geoxml/geoxml.h>

struct _GebrDictCompletePriv
{
	GebrGeoXmlDocument *proj;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
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
insert_dict_variable(GebrDictComplete *self,
		     GebrGeoXmlProgramParameter *param,
		     GebrGeoXmlDocumentType doc_type)
{
	const gchar *keyword;
	GebrGeoXmlParameterType type;
	GebrDictCompleteType complete_type;
	const gchar *value;
	gchar *result;

	keyword = gebr_geoxml_program_parameter_get_keyword(param);
	type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));
	complete_type = GEBR_DICT_COMPLETE_TYPE_VARIABLE;
	value = gebr_geoxml_program_parameter_get_first_value(param, FALSE);
	result = g_strdup_printf("= %s", value);

	GtkTreeIter iter;
	gtk_list_store_append(self->priv->store, &iter);
	gtk_list_store_set(self->priv->store, &iter,
			   GEBR_DICT_COMPLETE_KEYWORD, keyword,
			   GEBR_DICT_COMPLETE_COMPLETE_TYPE, complete_type,
			   GEBR_DICT_COMPLETE_VARIABLE_TYPE, type,
			   GEBR_DICT_COMPLETE_DOCUMENT_TYPE, doc_type,
			   GEBR_DICT_COMPLETE_RESULT, result,
			   -1);

	g_free(result);
}

static void
insert_path_variable(GebrDictComplete *self, gchar **path)
{
	const gchar *result;
	const gchar *keyword;
	GebrGeoXmlParameterType type;
	GebrDictCompleteType complete_type;
	GebrGeoXmlDocumentType doc_type;

	result = path[0];
	keyword = path[1];
	type = GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
	complete_type = GEBR_DICT_COMPLETE_TYPE_PATH;
	doc_type = GEBR_GEOXML_DOCUMENT_TYPE_LINE;

	GtkTreeIter iter;
	gtk_list_store_append(self->priv->store, &iter);
	gtk_list_store_set(self->priv->store, &iter,
			   GEBR_DICT_COMPLETE_KEYWORD, keyword,
			   GEBR_DICT_COMPLETE_COMPLETE_TYPE, complete_type,
			   GEBR_DICT_COMPLETE_VARIABLE_TYPE, type,
			   GEBR_DICT_COMPLETE_DOCUMENT_TYPE, doc_type,
			   GEBR_DICT_COMPLETE_RESULT, result,
			   -1);
}

static void
gebr_dict_complete_update_model(GebrDictComplete *self)
{
	gtk_list_store_clear(self->priv->store);

	GebrGeoXmlDocument *docs[] = {
		self->priv->proj,
		self->priv->line,
		self->priv->flow,
		NULL
	};

	for (int i = 0; docs[i]; i++) {
		GebrGeoXmlSequence *seq;
		seq = gebr_geoxml_document_get_dict_parameter(docs[i]);

		GebrGeoXmlDocumentType doc_type;
		doc_type = gebr_geoxml_document_get_type(docs[i]);

		for (; seq; gebr_geoxml_sequence_next(&seq))
			insert_dict_variable(self, GEBR_GEOXML_PROGRAM_PARAMETER(seq), doc_type);
	}

	gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(self->priv->line));
	for (int i = 0; paths[i]; i++)
		insert_path_variable(self, paths[i]);
	gebr_pairstrfreev(paths);
}

GebrDictComplete *
gebr_dict_complete_new(void)
{
	GebrDictComplete *dict = g_new0(GebrDictComplete, 1);
	dict->priv = g_new0(GebrDictCompletePriv, 1);
	dict->priv->store = gtk_list_store_new(GEBR_DICT_COMPLETE_NCOLS,
					       G_TYPE_STRING,  /* Keyword */
					       G_TYPE_INT,     /* Completion type */
					       G_TYPE_INT,     /* Variable type */
					       G_TYPE_INT,     /* Document type */
					       G_TYPE_STRING); /* Result */
	return dict;
}

void
gebr_dict_complete_set_documents(GebrDictComplete *self,
				 GebrGeoXmlDocument *proj,
				 GebrGeoXmlDocument *line,
				 GebrGeoXmlDocument *flow)
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

GtkTreeModel *
gebr_dict_complete_get_filter(GebrDictComplete *self,
			      GebrGeoXmlParameterType type)
{
	return gebr_dict_complete_get_filter_full(self, type, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
}

struct FilterData {
	GebrGeoXmlParameterType type;
	GebrGeoXmlDocumentType doc_type;
};

static gboolean visible_func(GtkTreeModel *model,
			     GtkTreeIter  *iter,
			     gpointer      user_data)
{
	struct FilterData *data = user_data;

	GebrDictCompleteType complete_type;
	GebrGeoXmlParameterType param_type;
	GebrGeoXmlDocumentType doc_type;

	gtk_tree_model_get(model, iter,
			   GEBR_DICT_COMPLETE_COMPLETE_TYPE, &complete_type,
			   GEBR_DICT_COMPLETE_VARIABLE_TYPE, &param_type,
			   GEBR_DICT_COMPLETE_DOCUMENT_TYPE, &doc_type,
			   -1);

	if (complete_type == GEBR_DICT_COMPLETE_TYPE_PATH)
		return data->type == GEBR_GEOXML_PARAMETER_TYPE_FILE;

	if (!gebr_geoxml_document_type_contains(doc_type, data->doc_type))
		return FALSE;

	return gebr_geoxml_parameter_type_is_compatible(data->type, param_type);
}

GtkTreeModel *
gebr_dict_complete_get_filter_full(GebrDictComplete *self,
				   GebrGeoXmlParameterType type,
				   GebrGeoXmlDocumentType doc_type)
{
	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(self->priv->store), NULL);

	struct FilterData *data = g_new(struct FilterData, 1);
	data->type = type;
	data->doc_type = doc_type;

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       visible_func, data, g_free);
	return filter;
}
