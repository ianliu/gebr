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
	gchar *value = NULL; \
	GError *error = NULL; \
	gebr_validator_evaluate(fixture->validator, NULL, (expr), \
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
				&value, &error); \
	g_assert_no_error(error); \
	g_assert_cmpstr(value, ==, (result)); \
	g_free(value); \
	} G_STMT_END

/*
 * VALIDATE_FLOAT_EXPR_WITH_ERROR:
 *
 * Validates the float expression @expr and check against @result with error
 */
#define VALIDATE_FLOAT_EXPR_WITH_ERROR(expr, domain, code) \
	G_STMT_START { \
	gchar *value = NULL; \
	GError *error = NULL; \
	gebr_validator_evaluate(fixture->validator, NULL, (expr), \
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
				&value, &error); \
	g_assert_error(error, domain, code); \
	g_clear_error(&error); \
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

#define DEF_STRING_WITH_ERROR(doc, name, value, domain, code) \
	G_STMT_START { \
	GError *error = NULL; \
	GebrGeoXmlParameter *param; \
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(doc), \
	                                              GEBR_GEOXML_PARAMETER_TYPE_STRING, \
	                                              name, value); \
	gebr_validator_insert(fixture->validator, param, NULL, &error); \
	g_assert_error(error, domain, code); \
	g_clear_error(&error); \
	} G_STMT_END

/*
 * VALIDATE_STRING_EXPR:
 *
 * Validates the string expression @expr and check against @result
 */
#define VALIDATE_STRING_EXPR(expr, result) \
	G_STMT_START { \
	gchar *value = NULL; \
	GError *error = NULL; \
	gebr_validator_evaluate(fixture->validator, NULL, (expr), \
				GEBR_GEOXML_PARAMETER_TYPE_STRING, \
				&value, &error); \
	g_assert_no_error(error); \
	g_assert_cmpstr(value, ==, (result)); \
	g_free(value); \
	} G_STMT_END

