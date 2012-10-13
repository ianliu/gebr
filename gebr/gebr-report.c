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

#include "document.h"
#include "ui_flow_program.h"
#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>

struct _GebrReportPriv {
	GebrGeoXmlDocument *document;

	/* For getting dictionary */
	GebrGeoXmlDocument *dict_line;
	GebrGeoXmlDocument *dict_project;

	GebrValidator *validator;
	GebrMaestroController *maestro_controller;

	gchar *css_url;
	gchar *error_message;
	GebrHelpParamTable detailed_parameter_table;
	gboolean include_commentary : 1;
	gboolean include_revisions : 1;
	gboolean include_flow_report : 1;
};

static void gebr_document_generate_flow_content(GebrReport *report,
						GebrGeoXmlDocument *document,
						GString *content,
						const gchar *inner_body,
						gboolean include_table,
						gboolean include_snapshots,
						gboolean include_comments,
						gboolean include_linepaths,
						const gchar *index,
						gboolean is_snapshot);

static void gebr_generate_variables_value_table(GebrReport *report,
						GebrGeoXmlDocument *doc,
						gboolean insert_header,
						gboolean close,
						GString *tables_content,
						const gchar *scope);

static void gebr_flow_generate_flow_revisions_index(GebrGeoXmlFlow *flow,
						    GString *content,
						    const gchar *index);

static void gebr_flow_generate_io_table(GebrGeoXmlFlow *flow,
					GString *tables_content);

static void gebr_flow_generate_parameter_value_table(GebrReport *report,
						     GebrGeoXmlFlow *flow,
						     GString *prog_content,
						     const gchar *index);

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
gebr_report_set_validator(GebrReport *report, GebrValidator *validator)
{
	report->priv->validator = validator;
}

void
gebr_report_set_maestro_controller(GebrReport *report, GebrMaestroController *maestro_controller)
{
	if (report->priv->maestro_controller)
		g_object_unref(report->priv->maestro_controller);

	report->priv->maestro_controller = g_object_ref(maestro_controller);
}

void
gebr_report_set_css_url(GebrReport *report, const gchar *css_url)
{
	if (report->priv->css_url)
		g_free(report->priv->css_url);

	report->priv->css_url = g_strdup(css_url);
}

void
gebr_report_set_include_commentary(GebrReport *report, gboolean setting)
{
	report->priv->include_commentary = setting;
}

void
gebr_report_set_include_revisions(GebrReport *report, gboolean setting)
{
	report->priv->include_revisions = setting;
}

void
gebr_report_set_detailed_parameter_table(GebrReport *report, GebrHelpParamTable detailed_parameter_table)
{
	report->priv->detailed_parameter_table = detailed_parameter_table;
}

void
gebr_report_set_include_flow_report(GebrReport *report, gboolean setting)
{
	report->priv->include_flow_report = setting;
}

