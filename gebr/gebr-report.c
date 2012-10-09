/*
 * gebr-report.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
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

#ifdef HAVE_CONFIG
# include <config.h>
#endif

#include "gebr-report.h"

struct _GebrReportPriv {
	GebrGeoXmlDocument *document;
};

gpointer
gebr_report_copy(gpointer boxed)
{
	GebrReport *report = boxed;
	GebrReport *copy = gebr_report_new(report->priv->document);
	return copy;
}

void
gebr_report_free(gpointer boxed)
{
	GebrReport *report = boxed;
	gebr_geoxml_document_unref(report->priv->document);
	g_free(report->priv);
	g_free(report);
}

GType
gebr_report_get_type(void)
{
	static GType type_id = 0;

	if (type_id == 0)
		type_id = g_boxed_type_register_static(
				g_intern_static_string("GebrReport"),
				gebr_report_copy,
				gebr_report_free);

	return type_id;
}

GebrReport *
gebr_report_new(GebrGeoXmlDocument *document)
{
	GebrReport *report = g_new0(GebrReport, 1);
	report->priv = g_new0(GebrReportPriv, 1);
	report->priv->document = gebr_geoxml_document_ref(document);
	return report;
}

gchar *
gebr_report_get_css_header_field(const gchar *filename, const gchar *field)
{
	GString *search;
	gchar *contents;
	gchar *escaped;
	gchar *word = NULL;

	if (!g_file_get_contents(filename, &contents, NULL, NULL))
		g_return_val_if_reached(NULL);

	escaped = g_regex_escape_string(field, -1);
	search = g_string_new("@");

	/* g_regex_escape_string don't escape '-',
	 * so we need to do it manually. */
	for (int i = 0; escaped[i]; i++) {
		if (escaped[i] != '-')
			g_string_append_c(search, escaped[i]);
		else
			g_string_append(search, "\\-");
	}
	g_free(escaped);
	g_string_append(search, ":(.*)");

	GMatchInfo * match_info;
	GRegex *regex = g_regex_new(search->str, G_REGEX_CASELESS, 0, NULL);
	g_regex_match(regex, contents, 0, &match_info);

	if (g_match_info_matches(match_info))
	{
		word = g_match_info_fetch (match_info, 1);
		g_strstrip(word);
	}

	g_string_free(search, TRUE);
	g_free(contents);
	g_match_info_free(match_info);
	g_regex_unref(regex);

	return word;
}
