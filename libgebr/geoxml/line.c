/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <gdome.h>
#include <glib/gi18n-lib.h>

#include "document.h"
#include "document_p.h"
#include "error.h"
#include "line.h"
#include "object.h"
#include "sequence.h"
#include "types.h"
#include "value_sequence.h"
#include "xml.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_line {
	GebrGeoXmlDocument *document;
};

struct gebr_geoxml_line_flow {
	GdomeElement *element;
};

struct gebr_geoxml_line_path {
	GdomeElement *element;
};

/*
 * library functions.
 */

GebrGeoXmlLine *gebr_geoxml_line_new()
{
	GebrGeoXmlDocument *document = gebr_geoxml_document_new("line", GEBR_GEOXML_LINE_VERSION);
	GdomeElement *root = gebr_geoxml_document_root_element(document);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root, "nfs", NULL), &exception);
	gdome_el_unref(root, &exception);
	return GEBR_GEOXML_LINE(document);
}

static gchar *base_dirs[][2] = {
		{"BASE", ""},
		{"DATA", "data"},
		{"EXPORT", "export"},
		{"TMP", "tmp"},
		{"TABLE", "table"},
		{"MISC", "misc"},
};

void
gebr_geoxml_line_set_base_path(GebrGeoXmlLine *line, const gchar *base)
{
	GebrGeoXmlSequence *seq, *aux;

	gebr_geoxml_line_get_path(line, &seq, 0);

	while (seq) {
		gchar *name = gebr_geoxml_line_path_get_name((GebrGeoXmlLinePath*)seq);
		if (g_strcmp0(name, "HOME") == 0) {
			gebr_geoxml_sequence_next(&seq);
			continue;
		}
		gebr_geoxml_object_ref(seq);
		aux = seq;
		gebr_geoxml_sequence_next(&aux);
		gebr_geoxml_sequence_remove(seq);
		seq = aux;
	}

	for (gint i = 0; i < G_N_ELEMENTS(base_dirs); i++) {
		gchar *path = g_build_filename(base, base_dirs[i][1], NULL);
		GString *path_str = g_string_new(path);
		gebr_path_set_to(path_str, TRUE);
		gebr_geoxml_line_append_path(line, base_dirs[i][0], path_str->str);
		g_free(path);
		g_string_free(path_str, TRUE);
	}
}

gchar *
gebr_geoxml_line_get_import_path(GebrGeoXmlLine *line){
	GebrGeoXmlSequence *seq;

	gebr_geoxml_line_get_path(line, &seq, 0);

	gchar *path_name;
	while (seq) {
		path_name = gebr_geoxml_line_path_get_name((GebrGeoXmlLinePath*) seq);
		if(g_strcmp0(path_name, "IMPORT")==0)
			return g_strdup(gebr_geoxml_value_sequence_get((GebrGeoXmlValueSequence*)seq));
		gebr_geoxml_sequence_next(&seq);
	}
	return "";
}
void gebr_geoxml_line_set_import_path(GebrGeoXmlLine *line, const gchar *import_path){
	GebrGeoXmlSequence *seq;

	gebr_geoxml_line_get_path(line, &seq, 0);

	gchar *path_name;
	while (seq) {
		//GebrLinePath *line_path = gebr_geoxml_line_get_path(line, &seq, 0);
		path_name = gebr_geoxml_line_path_get_name((GebrGeoXmlLinePath*) seq);
		if(g_strcmp0(path_name, "IMPORT")==0)
			break;
		gebr_geoxml_sequence_next(&seq);
	}

	if (seq)
		gebr_geoxml_value_sequence_set((GebrGeoXmlValueSequence*) seq, path_name);
	else
		gebr_geoxml_line_append_path(line, "IMPORT", import_path);
}

GebrGeoXmlLineFlow *gebr_geoxml_line_append_flow(GebrGeoXmlLine * line, const gchar * source)
{
	if (line == NULL)
		return NULL;

	GebrGeoXmlLineFlow *line_flow;

	line_flow = (GebrGeoXmlLineFlow *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "flow", NULL);
	__gebr_geoxml_set_attr_value((GdomeElement *) line_flow, "source", source);

	return line_flow;
}

