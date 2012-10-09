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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gebr-report.h"
#include "ui_help.h" // For GebrHelpParamTable enum. TODO: Move this enum to gebr-report module!
//#include "gebr.h"
#include "document.h" // For document_load
#include <libgebr/utils.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>

struct _GebrReportPriv {
	GebrGeoXmlDocument *document;
	gboolean include_commentary : 1;
};


static void gebr_document_generate_flow_content(GebrGeoXmlDocument *document,
						GString *content,
						const gchar *inner_body,
						gboolean include_table,
						gboolean include_snapshots,
						gboolean include_comments,
						gboolean include_linepaths,
						const gchar *index,
						gboolean is_snapshot);

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

void
gebr_report_set_include_commentary(GebrReport *report, gboolean setting)
{
	report->priv->include_commentary = setting;
}

/*
 * gebr_document_report_get_inner_body:
 * @report: an html markup
 *
 * Returns: a newly allocated string containing the inner
 * html of the body tag from @report.
 */
static gchar *
gebr_document_report_get_inner_body(const gchar * report)
{
	GRegex * body;
	gchar * inner_body;
	GMatchInfo * match;

	body = g_regex_new("<body[^>]*>(.*?)<\\/body>",
			   G_REGEX_DOTALL, 0, NULL);

	if (!g_regex_match(body, report, 0, &match)) {
		g_regex_unref(body);
		return NULL;
	}

	inner_body = g_match_info_fetch(match, 1);

	g_match_info_free(match);
	g_regex_unref(body);
	return inner_body;
}

static void
gebr_document_generate_flow_revisions_content(GebrReport *report,
					      GebrGeoXmlDocument *flow,
                                              GString *content,
                                              const gchar *index,
                                              gboolean include_tables,
                                              gboolean include_snapshots)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gboolean include_comments;
	gboolean has_snapshots = FALSE;
	GString *snap_content = g_string_new(NULL);
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));

	GebrGeoXmlSequence *seq;
	gebr_geoxml_flow_get_revision(GEBR_GEOXML_FLOW(flow), &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		has_snapshots = TRUE;
		gchar *rev_xml;
		gchar *comment;
		gchar *date;
		GebrGeoXmlDocument *revdoc;

		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), &rev_xml, &date, &comment, NULL);

		if (gebr_geoxml_document_load_buffer(&revdoc, rev_xml) != GEBR_GEOXML_RETV_SUCCESS) {
			g_free(rev_xml);
			g_free(comment);
			g_free(date);
			g_warn_if_reached();
		}

		gchar *link = g_strdup_printf("%s%s%s%d",
		                              "snap",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		g_string_append_printf(snap_content,
		                       "  <div class=\"snapshot\">\n"
		                       "    <div class=\"header\">"
		                       "      <a name=\"%s\"></a>\n"
		                       "      <div class=\"title\">%s</div>\n"
		                       "      <div class=\"description\">%s%s</div>\n"
		                       "    </div>\n",
		                       link, comment, _("Snapshot taken on "), date);

		gchar *report_str = gebr_geoxml_document_get_help(revdoc);
		gchar *snap_inner_body = gebr_document_report_get_inner_body(report_str);

		include_comments = report->priv->include_commentary;

		gebr_document_generate_flow_content(revdoc, snap_content, snap_inner_body,
		                                    include_tables, include_snapshots,
		                                    include_comments, FALSE, link, TRUE);

		g_string_append(snap_content,
		                "  </div>");

		i++;

		g_free(rev_xml);
		g_free(comment);
		g_free(link);
		g_free(date);
		g_free(report_str);
		g_free(snap_inner_body);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(revdoc));
	}

	if (has_snapshots)
		g_string_append_printf(content,
		                       "<div class=\"snapshots\">\n"
		                       "  %s"
		                       "</div>\n",
		                       snap_content->str);

	g_string_free(snap_content, TRUE);
	g_free(flow_title);
}

