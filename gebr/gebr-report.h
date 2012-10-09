/*
 * gebr-report.h
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

#ifndef __GEBR_REPORT_H__
#define __GEBR_REPORT_H__

#include <glib-object.h>
#include <libgebr/geoxml/geoxml.h>

#define GEBR_TYPE_REPORT (gebr_report_get_type())

typedef struct _GebrReport GebrReport;
typedef struct _GebrReportPriv GebrReportPriv;

struct _GebrReport {
	GebrReportPriv *priv;
};

GType gebr_report_get_type(void) G_GNUC_CONST;

GebrReport *gebr_report_new(GebrGeoXmlDocument *document);

void gebr_report_set_include_commentary(GebrReport *report, gboolean setting);

gchar *gebr_report_generate(GebrReport *report);

/**
 * gebr_report_get_css_header_field:
 * @filename: Name of the css file. It trusts that the file is ok.
 * @field: Name of the field to be checked (ex. "title", "e-mail")
 *
 * Gets a @field from the CSS comment header. A field is defined as a JavaDoc comment. For instance, the value of the
 * field "title" in the example below is <emphasis>Foobar</emphasis>:
 * <programlisting>
 * /<!-- -->**
 *  * @title: Foobar
 *  * @author: John
 *  *<!-- -->/
 * body {
 *   color: blue;
 * }
 * </programlisting>
 *
 * Returns: A newly allocated string containing the field value.
 */
gchar *gebr_report_get_css_header_field(const gchar *filename, const gchar *field);

/**
 * gebr_flow_generate_variables_value_table:
 * @doc: a #GebrGeoXmlDocument
 * @insert_header: Pass %TRUE for include header, %FALSE otherwise
 * @close: Pass %TRUE for close table of header, %FALSE otherwise
 * @tables_content: a #GString to append content
 * @scope: a string with title of scope to include variables
 *
 * Creates a string containing a HTML table for the variables on dictionary of @doc.
 *
 */
void gebr_generate_variables_value_table(GebrGeoXmlDocument *doc,
                                         gboolean insert_header,
                                         gboolean close,
                                         GString *tables_content,
                                         const gchar *scope);

/**
 * gebr_flow_generate_io_table:
 * @flow: a #GebrGeoXmlFlow
 * @tables_content: A #GString to append content
 *
 * Creates a string containing a HTML table for I/O of @flow.
 *
 */
void gebr_flow_generate_io_table(GebrGeoXmlFlow *flow,
                                 GString *tables_content);

/**
 * gebr_flow_generate_parameter_value_table:
 * @flow: a #GebrGeoXmlFlow
 * @tables_content: A #GString to append content
 * @index: A index to link the table
 *
 * Creates a string containing a HTML table for the programs of @flow.
 *
 */
void gebr_flow_generate_parameter_value_table(GebrGeoXmlFlow *flow,
                                              GString *prog_content,
                                              const gchar *index,
                                              gboolean flow_review);

/**
 * gebr_flow_generate_flow_revisions_index:
 * @flow:
 * @content:
 *
 * Concatenate on @content a HTML with revisions content
 */
void gebr_flow_generate_flow_revisions_index(GebrGeoXmlFlow *flow,
                                             GString *content,
                                             const gchar *index);

#endif /* end of include guard: __GEBR_REPORT_H__ */
