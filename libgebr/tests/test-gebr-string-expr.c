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
#include <glib-object.h>

#include "../gebr-iexpr.h"
#include "../gebr-string-expr.h"

void test_gebr_string_expr_is_valid(void)
{
	GError *error = NULL;
	GebrStringExpr *s = gebr_string_expr_new();

	gebr_iexpr_is_valid(GEBR_IEXPR(s), "ola", &error);
	g_assert_no_error(error);

	gebr_iexpr_is_valid(GEBR_IEXPR(s), "foo]", &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	g_object_unref(s);
}

void test_gebr_string_expr_set_var()
{
	GError *error = NULL;
	GebrStringExpr *s = gebr_string_expr_new();

	gebr_iexpr_set_var(GEBR_IEXPR(s), "foo",
			   GEBR_GEOXML_PARAMETER_TYPE_STRING,
			   "Hello World!", &error);
	g_assert_no_error(error);

	/* FIXME: Implement this one! */

	/* gebr_iexpr_set_var(GEBR_IEXPR(s), "2foo",
			   GEBR_GEOXML_PARAMETER_TYPE_STRING,
			   "Foo", &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_INVAL_VAR); */
}

void test_gebr_string_expr_invalid(void)
{
	GebrStringExpr *s = gebr_string_expr_new();


	g_object_unref(s);
}

static void insert_variables(GebrStringExpr *expr)
{
	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "foo",
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    "FOO", NULL));

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "bar",
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    "BAR", NULL));

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "int",
				    GEBR_GEOXML_PARAMETER_TYPE_INT,
				    "100", NULL));

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "float",
				    GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
				    "3.14", NULL));
}

void test_gebr_string_expr_eval_normal(void)
{
	gchar *result;
	GError *error = NULL;
	GebrStringExpr *expr = gebr_string_expr_new();

	insert_variables(expr);

	gebr_string_expr_eval(expr, "", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "");
	g_free(result);

	gebr_string_expr_eval(expr, "[[]]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "[]");
	g_free(result);

	gebr_string_expr_eval(expr, "foo", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "foo");
	g_free(result);

	gebr_string_expr_eval(expr, "[int]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "100");
	g_free(result);

	gebr_string_expr_eval(expr, "[float]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "3.14");
	g_free(result);

	gebr_string_expr_eval(expr, "[bar]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "BAR");
	g_free(result);

	gebr_string_expr_eval(expr, "foo[bar]baz[int] [float]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "fooBARbaz100 3.14");
	g_free(result);

	gebr_string_expr_eval(expr, "foo[bar]baz[[s]]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "fooBARbaz[s]");
	g_free(result);

	gebr_string_expr_eval(expr, "[[[bar]]]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "[BAR]");
	g_free(result);

	gebr_string_expr_eval(expr, "[foo][bar]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "FOOBAR");
	g_free(result);

	gebr_string_expr_eval(expr, "[[", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "[");
	g_free(result);

	gebr_string_expr_eval(expr, "[[other", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "[other");
	g_free(result);

	gebr_string_expr_eval(expr, "]]", &result, &error);
	g_assert_no_error(error);
	g_assert_cmpstr(result, ==, "]");
	g_free(result);

	g_object_unref(expr);
}

void test_gebr_string_expr_eval_invalid(void)
{
	gchar *result = NULL;
	GError *error = NULL;
	GebrStringExpr *expr = gebr_string_expr_new();

	insert_variables(expr);

	gebr_string_expr_eval(expr, "[xyz]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[foo][baz]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_UNDEF_VAR);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[foo][bar]]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[iter", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	/* FIXME: Implement variable name validation! */

	/*gebr_string_expr_eval(expr, "[]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_INVAL_VAR);
	g_clear_error(&error);
	g_assert(result == NULL);*/

	gebr_string_expr_eval(expr, "[", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[[int]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[[]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	gebr_string_expr_eval(expr, "[[]]]", &result, &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);
	g_assert(result == NULL);

	g_object_unref(expr);
}

void test_gebr_string_expr_with_integers(void)
{
	gchar *result;
	GError *error = NULL;
	GebrStringExpr *s = gebr_string_expr_new();

	gebr_iexpr_set_var(GEBR_IEXPR(s), "pi",
			   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
			   "3.14", &error);
	g_assert_no_error(error);

	gebr_iexpr_set_var(GEBR_IEXPR(s), "r",
			   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
			   "10", &error);
	g_assert_no_error(error);

	gebr_iexpr_set_var(GEBR_IEXPR(s), "area",
			   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
			   "pi*r^2 + ", &error);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_clear_error(&error);

	gebr_iexpr_set_var(GEBR_IEXPR(s), "area",
			   GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
			   "pi*r^2", &error);
	g_assert_no_error(error);

	gebr_string_expr_eval(s, "Area = [area]", &result, &error);
	g_assert_no_error(error);
	g_assert(g_str_has_prefix(result, "Area = 314"));
	g_free(result);

	g_object_unref(s);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/string-expr/is_valid", test_gebr_string_expr_is_valid);
	g_test_add_func("/libgebr/string-expr/with_integers", test_gebr_string_expr_with_integers);
	g_test_add_func("/libgebr/string-expr/set_var", test_gebr_string_expr_set_var);
	g_test_add_func("/libgebr/string-expr/eval/normal-expression",  test_gebr_string_expr_eval_normal);
	g_test_add_func("/libgebr/string-expr/eval/invalid-expression", test_gebr_string_expr_eval_invalid);

	return g_test_run();
}