/*
 * gebr_document_generate_report:
 * @document:
 *
 * Returns: a newly allocated string containing the header for document
 */
static gchar *
gebr_document_generate_header(GebrGeoXmlDocument * document,
                                      gboolean is_internal,
                                      const gchar *index)
{
	if (index == NULL)
		index = "";
	GString * dump;
	GebrGeoXmlDocumentType type;

	type = gebr_geoxml_document_get_type (document);

	dump = g_string_new(NULL);

	/* Title and Description of Document */
	gchar *title = gebr_geoxml_document_get_title(document);

	gchar *description = gebr_geoxml_document_get_description(document);
	g_string_printf(dump,
	                "<a name=\"%s\"></a>\n"
			"<div class=\"title\">%s</div>\n"
			"<div class=\"description\">%s</div>\n",
			is_internal? index : "external", title, description);
	g_free(title);
	g_free(description);

	/* Credits */
	gchar *author = gebr_geoxml_document_get_author(document);
	gchar *email = gebr_geoxml_document_get_email(document);
	if (!*email)
		g_string_append_printf(dump,
		                       "<div class=\"credits\">\n"
		                       "  <span class=\"author\">%s</span>\n"
		                       "</div>\n",
		                       author);
	else
		g_string_append_printf(dump,
		                       "<div class=\"credits\">\n"
		                       "  <span class=\"author\">%s</span>\n"
		                       "  <span class=\"email\">%s</span>\n"
		                       "</div>\n",
		                       author, email);
	g_free(author);
	g_free(email);


	/* Date when generate this report */
	const gchar *date = gebr_localized_date(gebr_iso_date());
	g_string_append_printf(dump,
	                       "<div class=\"date\">%s%s</div>\n",
	                       _("Report generated at "),
	                       date);

	/* If Document is Line, include Maestro on header */
	if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
		gchar *maestro = gebr_geoxml_line_get_maestro(GEBR_GEOXML_LINE(document));
		g_string_append_printf(dump,
		                       "<div class=\"maestro\">%s%s</div>\n",
		                       _("This line belongs to "),
		                       maestro);
		g_free(maestro);
	}

	return g_string_free(dump, FALSE);
}

static GList *
generate_list_from_match_info(GMatchInfo * match)
{
	GList * list = NULL;
	while (g_match_info_matches(match)) {
		list = g_list_prepend(list, g_match_info_fetch(match, 0));
		g_match_info_next(match, NULL);
	}
	return g_list_reverse(list);
}

/*
 * gebr_document_report_get_styles:
 * @report: an html markup
 *
 * Returns: a list of strings containing all styles inside @report. It may return complete <style> tags and <link
 * rel='stylesheet' ... > tags. You must free all strings and the list itself.
 */
static GList *
gebr_document_report_get_styles(const gchar * report)
{
	GList * list;
	GRegex * links;
	GRegex * styles;
	GMatchInfo * match;

	links = g_regex_new("<\\s*link\\s+rel\\s*=\\s*\"stylesheet\"[^>]*>",
			    G_REGEX_DOTALL, 0, NULL);

	styles = g_regex_new("<style[^>]*>.*?<\\/style>",
			     G_REGEX_DOTALL, 0, NULL);

	g_regex_match(links, report, 0, &match);
	list = generate_list_from_match_info(match);
	g_match_info_free(match);

	g_regex_match(styles, report, 0, &match);
	list = g_list_concat(list, generate_list_from_match_info(match));
	g_match_info_free(match);

	g_regex_unref(links);
	g_regex_unref(styles);

	return list;
}

/*
 * gebr_document_report_get_styles_string:
 * See gebr_document_report_get_styles().
 * Returns: a newly allocated string containing the styles concatenated with line breaks.
 */
