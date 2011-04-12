/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2010 GeBR core team (http://www.gebrproject.com/)
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
#include <glib/gprintf.h>

#include <geoxml.h>
#include <libgebr.h>

int global_error_count;

GebrGeoXmlValidateOptions options;
gboolean nocolors = FALSE;
gchar **menu = NULL;

/* Private functions */
gboolean check(const gchar * str, int flags);
const gchar *report(const gchar * str, int flags);
void parse_command_line(int argc, char **argv);
void show_parameter(GebrGeoXmlParameter * parameter, gint ipar);
void show_program_parameter(GebrGeoXmlProgramParameter * pp, gint ipar, guint isubpar);

/* Command-line parameters definition */
static GOptionEntry entries[] = {
	{"all", 'a', 0, G_OPTION_ARG_NONE, &options.all, "show all tags (but -P)", NULL},
	{"title", 't', 0, G_OPTION_ARG_NONE, &options.title, "show title", NULL},
	{"desc", 'd', 0, G_OPTION_ARG_NONE, &options.desc, "show description", NULL},
	{"author", 'A', 0, G_OPTION_ARG_NONE, &options.author, "show authors", NULL},
	{"dates", 'D', 0, G_OPTION_ARG_NONE, &options.dates, "show created/modified dates", NULL},
	{"mhelp", 'H', 0, G_OPTION_ARG_NONE, &options.mhelp, "check help status", NULL},
	{"ehelp", 'e', 0, G_OPTION_ARG_INT, &options.ehelp, "extract help (0 for menu, >0 for program)", "index"},
	{"category", 'c', 0, G_OPTION_ARG_NONE, &options.category, "show categories", NULL},
	{"progs", 'p', 0, G_OPTION_ARG_NONE, &options.progs, "show programs", NULL},
	{"params", 'P', 0, G_OPTION_ARG_NONE, &options.params, "show parameters", NULL},
	{"nocolors", 'C', 0, G_OPTION_ARG_NONE, &nocolors, "disable colored output", NULL},
	{G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &menu, "",
	 "menu1.mnu menu2.mnu ..."},
	{NULL}
};

void append_text(gpointer data, const gchar * format, ...)
{
	va_list argp;
	va_start(argp, format);
	g_vprintf(format, argp);
	va_end(argp);
}

void append_text_error(gpointer data, gint failed_flags, const gchar * format, ...)
{
	gchar *string; 

	va_list argp;
	va_start(argp, format);
	string = g_strdup_vprintf(format, argp);
	va_end(argp);

	if (nocolors) 
		g_printf("**%s**", string);
	else
		g_printf("%c[0;37;40m%s%c[0m", 0x1B, string, 0x1B);

	g_free(string);
}

int main(int argc, char **argv)
{
	GebrGeoXmlValidate *validate;

	gebr_libinit("libgebr", argv[0]);

	/* default values */
	options.all = TRUE;
	options.ehelp = -1;

	parse_command_line(argc, argv);

	if (menu == NULL)
		return 0;

	GebrGeoXmlValidateOperations operations;
	operations.append_text = append_text;
	operations.append_text_emph = append_text;
	operations.append_text_error = append_text_error;
	operations.append_text_error_with_paths = NULL;
	validate = gebr_geoxml_validate_new(NULL, operations, options);
	global_error_count = 0;
	int nmenu = 0;
	while (menu[++nmenu] != NULL);
	int imenu;
	for (imenu = 0; imenu < nmenu; imenu++) {
		GebrGeoXmlFlow *flow;

		if (gebr_geoxml_document_load((GebrGeoXmlDocument **) (&flow), menu[imenu], TRUE, NULL) != GEBR_GEOXML_RETV_SUCCESS){
                        fprintf(stderr,"Unable to load %s\n", menu[imenu]);
                        break;
                }
		global_error_count += gebr_geoxml_validate_report_menu(validate, flow);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}
	gebr_geoxml_validate_free(validate);

	return global_error_count;
}

void parse_command_line(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;

	/* Summary */
	context = g_option_context_new(NULL);
	g_option_context_set_summary(context,
				     "Show information about menu files for GeBR. Many menu files can\n"
				     "be inspected at once. To obtain the most verbose information try\n"
				     "options -a -P together.");

	/* Description */
	g_option_context_set_description(context,
					 "Some checks are performed. They are:\n"
					 " 1. No blanks or tabs at the start or end of sentences\n"
					 " 2. First letter of sentences capitalized\n"
					 " 3. Well-formed email (just a guess)\n"
					 " 4. Sentences not ended by period mark\n"
					 " 5. Empty tags\n\n" "Copyright (C) 2008 Ricardo Biloti <biloti@gmail.com>");

	g_option_context_add_main_entries(context, entries, NULL);

	/* Complain about unknown options */
	g_option_context_set_ignore_unknown_options(context, FALSE);

	/* Parse command line */
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);
}

