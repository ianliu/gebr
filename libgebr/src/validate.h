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

#ifndef __GEBR_VALIDATE_H
#define __GEBR_VALIDATE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 */
enum GebrValidateCheckFlags {
	/**
	 * Field is empty.
	 * Can't be automatically fixed.
	 */
	GEBR_VALIDATE_CHECK_EMPTY		= 1 << 0,
	/**
	 * First letter should be capitalized.
	 */
	GEBR_VALIDATE_CHECK_CAPIT		= 1 << 1,
	/**
	 * Blank spaces at the begging and/or end of the field found.
	 */
	GEBR_VALIDATE_CHECK_NOBLK		= 1 << 2,
	/**
	 * Muliple blank spaces found.
	 */
	GEBR_VALIDATE_CHECK_MTBLK		= 1 << 3,
	/**
	 */
	GEBR_VALIDATE_CHECK_NOPNT		= 1 << 4,
	/**
	 * Invalid email address.
	 * Can't be automatically fixed.
	 */
	GEBR_VALIDATE_CHECK_EMAIL		= 1 << 5,
	/**
	 * Filename tag don't match menu's filename.
	 * Can't be automatically fixed.
	 */
	GEBR_VALIDATE_CHECK_FILEN		= 1 << 6,
	/**
	 * Hotkey duplicated for other parameter's label.
	 * Can't be automatically fixed.
	 */
	GEBR_VALIDATE_CHECK_LABEL_HOTKEY	= 1 << 7,
	/**
	 * URL isn't prefixed with the scheme (http://, mailto:, etc).
	 */
        GEBR_VALIDATE_CHECK_URL			= 1 << 8
};

/**
 */
typedef enum {
	GEBR_VALIDATE_CASE_AUTHOR,
	GEBR_VALIDATE_CASE_CATEGORY,
	GEBR_VALIDATE_CASE_DATE,
	GEBR_VALIDATE_CASE_DESCRIPTION,
	GEBR_VALIDATE_CASE_FILENAME,
	GEBR_VALIDATE_CASE_HELP,
	GEBR_VALIDATE_CASE_TITLE,
	GEBR_VALIDATE_CASE_EMAIL,

	GEBR_VALIDATE_CASE_PROGRAM_BINARY,
	GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION,
	GEBR_VALIDATE_CASE_PROGRAM_TITLE,
	GEBR_VALIDATE_CASE_PROGRAM_URL,
	GEBR_VALIDATE_CASE_PROGRAM_VERSION,

	GEBR_VALIDATE_CASE_PARAMETER_KEYWORD,
	GEBR_VALIDATE_CASE_PARAMETER_LABEL,
} GebrValidateCaseName;


/**
 */
struct _GebrValidateCase {
	GebrValidateCaseName name;
	gint flags;
	const gchar * validcase_msg;
};
typedef struct _GebrValidateCase GebrValidateCase;

/**
 */
GebrValidateCase * gebr_validate_get_validate_case(GebrValidateCaseName name);

/**
 */
gint gebr_validate_case_check_value(GebrValidateCase * self, const gchar * value, gboolean * can_fix);

/**
 */
gchar * gebr_validate_case_fix(GebrValidateCase * self, const gchar * value);

/**
 * Return a newly allocated Pango markup string explaining the automatic fixes available for \p value in validation case \p self.
 */
gchar *gebr_validate_case_automatic_fixes_msg(GebrValidateCase *self, const gchar * value, gboolean * can_fix);

/**
 * Return a newly allocated Pango markup string explaining the failed \p flags. 
 */
gchar *gebr_validate_flags_failed_msg(gint failed_flags);

/**
 * TRUE if str is not empty.
 */
gboolean gebr_validate_check_is_not_empty(const gchar * str);

/**
 * TRUE if str does not start with lower case letter.
 */
gboolean gebr_validate_check_no_lower_case(const gchar * sentence);

/**
 *  CHANGE \p sentence first letter to upper case.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_lower_case check
 */
gchar * gebr_validate_change_first_to_upper(const gchar * sentence);

/**
 * TRUE if str has not consecutive blanks.
 */
gboolean gebr_validate_check_no_multiple_blanks(const gchar * str);

/**
 *  CHANGE \p sentence to remove mutiple blanks.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_multiple_blanks check
 */
gchar * gebr_validate_change_multiple_blanks(const gchar * sentence);

/**
 * TRUE if str does not start or end with blanks/tabs.
 */
gboolean gebr_validate_check_no_blanks_at_boundaries(const gchar * str);

/**
 *  CHANGE \p sentence to remove blanks at boundaries.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_multiple_blanks check
 */
gchar * gebr_validate_change_no_blanks_at_boundaries(const gchar * sentence);

/**
 * TRUE if str does not end if a punctuation mark.
 */
gboolean gebr_validate_check_no_punctuation_at_end(const gchar * str);

/**
 *  CHANGE \p sentence to remove punctuation at string's end.
 *  \return  string with last punctuation changed by a NULL character 
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_no_punctuation_at_end check
 */
gchar * gebr_validate_change_no_punctuation_at_end(const gchar * sentence);

/**
 * TRUE if str has not path and ends with .mnu
 */
gboolean gebr_validate_check_menu_filename(const gchar * str);
 
/**
 * TRUE if \p str is of kind <code>xxx@yyy</code>, with xxx composed by letter, digits, underscores, dots and dashes,
 * and yyy composed by at least one dot, letter digits and dashes.
 *
 * \see http://en.wikipedia.org/wiki/E-mail_address
 */
gboolean gebr_validate_check_is_email(const gchar * str);

/**
 * TRUE if \p str starts with one of the following protocols: http, mailto, file or ftp.
 */
gboolean gebr_validate_check_is_url(const gchar * str);

/**
 *  GUESS which protocol should prefixes \p sentence, and add it. If
 *  sentence validates as an email, "mailto:" is added. If sentence
 *  starts with "www.", "http://" is added. If sentence starts with
 *  "ftp.", "ftp://" is added. If sentence starts with "/", "file://"
 *  is added. Otherwise, a copy of sentence is returned.
 *  \return  string prefixed by a protocol, or the input string whenever there is no reasonable guess.
 *  \return  a newly allocated string that must be freed
 *  It implements the correction for \ref gebr_validate_check_is_url check
 */
gchar * gebr_validate_change_url(const gchar * sentence);


G_END_DECLS

#endif				//__GEBR_VALIDATE_H