static gchar *
gebr_document_report_get_styles_string(const gchar * report)
{
	GList * list, * i;
	GString * string;

	list = gebr_document_report_get_styles(report);

	if (list)
		string = g_string_new(list->data);
	else
		return g_strdup ("");

	i = list->next;
	while (i) {
		g_string_append_c(string, '\n');
		g_string_append(string, i->data);
		i = i->next;
	}

	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);

	return g_string_free(string, FALSE);
}

static gchar *
gebr_document_report_get_styles_css(gchar *report,GString *css)
{
	gchar * styles = "";
	if (css->len != 0)
		styles = g_strdup_printf ("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/%s\" />",
					  LIBGEBR_STYLES_DIR, css->str);
	else
		styles = gebr_document_report_get_styles_string (report);

	return styles;
}

static void
gebr_document_create_section(GString *destiny,
		const gchar *source,
		const gchar *class_name)
{
	g_string_append_printf(destiny, "<div class=\"%s\">%s</div>\n ", class_name, source);
}

static void
gebr_document_generate_project_index_content(GebrGeoXmlDocument *document,
                                             GString *index_content)
{
	GebrGeoXmlSequence *lines;

	gebr_geoxml_project_get_line(GEBR_GEOXML_PROJECT(document), &lines, 0);
	while (lines) {
		const gchar *lname;
		GebrGeoXmlDocument *line;

		lname = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(lines));
		document_load(&line, lname, FALSE);

		gchar *title = gebr_geoxml_document_get_title(line);
		gchar *link = g_strdup(title);
		link = g_strdelimit(link, " ", '_');
		gchar *description = gebr_geoxml_document_get_description(line);
		g_string_append_printf (index_content,
		                        "      <li>\n"
		                        "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                        "        <span class=\"description\">%s</span>\n"
		                        "      </li>\n",
		                        link, title, description);
		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(line));
		gebr_geoxml_sequence_next (&lines);
	}
}

static void
gebr_document_generate_line_index_content(GebrGeoXmlDocument *document,
                                          GString *index_content)
{
	gint i = 1;
	GebrGeoXmlSequence *flows;

	gebr_geoxml_line_get_flow (GEBR_GEOXML_LINE (document), &flows, 0);
	while (flows) {
		const gchar *fname;
		GebrGeoXmlDocument *flow;

		fname = gebr_geoxml_line_get_flow_source (GEBR_GEOXML_LINE_FLOW (flows));
		document_load(&flow, fname, FALSE);

		gchar *title = gebr_geoxml_document_get_title(flow);

		gchar *link = g_strdup_printf("%d", i);

		gchar *description = gebr_geoxml_document_get_description(flow);
		g_string_append_printf (index_content,
		                        "      <li>\n"
		                        "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                        "        <span class=\"description\">%s</span>\n"
		                        "      </li>\n",
		                        link, title, description);

		i++;

		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
		gebr_geoxml_sequence_next (&flows);
	}
}

