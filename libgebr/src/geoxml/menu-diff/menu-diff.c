/*   libgebr - GeBR Library
 *   Copyright (C) 2009 Ricardo Biloti (http://www.gebrproject.com/)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <glib.h>

#include <geoxml.h>
#include <libgebr.h>

int diff_count = 0;
gboolean nocolors = FALSE;
gchar **fnmenu = NULL;

gsize iprog = 0;
gsize ipar = 0;
gsize isubpar = 0;

GString *offset;
GString *prefix;

/* Prototypes */
void parse_command_line(int argc, char **argv);
void make_prefix(void);
void report(const gchar * label, const gchar * str1, const gchar * str2);
void report_help(const gchar * label, const gchar * str1, const gchar * str2);
void compare_parameters(GebrGeoXmlProgram * prog1, GebrGeoXmlProgram * prog2);
void compare_parameter(GebrGeoXmlParameter * par1, GebrGeoXmlParameter * par2);
void compare_program_parameter(GebrGeoXmlProgramParameter * ppar1, GebrGeoXmlProgramParameter * ppar2);
void compare_file(GebrGeoXmlProgramParameter * ppar1, GebrGeoXmlProgramParameter * ppar2);
const gchar *par_type_str(GebrGeoXmlParameter * par);

/* Command-line parameters definition */
static GOptionEntry entries[] = {
	{"nocolors", 'C', 0, G_OPTION_ARG_NONE, &nocolors, "disable colored output", NULL},
	{G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &fnmenu, "",
	 "menu1.mnu menu2.mnu"},
	{NULL}
};

int main(int argc, char **argv)
{
	GebrGeoXmlDocument *doc[2];
	GebrGeoXmlFlow *menu[2];
	GebrGeoXmlSequence *seq[2];
	int nmenu;

	parse_command_line(argc, argv);

	if (fnmenu == NULL) {
		fprintf(stderr, "You should provide two menus to compare.\n");
		exit(EXIT_FAILURE);
	}

	nmenu = 0;
	while (fnmenu[++nmenu] != NULL) ;

	if (nmenu < 2) {
		fprintf(stderr, "You should provide two menus to compare.\n");
		exit(EXIT_FAILURE);
	}

	diff_count = 0;

	prefix = g_string_new(NULL);
	offset = g_string_new(NULL);

	gebr_geoxml_document_load((GebrGeoXmlDocument **) (&menu[0]), fnmenu[nmenu - 2], TRUE, NULL);
	gebr_geoxml_document_load((GebrGeoXmlDocument **) (&menu[1]), fnmenu[nmenu - 1], TRUE, NULL);

	doc[0] = GEBR_GEOXML_DOC(menu[0]);
	doc[1] = GEBR_GEOXML_DOC(menu[1]);

	report("Comparing files", fnmenu[nmenu - 2], fnmenu[nmenu - 1]);
	diff_count--;

	report("Filename", gebr_geoxml_document_get_filename(doc[0]), gebr_geoxml_document_get_filename(doc[1]));

	report("Title", gebr_geoxml_document_get_title(doc[0]), gebr_geoxml_document_get_title(doc[1]));

	report("Description",
	       gebr_geoxml_document_get_description(doc[0]), gebr_geoxml_document_get_description(doc[1]));

	report("Author", gebr_geoxml_document_get_author(doc[0]), gebr_geoxml_document_get_author(doc[1]));

	report("Email", gebr_geoxml_document_get_email(doc[0]), gebr_geoxml_document_get_email(doc[1]));

	report("Created date",
	       gebr_geoxml_document_get_date_created(doc[0]), gebr_geoxml_document_get_date_created(doc[1]));

	report("Modified date",
	       gebr_geoxml_document_get_date_modified(doc[0]), gebr_geoxml_document_get_date_modified(doc[1]));

	report_help("Help", gebr_geoxml_document_get_help(doc[0]), gebr_geoxml_document_get_help(doc[1]));

	{
		GString *cat[2];

		cat[0] = g_string_new(NULL);
		cat[1] = g_string_new(NULL);

		gebr_geoxml_flow_get_category(menu[0], &seq[0], 0);
		gebr_geoxml_flow_get_category(menu[1], &seq[1], 0);

		while (seq[0] != NULL) {
			g_string_append(cat[0], gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq[0])));
			g_string_append(cat[0], "   ");

			gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & seq[0]);
		}

		while (seq[1] != NULL) {
			g_string_append(cat[1], gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq[1])));
			g_string_append(cat[1], "   ");

			gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & seq[1]);
		}

		report("Categories", cat[0]->str, cat[1]->str);

		g_string_free(cat[0], TRUE);
		g_string_free(cat[1], TRUE);
	}

	{
		GString *str[2];

		str[0] = g_string_new(NULL);
		str[1] = g_string_new(NULL);

		g_string_printf(str[0], "It has %ld program(s)", gebr_geoxml_flow_get_programs_number(menu[0]));
		g_string_printf(str[1], "It has %ld program(s)", gebr_geoxml_flow_get_programs_number(menu[1]));

		report("Programs", str[0]->str, str[1]->str);

		g_string_free(str[0], TRUE);
		g_string_free(str[1], TRUE);

		if (gebr_geoxml_flow_get_programs_number(menu[0]) != gebr_geoxml_flow_get_programs_number(menu[1]))
			goto out;
	}

	gebr_geoxml_flow_get_program(menu[0], &seq[0], 0);
	gebr_geoxml_flow_get_program(menu[1], &seq[1], 0);

	while (seq[0] != NULL) {

		GebrGeoXmlProgram *prog[2];

		prog[0] = GEBR_GEOXML_PROGRAM(seq[0]);
		prog[1] = GEBR_GEOXML_PROGRAM(seq[1]);

		iprog++;

		printf("+------------------------------------------------------------------------------\n");
		printf("|  Program %02i\n|\n", iprog);

		report("Title", gebr_geoxml_program_get_title(prog[0]), gebr_geoxml_program_get_title(prog[1]));

		report("Description",
		       gebr_geoxml_program_get_description(prog[0]), gebr_geoxml_program_get_description(prog[1]));

		report("stdin treatment",
		       (gebr_geoxml_program_get_stdin(prog[0]) ? "Read" : "Ignore"),
		       (gebr_geoxml_program_get_stdin(prog[1]) ? "Read" : "Ignore"));

		report("stdout treatment",
		       (gebr_geoxml_program_get_stdout(prog[0]) ? "Write" : "Ignore"),
		       (gebr_geoxml_program_get_stdout(prog[1]) ? "Write" : "Ignore"));

		report("stderr treatment",
		       (gebr_geoxml_program_get_stderr(prog[0]) ? "Append" : "Ignore"),
		       (gebr_geoxml_program_get_stderr(prog[1]) ? "Append" : "Ignore"));

		report("Executable", gebr_geoxml_program_get_binary(prog[0]), gebr_geoxml_program_get_binary(prog[1]));

		report("URL", gebr_geoxml_program_get_url(prog[0]), gebr_geoxml_program_get_url(prog[1]));

		report_help("Help", gebr_geoxml_program_get_help(prog[0]), gebr_geoxml_program_get_help(prog[1]));

		compare_parameters(prog[0], prog[1]);

		printf("+------------------------------------------------------------------------------\n");

		gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & seq[0]);
		gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & seq[1]);
	}

 out:
	printf("%d difference(s)\n\n", diff_count);
	gebr_geoxml_document_free(doc[0]);
	gebr_geoxml_document_free(doc[1]);

	g_string_free(prefix, TRUE);
	g_string_free(offset, TRUE);

	return EXIT_SUCCESS;
}

