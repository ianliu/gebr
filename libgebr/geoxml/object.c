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

#include <string.h>
#include <gdome.h>
#include <utils.h>
#include <defines.h>

#include "document.h"
#include "sequence.h"
#include "flow.h"
#include "program.h"
#include "object.h"
#include "types.h"
#include "xml.h"
#include "value_sequence.h"
#include "document_p.h"
#include "gebr-geoxml-tmpl.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_object {
	GdomeElement *element;
};

/*
 * library functions.
 */

GebrGeoXmlObjectType gebr_geoxml_object_get_type(GebrGeoXmlObject * object)
{
	GdomeElement * element;

	static const gchar *tag_map[] = { "",
		"project", "line", "flow",
		"program", "parameters", "parameter",
		"option"
	};
	guint i;

	g_return_val_if_fail(object != NULL, GEBR_GEOXML_OBJECT_TYPE_UNKNOWN);

	if (gdome_n_nodeType((GdomeNode*)object, &exception) == GDOME_DOCUMENT_NODE)
		element = gdome_doc_documentElement((GdomeDocument*)object, &exception);
	else {
		gebr_geoxml_object_ref(object);
		element = (GdomeElement*)object;
	}
	GdomeDOMString *tag;
	for (i = 1; i <= 7; ++i) {
		tag = gdome_el_tagName(element, &exception);
		if (!strcmp(tag->str, tag_map[i])) {
			gdome_str_unref(tag);
			gdome_el_unref(element, &exception);
			return (GebrGeoXmlObjectType)i;
		}
		gdome_str_unref(tag);
	}
	gdome_el_unref(element, &exception);
	return GEBR_GEOXML_OBJECT_TYPE_UNKNOWN;
}

void gebr_geoxml_object_set_user_data(GebrGeoXmlObject * object, gpointer user_data)
{
	g_return_if_fail(object != NULL);

	GebrGeoXmlObjectType type = gebr_geoxml_object_get_type(object);
	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW || type == GEBR_GEOXML_OBJECT_TYPE_LINE || type == GEBR_GEOXML_OBJECT_TYPE_PROJECT)
		_gebr_geoxml_document_get_data(object)->user_data = user_data;
	else
		((GdomeNode *) object)->user_data = user_data;
}

gpointer gebr_geoxml_object_get_user_data(GebrGeoXmlObject * object)
{
	g_return_val_if_fail(object != NULL, NULL);

	GebrGeoXmlObjectType type = gebr_geoxml_object_get_type(object);
	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW || type == GEBR_GEOXML_OBJECT_TYPE_LINE || type == GEBR_GEOXML_OBJECT_TYPE_PROJECT)
		return _gebr_geoxml_document_get_data(object)->user_data;
	else
		return ((GdomeNode *) object)->user_data;
}

GebrGeoXmlDocument *gebr_geoxml_object_get_owner_document(GebrGeoXmlObject * object)
{
	g_return_val_if_fail(object != NULL, NULL);

	return (GebrGeoXmlDocument *) gdome_el_ownerDocument((GdomeElement *) object, &exception);
}

GebrGeoXmlObject *gebr_geoxml_object_copy(GebrGeoXmlObject * object)
{
	g_return_val_if_fail(object != NULL, NULL);

	return (GebrGeoXmlObject *) gdome_el_cloneNode((GdomeElement *) object, TRUE, &exception);
}