static void
gebr_document_generate_flow_index_content(GebrGeoXmlDocument *document,
                                          GString *index_content,
                                          const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gchar *flow_title = gebr_geoxml_document_get_title(document);
	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program (GEBR_GEOXML_FLOW(document), &program, 0);
	while (program) {
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;
		gchar *title;
		gchar *description;

		prog = GEBR_GEOXML_PROGRAM (program);
		status = gebr_geoxml_program_get_status (prog);
		title = gebr_geoxml_program_get_title (prog);
		description = gebr_geoxml_program_get_description(prog);

		gchar *link = g_strdup_printf("%s%s%d",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			g_string_append_printf(index_content,
			                       "      <li>\n"
			                       "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
			                       "        <span class=\"description\">%s</span>\n"
			                       "      </li>\n",
			                       link, title, description);

		i++;

		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_sequence_next (&program);
	}
	g_free(flow_title);
}

static void
gebr_document_generate_index(GebrGeoXmlDocument *document,
                             GString *content,
                             GebrGeoXmlDocumentType type,
                             const gchar *index,
                             const gchar *description)
{
	GString *index_content = g_string_new(NULL);

	if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_document_generate_project_index_content(document, index_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		gebr_document_generate_line_index_content(document, index_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		gebr_document_generate_flow_index_content(document, index_content, index);

	else
		g_warn_if_reached();

	g_string_append_printf(content,
	                       "<div class=\"index\">\n"
	                       "  %s\n"
	                       "  <ul>\n"
	                       "    %s\n"
	                       "  </ul>\n"
	                       "</div>\n",
	                       description, index_content->str);

	g_string_free(index_content, TRUE);
}

static void
gebr_document_generate_line_paths(GebrGeoXmlDocument *document,
                                  GString *content)
{
	GebrGeoXmlLine *line = GEBR_GEOXML_LINE(document);

	if (gebr_geoxml_line_get_paths_number(line) > 0) {
		// Comment for translators: HTML header for detailed report
		g_string_append_printf(content,
		                       "<div class=\"paths\">\n"
		                       "  <table>\n"
		                       "    <caption>%s</caption>\n"
		                       "    <tbody>\n",
		                       _("Line paths"));

		GString *buf = g_string_new(NULL);
		gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(document));
		for (gint i = 0; paths[i]; i++) {
			if (!*paths[i][0])
				continue;

			if (!g_strcmp0(paths[i][1], "HOME")) {
				g_string_append_printf(content,
				                       "   <tr>\n"
				                       "     <td class=\"type\">&lt;%s&gt;</td>\n"
				                       "     <td class=\"value\">%s</td>\n"
				                       "   </tr>\n",
				                       paths[i][1], paths[i][0]);
				continue;
			}

			gchar *resolved = gebr_resolve_relative_path(paths[i][0], paths);
			g_string_append_printf(buf,
			                       "   <tr>"
			                       "     <td class=\"type\">&lt;%s&gt;</td>\n"
			                       "     <td class=\"value\">%s</td>\n"
			                       "   </tr>",
			                       paths[i][1], resolved);
			g_free(resolved);
		}
		g_string_append(content, buf->str);
		g_string_append(content,
		                "    </tbody>\n"
				"  </table>\n"
		                "</div>\n");

		g_string_free(buf, TRUE);
		gebr_pairstrfreev(paths);
	}
}

static void
gebr_document_generate_project_tables_content(GebrGeoXmlDocument *document,
                                              GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(document, TRUE, TRUE, tables_content, _("Project"));
}

static void
gebr_document_generate_line_tables_content(GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE, tables_content, _("Project"));
	gebr_generate_variables_value_table(document, FALSE, TRUE, tables_content, _("Line"));
}

static void
gebr_document_generate_flow_tables_content(GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE, tables_content, _("Project"));
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.line), FALSE, FALSE, tables_content, _("Line"));
	gebr_generate_variables_value_table(document, FALSE, TRUE, tables_content, _("Flow"));

	/* I/O table */
	gebr_flow_generate_io_table(GEBR_GEOXML_FLOW(document), tables_content);
}