#define VALIDATE_STRING_EXPR_WITH_ERROR(expr, domain, code) \
	G_STMT_START { \
		gchar *value = NULL; \
		GError *error = NULL; \
		gebr_validator_evaluate(fixture->validator, NULL, (expr), \
					GEBR_GEOXML_PARAMETER_TYPE_STRING, \
					&value, &error); \
		g_assert_error(error, domain, code); \
		g_clear_error(&error); \
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

void test_gebr_validator_simple(Fixture *fixture, gconstpointer data)
{
	// Project variables
	DEF_FLOAT(fixture->proj, "pi", "3.14");
	DEF_FLOAT(fixture->proj, "x", "pi*pi");
	DEF_FLOAT(fixture->proj, "y", "x/3");
	DEF_STRING(fixture->proj, "s1", "foo");

	// Line variables
	DEF_FLOAT(fixture->line, "x", "42");

	// Flow variables
	DEF_FLOAT(fixture->flow, "x", "10");

	// Test!
	VALIDATE_FLOAT_EXPR("x", "10");
	VALIDATE_FLOAT_EXPR("pi*x", "31.4");
	VALIDATE_STRING_EXPR("[pi]", "3.14");
	VALIDATE_STRING_EXPR("out.[x]", "out.10");

	// Define wrong variables
	DEF_FLOAT_WITH_ERROR(fixture->proj, "e", "2.718[", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	DEF_FLOAT_WITH_ERROR(fixture->proj, "y", "pi*bo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_STRING_WITH_ERROR(fixture->proj, "s2", "[foo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	DEF_STRING_WITH_ERROR(fixture->proj, "s3", "[bo]", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	// Validate wrong expressions
	VALIDATE_STRING_EXPR_WITH_ERROR("[bobo]", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
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
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi3", "pi2*pi");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
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
}

void test_gebr_validator_cyclic_errors(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT_WITH_ERROR(fixture->flow, "a", "2+x", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "b", "a+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "c", "b+", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	DEF_FLOAT(fixture->flow, "x", "10");
	VALIDATE_FLOAT_EXPR("x", "10");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("c", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT_WITH_ERROR(fixture->flow, "x", "10+", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("x", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("c", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT(fixture->flow, "x", "10");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("2+x+foo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "a", "2+x+foo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("c", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT_WITH_ERROR(fixture->flow, "z", "z+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
}

void test_gebr_validator_scope_errors(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT_WITH_ERROR(fixture->line, "a", "2+x", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	DEF_FLOAT(fixture->line, "x", "30");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR("x", "30");
	DEF_FLOAT(fixture->flow, "x", "10");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR("x", "10");

	DEF_FLOAT(fixture->line, "y", "2");
	DEF_FLOAT(fixture->flow, "b", "y+100");
	DEF_FLOAT_WITH_ERROR(fixture->proj, "z", "y*2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	DEF_FLOAT_WITH_ERROR(fixture->line, "y", "2+", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("z", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	// Define the same variable in all scopes
	DEF_FLOAT(fixture->proj, "f", "1");
	DEF_FLOAT(fixture->line, "f", "2");
	DEF_FLOAT(fixture->flow, "f", "3");

	GError *error = NULL;
	GebrGeoXmlParameter *param;
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_STRING,
	                                              "foo", "[f].out");
	gebr_validator_insert(fixture->validator, param, NULL, &error);
	g_assert_no_error(error);

	gchar *result = NULL;
	gebr_validator_evaluate(fixture->validator, param, NULL,
				GEBR_GEOXML_PARAMETER_TYPE_STRING,
				&result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "2.out");
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

	VALIDATE_STRING_EXPR("", "");
	VALIDATE_FLOAT_EXPR("", "");

	/* Tests for mathematical expressions */
	DEF_FLOAT(fixture->proj, "a", "2");
	DEF_FLOAT(fixture->proj, "b", "3");
	DEF_FLOAT(fixture->proj, "c", "a+b");
	DEF_FLOAT(fixture->proj, "d", "c+a+b+2");
	DEF_FLOAT(fixture->proj, "e", "1");

	VALIDATE_FLOAT_EXPR("a", "2");
	VALIDATE_FLOAT_EXPR("b", "3");
	VALIDATE_FLOAT_EXPR("a+b", "5");
	VALIDATE_FLOAT_EXPR("a*b", "6");
	VALIDATE_FLOAT_EXPR("a+b+c", "10");
	VALIDATE_FLOAT_EXPR("d", "12");
	VALIDATE_FLOAT_EXPR("a+b+c+d-100", "-78");

	/* Tests for textual expressions */
	DEF_STRING(fixture->proj, "str42", "the life universe and everything");
	DEF_STRING(fixture->proj, "foo", "FOO");
	DEF_STRING(fixture->proj, "bar", "[foo]BAR");
	VALIDATE_STRING_EXPR("str42", "str42");
	VALIDATE_STRING_EXPR("[str42]", "the life universe and everything");
	VALIDATE_STRING_EXPR("[str42][foo]", "the life universe and everythingFOO");
	VALIDATE_STRING_EXPR("str42+[foo]", "str42+FOO");
	VALIDATE_STRING_EXPR("str42+[bar]", "str42+FOOBAR");
	VALIDATE_STRING_EXPR("[str42] [foo]", "the life universe and everything FOO");
	VALIDATE_STRING_EXPR("42: [str42]", "42: the life universe and everything");
	VALIDATE_STRING_EXPR("[a]: [str42]", "2: the life universe and everything");

//	DEF_STRING(fixture->line, "foo", "foo");
//	DEF_STRING(fixture->flow, "var", "[bar][foo]");
//	VALIDATE_STRING_EXPR("[var]", "FOOBARfoo");

	fixture_add_loop(fixture);
	DEF_FLOAT(fixture->flow, "x", "iter+1");
	VALIDATE_FLOAT_EXPR("iter+1", "[2, ..., 8]");
	VALIDATE_FLOAT_EXPR("x+1", "[3, ..., 9]");
	VALIDATE_STRING_EXPR("out-[iter].dat", "[\"out-1.dat\", ..., \"out-7.dat\"]");
}

void test_gebr_validator_eval1(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT(fixture->flow, "a", "2");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("iter+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "b", "iter+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	VALIDATE_FLOAT_EXPR("a+2", "4");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "c", "b+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
}

void test_gebr_validator_weight(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT(fixture->proj, "p1", "1");
	DEF_FLOAT_WITH_ERROR(fixture->proj, "p2", "p1+p3", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT(fixture->proj, "p3", "3");
	DEF_FLOAT_WITH_ERROR(fixture->proj, "p4", "p2+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("p2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT(fixture->flow, "f1", "p1+1");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "f2", "f1+f3", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT(fixture->flow, "f3", "p3+1");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "f4", "f2+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("f2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
}

void test_gebr_validator_scope(Fixture *fixture, gconstpointer data)
{

	GError *error = NULL;
	GebrGeoXmlParameter *param;
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_STRING,
	                                              "a", "foo");
	gebr_validator_insert(fixture->validator, param, NULL, &error);
	g_assert_no_error(error);

	DEF_STRING(fixture->proj, "a", "bar");

	gebr_validator_remove(fixture->validator, param, NULL, &error);
	VALIDATE_STRING_EXPR("[a]","bar");
}

void test_gebr_validator_expression_check_using_var(Fixture *fixture, gconstpointer data)
{
	fixture_add_loop(fixture);

	/* Tests for mathematical expressions */
	DEF_FLOAT(fixture->flow, "a", "2");
	DEF_FLOAT(fixture->flow, "b", "3");
	DEF_STRING(fixture->flow, "c", "a[b]");
	DEF_STRING(fixture->flow, "d", "[c]");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "e", "d", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_TYPE_MISMATCH);

	g_assert(gebr_validator_expression_check_using_var(fixture->validator,
							   "banana",
							   GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
							   "a") == FALSE);
	g_assert(gebr_validator_expression_check_using_var(fixture->validator,
							   "[a]",
							   GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
							   "a"));
	g_assert(gebr_validator_expression_check_using_var(fixture->validator,
							   "[c].dat",
							   GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
							   "b"));
	g_assert(gebr_validator_expression_check_using_var(fixture->validator,
							   "[c].dat",
							   GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
							   "a") == FALSE);

}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add("/libgebr/validator/simple", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_simple,
		   fixture_teardown);

	g_test_add_func("/libgebr/validator/insert", test_gebr_validator_insert);
	g_test_add_func("/libgebr/validator/remove", test_gebr_validator_remove);
	g_test_add_func("/libgebr/validator/rename", test_gebr_validator_rename);
	g_test_add_func("/libgebr/validator/change", test_gebr_validator_change);
	g_test_add_func("/libgebr/validator/move", test_gebr_validator_move);
	g_test_add_func("/libgebr/validator/using_var", test_gebr_validator_check_using_var);

	g_test_add("/libgebr/validator/weight", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_weight,
		   fixture_teardown);

	g_test_add("/libgebr/validator/eval1", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_eval1,
		   fixture_teardown);

	g_test_add("/libgebr/validator/evaluate", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_evaluate,
		   fixture_teardown);

	g_test_add("/libgebr/validator/scope", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_scope,
		   fixture_teardown);

	g_test_add("/libgebr/validator/using_var_expression", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_expression_check_using_var,
		   fixture_teardown);

	g_test_add("/libgebr/validator/cyclic_errors", Fixture, NULL,
	           fixture_setup,
	           test_gebr_validator_cyclic_errors,
	           fixture_teardown);

	g_test_add("/libgebr/validator/scope_errors", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_scope_errors,
		   fixture_teardown);

	return g_test_run();
}


