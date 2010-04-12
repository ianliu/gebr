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
		GEBR_VALIDATE_CHECK_NOBLK | GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_FILEN,
		N_("File names must be a valid path.")},

	{GEBR_VALIDATE_CASE_TITLE,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_NOBLK | GEBR_VALIDATE_CHECK_NOPNT
			| GEBR_VALIDATE_CHECK_MTBLK,
		N_("Titles should not start with spaces or end with punctuations characters.")},

	{GEBR_VALIDATE_CASE_DESCRIPTION,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT,
		N_("Description should be capitalized and have no punctuations at the end.")},

	{GEBR_VALIDATE_CASE_AUTHOR,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT,
		N_("Author should be capitalized and have no punctuations at the end.")},

	{GEBR_VALIDATE_CASE_DATE,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Date should not be empty.")},

	{GEBR_VALIDATE_CASE_HELP,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Help should not be empty.")},

	{GEBR_VALIDATE_CASE_CATEGORY,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT,
		N_("Categories should be capitalized and have no punctuations at the end.")},
	
	{GEBR_VALIDATE_CASE_EMAIL,
		GEBR_VALIDATE_CHECK_EMAIL,
		N_("Invalid email address.")},

	/* program */
	{GEBR_VALIDATE_CASE_PROGRAM_TITLE,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_NOBLK | GEBR_VALIDATE_CHECK_MTBLK,
		N_("Program titles should not have extra spaces.")},

	{GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT,
		N_("Program description should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_PROGRAM_BINARY,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Binaries should not be empty.")},

	{GEBR_VALIDATE_CASE_PROGRAM_VERSION,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Program version should be filled.")},

	{GEBR_VALIDATE_CASE_PROGRAM_URL,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_URL,
		N_("Program url should starts with a protocol.")},

	/* parameter */
	{GEBR_VALIDATE_CASE_PARAMETER_LABEL,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_LABEL_HOTKEY,
		N_("Parameter label should be capitalized and have no punctuations characters at the end. Also, careful with colliding shortcuts.")},

	{GEBR_VALIDATE_CASE_PARAMETER_KEYWORD,
		GEBR_VALIDATE_CHECK_EMPTY,
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

gint gebr_validate_case_check_value(GebrValidateCase * self, const gchar * value, gboolean * can_fix)
{
	gint flags = self->flags;
	gint failed = 0;

	if (can_fix != NULL)
		*can_fix = (strlen(value) == 0 ||
			    flags & GEBR_VALIDATE_CHECK_EMAIL ||
			    flags & GEBR_VALIDATE_CHECK_FILEN ||
			    flags & GEBR_VALIDATE_CHECK_LABEL_HOTKEY) ? FALSE : TRUE;

	if (flags & GEBR_VALIDATE_CHECK_EMPTY && !gebr_validate_check_is_not_empty(value))
		failed |= GEBR_VALIDATE_CHECK_EMPTY;
	if (flags & GEBR_VALIDATE_CHECK_CAPIT && !gebr_validate_check_no_lower_case(value))
		failed |= GEBR_VALIDATE_CHECK_CAPIT;
	if (flags & GEBR_VALIDATE_CHECK_NOBLK && !gebr_validate_check_no_blanks_at_boundaries(value))
		failed |= GEBR_VALIDATE_CHECK_NOBLK;
	if (flags & GEBR_VALIDATE_CHECK_MTBLK && !gebr_validate_check_no_multiple_blanks(value))
		failed |= GEBR_VALIDATE_CHECK_MTBLK;
	if (flags & GEBR_VALIDATE_CHECK_NOPNT && !gebr_validate_check_no_punctuation_at_end(value))
		failed |= GEBR_VALIDATE_CHECK_NOPNT;
	if (flags & GEBR_VALIDATE_CHECK_EMAIL && !gebr_validate_check_is_email(value))
		failed |= GEBR_VALIDATE_CHECK_EMAIL;
	if (flags & GEBR_VALIDATE_CHECK_FILEN && !gebr_validate_check_menu_filename(value))
		failed |= GEBR_VALIDATE_CHECK_FILEN;

	return failed;
}

gchar * gebr_validate_case_fix(GebrValidateCase * self, const gchar * value)
{
	gchar * tmp;
	gchar * fix;
	gboolean can_fix;

	gebr_validate_case_check_value(self, value, &can_fix);
	if (!can_fix)
		return NULL;

	tmp = NULL;
	fix = g_strdup(value);

	if (self->flags & GEBR_VALIDATE_CHECK_CAPIT
	    && !gebr_validate_check_no_lower_case(fix)) {
		if (fix != NULL)
			tmp = fix;
		fix = gebr_validate_change_first_to_upper(fix);
		if (tmp) {
			g_free(tmp);
			tmp = NULL;
		}
	}
	if (self->flags & GEBR_VALIDATE_CHECK_NOBLK
	    && !gebr_validate_check_no_blanks_at_boundaries(fix)) {
		if (fix != NULL)
			tmp = fix;
		fix = gebr_validate_change_no_blanks_at_boundaries(fix);
		if (tmp) {
			g_free(tmp);
			tmp = NULL;
		}
	}
	if (self->flags & GEBR_VALIDATE_CHECK_MTBLK
	    && !gebr_validate_check_no_multiple_blanks(fix)) {
		if (fix != NULL)
			tmp = fix;
		fix = gebr_validate_change_multiple_blanks(fix);
		if (tmp) {
			g_free(tmp);
			tmp = NULL;
		}
	}
	if (self->flags & GEBR_VALIDATE_CHECK_NOPNT
	    && !gebr_validate_check_no_punctuation_at_end(fix)) {
		if (fix != NULL)
			tmp = fix;
		fix = gebr_validate_change_no_punctuation_at_end(fix);
		if (tmp) {
			g_free(tmp);
			tmp = NULL;
		}
	}

	return fix;
}

gchar *gebr_validate_case_automatic_fixes_msg(GebrValidateCase *self, const gchar * value, gboolean * can_fix)
{
	gint failed = gebr_validate_case_check_value(self, value, can_fix);
	if (!(*can_fix))
		return g_strdup(_("Any automatic fix available"));

	GString *msg = g_string_new(_("Automatic fix(es) available:"));
	if (failed & GEBR_VALIDATE_CHECK_CAPIT)
		g_string_append(msg, _("\n - Capitalize first letter"));
	if (failed & GEBR_VALIDATE_CHECK_NOBLK)
		g_string_append(msg, _("\n - Remove spaces at the beggining/end"));
	if (failed & GEBR_VALIDATE_CHECK_MTBLK)
		g_string_append(msg, _("\n - Remove multiple spaces"));
	if (failed & GEBR_VALIDATE_CHECK_NOPNT)
		g_string_append(msg, _("\n - Remove final pontuaction"));
	if (failed & GEBR_VALIDATE_CHECK_URL)
		g_string_append(msg, _("\n - Add url scheme"));

	gchar *ret = msg->str;
	g_string_free(msg, FALSE);

	return ret;
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

gchar * gebr_validate_change_first_to_upper(const gchar * sentence)
{
	gchar char_utf8[6];
	gchar * uppercase;
	gint length = g_unichar_to_utf8(g_utf8_get_char(sentence), char_utf8);

	uppercase = g_utf8_strup(char_utf8, length);
	return g_strjoin(NULL, uppercase, sentence+length, NULL);
}

gboolean gebr_validate_check_no_multiple_blanks(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "   *", REG_NOSUB);
	return (regexec(&pattern, str, 0, 0, 0) ? TRUE : FALSE);
}

gchar * gebr_validate_change_multiple_blanks(const gchar * sentence)
{
	GRegex *regex;
	GError *error = NULL;
	regex = g_regex_new("[[:space:]]{2,}", 0, 0, &error);
	if (error != NULL) {
		g_warning("%s:%d %s", __FILE__, __LINE__, error->message);
		g_error_free(error);
		return NULL;
	}
	return g_regex_replace(regex, sentence, -1, 0, " ", 0, NULL);
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

gchar * gebr_validate_change_no_blanks_at_boundaries(const gchar * sentence)
{
	GRegex *regex;
	GError *error = NULL;
	regex = g_regex_new("^[[:space:]]+|[[:space:]]+$", 0, 0, NULL);
	if (error != NULL) {
		g_warning("%s:%d %s", __FILE__, __LINE__, error->message);
		g_error_free(error);
		return NULL;
	}
	return g_regex_replace(regex, sentence, -1, 0, "", 0, NULL);
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

gchar * gebr_validate_change_no_punctuation_at_end(const gchar * sentence)
{
	gchar *str = g_strdup(sentence);

	if (str) {
		int n = strlen(str) - 1;
		while(str[n] != ')' && g_ascii_ispunct(str[n]))
			n--;
		str[n + 1] = '\0';
	}
	return str;
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

gboolean gebr_validate_check_is_url(const gchar * str)
{
        return (g_str_has_prefix(str, "http://") ||
                g_str_has_prefix(str, "mailto:") ||
                g_str_has_prefix(str, "file://") ||
                g_str_has_prefix(str, "ftp://") );
}

gchar * gebr_validate_change_url(const gchar * sentence)
{	
        gchar *str;

        if (gebr_validate_check_is_email(sentence))
                return g_strconcat("mailto:", sentence, NULL);
        if (g_str_has_prefix(sentence, "www."))
                return g_strconcat("http://", sentence, NULL);
        if (g_str_has_prefix(sentence, "ftp."))
                return g_strconcat("ftp://", sentence, NULL);
        if (g_str_has_prefix(sentence, "/"))
                return g_strconcat("file://", sentence, NULL);

        str = g_strdup(sentence);

	return str;
}