void parse_command_line(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;

	/* Summary */
	context = g_option_context_new(NULL);
	g_option_context_set_summary(context, "Show differences between menus.");

	/* Description */
	g_option_context_set_description(context, "Copyright (C) 2009 Ricardo Biloti <biloti@gmail.com>");

	g_option_context_add_main_entries(context, entries, NULL);

	/* Complain about unknown options */
	g_option_context_set_ignore_unknown_options(context, TRUE);

	/* Parse command line */
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

}

void compare_parameters(GebrGeoXmlProgram * prog1, GebrGeoXmlProgram * prog2)
{
	GebrGeoXmlParameter *par1;
	GebrGeoXmlParameter *par2;

	par1 =
	    GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter
				  (gebr_geoxml_program_get_parameters(prog1)));
	par2 =
	    GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter
				  (gebr_geoxml_program_get_parameters(prog2)));

	report("Parameters",
	       ((par1 == NULL) ? "It has no parameters" : "It has parameters"),
	       ((par2 == NULL) ? "It has no parameters" : "It has parameters"));

	if ((par1 != NULL) && (par2 != NULL)) {

		printf("|  +...........................................................................\n");
		printf("|  | Parameters\n|  |\n");

		while ((par1 != NULL) || (par2 != NULL)) {

			ipar++;
			compare_parameter(par1, par2);

			gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & par1);
			gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & par2);
		}

		printf("|  |\n|  +...........................................................................\n");

	}
}

