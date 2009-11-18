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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <glib.h>

#include <geoxml.h>
#include <libgebr.h>

enum FLAGS{
	EMPTY   =  1 << 0,
	CAPIT   =  1 << 1,
	NOBLK   =  1 << 2,
	MTBLK   =  1 << 3,
	NOPNT   =  1 << 4,
	EMAIL   =  1 << 5,
	FILEN   =  1 << 6
};

#define  VALID     TRUE
#define  INVALID   FALSE

int error_count;
int global_error_count;

gboolean             all      = FALSE;
gboolean             filename = FALSE;
gboolean             title    = FALSE;
gboolean             desc     = FALSE;
gboolean             author   = FALSE;
gboolean             dates    = FALSE;
gboolean             category = FALSE;
gboolean             mhelp    = FALSE;
gint                 ehelp    = -1;
gboolean             progs    = FALSE;
gboolean             params   = FALSE;
gboolean             nocolors = FALSE;
gchar**              menu     = NULL;

/* Private functions */
gboolean     check (const gchar *str, int flags);
const gchar* report(const gchar *str, int flags);
void         parse_command_line(int argc, char **argv);
void         show_parameter(GebrGeoXmlParameter *parameter, gint ipar);
void         show_program_parameter(GebrGeoXmlProgramParameter *pp, gint ipar, guint isubpar);


/* Command-line parameters definition */
static GOptionEntry entries[] =    {
	{ "all", 'a', 0, G_OPTION_ARG_NONE, &all , "show all tags (but -P)", NULL},
	{ "filename", 'f', 0, G_OPTION_ARG_NONE, &filename, "show filename", NULL},
	{ "title", 't', 0, G_OPTION_ARG_NONE, &title, "show title", NULL},	
	{ "desc", 'd', 0, G_OPTION_ARG_NONE, &desc, "show description", NULL},
	{ "author", 'A', 0, G_OPTION_ARG_NONE, &author, "show authors", NULL},
        { "dates", 'D', 0,  G_OPTION_ARG_NONE, &dates, "show created/modified dates", NULL},
	{ "mhelp", 'H', 0, G_OPTION_ARG_NONE, &mhelp, "check help status", NULL},
	{ "ehelp", 'e', 0, G_OPTION_ARG_INT, &ehelp, "extract help (0 for menu, >0 for program)", "index"},
	{ "category", 'c', 0, G_OPTION_ARG_NONE, &category, "show categories", NULL},
	{ "progs", 'p', 0, G_OPTION_ARG_NONE, &progs, "show programs", NULL},
	{ "params", 'P', 0, G_OPTION_ARG_NONE, &params, "show parameters", NULL},
        { "nocolors", 'C', 0, G_OPTION_ARG_NONE, &nocolors, "disable colored output", NULL},
	{ G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &menu, "", "menu1.mnu menu2.mnu ..." },
	{ NULL }
};


