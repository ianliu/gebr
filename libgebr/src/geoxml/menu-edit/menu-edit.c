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
#include <glib.h>
#include <geoxml.h>

gboolean             fixfname = FALSE;
gchar*               setfname = NULL;
gchar*               title    = NULL;
gchar*               desc     = NULL;
gchar*               author   = NULL;
gchar*               email    = NULL;
gboolean             helpdel  = FALSE;
gint                 iprog    = 0;
gchar*               binary   = NULL;
gchar*               url      = NULL;
gchar**              menu     = NULL;
gboolean             capt     = FALSE;

/* Command-line parameters definition */
static GOptionEntry entries[] =    {
	{ "iprog", 'i', 0, G_OPTION_ARG_INT, &iprog, "index of program to edit", NULL},
	{ "fixfname", 'f', 0, G_OPTION_ARG_NONE, &fixfname, "fix filename", NULL},
        { "setfname", 'F', 0, G_OPTION_ARG_FILENAME, &setfname, "set filename", NULL},
	{ "title", 't', 0, G_OPTION_ARG_STRING, &title, "set title", ""},	
	{ "desc", 'd', 0, G_OPTION_ARG_STRING, &desc, "set description", "one line description"},
	{ "author", 'A', 0, G_OPTION_ARG_STRING, &author, "set authors", "name"},
	{ "email", 'e', 0, G_OPTION_ARG_STRING, &email, "set email", "email address"},
	{ "helpdel", 0, 0, G_OPTION_ARG_NONE, &helpdel, "delete help text", NULL},
	{ "binary", 'b', 0, G_OPTION_ARG_STRING, &binary, "Binary command (without path)", "program/script"},
	{ "url", 'u', 0, G_OPTION_ARG_STRING, &url, "URL for a program", "http://www.../"},
	//{ "capitalize", 'c', 0, G_OPTION_ARG_NONE, &capt, "capitalize descriptions and labels", NULL},
	{ G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &menu, "", "menu1.mnu menu2.mnu ..." },
	{ NULL }
};

int main (int argc, char** argv)
{
	
	int                  nmenu;
	int                  imenu;
	
	GError *error = NULL;
	GOptionContext *context;
	
	/* Summary */
	context = g_option_context_new (NULL);
	g_option_context_set_summary (context,
				      "Edit tags of menu files for GeBR. Many menu files can\n"
				      "be edited at once, but using the same tag values to all menus.");
	
	/* Description */
	g_option_context_set_description (context,
					  "If iprog is 0, then title and description options refers to menu's\n"
					  "title and description. If iprog > 0, then ith program is edited.\n"
					  "Copyright (C) 2008 Ricardo Biloti <biloti@gmail.com>");
	
	g_option_context_add_main_entries (context, entries, NULL);
	
	/* Complain about unknown options */
	g_option_context_set_ignore_unknown_options (context, FALSE);
	
	/* Parse command line */
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE){
		fprintf(stderr, "%s: syntax error\n", argv[0]);
		fprintf(stderr, "Try %s --help\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	g_option_context_free (context);
	
	/* End of command line parse */
	
	if (menu == NULL)
		return 0;
	
	nmenu = 0;
	while (menu[++nmenu] != NULL);
	
	for (imenu=0; imenu<nmenu; imenu++){
		GeoXmlDocument *     doc;
		GeoXmlFlow *         flow;
		GeoXmlSequence *     seq;
		GeoXmlProgram *      prog;
		gint                 nprog;
		
		geoxml_document_load((GeoXmlDocument**) (&flow), menu[imenu]);
		doc = GEOXML_DOC(flow);
		
		nprog = geoxml_flow_get_programs_number(flow);
			
		if (fixfname){
			geoxml_document_set_filename(doc, menu[imenu]);
			geoxml_flow_get_program (flow, &seq, 0);
			prog = GEOXML_PROGRAM(seq);
			for (iprog=0; iprog < nprog; iprog++){
				geoxml_program_set_menu(prog, menu[imenu], iprog);
				geoxml_sequence_next(&seq);
			}
		}

                if (setfname != NULL){
                        geoxml_document_set_filename(doc, setfname);
			geoxml_flow_get_program (flow, &seq, 0);
			prog = GEOXML_PROGRAM(seq);
			for (iprog=0; iprog < nprog; iprog++){
				geoxml_program_set_menu(prog, setfname, iprog);
				geoxml_sequence_next(&seq);
			}
                }

		if (author != NULL)
			geoxml_document_set_author(doc, author);

		if (email != NULL)
			geoxml_document_set_email(doc, email);


		if (iprog == 0){
			
			if (title != NULL)
				geoxml_document_set_title(doc, title);
			
			if (desc != NULL)
				geoxml_document_set_description(doc, desc);

			if (helpdel)
				geoxml_document_set_help(doc, "");				

			if (url || binary)
				printf("To set URL ou binary, you must specify iprog\n");
		}
		else{	
			if (iprog > nprog){
				printf("Invalid program index for menu %s\n", menu[imenu]);
				goto out;
			}
		
			geoxml_flow_get_program (flow, &seq, iprog-1);

			prog = GEOXML_PROGRAM(seq);

			if (title != NULL)
				geoxml_program_set_title(prog, title);
			
			if (desc != NULL)
				geoxml_program_set_description(prog, desc);
			
			if (binary != NULL)
				geoxml_program_set_binary(prog, binary);

			if (url != NULL)
				geoxml_program_set_url(prog, url);

			if (helpdel)
				geoxml_program_set_help(prog, "");

		}	
	
	out:
		geoxml_document_save(doc, menu[imenu]);
		geoxml_document_free(doc);
	}

	return 0;
}
