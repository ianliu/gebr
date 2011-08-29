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

#include <stdlib.h>
#include <glib.h>
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

gboolean gebr_validator_update_vars(GebrValidator *, GebrGeoXmlDocumentType, GError **);

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
	gebr_geoxml_object_unref(param); \
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
	gebr_geoxml_object_unref(param); \
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
	gebr_validator_evaluate(fixture->validator, expr, \
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
				GEBR_GEOXML_DOCUMENT_TYPE_FLOW, \
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
	gebr_validator_evaluate(fixture->validator, expr, \
				GEBR_GEOXML_PARAMETER_TYPE_FLOAT, \
				GEBR_GEOXML_DOCUMENT_TYPE_FLOW, \
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
	gebr_geoxml_object_unref(param); \
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
	gebr_geoxml_object_unref(param); \
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
	gebr_validator_evaluate(fixture->validator, expr, \
				GEBR_GEOXML_PARAMETER_TYPE_STRING, \
				GEBR_GEOXML_DOCUMENT_TYPE_FLOW, \
				&value, &error); \
	g_assert_no_error(error); \
	g_assert_cmpstr(value, ==, (result)); \
	g_free(value); \
	} G_STMT_END

#define VALIDATE_STRING_EXPR_WITH_ERROR(expr, domain, code) \
	G_STMT_START { \
		gchar *value = NULL; \
		GError *error = NULL; \
		gebr_validator_evaluate(fixture->validator, expr, \
					GEBR_GEOXML_PARAMETER_TYPE_STRING, \
					GEBR_GEOXML_DOCUMENT_TYPE_FLOW, \
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
fixture_add_loop(Fixture *fixture)
{
	GebrGeoXmlProgram *prog_loop;
	GebrGeoXmlDocument *loop;
	GebrGeoXmlParameter *iter_param = NULL;

	gebr_geoxml_document_load(&loop, TEST_SRCDIR "/forloop.mnu", FALSE, NULL);
	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(loop), (GebrGeoXmlSequence**) &prog_loop, 0);
	gebr_geoxml_program_set_status(prog_loop, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	gebr_geoxml_program_control_set_n(prog_loop, "1", "1", "7");
	gebr_geoxml_flow_add_flow(GEBR_GEOXML_FLOW(fixture->flow), GEBR_GEOXML_FLOW(loop));
	iter_param = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(fixture->flow));
	g_assert(gebr_validator_insert(fixture->validator, iter_param, NULL, NULL));

	gebr_geoxml_object_unref(prog_loop);
	gebr_geoxml_document_free(loop);
	gebr_geoxml_object_unref(iter_param);
}

gboolean
fixture_change_iter_value (Fixture *fixture,
                           gchar *ini,
                           gchar *step,
                           gchar *n,
                           GError **error)
{
	GebrGeoXmlProgram *prog_loop;
	GebrGeoXmlProgramParameter *dict_iter;
	gchar *value = NULL;

	prog_loop = gebr_geoxml_flow_get_control_program((GebrGeoXmlFlow*)fixture->flow);

	if(!prog_loop) {
		fixture_add_loop(fixture);
		prog_loop = gebr_geoxml_flow_get_control_program((GebrGeoXmlFlow*)fixture->flow);
	}

	gebr_geoxml_program_control_set_n(prog_loop, step, ini, n);

	gebr_geoxml_flow_update_iter_dict_value((GebrGeoXmlFlow*)fixture->flow);
	dict_iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(fixture->flow)));

	value = gebr_geoxml_program_parameter_get_first_value(dict_iter, FALSE);
	gboolean retval = gebr_validator_change_value(fixture->validator, GEBR_GEOXML_PARAMETER(dict_iter), value, NULL, error);

	g_free(value);
	gebr_geoxml_object_unref(dict_iter);
	gebr_geoxml_object_unref(prog_loop);
	return retval;
}