int main (int argc, char** argv)
{
	
	GebrGeoXmlDocument *     doc;
	GebrGeoXmlFlow *         flow;
	GebrGeoXmlSequence *     seq;
	int                  nprog;
	int                  iprog;
	int                  nmenu;
	int                  imenu;
	
	parse_command_line(argc, argv);

	if (menu == NULL)
		return 0;
	
	nmenu = 0;
	while (menu[++nmenu] != NULL);

	global_error_count = 0;
	for (imenu=0; imenu<nmenu; imenu++){
		error_count = 0;

		gebr_geoxml_document_load((GebrGeoXmlDocument**) (&flow), menu[imenu]);
		doc = GEBR_GEOXML_DOC(flow);
		
		if (filename || all){
			gchar *   filename;
			filename = g_path_get_basename (menu[imenu]);
			if (strcmp(gebr_geoxml_document_get_filename(doc), filename)){
                                if (nocolors){
                                        printf("Filename:      %s **DIFFERS FROM SET** %s\n",
                                               filename, 
                                               report(gebr_geoxml_document_get_filename(doc), EMPTY));
                                }else{
                                        printf("Filename:      %s %c[0;31;40mDIFFERS FROM SET%c[1;37;40m %s\n",
                                               filename, 0x1B, 0x1B,
                                               report(gebr_geoxml_document_get_filename(doc), EMPTY));
                                }
				error_count++;
			}
			else{
				printf("Filename:      %s\n",
				       report(gebr_geoxml_document_get_filename(doc),
					      NOBLK | MTBLK | FILEN));
			}
			g_free(filename);
		}

		if (title || all)
			printf("Title:         %s\n",
			       report(gebr_geoxml_document_get_title(doc),
				      EMPTY | NOBLK | NOPNT | MTBLK));
			       
		if (desc || all)
			printf("Description:   %s\n",
			       report(gebr_geoxml_document_get_description(doc),
				      EMPTY | CAPIT | NOBLK | MTBLK | NOPNT) );

		if (author || all)
			printf("Author:        %s <%s>\n",
			       report(gebr_geoxml_document_get_author(doc),
				      EMPTY | CAPIT | NOBLK | MTBLK | NOPNT),
			       report(gebr_geoxml_document_get_email(doc), EMAIL));

                if (dates || all){
                        printf("Created:       %s\n",
                               strlen(gebr_geoxml_document_get_date_created(doc)) ? 
                               gebr_localized_date(gebr_geoxml_document_get_date_created(doc)) :
                               report("", EMPTY));

                        printf("Modified:      %s\n",
                               strlen(gebr_geoxml_document_get_date_modified(doc)) ? 
                               gebr_localized_date(gebr_geoxml_document_get_date_modified(doc)) :
                               report("", EMPTY));
                }
		
		if (mhelp || all)
			printf("Help:          %s\n",
			       strlen(gebr_geoxml_document_get_help(doc))?"Defined":report("", EMPTY));

		if (ehelp == 0){
			printf("%s",gebr_geoxml_document_get_help(doc));
			goto out2;
		}
		
		if (category || all){
			int              icat;
			int              ncat;
			ncat = gebr_geoxml_flow_get_categories_number(flow);
			
			if (ncat == 0)
				printf("Category:      %s\n", report("", EMPTY));
			
			for (icat = 0; icat < ncat; icat++){
				GebrGeoXmlSequence *cat;

				gebr_geoxml_flow_get_category(flow, &cat, icat);
				printf("Category:      %s\n",
				       report(gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE(cat)),
					      EMPTY | CAPIT | NOBLK | MTBLK | NOPNT));
				
			}
		}

		nprog = gebr_geoxml_flow_get_programs_number(flow);
		if (ehelp > 0 && ehelp <= nprog){
			gebr_geoxml_flow_get_program (flow, &seq, ehelp-1);
			printf("%s", gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(seq)));
			goto out2;
		}
			
		if (nprog == 0 || (!progs && !all && !params))
			goto out;

		printf("Menu with:     %d program(s)\n", nprog);
		
		gebr_geoxml_flow_get_program (flow, &seq, 0);
		for (iprog = 0; iprog < nprog; iprog++){
			GebrGeoXmlProgram *    prog;
			GebrGeoXmlParameter *  parameter;
			gint               ipar = 0;
			
			prog = GEBR_GEOXML_PROGRAM(seq);
			
			printf(">>Program:     %d\n", iprog+1);
			printf("  Title:       %s\n",
			       report(gebr_geoxml_program_get_title(prog),
				      EMPTY | NOBLK | MTBLK));
			printf("  Description: %s\n",
			       report(gebr_geoxml_program_get_description(prog),
				       EMPTY | CAPIT | NOBLK | MTBLK | NOPNT));
			printf("  In/out/err:  %s/%s/%s\n",
			       (gebr_geoxml_program_get_stdin(prog)?"Read":"Ignore"),
			       (gebr_geoxml_program_get_stdout(prog)?"Write":"Ignore"),
			       (gebr_geoxml_program_get_stdin(prog)?"Append":"Ignore"));
			printf("  Binary:      %s\n",
			       report(gebr_geoxml_program_get_binary(prog), EMPTY));
			printf("  URL:         %s\n",
			       report(gebr_geoxml_program_get_url(prog), EMPTY));
			printf("  Help:        %s\n",
			       strlen(gebr_geoxml_program_get_help(prog))?"Defined":report("", EMPTY));

			if (params || all){
				printf("  >>Parameters:\n");

				parameter = GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter(gebr_geoxml_program_get_parameters(prog)));

				while (parameter != NULL){

					show_parameter(parameter, ++ipar);
				
					gebr_geoxml_sequence_next((GebrGeoXmlSequence **) &parameter);
				}
			}
		
			gebr_geoxml_sequence_next(&seq);
		}
	
	out:    printf("%d potencial error(s)\n\n", error_count);
	out2:	gebr_geoxml_document_free(doc);
		global_error_count += error_count;
	}

	return global_error_count;
}