static void
gebr_document_generate_tables(GebrGeoXmlDocument *document,
                              GString *content,
                              GebrGeoXmlDocumentType type)
{
	GString *tables_content = g_string_new(NULL);

	if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_document_generate_project_tables_content(document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		gebr_document_generate_line_tables_content(document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		gebr_document_generate_flow_tables_content(document, tables_content);

	else
		g_warn_if_reached();

	g_string_append_printf(content,
	                       "<div class=\"tables\">\n"
	                       "  %s\n"
	                       "</div>\n",
	                       tables_content->str);

	g_string_free(tables_content, TRUE);
}

static void
gebr_document_generate_flow_programs(GebrGeoXmlDocument *document,
                                     GString *content,
                                     const gchar *index)
{
	gebr_flow_generate_parameter_value_table(GEBR_GEOXML_FLOW (document), content, index, FALSE);
}

static void
gebr_document_generate_flow_content(GebrGeoXmlDocument *document,
                                    GString *content,
                                    const gchar *inner_body,
                                    gboolean include_table,
                                    gboolean include_snapshots,
                                    gboolean include_comments,
                                    gboolean include_linepaths,
                                    const gchar *index,
                                    gboolean is_snapshot)
{
	gboolean has_snapshots = (gebr_geoxml_flow_get_revisions_number(GEBR_GEOXML_FLOW(document)) > 0);

	if (has_snapshots && include_snapshots)
		gebr_flow_generate_flow_revisions_index(GEBR_GEOXML_FLOW(document), content, index);

	if (include_comments && inner_body)
		gebr_document_create_section(content, inner_body, "comments");

	gebr_document_generate_index(document, content, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, index,
	                             is_snapshot? _("Snapshot composed by the program(s):") : _("Flow composed by the program(s):"));

	if (include_table) {
		if (include_linepaths)
			gebr_document_generate_line_paths(GEBR_GEOXML_DOCUMENT(gebr.line), content);
		gebr_document_generate_tables(document, content, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		gebr_document_generate_flow_programs(document, content, index);
	}

	if (has_snapshots && include_snapshots)
		gebr_document_generate_flow_revisions_content(document, content, index, include_table, include_comments);
}

static void
gebr_document_generate_internal_flow(GebrGeoXmlDocument *document,
                                     GString *content)
{
	gint i = 1;
	GebrGeoXmlSequence *line_flow;

	gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(document), &line_flow, 0);

	g_string_append(content,
	                "      <div class=\"contents\">\n");

	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		GebrGeoXmlFlow *flow;
		gboolean include_table;
		gboolean include_snapshots;
		gboolean include_comments;

		include_table = gebr.config.detailed_line_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		include_snapshots = gebr.config.detailed_line_include_revisions_report;
		include_comments = gebr.config.detailed_line_include_report;

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		document_load((GebrGeoXmlDocument**)(&flow), filename, FALSE);
		gebr_validator_set_document(gebr.validator,(GebrGeoXmlDocument**)(&flow), GEBR_GEOXML_DOCUMENT_TYPE_FLOW, TRUE);

		gchar *report = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(flow));
		gchar *flow_inner_body = gebr_document_report_get_inner_body(report);

		gchar *index = g_strdup_printf("%d", i);

		GString *flow_content = g_string_new(NULL);
		gchar *header = gebr_document_generate_header(GEBR_GEOXML_DOCUMENT(flow), TRUE, index);
		gebr_document_generate_flow_content(GEBR_GEOXML_DOCUMENT(flow), flow_content,
		                                    flow_inner_body, include_table,
		                                    include_snapshots, include_comments, FALSE, index, FALSE);

		gchar *internal_html = gebr_generate_content_report("flow", header, flow_content->str);

		g_string_append_printf(content,
		                       "        %s",
		                       internal_html);

		i++;

		g_free(flow_inner_body);
		g_free(report);
		g_free(internal_html);
		g_free(index);
		g_string_free(flow_content, TRUE);

		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}
	g_string_append(content,
	                "      </div>\n");

	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**)(&gebr.flow), GEBR_GEOXML_DOCUMENT_TYPE_FLOW, TRUE);
}

static void
gebr_document_generate_line_content(GebrGeoXmlDocument *document,
                                    GString *content,
                                    const gchar *inner_body,
                                    gboolean include_table)
{
	if (gebr.config.detailed_line_include_report && inner_body)
		gebr_document_create_section(content, inner_body, "comments");

	gebr_document_generate_index(document, content,
	                             GEBR_GEOXML_DOCUMENT_TYPE_LINE, NULL,
	                             _("Line composed by the Flow(s):"));

	if (include_table) {
		gebr_document_generate_line_paths(document, content);
		gebr_document_generate_tables(document, content, GEBR_GEOXML_DOCUMENT_TYPE_LINE);
	}

	if (gebr.config.detailed_line_include_flow_report)
		gebr_document_generate_internal_flow(document, content);
}

