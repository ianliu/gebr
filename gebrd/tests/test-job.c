/*   GeBR Daemon - Process and control execution of flows
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
#include <glib.h>
#include <gebrd-job.h>

static GHashTable *build_hash_table(void) {
	GHashTable *ht;
	guint *i;

	ht = g_hash_table_new_full(gebrd_bc_hash_func, gebrd_bc_equal_func,
				   g_free, g_free);

	i = g_new(guint, 1), *i = 0;
	g_hash_table_insert(ht, g_strdup("number:iter"), i);

	i = g_new(guint, 1), *i = 1;
	g_hash_table_insert(ht, g_strdup("number:foo"), i);

	i = g_new(guint, 1), *i = 2;
	g_hash_table_insert(ht, g_strdup("number:bar"), i);

	g_hash_table_insert(ht, g_strdup("string:s1"), g_strdup("String 1"));
	g_hash_table_insert(ht, g_strdup("string:s2"), g_strdup("String 2"));
	g_hash_table_insert(ht, g_strdup("string:s3"), g_strdup("\\String \"3\""));

	return ht;
}

void test_gebrd_job_parse_string_normal(void)
{
	gchar *result;
	GHashTable *ht;

	ht = build_hash_table();

	g_assert(parse_string_expression("", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "");
	g_free(result);

	g_assert(parse_string_expression("[[]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "[]");
	g_free(result);

	g_assert(parse_string_expression("foo", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo");
	g_free(result);

	g_assert(parse_string_expression("[bar]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "${V[2]}");
	g_free(result);

	g_assert(parse_string_expression("foo[bar]baz", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo${V[2]}baz");
	g_free(result);

	g_assert(parse_string_expression("foo[bar]baz[[s]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo${V[2]}baz[s]");
	g_free(result);

	g_assert(parse_string_expression("[[[bar]]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "[${V[2]}]");
	g_free(result);

	g_assert(parse_string_expression("[iter][foo][bar]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "${V[0]}${V[1]}${V[2]}");
	g_free(result);

	g_assert(parse_string_expression("[s1]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "String 1");
	g_free(result);

	g_assert(parse_string_expression("[s1][foo][s2]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "String 1${V[1]}String 2");
	g_free(result);

	g_assert(parse_string_expression("[[", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "[");
	g_free(result);

	g_assert(parse_string_expression("[[other", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "[other");
	g_free(result);

	g_assert(parse_string_expression("]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "]");
	g_free(result);

	g_hash_table_unref(ht);
}

void test_gebrd_job_parse_string_invalid(void)
{
	gchar *result = NULL;
	GHashTable *ht;

	ht = build_hash_table();

	g_assert(parse_string_expression("[iter][foo][baz]", ht, &result) == GEBRD_STRING_PARSER_ERROR_UNDEF_VAR);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[iter][foo][bar]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[iter", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[[int]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[[]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[[]]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);


	g_hash_table_unref(ht);
}

void test_gebrd_job_parse_string_escape(void)
{
	gchar *result;
	GHashTable *ht;

	ht = build_hash_table();

	g_assert(parse_string_expression("\"hi\"", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "\\\"hi\\\"");
	g_free(result);

	g_assert(parse_string_expression("'hi'", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "'hi'");
	g_free(result);

	g_assert(parse_string_expression("\\\"hi", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "\\\\\\\"hi");
	g_free(result);

	g_assert(parse_string_expression("foo[s3]bar", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo\\\\String \\\"3\\\"bar");
	g_free(result);

	g_hash_table_unref(ht);
}

int main(int argc, char * argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/job/parse_string/normal-expression", test_gebrd_job_parse_string_normal);
	g_test_add_func("/gebrd/job/parse_string/invalid-expression", test_gebrd_job_parse_string_invalid);
	g_test_add_func("/gebrd/job/parse_string/escape-expression", test_gebrd_job_parse_string_escape);

	return g_test_run();
}