/* ------------------------------------------------------------*/
/* Checks */

/* VALID if str is not empty */
gboolean
check_is_not_empty(const gchar *str)
{
	return (strlen(str) ? VALID : INVALID);
}

/* VALID if str does not start with lower case letter */
gboolean
check_no_lower_case(const gchar* sentence)
{
	
	if (!check_is_not_empty(sentence))
		return VALID;

	if (g_ascii_islower(sentence[0]))
		return INVALID;

	return VALID;
}

/* VALID if str has not consecutive blanks */
gboolean
check_no_multiple_blanks(const gchar *str)
{
	regex_t pattern;
	regcomp (&pattern, "   *", REG_NOSUB);
	return (regexec (&pattern, str, 0, 0, 0) ? VALID : INVALID );
}

/* VALID if str does not start or end with blanks/tabs */
gboolean
check_no_blanks_at_boundaries(const gchar *str)
{
	int n = strlen(str);

	if (n == 0)
		return VALID;
	
	if (str[0]   == ' '  ||
	    str[0]   == '\t' ||
	    str[n-1] == ' '  ||
	    str[n-1] == '\t' )
		return INVALID;

	return VALID;
}

/* VALID if str does not end if a punctuation mark */
gboolean
check_no_punctuation_at_end(const gchar *str)
{
	int n = strlen(str);

	if (n == 0)
		return VALID;

	if (g_ascii_ispunct(str[n-1]))
		return INVALID;

	return VALID;
}

/* VALID if str has not path and ends with .mnu */
gboolean
check_menu_filename(const gchar *str)
{
	gchar *base;

	base = g_path_get_basename(str);
	if (strcmp(base, str)){
		g_free(base);
		return INVALID;
	}
	g_free(base);
	
	if (!g_str_has_suffix (str, ".mnu"))
		return INVALID;
	
	return VALID;
}

/* VALID if str xxx@yyy, with xxx  composed    */
/* by letter, digits, underscores, dots and    */
/* dashes, and yyy composed by at least one    */
/* dot, letter digits and dashes.              */
/*                                             */
/* To do something right, take a look at       */
/* http://en.wikipedia.org/wiki/E-mail_address */
gboolean
check_is_email(const gchar *str)
{
	regex_t pattern;
	regcomp (&pattern, "^[a-z0-9_.-][a-z0-9_.-]*@[a-z0-9.-]*\\.[a-z0-9-][a-z0-9-]*$", REG_NOSUB | REG_ICASE);
	return ( !regexec (&pattern, str, 0, 0, 0) ? VALID : INVALID );
}

/* VALID if str passed through all selected tests */
gboolean
check (const gchar *str, int flags)
{

	gboolean result = VALID;
	
	if (flags & EMPTY) result = result && check_is_not_empty(str);
	if (flags & CAPIT) result = result && check_no_lower_case(str);
	if (flags & NOBLK) result = result && check_no_blanks_at_boundaries(str);
	if (flags & MTBLK) result = result && check_no_multiple_blanks(str);
	if (flags & NOPNT) result = result && check_no_punctuation_at_end(str);
	if (flags & EMAIL) result = result && check_is_email(str);
	if (flags & FILEN) result = result && check_menu_filename(str);
	
	return result;
}

const gchar *
report(const gchar *str, int flags)
{

	if (check(str, flags))
		return str;
	else{
		GString *msg;
		msg = g_string_new(NULL);
		if (check_is_not_empty(str))
                        (nocolors) ? 
                                g_string_printf(msg,"**%s**", str) :
                                g_string_printf(msg,"%c[0;37;40m%s%c[0m", 0x1B, str, 0x1B);
		else
                        (nocolors) ?
                                g_string_printf(msg,"**UNSET**") :
                                g_string_printf(msg,"%c[0;31mUNSET%c[0m", 0x1B, 0x1B);
		error_count++;
		
		return msg->str;
	}
}

