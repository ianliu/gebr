/*   libgebr - GeBR Library
 *   Copyright (C) 2010 GeBR core team (http://www.gebrproject.com/)
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
#include <libgebr.h>

gboolean hide_pars_in_default = FALSE;
gboolean dump_all = FALSE;

gchar **flowfn = NULL;

/* Private functions */
void parse_command_line(int argc, char **argv);
void show_parameter(GebrGeoXmlParameter * parameter);
GString * dump_program_parameter(GebrGeoXmlProgramParameter * pp, gboolean is_subpar);

/* Command-line parameters definition */
static GOptionEntry entries[] = {
        {"hide-defaults", 'H', 0, G_OPTION_ARG_NONE, &hide_pars_in_default, "hide parameters set to default", NULL},
        {"dump-all", 'A', 0, G_OPTION_ARG_NONE, &dump_all, "dump all parameters", NULL},
	{G_OPTION_REMAINING, 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &flowfn, "",
	 "flow1.mnu flow2.mnu ..."},
	{NULL}
};

int main(int argc, char **argv)
{

	GebrGeoXmlDocument *doc;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *seq;
	int nprog;
	int iprog;
	int nflow;
	int iflow;

	parse_command_line(argc, argv);

	if (flowfn == NULL)
		return 0;

	nflow = 0;
	while (flowfn[++nflow] != NULL) ;

	for (iflow = 0; iflow < nflow; iflow++) {
		if (gebr_geoxml_document_load((GebrGeoXmlDocument **) (&flow), flowfn[iflow], TRUE, NULL) != GEBR_GEOXML_RETV_SUCCESS){
                        fprintf(stderr,"Unable to load %s\n", flowfn[iflow]);
                        break;
                }
		doc = GEBR_GEOXML_DOC(flow);

                printf("%s -- %s\n",
                       gebr_geoxml_document_get_title(doc),
                       gebr_geoxml_document_get_description(doc));

                printf("By %s <%s>, %s\n",
                       gebr_geoxml_document_get_author(doc),
                       gebr_geoxml_document_get_email(doc),
                       gebr_localized_date(gebr_geoxml_document_get_date_modified(doc)));

		nprog = gebr_geoxml_flow_get_programs_number(flow);

		if (nprog == 0)
			goto out;

		printf("Flow with %d program(s)\n", nprog);

		gebr_geoxml_flow_get_program(flow, &seq, 0);
		for (iprog = 0; iprog < nprog; iprog++) {
			GebrGeoXmlProgram *prog;
			GebrGeoXmlParameter *parameter;

			prog = GEBR_GEOXML_PROGRAM(seq);

			printf("\nProgram %s\n",gebr_geoxml_program_get_title(prog));
                        parameter = GEBR_GEOXML_PARAMETER(gebr_geoxml_parameters_get_first_parameter
							  (gebr_geoxml_program_get_parameters(prog)));
                        while (parameter != NULL) {
                                show_parameter(parameter);
                                gebr_geoxml_sequence_next((GebrGeoXmlSequence **) & parameter);
                        }

			gebr_geoxml_sequence_next(&seq);
		}

 out:		
 		gebr_geoxml_document_free(doc);
	}

	return EXIT_SUCCESS;
}

void parse_command_line(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;

	/* Summary */
	context = g_option_context_new(NULL);
	g_option_context_set_summary(context,
				     "Show information about flow files for GeBR. Many flow files can\n"
				     "be inspected at once.");

	/* Description */
	g_option_context_set_description(context,"Copyright (C) 2010 Ricardo Biloti <biloti@gebrproject.com>");

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

GString *dump_program_parameter(GebrGeoXmlProgramParameter * pp, gboolean is_subpar)
{
	GString *value;
        GString *default_value;
        static GString *par = NULL;

        if (par == NULL)
                par = g_string_new(NULL);

        g_string_assign(par, "");
        
        value = gebr_geoxml_program_parameter_get_string_value(pp, FALSE);
        default_value = gebr_geoxml_program_parameter_get_string_value(pp, TRUE);

        if ( !dump_all && (
                           (hide_pars_in_default && (strcmp(value->str, default_value->str) == 0)) || 
                           strlen(value->str) == 0)){
                g_string_free(value, TRUE);
                return NULL;
        }

        if (is_subpar) g_string_append(par, "   ");
        g_string_append_printf(par, "   %s = %s\n",
                               gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(pp)), value->str);

	g_string_free(value, TRUE);
        return par;
}

void show_parameter(GebrGeoXmlParameter * parameter)
{
        GString *par;

	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)){
		par = dump_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE);
                if (par != NULL)
                        printf("%s", par->str);
        }
	else {
		GebrGeoXmlSequence *subpar;
		GebrGeoXmlSequence *instance;
                GString * group;
                gboolean show = FALSE;

                group = g_string_new(NULL);

		g_string_printf(group, "   Group %s\n", gebr_geoxml_parameter_get_label(parameter));

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
                while (instance != NULL){
                        subpar = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));
                        
                        while (subpar != NULL) {
                                par = dump_program_parameter(GEBR_GEOXML_PROGRAM_PARAMETER(subpar), TRUE);
                                if (par != NULL){
                                        g_string_append(group, par->str);
                                        show = TRUE;
                                }
                                gebr_geoxml_sequence_next(&subpar);
                        }
                        gebr_geoxml_sequence_next(&instance);
                        g_string_append(group, "\n");
                }
                if (show)
                        printf("%s", group->str);
                g_string_free(group, TRUE);
        }

	return;
}
