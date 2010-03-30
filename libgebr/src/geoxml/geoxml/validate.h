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

#ifndef __GEBR_GEOXML_VALIDATE_H
#define __GEBR_GEOXML_VALIDATE_H

#include <glib.h>
#include "flow.h"

/**
 * \struct GebrGeoXmlValidate validate.h geoxml/validate.h
 * \brief
 * Semantic validator for GeoXml
 */

/**
 * \internal
 */
enum GebrGeoXmlValidateCheckFlags {
	GEBR_GEOXML_VALIDATE_CHECK_EMPTY	= 1 << 0,
	GEBR_GEOXML_VALIDATE_CHECK_CAPIT	= 1 << 1,
	GEBR_GEOXML_VALIDATE_CHECK_NOBLK	= 1 << 2,
	GEBR_GEOXML_VALIDATE_CHECK_MTBLK	= 1 << 3,
	GEBR_GEOXML_VALIDATE_CHECK_NOPNT	= 1 << 4,
	GEBR_GEOXML_VALIDATE_CHECK_EMAIL	= 1 << 5,
	GEBR_GEOXML_VALIDATE_CHECK_FILEN	= 1 << 6,
	GEBR_GEOXML_VALIDATE_CHECK_LABEL_HOTKEY	= 1 << 7,
};

typedef struct _GebrGeoXmlValidate GebrGeoXmlValidate;
typedef struct _GebrGeoXmlValidateOperations GebrGeoXmlValidateOperations;
typedef struct _GebrGeoXmlValidateOptions GebrGeoXmlValidateOptions;
struct _GebrGeoXmlValidate {
	struct _GebrGeoXmlValidateOperations {
		void (*append_text)(gpointer data, const gchar * format, ...);
		/**
		 * Append text with emphasys.
		 * This function is optional. If NULL #append_text is used instead.
		 */
		void (*append_text_emph)(gpointer data, const gchar * format, ...);
		/**
		 * Append error text.
		 * This function is optional. If NULL #append_text is used instead.
		 */
		void (*append_text_error)(gpointer data, const gchar * format, ...);
		/**
		 * Append error text with the paths strings (as for GtkTreeModels)
		 * for programs and parameters.
		 * If set then it is preferred over #append_text_error.
		 */
		void (*append_text_error_with_paths)(gpointer data, const gchar *program_path, const
						     gchar *parameter_path, const gchar * format, ...);
	} operations;
	gpointer data;

	struct _GebrGeoXmlValidateOptions {
		/**
		 * TRUE to enable all subsequent tests.
		 * If so, don't to set others
		 */
		gboolean all;
		gboolean filename;
		gboolean title;
		gboolean desc;
		gboolean author;
		gboolean dates;
		gboolean category;
		gboolean mhelp;
		gboolean progs;
		gboolean params;
	} options;

	gint potential_errors;
	
	GHashTable *hotkey_table;
	gint iprog;
	gint ipar;
	gint isubpar;
};

/**
 * This class is not thread-safe.
 */
GebrGeoXmlValidate *gebr_geoxml_validate_new(gpointer data, GebrGeoXmlValidateOperations operations,
					     GebrGeoXmlValidateOptions options);

/**
 * Frees \ref GebrGeoXmlValidate structure and members.
 */
void gebr_geoxml_validate_free(GebrGeoXmlValidate * validate);

/**
 * Return the number o potential errors found.
 * This function is not thread-safe.
 */
gint gebr_geoxml_validate_report_menu(GebrGeoXmlValidate * validate, GebrGeoXmlFlow * menu);

#endif //__GEBR_GEOXML_VALIDATE_H
