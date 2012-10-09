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

#include "ui_help.h" // For GebrHelpParamTable enum. TODO: Move this enum to gebr-report module!
#include "gebr-maestro-controller.h"

#define GEBR_TYPE_REPORT (gebr_report_get_type())

typedef struct _GebrReport GebrReport;
typedef struct _GebrReportPriv GebrReportPriv;

struct _GebrReport {
	GebrReportPriv *priv;
};

GType gebr_report_get_type(void) G_GNUC_CONST;

GebrReport *gebr_report_new(GebrGeoXmlDocument *document);

void gebr_report_set_validator(GebrReport *report, GebrValidator *validator);

void gebr_report_set_maestro_controller(GebrReport *report, GebrMaestroController *maestro_controller);

void gebr_report_set_css_url(GebrReport *report, const gchar *css_url);

void gebr_report_set_include_commentary(GebrReport *report, gboolean setting);

void gebr_report_set_include_revisions(GebrReport *report, gboolean setting);

void gebr_report_set_detailed_parameter_table(GebrReport *report, GebrHelpParamTable detailed_parameter_table);

void gebr_report_set_include_flow_report(GebrReport *report, gboolean setting);

void gebr_report_set_error_message(GebrReport *report, const gchar *error_message);

void gebr_report_set_dictionary_documents(GebrReport *report,
					  GebrGeoXmlDocument *line,
					  GebrGeoXmlDocument *project);

gchar *gebr_report_generate(GebrReport *report);

gchar *gebr_report_generate_flow_review(GebrReport *report);

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

#endif /* end of include guard: __GEBR_REPORT_H__ */
