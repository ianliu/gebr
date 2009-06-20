/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include <time.h>
#include <limits.h>
#include <string.h>

#include "date.h"
#include "intl.h"
#include "utils.h"

/*
 * Function: iso_date
 * Returns an (static allocated) string with the current date in ISO-8601 format
 *
 */
gchar *
iso_date(void)
{
	static gchar	date[100];
	GTimeVal	time_val;
	gchar *		tmp;

	g_get_current_time(&time_val);
	tmp = g_time_val_to_iso8601(&time_val);

	strcpy(date, tmp);
	g_free(tmp);

	/* convert to utf-8 if necessary */
	if (g_utf8_validate(date, -1, NULL) == FALSE) {
		tmp = g_simple_locale_to_utf8(date);
		if (tmp != NULL) {
			strcpy(date, tmp);
			g_free(tmp);
		} else
			strcpy(date, "");
	}

	return date;
}

/*
 * Function: localized_date
 * Returns an (static allocated) string with _iso_date_ converted to a localized date
 * If _iso_date_ is NULL, current date is used.
 */
gchar *
localized_date(const gchar * iso_date)
{
	static gchar	date[100];
	GTimeVal	time_val;
	struct tm *	tm;

	if (iso_date != NULL) {
		if (g_time_val_from_iso8601(iso_date, &time_val) == FALSE)
			return _("Unknown");
	} else
		g_get_current_time(&time_val);

	tm = localtime(&time_val.tv_sec);
	strftime(date, 100, "%c", tm);

	/* convert to utf-8 if necessary */
	if (g_utf8_validate(date, -1, NULL) == FALSE) {
		gchar *	tmp;

		tmp = g_simple_locale_to_utf8(date);
		if (tmp != NULL) {
			strcpy(date, tmp);
			g_free(tmp);
		} else
			strcpy(date, "");
	}

	return date;
}

GTimeVal
libgebr_iso_date_to_g_time_val(const gchar * iso_date)
{
	GTimeVal	time_val;

	if (!g_time_val_from_iso8601(iso_date, &time_val))
		time_val.tv_sec = LONG_MIN;

	return time_val;
}