void
gebr_report_set_dictionary_documents(GebrReport *report,
				     GebrGeoXmlDocument *line,
				     GebrGeoXmlDocument *project)
{
	if (report->priv->dict_line)
		gebr_geoxml_document_unref(report->priv->dict_line);

	if (report->priv->dict_project)
		gebr_geoxml_document_unref(report->priv->dict_project);

	report->priv->dict_line = gebr_geoxml_document_ref(line);
	report->priv->dict_project = gebr_geoxml_document_ref(project);
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

		gebr_document_generate_flow_content(report, revdoc, snap_content, snap_inner_body,
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

	/* If Document is line, include Maestro on header */
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
gebr_document_report_get_styles_css(gchar *report, const gchar *css)
{
	gchar * styles = "";
	if (css[0] != '\0')
		styles = g_strdup_printf ("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/%s\" />",
					  LIBGEBR_STYLES_DIR, css);
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
gebr_document_generate_project_tables_content(GebrReport *report,
					      GebrGeoXmlDocument *document,
                                              GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(report, document, TRUE, TRUE, tables_content, _("Project"));
}

static void
gebr_document_generate_line_tables_content(GebrReport *report,
					   GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	if (report->priv->dict_project)
		gebr_generate_variables_value_table(report, report->priv->dict_project,
				TRUE, FALSE, tables_content, _("Project"));

	gebr_generate_variables_value_table(report, document,
			FALSE, TRUE, tables_content, _("Line"));
}

static void
gebr_document_generate_flow_tables_content(GebrReport *report,
					   GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	if (report->priv->dict_project)
		gebr_generate_variables_value_table(report, report->priv->dict_project,
				TRUE, FALSE, tables_content, _("Project"));

	if (report->priv->dict_line)
		gebr_generate_variables_value_table(report, report->priv->dict_line,
				FALSE, FALSE, tables_content, _("Line"));

	gebr_generate_variables_value_table(report, document, FALSE, TRUE, tables_content, _("Flow"));

	gebr_flow_generate_io_table(GEBR_GEOXML_FLOW(document), tables_content);
}

static void
gebr_document_generate_tables(GebrReport *report,
			      GebrGeoXmlDocument *document,
                              GString *content,
                              GebrGeoXmlDocumentType type)
{
	GString *tables_content = g_string_new(NULL);

	if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_document_generate_project_tables_content(report, document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		gebr_document_generate_line_tables_content(report, document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		gebr_document_generate_flow_tables_content(report, document, tables_content);

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
gebr_document_generate_flow_programs(GebrReport *report,
				     GebrGeoXmlDocument *document,
                                     GString *content,
                                     const gchar *index)
{
	gebr_flow_generate_parameter_value_table(report, GEBR_GEOXML_FLOW (document), content, index);
}

static void
gebr_document_generate_flow_content(GebrReport *report,
				    GebrGeoXmlDocument *document,
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
			gebr_document_generate_line_paths(report->priv->dict_line, content);
		gebr_document_generate_tables(report, document, content, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		gebr_document_generate_flow_programs(report, document, content, index);
	}

	if (has_snapshots && include_snapshots)
		gebr_document_generate_flow_revisions_content(report, document, content, index, include_table, include_comments);
}

static void
gebr_document_generate_internal_flow(GebrReport *report,
				     GebrGeoXmlDocument *document,
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

		include_table = report->priv->detailed_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		include_snapshots = report->priv->include_revisions;
		include_comments = report->priv->include_commentary;

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		document_load((GebrGeoXmlDocument**)(&flow), filename, FALSE);
		gebr_validator_push_document(report->priv->validator, (GebrGeoXmlDocument**)(&flow), GEBR_GEOXML_DOCUMENT_TYPE_FLOW);

		gchar *report_str = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(flow));
		gchar *flow_inner_body = gebr_document_report_get_inner_body(report_str);

		gchar *index = g_strdup_printf("%d", i);

		GString *flow_content = g_string_new(NULL);
		gchar *header = gebr_document_generate_header(GEBR_GEOXML_DOCUMENT(flow), TRUE, index);
		gebr_document_generate_flow_content(report, GEBR_GEOXML_DOCUMENT(flow), flow_content,
		                                    flow_inner_body, include_table,
		                                    include_snapshots, include_comments, FALSE, index, FALSE);

		gchar *internal_html = gebr_generate_content_report("flow", header, flow_content->str);

		g_string_append_printf(content, "        %s", internal_html);
		gebr_validator_pop_document(report->priv->validator, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);

		i++;

		g_free(flow_inner_body);
		g_free(report_str);
		g_free(internal_html);
		g_free(index);
		g_string_free(flow_content, TRUE);

		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}
	g_string_append(content, "      </div>\n");
}

static void
gebr_document_generate_line_content(GebrReport *report,
				    GebrGeoXmlDocument *document,
                                    GString *content,
                                    const gchar *inner_body,
                                    gboolean include_table)
{
	if (report->priv->include_commentary && inner_body)
		gebr_document_create_section(content, inner_body, "comments");

	gebr_document_generate_index(document, content,
	                             GEBR_GEOXML_DOCUMENT_TYPE_LINE, NULL,
	                             _("Line composed by the flow(s):"));

	if (include_table) {
		gebr_document_generate_line_paths(document, content);
		gebr_document_generate_tables(report, document, content, GEBR_GEOXML_DOCUMENT_TYPE_LINE);
	}

	if (report->priv->include_flow_report)
		gebr_document_generate_internal_flow(report, document, content);
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

		styles = gebr_document_report_get_styles_css(report_str, report->priv->css_url);

		gboolean include_table = report->priv->detailed_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;

		gebr_document_generate_line_content(report, document, content, inner_body, include_table);

	} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		scope = g_strdup(_("flow"));

		styles = gebr_document_report_get_styles_css(report_str, report->priv->css_url);

		gboolean include_table = report->priv->detailed_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		gboolean include_snapshots = report->priv->include_revisions;
		gboolean include_comments = report->priv->include_commentary;

		gebr_document_generate_flow_content(report, document, content, inner_body,
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

void
gebr_report_set_error_message(GebrReport *report, const gchar *error_message)
{
	if (report->priv->error_message)
		g_free(report->priv->error_message);

	report->priv->error_message = g_strdup(error_message);
}

gchar *
gebr_report_generate_flow_review(GebrReport *report)
{
	GString *prog_content = g_string_new("");
	g_string_append_printf(prog_content, "<html>\n"
					     "  <head>\n"
					     "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	g_string_append_printf(prog_content, "    <link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/gebr-report.css\" />"
					     "    <link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/gebr-flow-review.css\" />",
						  LIBGEBR_STYLES_DIR, LIBGEBR_DATA_DIR);

	gboolean debug_is_on = FALSE;
#ifdef DEBUG
	debug_is_on = TRUE;
#endif
	if (debug_is_on)
		g_string_append_printf(prog_content, "  </head>\n"
						     "  <body>\n");
	else
		g_string_append_printf(prog_content, "  </head>\n"
						     "  <body oncontextmenu=\"return false;\"/>\n");

	if (report->priv->error_message && *report->priv->error_message)
		g_string_append_printf(prog_content, "<div class='flow-error'>\n  <p>%s</p>\n</div>\n",
				report->priv->error_message);

	gebr_flow_generate_io_table(GEBR_GEOXML_FLOW(report->priv->document), prog_content);
	gebr_flow_generate_parameter_value_table(report, GEBR_GEOXML_FLOW(report->priv->document), prog_content, NULL);

	g_string_append_printf(prog_content, "  </body>\n</html>");

	return g_string_free(prog_content, FALSE);
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

/*
 * gebr_flow_generate_variables_value_table:
 * @doc: a #GebrGeoXmlDocument
 * @insert_header: Pass %TRUE for include header, %FALSE otherwise
 * @close: Pass %TRUE for close table of header, %FALSE otherwise
 * @tables_content: a #GString to append content
 * @scope: a string with title of scope to include variables
 *
 * Creates a string containing a HTML table for the variables on dictionary of @doc.
 */
static void
gebr_generate_variables_value_table(GebrReport *report,
				    GebrGeoXmlDocument *doc,
                                    gboolean insert_header,
                                    gboolean close,
                                    GString *tables_content,
                                    const gchar *scope)
{
	/* Set validator to @doc */
	gebr_validator_push_document(report->priv->validator, (GebrGeoXmlDocument**) &doc, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);

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

		gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(report->priv->dict_line));
		gchar *mount_point;
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(report->priv->maestro_controller,
				GEBR_GEOXML_LINE(report->priv->dict_line));
		if (maestro)
			mount_point = gebr_maestro_info_get_home_mount_point(gebr_maestro_server_get_info(maestro));
		else
			mount_point = NULL;
		value = gebr_relativise_path(value,mount_point,paths);
		value = g_markup_printf_escaped("%s",value);
		gebr_validator_evaluate_param(report->priv->validator, GEBR_GEOXML_PARAMETER(sequence), &eval, &error);

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
	gebr_validator_pop_document(report->priv->validator, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
}


/*
 * gebr_flow_generate_io_table:
 * @flow: a #GebrGeoXmlFlow
 * @tables_content: A #GString to append content
 *
 * Creates a string containing a HTML table for I/O of @flow.
 */
static void
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

/*
 * gebr_flow_generate_flow_revisions_index:
 * @flow:
 * @content:
 *
 * Concatenate on @content a HTML with revisions content
 */
static void
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

static void append_parameter_row(GebrReport *report,
				 GebrGeoXmlParameter * parameter,
                                 GString * dump)
{
	gint i, n_instances;
	GebrGeoXmlSequence * param;
	GebrGeoXmlSequence * instance;
	GebrGeoXmlParameters * parameters;
	GebrGeoXmlDocumentType type;

	type = gebr_geoxml_document_get_type(report->priv->document);

	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)) {
		GString * str_value;
		GString * default_value;
		GebrGeoXmlProgramParameter * program;
		gint radio_value = GEBR_PARAM_TABLE_NO_TABLE;

		program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
		str_value = gebr_geoxml_program_parameter_get_string_value(program, FALSE);
                default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);

                gboolean is_required = gebr_geoxml_program_parameter_get_required(program);

		switch (type) {
			case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
			case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
				radio_value = report->priv->detailed_parameter_table;
				break;
			default:
				radio_value = GEBR_PARAM_TABLE_ONLY_CHANGED;
				break;
		}

		if (((radio_value == GEBR_PARAM_TABLE_ONLY_CHANGED) && (g_strcmp0(str_value->str, default_value->str) != 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ONLY_FILLED) && (str_value->len > 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ALL)) || is_required)
		{
			/* Translating enum values to labels */
			GebrGeoXmlSequence *enum_option = NULL;

			gebr_geoxml_program_parameter_get_enum_option(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), &enum_option, 0);

			for (; enum_option; gebr_geoxml_sequence_next(&enum_option))
			{
				gchar *enum_value = gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option));
				if (g_strcmp0(str_value->str, enum_value) == 0)
				{
					gchar *label = gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option));
					g_string_printf(str_value, "%s", label);
					g_free(enum_value);
					g_free(label);
					gebr_geoxml_object_unref(enum_option);
					break;
				}
				g_free(enum_value);
			}
			gchar *label = gebr_geoxml_parameter_get_label(parameter);
			str_value->str = g_markup_printf_escaped("%s",str_value->str);
			g_string_append_printf(dump,
			                       "      <tr class=\"%s\">\n  "
			                       "        <td class=\"label\">%s</td>\n"
			                       "        <td class=\"value\">%s</td>\n"
			                       "      </tr>\n",
					       is_required? "param-required" : "param", label, str_value->str);
			g_free(label);
		}
		g_string_free(str_value, TRUE);
		g_string_free(default_value, TRUE);
	} else {
		GString * previous_table = g_string_new(dump->str);
		gchar *label = gebr_geoxml_parameter_get_label(parameter);
		g_string_append_printf(dump,
		                       "      <tr class=\"group\">\n  "
		                       "        <td class=\"label\" colspan=\"2\">%s</td>\n"
		                       "      </tr>\n",
		                       label);
		g_free(label);
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		n_instances = gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter));

		i = 1;
		GString * group_table = g_string_new(dump->str);

		while (instance) {
			GString * instance_table = g_string_new(dump->str);
			if (n_instances > 1)
				g_string_append_printf(dump,
				                       "      <tr class=\"group\">\n  "
				                       "        <td class=\"label\" colspan=\"2\">%s %d</td>\n"
				                       "      </tr>\n",
						       _("Instance"), i++);
			parameters = GEBR_GEOXML_PARAMETERS(instance);
			gebr_geoxml_parameters_get_parameter(parameters, &param, 0);

			GString * inner_table = g_string_new(dump->str);

			while (param) {
				append_parameter_row(report, GEBR_GEOXML_PARAMETER(param), dump);
				gebr_geoxml_sequence_next(&param);
			}
			/*If there are no parameters returned by the dialog choice...*/
			if(g_string_equal(dump, inner_table))
			/*...return the table to it previous content*/
				g_string_assign(dump, instance_table->str);

			g_string_free(inner_table, TRUE);

			gebr_geoxml_sequence_next(&instance);
			g_string_free(instance_table, TRUE);
		}
		/* If there are no instance inside the group...*/
		if(g_string_equal(dump, group_table))
		/*...return the table to it outermost previous content*/
			g_string_assign(dump, previous_table->str);

		g_string_free(previous_table, TRUE);
		g_string_free(group_table, TRUE);
	}
}

