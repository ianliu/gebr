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

#include "intl.h"
#include "validate.h"
#include "geoxml/geoxml/validate.h"

/*
 * Prototypes
 */

static GebrValidateCase validate_cases[] = {
	/* menu */
	{GEBR_VALIDATE_CASE_FILENAME,
		GEBR_GEOXML_VALIDATE_CHECK_NOBLK | GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_FILEN,
		N_("File names must be a valid path.")},

	{GEBR_VALIDATE_CASE_TITLE,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_NOBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK,
		N_("Titles should not start with spaces or end with punctuations characters.")},

	{GEBR_VALIDATE_CASE_DESCRIPTION,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_CAPIT | GEBR_GEOXML_VALIDATE_CHECK_NOBLK
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT,
		N_("Description should be capitalized and have no punctuations at the end.")},

	{GEBR_VALIDATE_CASE_AUTHOR,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_CAPIT | GEBR_GEOXML_VALIDATE_CHECK_NOBLK
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT,
		N_("Author should be capitalized and have no punctuations at the end.")},

	{GEBR_VALIDATE_CASE_DATE,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Date should not be empty.")},

	{GEBR_VALIDATE_CASE_HELP,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Help should not be empty.")},

	{GEBR_VALIDATE_CASE_CATEGORY,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_CAPIT | GEBR_GEOXML_VALIDATE_CHECK_NOBLK
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT,
		N_("Categories should be capitalized and have no punctuations at the end.")},
	
	{GEBR_VALIDATE_CASE_EMAIL,
		GEBR_GEOXML_VALIDATE_CHECK_EMAIL,
		N_("Help should not be empty.")},

	/* program */
	{GEBR_VALIDATE_CASE_PROGRAM_TITLE,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_NOBLK | GEBR_GEOXML_VALIDATE_CHECK_MTBLK,
		N_("Program titles should not have extra spaces.")},

	{GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_CAPIT | GEBR_GEOXML_VALIDATE_CHECK_NOBLK
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT,
		N_("Program description should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_PROGRAM_BINARY,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Binaries should not be empty.")},

	{GEBR_VALIDATE_CASE_PROGRAM_VERSION,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Program version should be filled.")},

	{GEBR_VALIDATE_CASE_PROGRAM_URL,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Program url should not be empty.")},

	/* parameter */
	{GEBR_VALIDATE_CASE_PARAMETER_LABEL,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY | GEBR_GEOXML_VALIDATE_CHECK_CAPIT | GEBR_GEOXML_VALIDATE_CHECK_NOBLK
			| GEBR_GEOXML_VALIDATE_CHECK_MTBLK | GEBR_GEOXML_VALIDATE_CHECK_NOPNT | GEBR_GEOXML_VALIDATE_CHECK_LABEL_HOTKEY,
		N_("Parameter label should be capitalized and have no punctuations characters at the end. Also, careful with colliding shortcuts.")},

	{GEBR_VALIDATE_CASE_PARAMETER_KEYWORD,
		GEBR_GEOXML_VALIDATE_CHECK_EMPTY,
		N_("Parameter keyword should not be empty.")},
};

/*
 * Public functions
 */

GebrValidateCase * gebr_validate_get_validate_case(GebrValidateCaseName name)
{
	static gint n_elements = G_N_ELEMENTS(validate_cases);

	for (int i = 0; i < n_elements; i++)
		if (validate_cases[i].name == name)
			return &validate_cases[i];
	return NULL;
}

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

