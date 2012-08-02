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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <regex.h>

#include "libgebr-gettext.h"
#include <glib/gi18n-lib.h>

#include "validate.h"
#include "geoxml/gebr-geoxml-validate.h"

/*
 * Prototypes
 */

static GebrValidateCase validate_cases[] = {
	/* menu */
	{GEBR_VALIDATE_CASE_FILENAME,
		GEBR_VALIDATE_CHECK_NOBLK | GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_FILEN,
		N_("File names must be a valid path.")},

	{GEBR_VALIDATE_CASE_TITLE,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Titles should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_DESCRIPTION,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Description should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_AUTHOR,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Author should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_DATE,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Date should not be empty.")},

	{GEBR_VALIDATE_CASE_HELP,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Help should not be empty.")},

	{GEBR_VALIDATE_CASE_CATEGORY,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Categories should be capitalized and have no punctuation characters at the end.")},
	
	{GEBR_VALIDATE_CASE_EMAIL,
		GEBR_VALIDATE_CHECK_EMAIL,
		N_("Invalid email address.")},

	/* program */
	{GEBR_VALIDATE_CASE_PROGRAM_TITLE,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK 
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Program titles should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_TABS,
		N_("Program description should be capitalized and have no punctuation characters at the end.")},

	{GEBR_VALIDATE_CASE_PROGRAM_BINARY,
		GEBR_VALIDATE_CHECK_EMPTY,
		N_("Binaries should not be empty.")},

	{GEBR_VALIDATE_CASE_PROGRAM_VERSION,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_NOBLK | GEBR_VALIDATE_CHECK_NOPNT
                        | GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_TABS,
		N_("Program version should be filled in.")},

	{GEBR_VALIDATE_CASE_PROGRAM_URL,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_URL,
		N_("Program url should start with a protocol.")},

	/* parameter */
	{GEBR_VALIDATE_CASE_PARAMETER_LABEL,
		GEBR_VALIDATE_CHECK_EMPTY | GEBR_VALIDATE_CHECK_CAPIT | GEBR_VALIDATE_CHECK_NOBLK
			| GEBR_VALIDATE_CHECK_MTBLK | GEBR_VALIDATE_CHECK_NOPNT | GEBR_VALIDATE_CHECK_LABEL_HOTKEY
                        | GEBR_VALIDATE_CHECK_TABS, 
		N_("Parameter label should be capitalized and have no punctuation characters at the end. Also, be careful with colliding shortcuts.")},

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

const gchar * gebr_validate_case_get_message (GebrValidateCase *validate_case)
{
	return _(validate_case->validcase_msg);
}

gint gebr_validate_case_check_value(GebrValidateCase * self, const gchar * value, gboolean * can_fix)
{
	gint flags = self->flags;
	gint failed = 0;

	if (can_fix != NULL)
		*can_fix = (strlen(value) == 0 ||
			    flags & GEBR_VALIDATE_CHECK_EMAIL ||
			    flags & GEBR_VALIDATE_CHECK_FILEN) ? FALSE : TRUE;

	void gebr_validate_case_check_value_aux(const gchar *value) 
	{
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
		if (flags & GEBR_VALIDATE_CHECK_URL && !gebr_validate_check_is_url(value))
			failed |= GEBR_VALIDATE_CHECK_URL;
		if (flags & GEBR_VALIDATE_CHECK_TABS && !gebr_validate_check_tabs(value))
			failed |= GEBR_VALIDATE_CHECK_TABS;
	}

	if (self->name == GEBR_VALIDATE_CASE_CATEGORY) {
		gchar **cats = g_strsplit(value, "|", 0);
		for (int i = 0; cats[i] != NULL; ++i)
			gebr_validate_case_check_value_aux(cats[i]);
		g_strfreev(cats);
	} else
		gebr_validate_case_check_value_aux(value);

	return failed;
}

gchar * gebr_validate_case_fix(GebrValidateCase * self, const gchar * value)
{
	gboolean can_fix;

	gebr_validate_case_check_value(self, value, &can_fix);
	if (!can_fix)
		return NULL;

	gchar * gebr_validate_case_fix_aux(const gchar *value, gboolean *has_fix)
	{
		gchar * tmp = NULL;
		gchar * fix = g_strdup(value);
		*has_fix = FALSE;

		if (self->flags & GEBR_VALIDATE_CHECK_CAPIT
		    && !gebr_validate_check_no_lower_case(fix)) {
			if (fix != NULL)
				tmp = fix;
			fix = gebr_validate_change_first_to_upper(fix);
			if (tmp) {
				g_free(tmp);
				tmp = NULL;
			}
			*has_fix = TRUE;
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
			*has_fix = TRUE;
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
			*has_fix = TRUE;
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
			*has_fix = TRUE;
		}
		if (self->flags & GEBR_VALIDATE_CHECK_URL
		    && !gebr_validate_check_is_url(fix)) {
			if (fix != NULL)
				tmp = fix;
			fix = gebr_validate_change_url(fix);
			if (tmp) {
				g_free(tmp);
				tmp = NULL;
			}
			*has_fix = TRUE;
		}
		if (self->flags & GEBR_VALIDATE_CHECK_TABS
		    && !gebr_validate_check_tabs(fix)) {
			if (fix != NULL)
				tmp = fix;
			fix = gebr_validate_change_tabs(fix);
			if (tmp) {
				g_free(tmp);
				tmp = NULL;
			}
			*has_fix = TRUE;
		}
		
		return fix;
	}

	gchar * gebr_validate_case_fix_aux_iter(const gchar *value)
	{
		gchar *tmp;
		gboolean has_fix;
		gchar *fix = gebr_validate_case_fix_aux (value, &has_fix);

		while (has_fix) {
			tmp = fix;
			fix = gebr_validate_case_fix_aux (fix, &has_fix);
			g_free (tmp);
		}

		return fix;
	}

	if (self->name == GEBR_VALIDATE_CASE_CATEGORY) {
		GString *fix = g_string_new("");
		gchar **cats = g_strsplit(value, "|", 0);
		for (int i = 0; cats[i] != NULL; i++) {
			gchar *ifix = gebr_validate_case_fix_aux_iter(cats[i]);
			g_string_append_printf(fix, "%s%s", ifix != NULL ? ifix : cats[i], cats[i+1] != NULL ? "|" : "");
			if (ifix != NULL)
				g_free(ifix);
		}
		g_strfreev(cats);

		gchar *ret = fix->str;
		g_string_free(fix, FALSE);
		return ret;
	} else
		return gebr_validate_case_fix_aux_iter(value);
}

gchar *gebr_validate_case_automatic_fixes_msg(GebrValidateCase *self, const gchar * value, gboolean * can_fix)
{
	gint failed = gebr_validate_case_check_value(self, value, can_fix);
	if (!(*can_fix))
		return g_strdup(_("<b>No automatic fix available</b>"));

	GString *msg = g_string_new(_("<b>Click on the icon to fix:</b>"));
	if (failed & GEBR_VALIDATE_CHECK_CAPIT)
		g_string_append(msg, _("\n - Capitalize first letter"));
	else if (failed & GEBR_VALIDATE_CHECK_NOBLK)
		g_string_append(msg, _("\n - Remove spaces at the beginning/end"));
	else if (failed & GEBR_VALIDATE_CHECK_MTBLK)
		g_string_append(msg, _("\n - Remove multiples spaces"));
	else if (failed & GEBR_VALIDATE_CHECK_NOPNT)
		g_string_append(msg, _("\n - Remove final punctuation character"));
	else if (failed & GEBR_VALIDATE_CHECK_URL)
		g_string_append(msg, _("\n - Add URL scheme"));
	else if (failed & GEBR_VALIDATE_CHECK_TABS)
		g_string_append(msg, _("\n - Replace tabs by space"));

	gchar *ret = msg->str;
	g_string_free(msg, FALSE);

	return ret;
}

gchar *gebr_validate_flags_failed_msg(gint failed_flags)
{
	if (!failed_flags)
		return NULL;

	GString *msg = g_string_new(_("<b>Error(s) found:</b>"));
	if (failed_flags & GEBR_VALIDATE_CHECK_EMPTY)
		g_string_append(msg, _("\n - The field is not filled in"));
	if (failed_flags & GEBR_VALIDATE_CHECK_CAPIT)
		g_string_append(msg, _("\n - First letter should be capitalized"));
	if (failed_flags & GEBR_VALIDATE_CHECK_NOBLK)
		g_string_append(msg, _("\n - There are spaces at the beginning/end"));
	if (failed_flags & GEBR_VALIDATE_CHECK_MTBLK)
		g_string_append(msg, _("\n - There are multiples spaces"));
	if (failed_flags & GEBR_VALIDATE_CHECK_NOPNT)
		g_string_append(msg, _("\n - This field should not have a final punctuation"));
	if (failed_flags & GEBR_VALIDATE_CHECK_EMAIL)
		g_string_append(msg, _("\n - Invalid email address"));
	if (failed_flags & GEBR_VALIDATE_CHECK_FILEN)
		g_string_append(msg, _("\n - Invalid menu filename"));
	if (failed_flags & GEBR_VALIDATE_CHECK_LABEL_HOTKEY)
		g_string_append(msg, _("\n - Duplicated hotkey"));
	if (failed_flags & GEBR_VALIDATE_CHECK_URL)
		g_string_append(msg, _("\n - URL scheme is missing"));
	if (failed_flags & GEBR_VALIDATE_CHECK_TABS)
		g_string_append(msg, _("\n - There are tabs"));

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
	if (g_unichar_islower(g_utf8_get_char_validated(sentence, -1)))
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

gboolean gebr_validate_check_tabs(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "\t\t*", REG_NOSUB);
	return (regexec(&pattern, str, 0, 0, 0) ? TRUE : FALSE);
}

gchar * gebr_validate_change_tabs(const gchar * sentence)
{
#if GLIB_CHECK_VERSION(2,14,0)
	GRegex *regex;
	GError *error = NULL;
	regex = g_regex_new("\t\t*", 0, 0, &error);
	if (error != NULL) {
		g_warning("%s:%d %s", __FILE__, __LINE__, error->message);
		g_error_free(error);
		if (regex)
			g_regex_unref(regex);
		return NULL;
	}
	gchar *ret = g_regex_replace(regex, sentence, -1, 0, " ", 0, NULL);
	g_regex_unref(regex);
	return ret;
#else
	return NULL;
#endif
}

gboolean gebr_validate_check_no_multiple_blanks(const gchar * str)
{
	regex_t pattern;
	regcomp(&pattern, "   *", REG_NOSUB);
	return (regexec(&pattern, str, 0, 0, 0) ? TRUE : FALSE);
}

gchar * gebr_validate_change_multiple_blanks(const gchar * sentence)
{
#if GLIB_CHECK_VERSION(2,14,0)
	GRegex *regex;
	GError *error = NULL;
	regex = g_regex_new("[[:space:]]{2,}", 0, 0, &error);
	if (error != NULL) {
		g_warning("%s:%d %s", __FILE__, __LINE__, error->message);
		g_error_free(error);
		if (regex)
			g_regex_unref(regex);
		return NULL;
	}
	gchar *ret = g_regex_replace(regex, sentence, -1, 0, " ", 0, NULL);
	g_regex_unref(regex);
	return ret;
#else
	return NULL;
#endif
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
#if GLIB_CHECK_VERSION(2,14,0)
	GRegex *regex;
	GError *error = NULL;
	regex = g_regex_new("^[[:space:]]+|[[:space:]]+$", 0, 0, NULL);
	if (error != NULL) {
		g_warning("%s:%d %s", __FILE__, __LINE__, error->message);
		g_error_free(error);
		if (regex)
			g_regex_unref(regex);
		return NULL;
	}
	gchar *ret = g_regex_replace(regex, sentence, -1, 0, "", 0, NULL);
	g_regex_unref(regex);
	return ret;
#else
	return NULL;
#endif
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
	else if (g_str_has_prefix(sentence, "ftp."))
		return g_strconcat("ftp://", sentence, NULL);
	else if (g_str_has_prefix(sentence, "/"))
		return g_strconcat("file://", sentence, NULL);
	else 
		return g_strconcat("http://", sentence, NULL);

        str = g_strdup(sentence);

	return str;
}