static void
gebr_program_generate_parameter_value_table(GebrReport *report,
					    GebrGeoXmlProgram *program,
                                            GString *tables_content)
{
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *sequence;

	gchar *translated = g_strdup (_("Parameters"));
	
	g_string_append_printf(tables_content,
	                       "  <table class=\"parameters\">\n"
	                       "    <caption>%s</caption>\n"
	                       "    <thead>\n",
	                       translated);
	g_free (translated);

	parameters = gebr_geoxml_program_get_parameters (program);
	gebr_geoxml_parameters_get_parameter (parameters, &sequence, 0);
	gebr_geoxml_object_unref(parameters);

	if (sequence == NULL) {
		g_string_append(tables_content,
		                "      <tr>\n"
		                "        <td>this program has no parameters.</td>\n"
		                "      </tr>\n"
		                "    </thead>"
		                "    <tbody>"
		                "      <tr>"
		                "        <td></td>"
		                "      </tr>");
	} else {
		g_string_append_printf(tables_content,
		                       "      <tr>\n"
		                       "        <td>%s</td>\n"
		                       "        <td>%s</td>\n"
		                       "      </tr>\n"
		                       "    </thead>"
		                       "    <tbody>",
		                       _("Parameter"), _("Value"));

		GString * initial_table = g_string_new(tables_content->str);

		while (sequence) {
			append_parameter_row(report, GEBR_GEOXML_PARAMETER(sequence), tables_content);
			gebr_geoxml_sequence_next (&sequence);
		}

		if (g_string_equal(initial_table, tables_content)) {
			if (report->priv->detailed_parameter_table == GEBR_PARAM_TABLE_ONLY_CHANGED)
				g_string_append_printf(tables_content,
						"      <tr>\n"
						"        <td colspan=\"2\">%s</td>\n"
						"      </tr>\n",
						_("This program has only default parameters"));

			else if (report->priv->detailed_parameter_table == GEBR_PARAM_TABLE_ONLY_FILLED)
				g_string_append_printf(tables_content,
						"      <tr>\n"
						"        <td colspan=\"2\">%s</td>\n"
						"      </tr>\n",
						_("This program has only empty parameters"));

			g_string_free(initial_table, TRUE);
		}
	}

	g_string_append_printf (tables_content,
	                        "    </tbody>\n"
	                        "  </table>\n");
}

