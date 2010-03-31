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

#include "validate.h"
#include "../../validate.h"
#include "../../intl.h"
#include "../../date.h"

GebrGeoXmlValidate *gebr_geoxml_validate_new(gpointer data, GebrGeoXmlValidateOperations operations,
					     GebrGeoXmlValidateOptions options)
{
	if (operations.append_text == NULL)
		return NULL;
	GebrGeoXmlValidate *validate = g_new(GebrGeoXmlValidate, 1);

	validate->data = data;
	if (operations.append_text_emph == NULL)
		operations.append_text_emph = operations.append_text;
	if (operations.append_text_error == NULL)
		operations.append_text_error = operations.append_text;
	validate->operations = operations;
	if (options.all) {
		options.filename = TRUE;
		options.title = TRUE;
		options.desc = TRUE;
		options.author = TRUE;
		options.dates = TRUE;
		options.category = TRUE;
		options.mhelp = TRUE;
		options.progs = TRUE;
		options.params = TRUE;
	}
	validate->options = options;

	return validate;
}

void gebr_geoxml_validate_free(GebrGeoXmlValidate * validate)
{
	g_free(validate);
}

/**
 * \internal
 */
static void validate_append_text_error(GebrGeoXmlValidate * validate, const gchar * format, ...)
{
	va_list argp;
	va_start(argp, format);
	gchar *string; 
	string = g_strdup_vprintf(format, argp);
	va_end(argp);
	
	if (validate->operations.append_text_error_with_paths == NULL) {
		validate->operations.append_text_error(validate->data, string);
		goto out;
	}
	if (validate->iprog == -1) {
		validate->operations.append_text_error_with_paths(validate->data, NULL, NULL, string);
		goto out;
	}

	GString *path = g_string_new(NULL);
	g_string_printf(path, "%d", validate->iprog);
	if (validate->ipar == -1) {
		validate->operations.append_text_error_with_paths(validate->data, path->str, NULL, string);
		goto out2;
	}
	GString *path2 = g_string_new(NULL);
	if (validate->isubpar == -1)
		g_string_printf(path2, "%d", validate->ipar);
	else
		g_string_printf(path2, "%d:%d", validate->ipar, validate->isubpar);
	validate->operations.append_text_error_with_paths(validate->data, path->str, path2->str, string);

	g_string_free(path2, TRUE);
out2:	g_string_free(path, TRUE);
out:	g_free(string);
}

/**
 * \internal
 */
static void validate_append_item(GebrGeoXmlValidate * validate, const gchar * item)
{
	validate->operations.append_text_emph(validate->data, item);
}

static void validate_append_check(GebrGeoXmlValidate * validate, const gchar * value, int flags, const gchar * format, ...);

/**
 * \internal
 */
static void
validate_append_item_with_check(GebrGeoXmlValidate * validate, const gchar * item, const gchar * value, int flags)
{
	validate_append_item(validate, item);
	validate_append_check(validate, value, flags, "\n");
}

static void show_parameter(GebrGeoXmlValidate * validate, GebrGeoXmlParameter * parameter);


