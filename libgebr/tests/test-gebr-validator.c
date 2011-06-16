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

/* Some utilities macros for defining/evaluating expressions.
 * You must have a Fixture *fixture variable defined in the
 * calling scope.
 */

/*
 * DEF_FLOAT:
 *
 * Defines a float variable in scope @doc with @name and @value.
 */
#define DEF_FLOAT(doc, name, value) \
	G_STMT_START { \
	GError *error = NULL; \
	GebrGeoXmlParameter *param; \
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(doc), \
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
	                                              name, value); \
	gebr_validator_insert(fixture->validator, param, NULL, &error); \
	g_assert_no_error(error); \
	} G_STMT_END

#define DEF_FLOAT_WITH_ERROR(doc, name, value, domain, code) \
	G_STMT_START { \
	GError *error = NULL; \
	GebrGeoXmlParameter *param; \
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(doc), \
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
	                                              name, value); \
	gebr_validator_insert(fixture->validator, param, NULL, &error); \
	g_assert_error(error, domain, code); \
	g_clear_error(&error); \
	} G_STMT_END

/*
 * VALIDATE_FLOAT_EXPR:
 *
 * Validates the float expression @expr and check against @result
 */
#define VALIDATE_FLOAT_EXPR(expr, result) \
	G_STMT_START { \
	gchar *value; \
	GError *error = NULL; \
	gebr_validator_evaluate(fixture->validator, (expr), \
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
				&value, &error); \
	g_assert_no_error(error); \
	g_assert_cmpstr(value, ==, (result)); \
	g_free(value); \
	} G_STMT_END

/*
 * DEF_STRING:
 *
 * Defines a string variable in scope @doc with @name and @value.
 */
#define DEF_STRING(doc, name, value) \
	G_STMT_START { \
	GError *error = NULL; \
	GebrGeoXmlParameter *param; \
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(doc), \
	                                              GEBR_GEOXML_PARAMETER_TYPE_STRING, \
	                                              name, value); \
	gebr_validator_insert(fixture->validator, param, NULL, &error); \
	g_assert_no_error(error); \
	} G_STMT_END

/*
 * VALIDATE_STRING_EXPR:
 *
 * Validates the string expression @expr and check against @result
 */
#define VALIDATE_STRING_EXPR(expr, result) \
	G_STMT_START { \
	gchar *value; \
	GError *error = NULL; \
	gebr_validator_evaluate(fixture->validator, (expr), \
				GEBR_GEOXML_PARAMETER_TYPE_STRING, \
				&value, &error); \
	g_assert_no_error(error); \
	g_assert_cmpstr(value, ==, (result)); \
	g_free(value); \
	} G_STMT_END

typedef struct {
	GebrValidator *validator;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *proj;
} Fixture;

void
fixture_setup(Fixture *fixture, gconstpointer data)
{
	fixture->flow = GEBR_GEOXML_DOCUMENT(gebr_geoxml_flow_new());
	fixture->line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	fixture->proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());
	fixture->validator = gebr_validator_new(&fixture->flow,
						&fixture->line,
						&fixture->proj);
}

void
fixture_teardown(Fixture *fixture, gconstpointer data)
{
	gebr_geoxml_document_free(fixture->flow);
	gebr_geoxml_document_free(fixture->line);
	gebr_geoxml_document_free(fixture->proj);
	gebr_validator_free(fixture->validator);
}

GebrGeoXmlParameter *
fixture_add_loop(Fixture *fixture)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlDocument *loop;
	GebrGeoXmlProgram *loop_prog;
	GebrGeoXmlParameter *iter_param;

	gebr_geoxml_document_load(&loop, TEST_SRCDIR "/forloop.mnu", FALSE, NULL);
	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(loop), (GebrGeoXmlSequence**) &loop_prog, 0);
	gebr_geoxml_program_set_status(loop_prog, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	gebr_geoxml_flow_add_flow(GEBR_GEOXML_FLOW(fixture->flow), GEBR_GEOXML_FLOW(loop));
	loop_prog = gebr_geoxml_flow_get_control_program(GEBR_GEOXML_FLOW(fixture->flow));
	gebr_geoxml_program_set_n(loop_prog, "1", "1", "7");
	gebr_geoxml_flow_update_iter_dict_value(GEBR_GEOXML_FLOW(fixture->flow));
	seq = gebr_geoxml_document_get_dict_parameter(fixture->flow);
	iter_param = GEBR_GEOXML_PARAMETER(seq);

	g_assert(gebr_validator_insert(fixture->validator, iter_param, NULL, NULL));

	return iter_param;
}

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
	gebr_validator_insert(validator, pi, NULL, &error);
	pi_value = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(pi), FALSE);
	g_assert_no_error(error);
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

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
				       (GebrGeoXmlDocument**)&line,
				       (GebrGeoXmlDocument**)&proj);


	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	gebr_validator_remove (validator, param, NULL, &error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi2", "pi*pi");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi3", "pi2*pi");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi", "3.14");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	gebr_validator_remove (validator, param, NULL, &error);
}