/*
 * gebr_flow_generate_parameter_value_table:
 * @flow: a #GebrGeoXmlFlow
 * @tables_content: A #GString to append content
 * @index: A index to link the table
 *
 * Creates a string containing a HTML table for the programs of @flow.
 */
static void
gebr_flow_generate_parameter_value_table(GebrReport *report,
					 GebrGeoXmlFlow *flow,
                                         GString *prog_content,
                                         const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));
	GebrGeoXmlSequence * sequence;
	GString *programs_content = g_string_new(NULL);

	gebr_geoxml_flow_get_program(flow, &sequence, 0);
	while (sequence) {
		GString *single_prog = g_string_new(NULL);
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;

		prog = GEBR_GEOXML_PROGRAM (sequence);
		status = gebr_geoxml_program_get_status (prog);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			gchar *title = gebr_geoxml_program_get_title(prog);
			gchar *description = gebr_geoxml_program_get_description(prog);

			GebrUiFlowProgram *ui_program = gebr_ui_flow_program_new(prog);

			gchar *error_message = g_strdup(gebr_ui_flow_program_get_tooltip(ui_program));

			gchar *link = g_strdup_printf("%s%s%d",
			                              has_index? index : "",
	                        		      has_index? "." : "",
                       				      i);

			g_string_append_printf(single_prog,
			                       "  <div class=\"program\">\n"
			                       "    <a name=\"%s\"></a>\n"
			                       "    <span class=\"title\">%s</span>",
			                       link, title);

			if (error_message)
				g_string_append_printf(single_prog,
				                       "    <span class=\"error\">%s</span>",
				                       error_message);

			g_string_append_printf(single_prog,
			                       "\n    <div class=\"description\">%s</div>\n",
			                       description);

			gebr_program_generate_parameter_value_table(report, prog, single_prog);

			g_string_append(single_prog,
			                "  </div>\n");

			if (gebr_geoxml_program_get_control(prog) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
				g_string_prepend(programs_content, single_prog->str);
			else
				g_string_append(programs_content, single_prog->str);

			i++;

			g_free(title);
			g_free(description);
			g_free(link);
			g_free(error_message);
		}

		gebr_geoxml_sequence_next(&sequence);
	}

	g_string_append_printf(prog_content,
	                       "<div class=\"programs\">\n"
	                       "  %s"
	                       "</div>",
	                       programs_content->str);

	g_free(flow_title);
	g_string_free(programs_content, TRUE);
}