gchar *
gebr_report_generate(GebrReport *report)
{
	GebrGeoXmlObjectType type;
	GebrGeoXmlDocument *document = report->priv->document;
	gchar * title;
	gchar * report_str;
	gchar * detailed_html = "";
	gchar * inner_body = "";
	gchar * styles = "";
	gchar * header = "";
	gchar *scope;
	GString * content;

	content = g_string_new (NULL);

	type = gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(document));
	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) 
		return NULL;

	title = gebr_geoxml_document_get_title(document);
	report_str = gebr_geoxml_document_get_help(document);
	inner_body = gebr_document_report_get_inner_body(report_str);
	header = gebr_document_generate_header(document, FALSE, NULL);

	if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
		scope = g_strdup(_("line"));

		styles = gebr_document_report_get_styles_css(report_str, gebr.config.detailed_line_css);

		gboolean include_table = gebr.config.detailed_line_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;

		gebr_document_generate_line_content(document, content, inner_body, include_table);

	} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		scope = g_strdup(_("flow"));

		styles = gebr_document_report_get_styles_css(report_str, gebr.config.detailed_flow_css);

		gboolean include_table = gebr.config.detailed_flow_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		gboolean include_snapshots = gebr.config.detailed_flow_include_revisions_report;
		gboolean include_comments = gebr.config.detailed_flow_include_report;

		gebr_document_generate_flow_content(document, content, inner_body,
		                                    include_table, include_snapshots,
		                                    include_comments, TRUE, NULL, FALSE);

	} else if (type == GEBR_GEOXML_OBJECT_TYPE_PROJECT) {
		scope = g_strdup(_("project"));
		g_free (inner_body);
		return report_str;
	} else {
		g_free (inner_body);
		g_free (report_str);
		g_return_val_if_reached (NULL);
	}

	detailed_html = gebr_generate_report(title, styles, scope, header, content->str);

	g_free(header);
	g_free(styles);
	g_free(inner_body);
	g_string_free(content, TRUE);
	g_free (report_str);
	g_free(title);
	g_free(scope);

	return detailed_html;
}

static void
gebr_generate_variables_header_table(GString *tables_content,
                                     gboolean insert_header,
                                     const gchar *scope)
{
	if (insert_header)
		g_string_append_printf(tables_content,
		                       "  <div class=\"variables\">\n"
		                       "    <table>\n"
		                       "      <caption>%s</caption>\n"
		                       "      <thead>\n"
		                       "        <tr>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "          <td>%s</td>\n"
		                       "        </tr>\n"
		                       "      </thead>\n"
		                       "      <tbody>\n",
		                       _("Variables"), _("Keyword"), _("Value"),_("Result"), _("Comment"));


	g_string_append_printf(tables_content,
	                       "      <tr class=\"scope\">\n"
	                       "        <td colspan=\"4\">%s</td>\n"
	                       "      </tr>\n",
	                       scope);
}