void
fixture_teardown(Fixture *fixture, gconstpointer data)
{
	gebr_validator_free(fixture->validator);
	gebr_geoxml_document_free(fixture->flow);
	gebr_geoxml_document_free(fixture->line);
	gebr_geoxml_document_free(fixture->proj);
}

void test_gebr_validator_simple(Fixture *fixture, gconstpointer data)
{
	// Project variables
	DEF_FLOAT(fixture->proj, "pi", "3.14");
	DEF_FLOAT(fixture->proj, "x", "pi*pi");
	DEF_FLOAT(fixture->proj, "y", "x/3");
	DEF_STRING(fixture->proj, "s1", "foo");
	DEF_FLOAT(fixture->proj, "true", "pi<pi^2");
	DEF_FLOAT(fixture->proj, "false", "pi>pi^2");

	// Line variables
	DEF_FLOAT(fixture->line, "x", "42");

	// Flow variables
	DEF_FLOAT(fixture->flow, "x", "10");

	// Test!
	VALIDATE_FLOAT_EXPR("x", "10");
	VALIDATE_FLOAT_EXPR("10/5", "2.00000");
	VALIDATE_FLOAT_EXPR("pi*x", "31.40");
	VALIDATE_STRING_EXPR("x;[pi]", "x;3.14");

	DEF_STRING(fixture->proj, "quote", "\"");
	VALIDATE_STRING_EXPR("\"", "\"");
	VALIDATE_STRING_EXPR("\"[quote]\"", "\"\"\"");

	VALIDATE_STRING_EXPR("a\\nb", "a\\nb");
	VALIDATE_STRING_EXPR("[pi]", "3.14");
	VALIDATE_STRING_EXPR("out.[x]", "out.10");
	VALIDATE_FLOAT_EXPR("x>y", "1");
	VALIDATE_FLOAT_EXPR("x<y", "0");
	VALIDATE_FLOAT_EXPR("true", "1");
	VALIDATE_FLOAT_EXPR("false", "0");
	VALIDATE_FLOAT_EXPR("x==x", "1");
	VALIDATE_FLOAT_EXPR("x==y", "0");
	VALIDATE_FLOAT_EXPR("x>=y", "1");
	VALIDATE_FLOAT_EXPR("x<=y", "0");

	// Define wrong variables
	DEF_FLOAT_WITH_ERROR(fixture->proj, "e", "2.718[", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	DEF_FLOAT_WITH_ERROR(fixture->proj, "y", "pi*bo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_STRING_WITH_ERROR(fixture->proj, "s2", "[foo", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	DEF_STRING_WITH_ERROR(fixture->proj, "s3", "[bo]", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	// Validate wrong expressions
	VALIDATE_STRING_EXPR_WITH_ERROR("[bobo]", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("\"", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR(";", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("5;6", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("x=y", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("x=3", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("x=3;6", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);

	// Breaks bc wrapper with multiple errors in one line
	VALIDATE_FLOAT_EXPR_WITH_ERROR("??", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR("1", "1");

	// Big results
	VALIDATE_FLOAT_EXPR("10^67", "10000000000000000000000000000000000000000000000000000000000000000000");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("10^68", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_TOOBIG);
	DEF_FLOAT(fixture->proj, "line", "10^67");
	VALIDATE_STRING_EXPR("[line]/[line]/[line]", "10000000000000000000000000000000000000000000000000000000000000000000/10000000000000000000000000000000000000000000000000000000000000000000/10000000000000000000000000000000000000000000000000000000000000000000");
}

void test_gebr_validator_insert(Fixture *fixture, gconstpointer data)
{
	GError *error = NULL;
	gchar *pi_value;

	GebrGeoXmlDocument *proj = fixture->proj;
	GebrValidator *validator= fixture->validator;
	GebrGeoXmlParameter *pi;

	pi = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						   "pi", "3.14");
	gebr_validator_insert(validator, pi, NULL, &error);
	pi_value = gebr_geoxml_program_parameter_get_first_value(
			GEBR_GEOXML_PROGRAM_PARAMETER(pi), FALSE);
	g_assert_no_error(error);
	g_assert_cmpstr(pi_value, ==, "3.14");
	g_free(pi_value);
	gebr_geoxml_object_unref(pi);
}

void test_gebr_validator_remove(Fixture *fixture, gconstpointer data)
{
	GebrGeoXmlDocument *flow = fixture->flow;
	GebrGeoXmlDocument *line = fixture->line;
	GebrGeoXmlDocument *proj = fixture->proj;
	GebrValidator *validator= fixture->validator;

	GebrGeoXmlParameter *param, *x;
	GError *error = NULL;
	gchar *value = NULL;

	x = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "10+1");
	gebr_validator_insert(validator, x, NULL, &error);
	g_assert_no_error(error);

	gebr_geoxml_object_unref(x);
	x = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "10+2");
	gebr_validator_insert(validator, x, NULL, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(flow),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "x+1");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_evaluate_param(validator, param, &value, &error));
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"13");
	g_free(value);

	gebr_validator_remove (validator, x, NULL, &error);

	g_assert(gebr_validator_evaluate_param(validator, param, &value, &error));
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"12");
	g_free(value);

	gebr_geoxml_object_unref(param);
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi2", "pi*pi");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&error);

	gebr_geoxml_object_unref(param);
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi3", "pi2*pi");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	g_clear_error(&error);

	gebr_geoxml_object_unref(param);
	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi", "3.14");
	g_assert(gebr_validator_insert(validator, param, NULL, &error));
	g_assert_no_error(error);

	g_assert(gebr_validator_remove (validator, param, NULL, &error));
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		g_assert(gebr_validator_remove (validator, param, NULL, &error) == FALSE);
		exit(0);
	}
	g_test_trap_assert_failed();

	gebr_geoxml_object_unref(x);
	gebr_geoxml_object_unref(param);
}

void test_gebr_validator_rename(Fixture *fixture, gconstpointer data)
{
	GError *error = NULL;
	const gchar *varname = NULL;
	gchar *value = NULL;

	GebrGeoXmlDocument *line = fixture->line;
	GebrGeoXmlDocument *proj = fixture->proj;
	GebrValidator *validator= fixture->validator;

	GebrGeoXmlParameter *param, *rename_param;

	rename_param = gebr_geoxml_document_set_dict_keyword((line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi", "2.72");
	gebr_validator_insert(validator, rename_param, NULL, &error);
	g_assert_no_error(error);

	param = gebr_geoxml_document_set_dict_keyword((line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "pi^2");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	gebr_geoxml_object_unref(param);
	param = gebr_geoxml_document_set_dict_keyword((line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "e^2");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&error);

	gebr_validator_rename(validator, rename_param, "e", NULL, &error);
	varname = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(rename_param));
	g_assert(error == NULL);
	g_assert_cmpstr(varname, ==, "e");

	g_assert(gebr_validator_validate_expr(validator, "x", GEBR_GEOXML_PARAMETER_TYPE_FLOAT, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	g_clear_error(&error);

	gebr_validator_evaluate_param(validator, param, &value, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"7.3984");
	g_free(value);

	gebr_geoxml_object_unref(param);
	param = gebr_geoxml_document_set_dict_keyword((proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "pi", "3.14");
	gebr_validator_insert(validator, param, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_evaluate(validator, "x", GEBR_GEOXML_PARAMETER_TYPE_FLOAT, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &value, &error));
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"9.8596");
	g_free(value);

	gebr_geoxml_object_unref(param);
	gebr_geoxml_object_unref(rename_param);
}

void test_gebr_validator_change(Fixture *fixture, gconstpointer data)
{
	GError *error = NULL;
	gchar *pi_value;

	GebrGeoXmlDocument *proj = fixture->proj;
	GebrValidator *validator= fixture->validator;
	GebrGeoXmlParameter *pi;

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

	g_free(pi_value);
	gebr_geoxml_object_unref(pi);
}

void test_gebr_validator_move(Fixture *fixture, gconstpointer data)
{
	GebrGeoXmlDocument *line = fixture->line;
	GebrGeoXmlDocument *proj = fixture->proj;
	GebrValidator *validator= fixture->validator;

	GebrGeoXmlParameter *y, *x1, *x2, *moved;
	GError *error = NULL;
	gchar *value = NULL;

	x1 = gebr_geoxml_document_set_dict_keyword((proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "10");
	gebr_validator_insert(validator, x1, NULL, &error);
	g_assert_no_error(error);

	x2 = gebr_geoxml_document_set_dict_keyword((line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "x", "11");
	gebr_validator_insert(validator, x2, NULL, &error);
	g_assert_no_error(error);

	y = gebr_geoxml_document_set_dict_keyword((line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "y", "x*2");
	gebr_validator_insert(validator, y, NULL, &error);
	g_assert_no_error(error);

	gebr_validator_move(validator, y, NULL, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &moved, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_evaluate_param(validator, y, &value, &error));
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"20");
	g_free(value);

	gebr_validator_change_value(validator, x1, "10+", NULL, NULL);

	gebr_validator_evaluate_param(validator, y, &value, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	g_clear_error(&error);

	gebr_validator_change_value(validator, x1, "10", NULL, NULL);

	gebr_geoxml_object_unref(moved);
	gebr_validator_move(validator, moved, x2, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &moved, NULL, &error);
	g_assert_no_error(error);

	g_assert(gebr_validator_evaluate_param(validator, y, &value, &error));
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"22");
	g_free(value);

	gebr_geoxml_object_unref(y);
	gebr_geoxml_object_unref(x1);
	gebr_geoxml_object_unref(x2);
	gebr_geoxml_object_unref(moved);
}

void test_gebr_validator_cyclic_errors(Fixture *fixture, gconstpointer data)
{
	DEF_FLOAT_WITH_ERROR(fixture->flow, "a", "2+x", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "b", "a+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	DEF_FLOAT_WITH_ERROR(fixture->flow, "c", "b+", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
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
	gebr_validator_evaluate_param(fixture->validator, param, &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "2.out");
	g_free(result);
	gebr_geoxml_object_unref(param);
}

void test_gebr_validator_evaluate(Fixture *fixture, gconstpointer data)
{

	VALIDATE_STRING_EXPR("", "");
	VALIDATE_STRING_EXPR(" ", " ");
	VALIDATE_FLOAT_EXPR("", "");
	VALIDATE_FLOAT_EXPR_WITH_ERROR(" ", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_EMPTY_EXPR);

	// Empty String in dictionary
	DEF_STRING_WITH_ERROR(fixture->proj, "empty_str", "", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_EMPTY_EXPR);
	DEF_STRING(fixture->proj, "white_str", " ");
	DEF_FLOAT_WITH_ERROR(fixture->proj, "empty_num", "", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_EMPTY_EXPR);
	DEF_FLOAT_WITH_ERROR(fixture->proj, "white_num", " ", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_EMPTY_EXPR);

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

	DEF_STRING(fixture->line, "foo", "foo");
	DEF_STRING(fixture->flow, "var", "[bar][foo]");
	VALIDATE_STRING_EXPR("[var]", "FOOBARfoo");

	DEF_STRING(fixture->flow, "foo", "xxx");
	VALIDATE_STRING_EXPR("[var]", "FOOBARfoo");
	DEF_STRING(fixture->flow, "xyz", "file.[a]");
	DEF_FLOAT(fixture->flow, "a", "b");
	VALIDATE_STRING_EXPR("[xyz]", "file.2");

	fixture_change_iter_value(fixture, "1", "1", "7", NULL);
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
	VALIDATE_FLOAT_EXPR("f3", "4");
	DEF_FLOAT_WITH_ERROR(fixture->flow, "f4", "f2+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("f2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	DEF_FLOAT(fixture->flow, "p3", "30");
	VALIDATE_FLOAT_EXPR("f3", "4");
	DEF_FLOAT(fixture->line, "p3", "30");
	VALIDATE_FLOAT_EXPR("f3", "31");

	DEF_FLOAT(fixture->flow, "a", "3");
	VALIDATE_FLOAT_EXPR("a", "3");
	DEF_FLOAT(fixture->proj, "a", "1");
	VALIDATE_FLOAT_EXPR("a", "3");
	DEF_FLOAT(fixture->flow, "b", "a+1");
	VALIDATE_FLOAT_EXPR("b", "4");
	DEF_FLOAT_WITH_ERROR(fixture->line, "a", "1+", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	VALIDATE_FLOAT_EXPR("a", "3");
	VALIDATE_STRING_EXPR("[a]", "3");
	VALIDATE_FLOAT_EXPR("b", "4");
	VALIDATE_STRING_EXPR("[b]", "4");
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

	gebr_geoxml_object_unref(param);
}

void test_gebr_validator_update(Fixture *fixture, gconstpointer data)
{
	GebrGeoXmlDocument *flow1 = fixture->flow;
	GebrGeoXmlDocument *flow2 = GEBR_GEOXML_DOCUMENT(gebr_geoxml_flow_new());

	DEF_FLOAT(fixture->proj, "a", "1");
	VALIDATE_FLOAT_EXPR("a", "1");

	DEF_STRING(flow1, "a", "A");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_TYPE_MISMATCH);
	DEF_STRING(flow1, "b", "[a]B");
	VALIDATE_STRING_EXPR("[a]", "A");
	VALIDATE_STRING_EXPR("[b]", "AB");

	fixture->flow = NULL;
	gebr_validator_update(fixture->validator);

	fixture->flow = flow2;
	gebr_validator_update(fixture->validator);
	VALIDATE_FLOAT_EXPR("a", "1");
	VALIDATE_STRING_EXPR_WITH_ERROR("[b]", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	DEF_FLOAT(flow2, "b", "a+1");
	DEF_FLOAT(flow2, "a", "3");
	VALIDATE_FLOAT_EXPR("a", "3");
	VALIDATE_FLOAT_EXPR("b", "2");

	fixture->flow = flow1;
	gebr_validator_update(fixture->validator);
	VALIDATE_STRING_EXPR("[b]", "AB");

	fixture->flow = flow2;
	gebr_validator_update(fixture->validator);
	VALIDATE_FLOAT_EXPR("b", "2");

	gebr_geoxml_document_free(flow1);
}

void test_gebr_validator_string(Fixture *fixture, gconstpointer data)
{
	DEF_STRING(fixture->flow, "a", "ABC");
	DEF_STRING(fixture->flow, "b", "[a]");
	DEF_STRING(fixture->flow, "c", "[b]");
	DEF_STRING(fixture->flow, "d", "[c]DE");

	VALIDATE_STRING_EXPR("[d]", "ABCDE");
	VALIDATE_STRING_EXPR("[c]DE", "ABCDE");
	VALIDATE_STRING_EXPR("[c]", "ABC");
	VALIDATE_STRING_EXPR("[b]", "ABC");
	VALIDATE_STRING_EXPR("[a]", "ABC");
	VALIDATE_STRING_EXPR("[a]D[b]E[c]F", "ABCDABCEABCF");

	VALIDATE_STRING_EXPR("[[", "[");
	VALIDATE_STRING_EXPR("]]", "]");
	VALIDATE_STRING_EXPR("[[]]", "[]");
	VALIDATE_STRING_EXPR("]][[", "][");
	VALIDATE_STRING_EXPR("[[[a]]]", "[ABC]");
	VALIDATE_STRING_EXPR("[b][[s]]", "ABC[s]");
	VALIDATE_STRING_EXPR("a]]b[[c", "a]b[c");
}

void test_gebr_validator_divide_by_zero(Fixture *fixture, gconstpointer data)
{
	GError *error = NULL;
	GebrGeoXmlParameter *a, *b;
	gchar *value;

	VALIDATE_FLOAT_EXPR_WITH_ERROR("10/a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	// Divide by zero
	a = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->flow),
	                                          GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                          "a", "0");
	gebr_validator_insert(fixture->validator, a, NULL, &error);
	g_assert_no_error(error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("10/a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	b = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->flow),
	                                          GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                          "b", "10/a");
	gebr_validator_insert(fixture->validator, b, NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	g_clear_error(&error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	// Fix the divisor
	gebr_validator_change_value(fixture->validator, a, "10", NULL, &error);
	g_assert_no_error(error);
	gebr_validator_evaluate_param(fixture->validator, b, &value, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(value,==,"1.00000");
	g_free(value);
	VALIDATE_FLOAT_EXPR("b", "1.00000");

	// Divide by zero again
	gebr_validator_change_value(fixture->validator, a, "0", NULL, &error);
	g_assert_no_error(error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("10/a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	gebr_validator_evaluate_param(fixture->validator, b, &value, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	g_clear_error(&error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	fixture_change_iter_value(fixture, "1", "1", "7", NULL);

	VALIDATE_FLOAT_EXPR_WITH_ERROR("1/(iter-1)", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);

	// Divide by zero when iter has initial value
	gebr_validator_change_value(fixture->validator, a, "1/(iter-1)", NULL, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	g_clear_error(&error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	gebr_validator_change_value(fixture->validator, a, "iter-1", NULL, &error);
	g_assert_no_error(error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("10/a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	gebr_validator_evaluate_param(fixture->validator, b, &value, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	g_clear_error(&error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	// Divide by zero when iter has final value
//	gebr_validator_change_value(fixture->validator, a, "1/(iter-7)", NULL, &error);
//	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
//	g_clear_error(&error);
	gebr_validator_change_value(fixture->validator, a, "iter-7", NULL, &error);
	g_assert_no_error(error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("10/a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	gebr_validator_evaluate_param(fixture->validator, b, &value, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_RUNTIME);
	g_clear_error(&error);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("b", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	// Divide by not defined
	gebr_validator_remove(fixture->validator, a, NULL, &error);
	g_assert_no_error(error);
	gebr_validator_evaluate_param(fixture->validator, b, &value, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&error);

	gebr_geoxml_object_unref(a);
	gebr_geoxml_object_unref(b);
}

void test_gebr_validator_leaks(Fixture *fixture, gconstpointer data)
{
	GError *error = NULL;
	GebrGeoXmlParameter *a, *b;
	a = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->proj),
	                                              GEBR_GEOXML_PARAMETER_TYPE_STRING,
	                                              "a", "A");
	gebr_validator_insert(fixture->validator, a, NULL, &error);
	g_assert_no_error(error);
	b = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(fixture->line),
	                                              GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
	                                              "a", "1");
//	gebr_validator_insert(fixture->validator, b, NULL, &error);
//	g_assert_no_error(error);

	gebr_validator_update(fixture->validator);
	g_assert_no_error(error);
	gebr_validator_update_vars(fixture->validator, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, &error);
	g_assert_no_error(error);

	gebr_validator_validate_param(fixture->validator, a, NULL, &error);
	g_assert_no_error(error);
	gebr_validator_remove(fixture->validator, a, NULL, &error);
	g_assert_no_error(error);
//	gebr_validator_remove(fixture->validator, b, NULL, &error);
//	g_assert_no_error(error);
	gebr_geoxml_object_unref(a);
	gebr_geoxml_object_unref(b);
}

void test_gebr_geoxml_validate_flow(Fixture *fixture, gconstpointer data)
{
	GebrGeoXmlDocument *doc;
	GError *err = NULL;

	gebr_geoxml_document_load(&doc, TEST_SRCDIR "/nmo.flw", TRUE, NULL);

	gebr_geoxml_flow_validate(GEBR_GEOXML_FLOW(doc), fixture->validator, &err);
	g_assert_no_error(err);

	gebr_geoxml_document_free(doc);
}

void test_gebr_validator_iter(Fixture *fixture, gconstpointer data)
{
	GError *err = NULL;

	DEF_FLOAT_WITH_ERROR(fixture->flow, "iter2", "iter*2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);

	fixture_change_iter_value(fixture, "a", "1", "10", &err);
	g_assert_error(err, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&err);

	DEF_FLOAT_WITH_ERROR(fixture->flow, "iter2", "iter*2", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	VALIDATE_FLOAT_EXPR_WITH_ERROR("iter", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT(fixture->line, "n", "10");
	fixture_change_iter_value(fixture, "a", "1", "n", &err);
	g_assert_error(err, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	g_clear_error(&err);

	VALIDATE_FLOAT_EXPR("n", "10");
	VALIDATE_FLOAT_EXPR_WITH_ERROR("a", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("iter", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT_WITH_ERROR(fixture->flow, "x", "iter+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("iter+1", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	DEF_FLOAT(fixture->line, "a", "1");
	VALIDATE_FLOAT_EXPR("iter", "[1, ..., 10]");

	fixture_change_iter_value(fixture, "a", "1", "", &err);
	g_assert_error(err, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_EMPTY_EXPR);
	g_clear_error(&err);
	VALIDATE_FLOAT_EXPR_WITH_ERROR("iter", GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_BAD_REFERENCE);

	fixture_change_iter_value(fixture, "1", "1", "10", &err);
	g_assert_no_error(err);

	GebrGeoXmlParameter *iter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(fixture->flow));
	gebr_geoxml_parameter_set_type(iter, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_validator_change_value(fixture->validator, iter, "${V[[0]]}", NULL, &err);
	g_assert_no_error(err);

	gchar *result = NULL;
	gebr_validator_evaluate_interval(fixture->validator, "[iter]",
					 GEBR_GEOXML_PARAMETER_TYPE_STRING,
					 GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
					 FALSE, &result, &err);
	g_assert_no_error(err);
	g_assert_cmpstr(result, ==, "${V[0]}");
	g_free(result);
}

int main(int argc, char *argv[])
{
	g_type_init();
	gebr_geoxml_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add("/libgebr/validator/string", Fixture, NULL,
	           fixture_setup,
	           test_gebr_validator_string,
	           fixture_teardown);

	g_test_add("/libgebr/validator/simple", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_simple,
		   fixture_teardown);

	g_test_add("/libgebr/validator/update", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_update,
		   fixture_teardown);

	g_test_add("/libgebr/validator/insert", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_insert,
		   fixture_teardown);

	g_test_add("/libgebr/validator/remove", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_remove,
		   fixture_teardown);

	g_test_add("/libgebr/validator/change", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_change,
		   fixture_teardown);

	g_test_add("/libgebr/validator/move", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_move,
		   fixture_teardown);

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

	g_test_add("/libgebr/validator/cyclic_errors", Fixture, NULL,
	           fixture_setup,
	           test_gebr_validator_cyclic_errors,
	           fixture_teardown);

	g_test_add("/libgebr/validator/scope_errors", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_scope_errors,
		   fixture_teardown);

	g_test_add("/libgebr/validator/divide_by_zero", Fixture, NULL,
		   fixture_setup,
		   test_gebr_validator_divide_by_zero,
		   fixture_teardown);

	g_test_add("/libgebr/validator/leaks", Fixture, NULL,
		   fixture_setup,
			test_gebr_validator_leaks,
		   fixture_teardown);

	g_test_add("/libgebr/validator/rename", Fixture, NULL,
	           fixture_setup,
	           test_gebr_validator_rename,
	           fixture_teardown);

//	g_test_add("/libgebr/validator/validate_flow", Fixture, NULL,
//	           fixture_setup,
//	           test_gebr_geoxml_validate_flow,
//	           fixture_teardown);

	g_test_add("/libgebr/validator/iter", Fixture, NULL,
	           fixture_setup,
	           test_gebr_validator_iter,
	           fixture_teardown);

	gint result = g_test_run();
	gebr_geoxml_finalize();
	return result;
}