int gebr_geoxml_line_get_flow(GebrGeoXmlLine * line, GebrGeoXmlSequence ** line_flow, gulong index)
{
	gint retval;
	if (line == NULL) {
		*line_flow = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	*line_flow = (GebrGeoXmlSequence *)__gebr_geoxml_get_element_at(root, "flow", index, FALSE);
	gdome_el_unref(root, &exception);

	retval = (*line_flow == NULL) ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
	return retval;
}

glong gebr_geoxml_line_get_flows_number(GebrGeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "flow");
}

void gebr_geoxml_line_set_flow_source(GebrGeoXmlLineFlow * line_flow, const gchar * source)
{
	if (line_flow == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) line_flow, "source", source);
}

const gchar *gebr_geoxml_line_get_flow_source(GebrGeoXmlLineFlow * line_flow)
{
	if (line_flow == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement *) line_flow, "source");
}

GebrGeoXmlLinePath *gebr_geoxml_line_append_path(GebrGeoXmlLine * line, const gchar *name, const gchar * path)
{
	if (line == NULL || path == NULL)
		return NULL;

	GdomeElement *line_path;
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	GdomeElement *element = __gebr_geoxml_get_first_element(root, "flow");

	line_path = __gebr_geoxml_insert_new_element(root, "path", element);
	__gebr_geoxml_set_attr_value(line_path, "name", name);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path);

	gebr_geoxml_object_unref(root);
	gebr_geoxml_object_unref(element);

	return (GebrGeoXmlLinePath *) line_path;
}