gint gebr_geoxml_validate_report_menu(GebrGeoXmlValidate * validate, GebrGeoXmlFlow * menu)
{
	GebrGeoXmlSequence *seq;
	gint i;

	validate->iprog = -1;
	validate->ipar = -1;
	validate->isubpar = -1;
	validate->potential_errors = 0;
	if (validate->options.filename)
		validate_append_item_with_check(validate, _("Filename:      "),
						gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_FILENAME)->flags);
	if (validate->options.title)
		validate_append_item_with_check(validate, _("Title:         "),
						gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(menu)),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_TITLE)->flags);
	if (validate->options.desc)
		validate_append_item_with_check(validate, _("Description:   "),
						gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT(menu)),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_DESCRIPTION)->flags);
	if (validate->options.author) {
		validate_append_item(validate, _("Author:        "));
		validate_append_check(validate, gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(menu)),
				      gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_AUTHOR)->flags, " <");
		validate_append_check(validate, gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(menu)),
				      gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_EMAIL)->flags, ">");
		validate->operations.append_text(validate->data, "\n");
	}
	if (validate->options.dates) {
		validate_append_item_with_check(validate, _("Created:       "),
						gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOCUMENT(menu))),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_DATE)->flags);
		validate_append_item_with_check(validate, _("Modified:      "),
						gebr_localized_date(gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(menu))),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_DATE)->flags);
	}
	if (validate->options.mhelp) {
		validate_append_item(validate, _("Help:          "));
		if (strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(menu))) >= 1)
			validate->operations.append_text(validate->data, _("Defined"));
		else
			validate_append_check(validate, "", GEBR_GEOXML_VALIDATE_CHECK_EMPTY, "");
		validate->operations.append_text(validate->data, "\n");
	}
	if (validate->options.category) {
		gebr_geoxml_flow_get_category(menu, &seq, 0);
		if (seq == NULL)
			validate_append_item_with_check(validate, _("Category:      "), "", GEBR_GEOXML_VALIDATE_CHECK_EMPTY);
		else
			for (; seq != NULL; gebr_geoxml_sequence_next(&seq))
				validate_append_item_with_check(validate, _("Category:      "),
								gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq)),
								gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_CATEGORY)->flags);
	}

	if (!validate->options.progs && !validate->options.params)
		goto out;

	validate->operations.append_text_emph(validate->data, _("Menu with:     "));
	validate->operations.append_text(validate->data, _("%ld program(s)\n"), gebr_geoxml_flow_get_programs_number(menu));
	gebr_geoxml_flow_get_program(menu, &seq, 0);
	for (i = 0; seq != NULL; i++, gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlProgram *prog;
		GebrGeoXmlParameter *parameter;

		prog = GEBR_GEOXML_PROGRAM(seq);
		validate->iprog = i;

		validate->operations.append_text_emph(validate->data, _("\n>>Program:     "));
		validate->operations.append_text(validate->data, "%d\n", i + 1);
		validate_append_item_with_check(validate, _("  Title:       "),
						gebr_geoxml_program_get_title(prog),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_TITLE)->flags);
		validate_append_item_with_check(validate, _("  Description: "),
						gebr_geoxml_program_get_description(prog),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION)->flags);

		validate->operations.append_text_emph(validate->data, _("  In/out/err:  "));
		validate->operations.append_text(validate->data, "%s/%s/%s\n",
				     gebr_geoxml_program_get_stdin(prog) ? _("Read") : _("Ignore"),
				     gebr_geoxml_program_get_stdout(prog) ? _("Write") : _("Ignore"),
				     gebr_geoxml_program_get_stdin(prog) ? _("Append") : _("Ignore"));

		validate_append_item_with_check(validate, _("  Binary:      "),
						gebr_geoxml_program_get_binary(prog),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_BINARY)->flags);
                validate_append_item_with_check(validate, _("  Version:     "),
						gebr_geoxml_program_get_version(prog),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_VERSION)->flags);
                validate_append_item_with_check(validate, _("  URL:         "),
						gebr_geoxml_program_get_url(prog),
						gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_URL)->flags);
		validate_append_item(validate, _("  Help:        "));
		if (strlen(gebr_geoxml_program_get_help(prog)) >= 1)
			validate->operations.append_text(validate->data, _("Defined"));
		else
			validate_append_check(validate, "", GEBR_GEOXML_VALIDATE_CHECK_EMPTY, "");
		validate->operations.append_text(validate->data, "\n");

		if (validate->options.params) {
			GHashTable *hotkey_table;

			validate->hotkey_table = hotkey_table = g_hash_table_new_full((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, (GDestroyNotify)g_free, NULL);

			validate->operations.append_text_emph(validate->data, _("  >>Parameters:\n"));

			parameter = GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter
							  (gebr_geoxml_program_get_parameters(prog)));

			validate->ipar = 0;
			for (; parameter != NULL; gebr_geoxml_sequence_next((GebrGeoXmlSequence **)&parameter), ++validate->ipar)
				show_parameter(validate, parameter);
			g_hash_table_unref(hotkey_table);
		}
	}

out:	validate->operations.append_text_emph(validate->data, _("%d potential error(s)"), validate->potential_errors);

	return validate->potential_errors;
}

/**
 * \internal
 * TRUE if \p value passed through all selected tests
 */
static void validate_append_check(GebrGeoXmlValidate * validate, const gchar * value, int flags, const gchar * format, ...)
{
	gboolean result = TRUE;

	if (flags & GEBR_GEOXML_VALIDATE_CHECK_EMPTY)
		result = result && gebr_validate_check_is_not_empty(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_CAPIT)
		result = result && gebr_validate_check_no_lower_case(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_NOBLK)
		result = result && gebr_validate_check_no_blanks_at_boundaries(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_MTBLK)
		result = result && gebr_validate_check_no_multiple_blanks(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_NOPNT)
		result = result && gebr_validate_check_no_punctuation_at_end(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_EMAIL)
		result = result && gebr_validate_check_is_email(value);
	if (flags & GEBR_GEOXML_VALIDATE_CHECK_FILEN)
		result = result && gebr_validate_check_menu_filename(value);

	if (result)
		validate->operations.append_text(validate->data, value);
	else {
		if (gebr_validate_check_is_not_empty(value))
			validate_append_text_error(validate, "%s", value);
		else
			validate_append_text_error(validate, _("UNSET"));
		validate->potential_errors++;
	}

	if (flags & GEBR_GEOXML_VALIDATE_CHECK_LABEL_HOTKEY) {
		gchar * underscore;

		underscore = (gchar*)value;
		do {
			gint len;

			underscore = strchr(underscore, '_');
			if (!underscore)
				break;
			len = strlen(underscore);
			if (len == 1) //_ in the end of string	
				underscore = NULL;
			else if (len > 1 && *(underscore+1) == '_') {
				if (len == 2) //__ in the end of string	
					underscore = NULL;
				else
					underscore += 2;
			} else
				break;
		} while (underscore); 
		if (underscore) {
			GString *label_ext;
			gint length;
			gchar * uppercase;
			gchar hotkey[6];

			label_ext = g_string_new("");
			length = g_unichar_to_utf8(g_utf8_get_char(underscore + 1), hotkey);
			uppercase = g_utf8_strup(hotkey, length);
			if (g_hash_table_lookup(validate->hotkey_table, uppercase)) {
				g_string_printf(label_ext, " (Alt+%s%s)", uppercase, _(", already used above"));
				validate_append_text_error(validate, label_ext->str);
				++validate->potential_errors;
				result = FALSE;
			} else
				validate->operations.append_text(validate->data, _(" (Alt+%s)"), uppercase);
			g_hash_table_insert(validate->hotkey_table, uppercase, GINT_TO_POINTER(1));

			g_string_free(label_ext, TRUE);
		}
	}

	if (format == NULL)
		return;
	va_list argp;
	va_start(argp, format);
	gchar *string; 
	string = g_strdup_vprintf(format, argp);
	validate->operations.append_text(validate->data, string);
	g_free(string);
	va_end(argp);
}

