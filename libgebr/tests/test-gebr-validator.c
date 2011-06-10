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
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "e", "2.718[");
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      "abc", "[foo");
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "x", "pi*pi");
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "y", "pi*bo");
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
						      "xyz", "[bo]");
	gebr_validator_insert(validator, param, NULL, NULL);
	gebr_validator_validate_param(validator, param, &validated, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	gebr_validator_validate_expr(validator, "[bobo]", GEBR_GEOXML_PARAMETER_TYPE_STRING, &error);
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
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi3", "pi2*pi");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

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

	GebrGeoXmlParameter *param;

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						   "pi", "2.72");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	gebr_validator_rename(validator, param, "e", &affected, &error);
	varname = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));

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
	g_assert_no_error(error);

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

void test_gebr_validator_check_using_var(void)
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
	                                              "a", "12");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "100");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "b", "a+1");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_check_using_var(validator, "b", "a") == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "c", "b+1");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "d", "c*2");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_check_using_var(validator, "d", "a") == TRUE);
	g_assert(gebr_validator_check_using_var(validator, "d", "x") == FALSE);
}

void test_gebr_validator_evaluate(void)
{
	GebrValidator *validator;

	GebrGeoXmlFlow *loop;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlLine *line;
	GebrGeoXmlProject *proj;
	GebrGeoXmlProgram *loop_prog;

	GebrGeoXmlParameter *param;
	GebrGeoXmlParameter *iter_param;
	GError *error = NULL;
	GList *affected;

	gchar * value = NULL;
	gboolean eval_ok  = FALSE;
	gdouble double_value = 0.0;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&loop,
				  TEST_SRCDIR "/forloop.mnu",
				  FALSE, NULL);

	gebr_geoxml_flow_get_program(loop,(GebrGeoXmlSequence **) &loop_prog, 0);
	gebr_geoxml_program_set_status(loop_prog, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
	                               (GebrGeoXmlDocument**)&line,
	                               (GebrGeoXmlDocument**)&proj);


	gebr_geoxml_flow_add_flow(flow, loop);
	loop_prog = gebr_geoxml_flow_get_control_program(flow);
	gebr_geoxml_program_set_n(loop_prog, "1", "1", "7");
	iter_param = GEBR_GEOXML_PARAMETER(
			gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(iter_param),
						      FALSE, "1");

	g_assert(gebr_validator_insert(validator, iter_param, NULL, NULL));

	/* Tests for mathematical expressions */
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "a", "2");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      "b", "3");
	gebr_validator_insert(validator, param, &affected, &error);

	eval_ok = gebr_validator_evaluate(validator,
					  "a",
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, 2);
	g_free(value);

	eval_ok = gebr_validator_evaluate(validator,
					  "b",
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, 3);
	g_free(value);

	eval_ok = gebr_validator_evaluate(validator,
					  "a + b",
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, 5);
	g_free(value);

	eval_ok = gebr_validator_evaluate(validator,
					  "a * b",
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, 6);
	g_free(value);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "c", "a + b");
	gebr_validator_insert(validator, param, &affected, &error);

	eval_ok = gebr_validator_evaluate(validator,
					  "a + b + c",
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, 10);
	g_free(value);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "d", "c + a + b + 2");
	gebr_validator_insert(validator, param, &affected, &error);

	eval_ok = gebr_validator_evaluate(validator,
					  "a + b + c + d - 100", //2 + 3 + 5 + 12 - 100 = -78
					  GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_no_error(error);
	double_value = g_strtod(value,NULL);
	g_assert_cmpfloat(double_value, ==, -78);
	g_free(value);

	if (error)
		g_clear_error(&error);

	/* Tests for textual expressions */
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_STRING,
	                                              "str42", "the life universe and everything");
	gebr_validator_insert(validator, param, &affected, &error);

	eval_ok = gebr_validator_evaluate(validator,
					  "[str42]",
					  GEBR_GEOXML_PARAMETER_TYPE_STRING,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_cmpstr(value, ==, "the life universe and everything");
	g_free(value);

	eval_ok = gebr_validator_evaluate(validator,
					  "42: [str42]",
					  GEBR_GEOXML_PARAMETER_TYPE_STRING,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_cmpstr(value, ==, "42: the life universe and everything");
	g_free(value);

	if (error)
		g_clear_error(&error);

	eval_ok = gebr_validator_evaluate(validator,
					  "[a]: [str42]",
					  GEBR_GEOXML_PARAMETER_TYPE_STRING,
					  NULL,
					  &value,
					  &error);
	g_assert(eval_ok);
	g_assert_cmpstr(value, ==, "2: the life universe and everything");
	g_free(value);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "iter+1");
	gebr_validator_insert(validator, param, &affected, &error);
	g_assert_no_error(error);

	gebr_validator_evaluate(validator, "iter+1",
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
				loop_prog,
				&value,
				&error);
	g_assert_no_error(error);
	g_assert_cmpstr(value, ==, "[2, ..., 8]");
	g_free(value);

	gebr_validator_evaluate(validator, "x+1",
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
				loop_prog,
				&value,
				&error);
	g_assert_no_error(error);
	g_assert_cmpstr(value, ==, "[3, ..., 9]");
	g_free(value);

	gebr_validator_evaluate(validator, "out-[iter].dat",
				GEBR_GEOXML_PARAMETER_TYPE_STRING,
				loop_prog,
				&value,
				&error);
	g_assert_no_error(error);
	g_assert_cmpstr(value, ==, "out-[1, ..., 7].dat");
	g_free(value);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/validator/evaluate", test_gebr_validator_evaluate);
	g_test_add_func("/libgebr/validator/simple", test_gebr_validator_simple);
	g_test_add_func("/libgebr/validator/insert", test_gebr_validator_insert);
	g_test_add_func("/libgebr/validator/remove", test_gebr_validator_remove);
	g_test_add_func("/libgebr/validator/rename", test_gebr_validator_rename);
	g_test_add_func("/libgebr/validator/change", test_gebr_validator_change);
	g_test_add_func("/libgebr/validator/move", test_gebr_validator_move);
	g_test_add_func("/libgebr/validator/using_var", test_gebr_validator_check_using_var);

	return g_test_run();
}