gchar *gebr_geoxml_object_generate_help (GebrGeoXmlObject *object, const gchar *content)
{
	GebrGeoXmlObjectType type;
	GebrGeoXmlDocument *doc;
	GebrGeoXmlProgram *prog;
	gboolean is_program;
	gchar *tmp;
	gchar *escaped;
	gchar *tmpl_str;
	GError *error = NULL;
	GString *tmpl;

	g_return_val_if_fail (object != NULL, NULL);

	type = gebr_geoxml_object_get_type (object);

	g_return_val_if_fail (type == GEBR_GEOXML_OBJECT_TYPE_FLOW ||
			      type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM,
			      NULL);

	if (!g_file_get_contents (LIBGEBR_HELP_TEMPLATE, &tmpl_str, NULL, &error))
	{
		g_warning ("Error loading template file: %s", error->message);
		g_clear_error (&error);
		return "";
	}

	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) {
		is_program = TRUE;
		prog = GEBR_GEOXML_PROGRAM (object);
		doc = gebr_geoxml_object_get_owner_document (object);
	} else {
		is_program = FALSE;
		doc = GEBR_GEOXML_DOCUMENT (object);
		gdome_doc_ref((GdomeDocument*)doc, &exception);
		prog = NULL;
	}

	tmpl = g_string_new (tmpl_str);

	// Set the content!
	gebr_geoxml_tmpl_set (tmpl, "cnt", content);

	// Set the title!
	tmp = is_program? gebr_geoxml_program_get_title (prog) : gebr_geoxml_document_get_title (doc);

	escaped = g_markup_escape_text (tmp, -1);
	if (strlen (escaped)) {
		gebr_geoxml_tmpl_set (tmpl, "ttl", escaped);
		gebr_geoxml_tmpl_set (tmpl, "tt2", escaped);
	}
	g_free (escaped);
	g_free(tmp);

	// Set the description!
	tmp = is_program? gebr_geoxml_program_get_description (prog) : gebr_geoxml_document_get_description (doc);

	escaped = g_markup_escape_text (tmp, -1);
	if (strlen (escaped))
		gebr_geoxml_tmpl_set (tmpl, "des", escaped);
	g_free (escaped);
	g_free(tmp);

	// Set the categories!
	GString *catstr;
	GebrGeoXmlSequence *cat;

	catstr = g_string_new ("");
	gebr_geoxml_flow_get_category (GEBR_GEOXML_FLOW (doc), &cat, 0);

	if (cat) {
		tmp = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (cat));
		escaped = g_markup_escape_text (tmp, -1);
		g_string_append (catstr, escaped);
		g_free (escaped);
		g_free(tmp);
		gebr_geoxml_sequence_next (&cat);
	}
	while (cat) {
		tmp = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (cat));
		escaped = g_markup_escape_text (tmp, -1);
		g_string_append_printf (catstr, " | %s", escaped);
		g_free (escaped);
		g_free(tmp);
		gebr_geoxml_sequence_next (&cat);
	}
	if (catstr->len)
		gebr_geoxml_tmpl_set (tmpl, "cat", catstr->str);
	g_string_free (catstr, TRUE);

	// Set the DTD!
	tmp = gebr_geoxml_document_get_version (doc);
	gebr_geoxml_tmpl_set (tmpl, "dtd", tmp);
	g_free(tmp);

	// Sets the version!
	if (is_program) {
		tmp = gebr_geoxml_program_get_version (prog);
		gebr_geoxml_tmpl_set (tmpl, "ver", tmp);
		g_free(tmp);
	} else
		gebr_geoxml_tmpl_set (tmpl, "ver",
				      gebr_date_get_localized ("%b %d, %Y", "C"));

	g_free(tmpl_str);
	gdome_doc_unref((GdomeDocument*)doc, &exception);

	return g_string_free (tmpl, FALSE);
}

gchar *gebr_geoxml_object_get_help_content (GebrGeoXmlObject *object)
{
	gchar *help;
	gchar *retval;
	GebrGeoXmlObjectType type;

	g_return_val_if_fail (object != NULL, NULL);

	type = gebr_geoxml_object_get_type (object);

	g_return_val_if_fail (type == GEBR_GEOXML_OBJECT_TYPE_FLOW ||
			      type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM,
			      NULL);

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		help = gebr_geoxml_document_get_help (GEBR_GEOXML_DOCUMENT (object));
	else
		help = gebr_geoxml_program_get_help (GEBR_GEOXML_PROGRAM (object));

	retval = gebr_geoxml_object_get_help_content_from_str (help);
	g_free (help);

	return retval;
}

gchar *gebr_geoxml_object_get_help_content_from_str (const gchar *str)
{
	gchar *inner;
	GRegex *regex;
	GString *html;
	GMatchInfo *match;

	html = g_string_new (str);
	inner = gebr_geoxml_tmpl_get (html, "cnt");

	if (inner) {
		g_string_free (html, TRUE);
		return inner;
	}

	// We only needed 'html' for gebr_geoxml_tmpl, free it now
	g_string_free (html, TRUE);

	//Old help does not have end content mark
	regex = g_regex_new ("<div class=\"content\">(.*?)</div>[^<]*<div class=\"navigation\">",
			     G_REGEX_DOTALL, 0, NULL);

	if (g_regex_match (regex, str, 0, &match)) {
		inner = g_match_info_fetch (match, 1);
		g_match_info_free (match);
		g_regex_unref (regex);
		return inner;
	}

	g_regex_unref (regex);
	regex = g_regex_new ("<body[^>]*>(.*?)<\\/body>",
			     G_REGEX_DOTALL, 0, NULL);

	if (g_regex_match (regex, str, 0, &match)) {
		inner = g_match_info_fetch (match, 1);
		g_match_info_free (match);
		g_regex_unref (regex);
		return inner;
	}

	g_regex_unref (regex);

	// We could not get the 'cnt' tag, nor the '<div class="content">', nor the <body>.
	// Return the same string we've got!
	return g_strdup (str);
}

void gebr_geoxml_object_set_help (GebrGeoXmlObject *object, const gchar *help)
{
	GebrGeoXmlObjectType type;

	g_return_if_fail (object != NULL);

	type = gebr_geoxml_object_get_type(object);

	g_return_if_fail (type == GEBR_GEOXML_OBJECT_TYPE_FLOW ||
			      type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM);

	if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
		gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(object), help);
	else
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(object), help);
}

void
gebr_geoxml_object_unref(gpointer object)
{
	gdome_el_unref((GdomeElement*)object, &exception);
}

void
gebr_geoxml_object_ref(gpointer object)
{
	gdome_el_ref((GdomeElement*)object, &exception);
}