void
gebr_generate_variables_value_table(GebrGeoXmlDocument *doc,
                                    gboolean insert_header,
                                    gboolean close,
                                    GString *tables_content,
                                    const gchar *scope)
{
	/* Set validator to @doc */
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &doc, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	gebr_generate_variables_header_table(tables_content, insert_header, scope);

	GebrGeoXmlParameters *params;
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlParameterType type;

	gchar *name;
	gchar *value;
	gchar *comment;
	gchar *eval = NULL;
	gchar *result = NULL;
	gchar *var_type;
	gboolean have_vars = FALSE;
	GError *error = NULL;

	params = gebr_geoxml_document_get_dict_parameters(doc);
	gebr_geoxml_parameters_get_parameter(params, &sequence, 0);
	gebr_geoxml_object_unref(params);
	while (sequence) {
		have_vars = TRUE;
		name = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(sequence));
		value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(sequence), FALSE);
		comment = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(sequence));

		gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
		gchar *mount_point;
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
		if (maestro)
			mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
		else
			mount_point = NULL;
		value = gebr_relativise_path(value,mount_point,paths);
		value = g_markup_printf_escaped("%s",value);
		gebr_validator_evaluate_param(gebr.validator, GEBR_GEOXML_PARAMETER(sequence), &eval, &error);

		if (!error) {
			result = g_strdup(eval);
			type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(sequence));
			if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING)
				var_type = g_strdup("string");
			else
				var_type = g_strdup("numeric");
		} else {
			result = g_strdup(error->message);
			var_type = g_strdup("error");
		}

		g_string_append_printf(tables_content,
		                       "      <tr class=\"scope-content %s\">"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "      </tr>\n",
		                       var_type, name, value, result, comment);

		g_free(name);
		g_free(value);
		g_free(comment);
		g_free(result);

		if (error)
			g_clear_error(&error);
		else
			g_free(eval);

		gebr_geoxml_sequence_next(&sequence);
	}
	if (error)
		g_error_free(error);

	if(!have_vars)
		g_string_append_printf(tables_content,
		                       "      <tr class=\"scope-content\">\n"
		                       "        <td colspan='4'>%s</td>\n"
		                       "      </tr>\n",
		                       _("There are no variables in this scope"));

	if (close)
		g_string_append (tables_content,
		                 "      </tbody>\n"
		                 "    </table>\n"
		                 "  </div>\n");

	/* Set default validator */
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
}

void
gebr_flow_generate_io_table(GebrGeoXmlFlow *flow,
                            GString *tables_content)
{
	gchar * input;
	gchar * output;
	gchar * error;

	input = gebr_geoxml_flow_io_get_input_real (flow);
	output = gebr_geoxml_flow_io_get_output_real (flow);
	error = gebr_geoxml_flow_io_get_error (flow);

	input = g_markup_printf_escaped("%s",input);
	output = g_markup_printf_escaped("%s",output);
	error = g_markup_printf_escaped("%s",error);

	g_string_append_printf(tables_content,
	                       "  <div class=\"io\">\n"
	                       "    <table>\n"
	                       "      <caption>%s</caption>\n"
	                       "      <tbody>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "        <tr>\n"
	                       "          <td class=\"type\">%s</td>\n"
	                       "          <td class=\"value\">%s</td>\n"
	                       "        </tr>\n"
	                       "      </tbody>\n"
	                       "    </table>\n"
	                       "  </div>\n",
	                       _("I/O table"),
	                       _("Input"), strlen (input) > 0? input : _("(none)"),
	                       _("Output"), strlen (output) > 0? output : _("(none)"),
	                       _("Error"), strlen (error) > 0? error : _("(none)"));

	g_free(input);
	g_free(output);
	g_free(error);
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

void
gebr_flow_generate_flow_revisions_index(GebrGeoXmlFlow *flow,
                                        GString *content,
                                        const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gboolean has_snapshots = FALSE;
	GString *snapshots = g_string_new(NULL);
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));

	GebrGeoXmlSequence *seq;
	gebr_geoxml_flow_get_revision(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		has_snapshots = TRUE;
		gchar *comment;
		gchar *date;

		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), NULL, &date, &comment, NULL);

		gchar *link = g_strdup_printf("%s%s%s%d",
		                              "snap",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		g_string_append_printf(snapshots,
		                       "      <li>\n"
		                       "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                       "        <span class=\"description\">%s</span>\n"
		                       "      </li>\n",
		                       link, comment, date);

		i++;


		g_free(comment);
		g_free(date);
		g_free(link);
	}

	if (has_snapshots)
		g_string_append_printf(content,
		                       "<div class=\"index\">\n"
		                       "  %s\n"
		                       "  <ul>\n"
		                       "    %s\n"
		                       "  </ul>\n"
		                       "</div>\n",
		                       _("Snapshots of this Flow:"),
		                       snapshots->str);

	g_string_free(snapshots, TRUE);
	g_free(flow_title);
}
