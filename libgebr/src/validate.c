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
#include <regex.h>

#include "validate.h"

gboolean gebr_validate_check_is_not_empty(const gchar * str)
{
	return (strlen(str) ? TRUE : FALSE);
}

gboolean gebr_validate_check_no_lower_case(const gchar * sentence)
{
	if (!gebr_validate_check_is_not_empty(sentence))
		return TRUE;
	if (g_ascii_islower(sentence[0]))
		return FALSE;

	return TRUE;
}

gboolean gebr_validate_check_no_multiple_blanks(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "   *", REG_NOSUB);
	return (regexec(&pattern, str, 0, 0, 0) ? TRUE : FALSE);
}

gboolean gebr_validate_check_no_blanks_at_boundaries(const gchar * str)
{
	int n = strlen(str);

	if (n == 0)
		return TRUE;
	if (str[0] == ' ' || str[0] == '\t' || str[n - 1] == ' ' || str[n - 1] == '\t')
		return FALSE;

	return TRUE;
}

gboolean gebr_validate_check_no_punctuation_at_end(const gchar * str)
{
	int n = strlen(str);

	if (n == 0)
		return TRUE;
	if (str[n - 1] != ')' && g_ascii_ispunct(str[n - 1]))
		return FALSE;

	return TRUE;
}

gboolean gebr_validate_check_menu_filename(const gchar * str)
{
	gchar *base;

	base = g_path_get_basename(str);
	if (strcmp(base, str)) {
		g_free(base);
		return FALSE;
	}
	g_free(base);

	if (!g_str_has_suffix(str, ".mnu"))
		return FALSE;

	return TRUE;
}

gboolean gebr_validate_check_is_email(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "^[a-z0-9_.-][a-z0-9_.-]*@[a-z0-9.-]*\\.[a-z0-9-][a-z0-9-]*$", REG_NOSUB | REG_ICASE);
	return (!regexec(&pattern, str, 0, 0, 0) ? TRUE : FALSE);
}

