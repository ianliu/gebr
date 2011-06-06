/*   libgebr - GêBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <geoxml/geoxml.h>

#include "../gebr-expr.h"
#include "../gebr-iexpr.h"
#include "../gebr-validator.h"

void test_gebr_validator_simple(void)
{
	GebrValidator *validator;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;

	GebrGeoXmlParameter *param;
	gchar *validated = NULL;
	GError *error = NULL;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "pi", "3.14");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == TRUE);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "e", "2.718[");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      "abc", "[foo");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "x", "pi*pi");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == TRUE);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "y", "pi*bo");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      "xyz", "[bo]");
	g_assert(gebr_validator_validate_param(validator, param, &validated, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	g_assert(gebr_validator_validate_expr(validator, "[bobo]", GEBR_GEOXML_PARAMETER_TYPE_STRING, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(proj));
	gebr_validator_free(validator);
}

void test_gebr_validator_insert(void)
{
	GError *error = NULL;
	GList *affected = NULL;
	const gchar *pi_value;

	GebrValidator *validator;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;
	GebrGeoXmlParameter *pi;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);

	pi = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						   "pi", "3.14");
	gebr_validator_insert(validator, pi, &affected, &error);
	pi_value = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(pi), FALSE);
	g_assert_no_error(error);
	g_assert(affected == NULL);
	g_assert_cmpstr(pi_value, ==, "3.14");
}

void test_gebr_validator_remove(void)
{
	GebrValidator *validator;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;

	GebrGeoXmlParameter *param;
	GError *error = NULL;
	GList *affected;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);


	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	gebr_validator_remove (validator, param, &affected, &error);
	g_assert (affected == NULL);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi2", "pi*pi");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi3", "pi2*pi");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi", "3.14");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	gebr_validator_remove (validator, param, &affected, &error);
	g_assert (affected != NULL);
}

void test_gebr_validator_rename(void)
{
	GError *error = NULL;
	GList *affected = NULL;
	const gchar * varname = NULL;

	GebrValidator *validator;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;

	GebrGeoXmlParameter *e;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);

	e = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						   "pi", "2.72");
	gebr_validator_rename(validator,e,"e",&affected,&error);
	varname = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(e));

	g_assert(affected == NULL);
	g_assert(error == NULL);
	g_assert_cmpstr(varname, ==, "e");
}

void test_gebr_validator_change(void)
{
	GError *error = NULL;
	GList *affected = NULL;
	const gchar *pi_value;

	GebrValidator *validator;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;
	GebrGeoXmlParameter *pi;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);

	pi = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						   "pi", "3.14");

	gebr_validator_insert(validator, pi, NULL, NULL);
	gebr_validator_change_value(validator, pi, "3.1415", &affected, &error);
	pi_value = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(pi), FALSE);
	g_assert(affected == NULL);
	g_assert_no_error(error);
	g_assert_cmpstr(pi_value, ==, "3.1415");
}

void test_gebr_validator_move(void)
{
	GebrValidator *validator;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;

	GebrGeoXmlParameter *param, *pivot, *result;
	GError *error = NULL;
	GList *affected;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
	                               (GebrGeoXmlDocument**)&line,
	                               (GebrGeoXmlDocument**)&proj);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, &affected, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "40");
	g_assert (gebr_validator_insert(validator, param, &affected, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, &affected);
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(result), FALSE),==,"40");
	g_assert (gebr_geoxml_parameter_get_scope(result) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, &affected, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "40");
	g_assert (gebr_validator_insert(validator, param, &affected, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, &affected);
	g_assert (gebr_geoxml_parameter_get_scope(result) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, &affected, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "40");
	g_assert (gebr_validator_insert(validator, param, &affected, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, &affected);
	g_assert (gebr_geoxml_parameter_get_scope(result) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/validator/simple", test_gebr_validator_simple);
	g_test_add_func("/libgebr/validator/insert", test_gebr_validator_insert);
	g_test_add_func("/libgebr/validator/remove", test_gebr_validator_remove);
	g_test_add_func("/libgebr/validator/rename", test_gebr_validator_rename);
	g_test_add_func("/libgebr/validator/change", test_gebr_validator_change);
	g_test_add_func("/libgebr/validator/move", test_gebr_validator_move);

	return g_test_run();
}