void
parse_command_line(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	
	/* Summary */
	context = g_option_context_new (NULL);
	g_option_context_set_summary (context,
				      "Show information about menu files for GeBR. Many menu files can\n"
				      "be inspected at once. To obtain the most verbose information try\n"
                                      "options -a -P togheter.");

	/* Description */
	g_option_context_set_description (context,
					  "Some checks are performed. They are:\n"
					  " 1. No blanks or tabs at the start or end of sentences\n"
					  " 2. First letter of sentences capitalized\n"
					  " 3. Well-formed email (just a guess)\n"
					  " 4. Sentences not ended by period mark\n"
					  " 5. Filename correted set\n"
					  " 6. Empty tags\n\n"
					  "Copyright (C) 2008 Ricardo Biloti <biloti@gmail.com>");
	
	g_option_context_add_main_entries (context, entries, NULL);
	
	/* Complain about unknown options */
	g_option_context_set_ignore_unknown_options (context, FALSE);
	
	/* Parse command line */
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE){
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	g_option_context_free (context);

	if ( !(all || filename || title || desc || author || dates || mhelp || (ehelp != -1) || category || progs) )
		all = TRUE;



}

void
show_program_parameter(GebrGeoXmlProgramParameter *pp, gint ipar, guint isubpar)
{

        GString *default_value;

 
        if (isubpar) {
                printf("       %2d.%02d: %s\n", ipar, isubpar,
                       report(gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(pp)),
                              EMPTY | CAPIT | NOBLK | MTBLK | NOPNT));
        }else{
                printf("    %2d: %s\n", ipar,
                       report(gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(pp)),
                              EMPTY | CAPIT | NOBLK | MTBLK | NOPNT));
        }

	printf("        ");
        if (isubpar) 
                printf("      ");

	switch (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp))){
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		printf("[string]     ");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		printf("[integer]    ");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
		printf("[file]       ");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
		printf("[flag]       ");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		printf("[real number]");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		printf("[range]      ");
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM:
		printf("[enum]       ");
		break;
	default:
		printf("[UNKNOWN]    ");
		break;
	}

	printf(" '%s'", report(gebr_geoxml_program_parameter_get_keyword(pp), EMPTY));

        default_value = gebr_geoxml_program_parameter_get_string_value(pp,TRUE);
	if ( strlen(default_value->str))
		printf(" [%s]", report(default_value->str, EMPTY));
        g_string_free(default_value, TRUE);
	
	if (gebr_geoxml_program_parameter_get_required(pp))
		printf("  REQUIRED ");

        /* enum details */
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(pp)) == GEBR_GEOXML_PARAMETER_TYPE_ENUM){
		GebrGeoXmlSequence *enum_option;
                
		gebr_geoxml_program_parameter_get_enum_option(pp, &enum_option, 0);
                
                if (enum_option == NULL){
                        printf("\n        %s", report("missing options", FILEN));
                }

		for (; enum_option != NULL; gebr_geoxml_sequence_next(&enum_option)){
			printf("\n");
			if (isubpar)
				printf("      ");
                        
			printf("        %s (%s)",
                               gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option)),
                               gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option)));
		}
	}
        
        
	printf("\n\n");
}

void
show_parameter(GebrGeoXmlParameter *parameter, gint ipar)
{
        
        if (gebr_geoxml_parameter_get_is_program_parameter(parameter))
                show_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), ipar, 0);

        else{
                GebrGeoXmlSequence  *subpar;
                GebrGeoXmlSequence  *instance;

                gint subipar = 0;

                printf("    %2d: %s\n", ipar,
                       report(gebr_geoxml_parameter_get_label(parameter), 
                              EMPTY | CAPIT | NOBLK | MTBLK | NOPNT));

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
                subpar = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));

                while (subpar != NULL){

                        show_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(subpar), ipar, ++subipar);
                        
                        gebr_geoxml_sequence_next(&subpar);
                }

        }
        
	return;
}