/**
 * \internal
 */
static void show_parameter(GebrGeoXmlValidate * validate, GebrGeoXmlParameter * parameter)
{
	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)) {
		GString *default_value;
		const gchar * label;
		GebrGeoXmlProgramParameter *pp = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);

		if (validate->isubpar != -1)
			validate->operations.append_text(validate->data, "       %2d.%02d: ", validate->ipar, validate->isubpar);
		else
			validate->operations.append_text(validate->data, "    %2d: ", validate->ipar);

		label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(pp));
		validate_append_check(validate, label, gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PARAMETER_LABEL)->flags, "\n");

		validate->operations.append_text(validate->data, "        ");
		if (validate->isubpar != -1)
			validate->operations.append_text(validate->data, "      ");

		validate->operations.append_text(validate->data, "[");
		validate->operations.append_text(validate->data, gebr_geoxml_parameter_get_type_name(GEBR_GEOXML_PARAMETER(pp)));
		if (gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(pp)))
			validate->operations.append_text(validate->data, "(s)");
		validate->operations.append_text(validate->data, "] ");

		validate->operations.append_text(validate->data, "'");
		validate_append_check(validate, gebr_geoxml_program_parameter_get_keyword(pp),
				      gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PARAMETER_KEYWORD)->flags, "'");

		default_value = gebr_geoxml_program_parameter_get_string_value(pp, TRUE);
		if (default_value->len)
			validate->operations.append_text(validate->data, " [%s]", default_value->str);
		g_string_free(default_value, TRUE);

		if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_INT ||
		    gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
		    gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {

			gchar *min_str;
			gchar *max_str;

			gebr_geoxml_program_parameter_get_number_min_max(pp, &min_str, &max_str);
			validate->operations.append_text(validate->data, _(" in [%s,%s]"), (strlen(min_str) <= 0 ? "*" : min_str),
					     (strlen(max_str) <= 0 ? "*" : max_str));
		}

		if (gebr_geoxml_program_parameter_get_is_list(GEBR_GEOXML_PROGRAM_PARAMETER(pp))) {
			if (strlen(gebr_geoxml_program_parameter_get_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER(pp))))
				validate->operations.append_text(validate->data, _(" (entries separated by '%s')"),
						     gebr_geoxml_program_parameter_get_list_separator
						     (GEBR_GEOXML_PROGRAM_PARAMETER(pp)));
			else {
				validate_append_text_error(validate, _(" (missing entries' separator)"));
				validate->potential_errors++;
			}
		}

		if (gebr_geoxml_program_parameter_get_required(pp))
			validate->operations.append_text(validate->data, _("  REQUIRED "));

		/* enum details */
		if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_ENUM) {
			GebrGeoXmlSequence *enum_option;

			gebr_geoxml_program_parameter_get_enum_option(pp, &enum_option, 0);

			if (enum_option == NULL) {
				validate_append_text_error(validate, _("\n        missing options"));
				validate->potential_errors++;
			}

			for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option)) {
				validate->operations.append_text(validate->data, "\n");
				if (validate->isubpar != -1)
					validate->operations.append_text(validate->data, "      ");

				validate->operations.append_text(validate->data, "        %s (%s)",
						     gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)),
						     gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option)));
			}
		}

		validate->operations.append_text(validate->data, "\n\n");
	} else {
		GebrGeoXmlSequence *subpar;
		GebrGeoXmlSequence *instance;

		validate->operations.append_text(validate->data, "    %2d: ", validate->ipar);
		validate_append_check(validate, gebr_geoxml_parameter_get_label(parameter),
				      gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PARAMETER_LABEL)->flags, NULL);

		if (gebr_geoxml_parameter_group_get_is_instanciable(GEBR_GEOXML_PARAMETER_GROUP(parameter)))
			validate->operations.append_text(validate->data, _("   [Instanciable]\n"));
		else
			validate->operations.append_text(validate->data, "\n");

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		subpar = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));
		validate->isubpar = 0;
		for (; subpar != NULL; gebr_geoxml_sequence_next(&subpar), ++validate->isubpar)
			show_parameter(validate, GEBR_GEOXML_PARAMETER(subpar));
		validate->isubpar = -1;
	}
}