void compare_parameter(GebrGeoXmlParameter * par1, GebrGeoXmlParameter * par2)
{

	report("Type", par_type_str(par1), par_type_str(par2));

	if (!strcmp(par_type_str(par1), par_type_str(par2))) {

		report("Label", gebr_geoxml_parameter_get_label(par1), gebr_geoxml_parameter_get_label(par2));
		if (gebr_geoxml_parameter_get_is_program_parameter(par1)) {
			compare_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(par1),
						  GEBR_GEOXML_PROGRAM_PARAMETER(par2));
		} else {	/* They are groups */

			GebrGeoXmlSequence *instance1;
			GebrGeoXmlSequence *instance2;
			GebrGeoXmlSequence *subpar1;
			GebrGeoXmlSequence *subpar2;

			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(par1), &instance1, 0);
			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(par2), &instance2, 0);

			subpar1 = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance1));
			subpar2 = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance2));

			while (subpar1 != NULL || subpar2 != NULL) {
				isubpar++;

				compare_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(subpar1),
							  GEBR_GEOXML_PROGRAM_PARAMETER(subpar2));

				gebr_geoxml_sequence_next(&subpar1);
				gebr_geoxml_sequence_next(&subpar2);
			}

			isubpar = 0;
		}
	}

}

void compare_program_parameter(GebrGeoXmlProgramParameter * ppar1, GebrGeoXmlProgramParameter * ppar2)
{
	GString *default1;
	GString *default2;

	if (ppar1 == NULL || ppar2 == NULL) {
		report("Present", (ppar1 != NULL ? "Yes" : "No"), (ppar2 != NULL ? "Yes" : "No"));
		return;
	}

	report("Keyword",
	       (ppar1 != NULL ? gebr_geoxml_program_parameter_get_keyword(ppar1) : ""),
	       (ppar2 != NULL ? gebr_geoxml_program_parameter_get_keyword(ppar2) : ""));

	report("Required",
	       (ppar1 != NULL ? (gebr_geoxml_program_parameter_get_required(ppar1) ? "Yes" : "No") : ""),
	       (ppar2 != NULL ? (gebr_geoxml_program_parameter_get_required(ppar2) ? "Yes" : "No") : ""));

	default1 = (ppar1 != NULL ? gebr_geoxml_program_parameter_get_string_value(ppar1, TRUE) : NULL);
	default2 = (ppar2 != NULL ? gebr_geoxml_program_parameter_get_string_value(ppar2, TRUE) : NULL);

	report("Default value", (ppar1 != NULL ? default1->str : ""), (ppar2 != NULL ? default2->str : ""));

	switch (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(ppar1))) {
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		compare_file(ppar1, ppar2);
		break;
	default:
		break;
	}

	if (ppar1 != NULL)
		g_string_free(default1, TRUE);

	if (ppar2 != NULL)
		g_string_free(default2, TRUE);

}

void compare_file(GebrGeoXmlProgramParameter * ppar1, GebrGeoXmlProgramParameter * ppar2)
{
	report("Accept directory?",
	       (gebr_geoxml_program_parameter_get_file_be_directory(ppar1) ? "Yes" : "No"),
	       (gebr_geoxml_program_parameter_get_file_be_directory(ppar2) ? "Yes" : "No"));

}

const gchar *par_type_str(GebrGeoXmlParameter * par)
{

	if (par == NULL)
		return "";

	switch (gebr_geoxml_parameter_get_type(par)) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		return "string";
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		return "integer";
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		return "file";
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
		return "flag";
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		return "real number";
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		return "range";
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM:
		return "enum";
	case GEBR_GEOXML_PARAMETER_TYPE_GROUP:
		return "group";
	default:
		return "UNKNOWN";
	}

}

void report(const gchar * label, const gchar * str1, const gchar * str2)
{
	if (strcmp(str1, str2)) {
		make_prefix();

		if (nocolors) {
			printf("%s%s\n", prefix->str, label);
			printf("%s- %s\n", offset->str, str1);
			printf("%s+ %s\n%s\n", offset->str, str2, offset->str);
		} else {
			printf("%s%s\n", prefix->str, label);
			printf("%s- %c[0;31m%s%c[0m\n", offset->str, 0x1B, str1, 0x1B);
			printf("%s+ %c[0;34m%s%c[0m\n%s\n", offset->str, 0x1B, str2, 0x1B, offset->str);
		}

		diff_count++;
	}
}

void report_help(const gchar * label, const gchar * str1, const gchar * str2)
{
	if (strcmp(str1, str2)) {
		make_prefix();

		if (nocolors) {
			printf("%s%s differ\n%s\n", prefix->str, label, offset->str);
		} else {
			printf("%s%c[0;31m%s differ%c[0m\n%s\n", prefix->str, 0x1B, label, 0x1B, offset->str);
		}
		diff_count++;
	}
}

void make_prefix(void)
{
	if (iprog == 0) {
		g_string_assign(prefix, "");
		g_string_assign(offset, "");
		return;
	}

	if (ipar == 0) {
		g_string_printf(prefix, "|    ");
		g_string_assign(offset, "|    ");
		return;
	}

	if (isubpar == 0) {
		g_string_printf(prefix, "|  |  %3i. ", ipar);
		g_string_assign(offset, "|  |       ");
		return;
	}

	g_string_printf(prefix, "|  |       %3i.%03i. ", ipar, isubpar);
	g_string_assign(offset, "|  |            ");
}