void test_gebr_validator_rename(void)
{
	GError *error = NULL;
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
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	gebr_validator_rename(validator, param, "e", NULL, &error);
	varname = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));

	g_assert(error == NULL);
	g_assert_cmpstr(varname, ==, "e");
}

void test_gebr_validator_change(void)
{
	GError *error = NULL;
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

	gebr_validator_change_value(validator, pi, "3.1415", NULL, &error);
	pi_value = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(pi), FALSE);
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

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
	                               (GebrGeoXmlDocument**)&line,
	                               (GebrGeoXmlDocument**)&proj);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, NULL, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "40");
	g_assert (gebr_validator_insert(validator, param, NULL, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, NULL);
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(result), FALSE),==,"40");
	g_assert (gebr_geoxml_parameter_get_scope(result) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, NULL, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "40");
	g_assert (gebr_validator_insert(validator, param, NULL, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, NULL);
	g_assert (gebr_geoxml_parameter_get_scope(result) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);


	pivot = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "12");
	g_assert (gebr_validator_insert(validator, pivot, NULL, &error) == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "40");
	g_assert (gebr_validator_insert(validator, param, NULL, &error) == TRUE);

	result = gebr_validator_move(validator, param, pivot, NULL);
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

	flow = gebr_geoxml_flow_new();
	line = gebr_geoxml_line_new();
	proj = gebr_geoxml_project_new();

	validator = gebr_validator_new((GebrGeoXmlDocument**)&flow,
	                               (GebrGeoXmlDocument**)&line,
	                               (GebrGeoXmlDocument**)&proj);


	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "a", "12");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "100");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "b", "a+1");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_check_using_var(validator, "b", GEBR_GEOXML_DOCUMENT_TYPE_PROJECT, "a") == TRUE);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "c", "b+1");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "d", "c*2");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_check_using_var(validator, "d", GEBR_GEOXML_DOCUMENT_TYPE_PROJECT, "a") == TRUE);
	g_assert(gebr_validator_check_using_var(validator, "d", GEBR_GEOXML_DOCUMENT_TYPE_PROJECT, "x") == FALSE);
}

void test_gebr_validator_evaluate(Fixture *fixture, gconstpointer data)
{
	fixture_add_loop(fixture);

	/* Tests for mathematical expressions */
	DEF_FLOAT(fixture->proj, "a", "2");
	DEF_FLOAT(fixture->proj, "b", "3");
	DEF_FLOAT(fixture->proj, "c", "a+b");
	DEF_FLOAT(fixture->proj, "d", "c+a+b+2");

	VALIDATE_FLOAT_EXPR("a", "2");
	VALIDATE_FLOAT_EXPR("b", "3");
	VALIDATE_FLOAT_EXPR("a+b", "5");
	VALIDATE_FLOAT_EXPR("a*b", "6");
	VALIDATE_FLOAT_EXPR("a+b+c", "10");
	VALIDATE_FLOAT_EXPR("a+b+c+d-100", "-78");

	/* Tests for textual expressions */
	DEF_STRING(fixture->proj, "str42", "the life universe and everything");
	VALIDATE_STRING_EXPR("[str42]", "the life universe and everything");
	VALIDATE_STRING_EXPR("42: [str42]", "42: the life universe and everything");
	VALIDATE_STRING_EXPR("[a]: [str42]", "2: the life universe and everything");

	DEF_FLOAT(fixture->proj, "x", "iter+1");
	VALIDATE_FLOAT_EXPR("iter+1", "[2, ..., 8]");
	VALIDATE_FLOAT_EXPR("x+1", "[3, ..., 9]");
	VALIDATE_STRING_EXPR("out-[iter].dat", "[out-1.dat, ..., out-7.dat]");
}

void test_gebr_validator_eval1(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT(fixture->flow, "a", "2");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "b", "iter+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	VALIDATE_FLOAT_EXPR("a+2", "4");
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/validator/using_var", test_gebr_validator_check_using_var);

	g_test_add("/libgebr/validator/eval1", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_eval1,
		   fixture_teardown);

	g_test_add("/libgebr/validator/evaluate", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_evaluate,
		   fixture_teardown);

	g_test_add_func("/libgebr/validator/simple", test_gebr_validator_simple);
	g_test_add_func("/libgebr/validator/insert", test_gebr_validator_insert);
	g_test_add_func("/libgebr/validator/remove", test_gebr_validator_remove);
	g_test_add_func("/libgebr/validator/rename", test_gebr_validator_rename);
	g_test_add_func("/libgebr/validator/change", test_gebr_validator_change);
	g_test_add_func("/libgebr/validator/move", test_gebr_validator_move);

	return g_test_run();
}