int gebr_geoxml_line_get_path(GebrGeoXmlLine * line, GebrGeoXmlSequence ** path, gulong index)
{
	if (line == NULL) {
		*path = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	*path = (GebrGeoXmlSequence *)__gebr_geoxml_get_element_at(root, "path", index, FALSE);

	gdome_el_unref(root, &exception);

	return (*path == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

gchar *
gebr_geoxml_line_path_get_name(GebrGeoXmlLinePath *line_path)
{
	return __gebr_geoxml_get_attr_value((GdomeElement*)line_path, "name");
}

glong gebr_geoxml_line_get_paths_number(GebrGeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	gulong retval = __gebr_geoxml_get_elements_number(root, "path");
	gdome_el_unref(root, &exception);
	return retval;
}

gchar *
gebr_geoxml_line_get_maestro(GebrGeoXmlLine *line)
{
	GdomeElement *root;
	GdomeElement *group_el;

	g_return_val_if_fail(line != NULL, NULL);

	root = gebr_geoxml_document_root_element(line);
	group_el = __gebr_geoxml_get_first_element(root, "nfs");

	gchar *nfsid = __gebr_geoxml_get_attr_value(group_el, "id");

	gdome_el_unref(root, &exception);
	gdome_el_unref(group_el, &exception);

	return nfsid;
}

gchar *
gebr_geoxml_line_get_maestro_label(GebrGeoXmlLine *line)
{
	GdomeElement *root;
	GdomeElement *group_el;

	g_return_val_if_fail(line != NULL, NULL);

	root = gebr_geoxml_document_root_element(line);
	group_el = __gebr_geoxml_get_first_element(root, "nfs");

	gchar *nfs_label = __gebr_geoxml_get_attr_value(group_el, "label");

	gdome_el_unref(root, &exception);
	gdome_el_unref(group_el, &exception);

	return nfs_label;
}

void
gebr_geoxml_line_set_maestro(GebrGeoXmlLine *line,
			     const gchar *nfsid)
{
	GdomeElement *root;
	GdomeElement *group_el;

	g_return_if_fail(line != NULL);
	g_return_if_fail(nfsid != NULL);

	root = gebr_geoxml_document_root_element(line);
	group_el = __gebr_geoxml_get_first_element(root, "nfs");

	__gebr_geoxml_set_attr_value(group_el, "id", nfsid);

	gdome_el_unref(root, &exception);
	gdome_el_unref(group_el, &exception);
}

gchar ***
gebr_geoxml_line_get_paths(GebrGeoXmlLine *line)
{
	gint len = 0;
	GList *list = NULL;
	GebrGeoXmlSequence *seq;
	gebr_geoxml_line_get_path(line, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		gchar **tmp = g_new(gchar *, 2);

		gchar *path_tmp = __gebr_geoxml_get_element_value((GdomeElement*)seq);
		GString *value_path = g_string_new(path_tmp);
		gebr_path_set_to(value_path, FALSE);

		tmp[0] = g_string_free(value_path, FALSE);
		tmp[1] = __gebr_geoxml_get_attr_value((GdomeElement*)seq, "name");
		list = g_list_prepend(list, tmp);
		len++;
	}

	list = g_list_reverse(list);
	gchar ***vect = g_new0(gchar **, len + 1);
	vect[len] = NULL;

	gint j = 0;
	for (GList *i = list; i; i = i->next)
		vect[j++] = i->data;

	g_list_free(list);

	return vect;
}

gchar *
gebr_geoxml_escape_path(const gchar *path)
{
	gchar **tmp = g_strsplit(path, ",", -1);
	gchar *ret = g_strjoinv(",,", tmp);
	g_strfreev(tmp);
	return ret;
}

gchar *
gebr_geoxml_get_paths_for_base(const gchar *base)
{
	GString *buf = g_string_new(NULL);

	for (gint i = 0; i < G_N_ELEMENTS(base_dirs); i++) {
		gchar *path = g_build_filename(base, base_dirs[i][1], NULL);
		GString *path_str = g_string_new(path);
		gebr_path_set_to(path_str, TRUE);
		g_string_append_c(buf, ',');
		g_string_append(buf, gebr_geoxml_escape_path(path_str->str));
		g_free(path);
		g_string_free(path_str, TRUE);
	}

	if (buf->len)
		g_string_erase(buf, 0, 1);

	return g_string_free(buf, FALSE);
}

GebrGeoXmlSequence *
gebr_geoxml_line_get_path_sequence_by_name(GebrGeoXmlLine *line,
					   const gchar *name)
{
	GebrGeoXmlSequence *seq;
	gebr_geoxml_line_get_path(line, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlLinePath *path = (GebrGeoXmlLinePath *)seq;
		gchar *tmp = gebr_geoxml_line_path_get_name(path);
		if (g_strcmp0(tmp, name) == 0) {
			g_free(tmp);
			return seq;
		}
		g_free(tmp);
	}
	return NULL;
}

void
gebr_geoxml_line_set_path_by_name(GebrGeoXmlLine *line,
				  const gchar *name,
				  const gchar *new_value)
{
	GebrGeoXmlSequence *seq = gebr_geoxml_line_get_path_sequence_by_name(line, name);

	if (!seq) {
		gebr_geoxml_line_append_path(line, name, new_value);
	} else {
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), new_value);
		gebr_geoxml_object_unref(seq);
	}
}

gchar *
gebr_geoxml_line_get_path_by_name(GebrGeoXmlLine *line,
				  const gchar *name)
{
	GebrGeoXmlSequence *seq = gebr_geoxml_line_get_path_sequence_by_name(line, name);

	if (!seq)
		return NULL;

	gchar *value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
	gebr_geoxml_object_unref(seq);
	return value;
}

gchar *
gebr_geoxml_line_create_key(const gchar *title)
{
	gchar *lower_no_accents = g_utf8_strdown(title, -1);
	lower_no_accents = gebr_g_string_remove_accents(lower_no_accents);
	GString *buffer = g_string_new(NULL);
	gchar *tmp = lower_no_accents;
	gunichar c;

	for (; tmp && *tmp; tmp = g_utf8_next_char(tmp)) {
		c = g_utf8_get_char(tmp);

		if (c == ' ' || !g_ascii_isalnum((char)c)) {
			g_string_append_c(buffer, '_');
		} else {
			gchar str[7];
			gint len = g_unichar_to_utf8(c, str);
			g_string_append_len(buffer, str, len);
		}
	}
	g_string_append_c(buffer, '\0');

	g_free(lower_no_accents);
	return g_string_free(buffer, FALSE);
}
