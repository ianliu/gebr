/*   libgebr - GêBR Library
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
#include <glib/gstdio.h>

#include "../utils.h"

void test_gebr_str_escape (void)
{
	gchar *escaped;

	escaped = gebr_str_escape ("foo");
	g_assert_cmpstr (escaped, ==, "foo");
	g_free (escaped);

	escaped = gebr_str_escape ("foo\n");
	g_assert_cmpstr (escaped, ==, "foo\\n");
	g_free (escaped);

	escaped = gebr_str_escape ("\"foo\"");
	g_assert_cmpstr (escaped, ==, "\\\"foo\\\"");
	g_free (escaped);
}

void test_gebr_str_word_before_pos(void)
{
	gchar *word;
	gchar *text;
	gint pos;

	text = "foo bar baz";
	pos = 6;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert_cmpint(pos, ==, 4);
	g_assert_cmpstr(word, ==, "bar");

	text = "olá, tudo bem?";
	pos = 2;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert_cmpstr(word, ==, "olá");
	g_assert_cmpint(pos, ==, 0);

	pos = 8;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert_cmpstr(word, ==, "tudo");
	g_assert_cmpint(pos, ==, 5);

	pos = 3;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert(word == NULL);
	g_assert_cmpint(pos, ==, 3);
}

void test_gebr_str_remove_trailing_zeros(void)
{
	gchar *no_zeros = g_strdup("1.345");
	g_assert_cmpstr(gebr_str_remove_trailing_zeros(no_zeros), ==, "1.345");
	g_free(no_zeros);

	gchar *one_zero = g_strdup("1.3450");
	g_assert_cmpstr(gebr_str_remove_trailing_zeros(one_zero), ==, "1.345");
	g_free(one_zero);

	gchar *only_zeros = g_strdup("1.0000");
	g_assert_cmpstr(gebr_str_remove_trailing_zeros(only_zeros), ==, "1");
	g_free(only_zeros);

	gchar *only_zeros2 = g_strdup("0.0000");
	g_assert_cmpstr(gebr_str_remove_trailing_zeros(only_zeros2), ==, "0");
	g_free(only_zeros2);
}

void test_gebr_str_canonical_var_name(void)
{
	gchar * canonical = NULL;

	gebr_str_canonical_var_name("CDP EM METROS", &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "cdp_em_metros");
	g_free(canonical);
	canonical = NULL;

	gebr_str_canonical_var_name("CDP EM METROS (m)", &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "cdp_em_metros__m");
	g_free(canonical);
	canonical = NULL;

	gebr_str_canonical_var_name("123", &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "var_123");
	g_free(canonical);
	canonical = NULL;

	gebr_str_canonical_var_name("aaa", &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "aaa");
	g_free(canonical);
	canonical = NULL;

	gebr_str_canonical_var_name("aAA", &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "aaa");
	g_free(canonical);
	canonical = NULL;

	gchar * key = g_strdup("aAA");
	gebr_str_canonical_var_name(key, &canonical, NULL);
	g_assert_cmpstr(canonical, ==, "aaa");
	g_free(canonical);
	canonical = NULL;
	g_free(key);
	key = NULL;
}

void test_gebr_str_replace(void)
{
	GString * str = g_string_new("[ ]");
	gebr_g_string_replace(str, "[", "[[");
	g_assert_cmpstr(str->str, ==, "[[ ]");

	gebr_g_string_replace(str, "]", "]]");
	g_assert_cmpstr(str->str, ==, "[[ ]]");

	g_string_free(str, TRUE);

	str = g_string_new("ruim");
	gebr_g_string_replace(str, "ruim", "bom");
	g_assert_cmpstr(str->str, ==, "bom");
	g_string_free(str, TRUE);

	str = g_string_new("1e-10");
	gebr_g_string_replace(str, "e", "*10^");
	gebr_g_string_replace(str, "E", "*10^");
	g_assert_cmpstr(str->str, ==, "1*10^-10");
	g_string_free(str, TRUE);

	str = g_string_new("1E-10");
	gebr_g_string_replace(str, "e", "*10^");
	gebr_g_string_replace(str, "E", "*10^");
	g_assert_cmpstr(str->str, ==, "1*10^-10");
	g_string_free(str, TRUE);

	str = g_string_new("sim não sim");
	gebr_g_string_replace(str, "não", "sim");
	g_assert_cmpstr(str->str, ==, "sim sim sim");
	g_string_free(str, TRUE);

	str = g_string_new("áéíóúâê ô _ ô");
	gebr_g_string_replace(str, "á", "a");
	gebr_g_string_replace(str, "é", "e");
	gebr_g_string_replace(str, "í", "i");
	gebr_g_string_replace(str, "ó", "o");
	gebr_g_string_replace(str, "ú", "u");
	gebr_g_string_replace(str, "â", "a");
	gebr_g_string_replace(str, "ê", "e");
	gebr_g_string_replace(str, "ô", "o");

	g_assert_cmpstr(str->str, ==, "aeiouae o _ o");
	g_string_free(str, TRUE);

	str = g_string_new("Isso deve sair: banana");
	gebr_g_string_replace(str, "banana", NULL);
	g_assert_cmpstr(str->str, ==, "Isso deve sair: ");
	g_string_free(str, TRUE);

	str = g_string_new("oi\r\n");
	gebr_g_string_replace(str, "\r\n", "\n");
	g_assert_cmpstr(str->str, ==, "oi\n");
	g_string_free(str, TRUE);

	str = g_string_new("oi\r\n");
	gebr_g_string_replace(str, "\r\n", NULL);
	g_assert_cmpstr(str->str, ==, "oi");
	g_string_free(str, TRUE);

}

void test_gebr_calculate_relative_time(void){
	GTimeVal time1, time2;
	time1.tv_sec = 0;
	time1.tv_usec = 0;
	gchar *string;

	time2.tv_usec = 0;
	time2.tv_sec = 3;
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==,"Moments ago");

	time2.tv_usec = 0;
	time2.tv_sec = 60*6;
	string = g_strdup(("Moments ago"));
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==, string);
	g_free(string);

	time2.tv_usec = 0;
	time2.tv_sec = 3600*2;
	string = g_strdup(("Hours ago"));
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==, string);
	g_free(string);

	time2.tv_usec = 0;
	time2.tv_sec = 86400*2;
	string = g_strdup(("Days ago"));
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==, string);
	g_free(string);

	time2.tv_usec = 0;
	time2.tv_sec = 86400*10;
	string = g_strdup(("Weeks ago"));
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==, string);
	g_free(string);

	time2.tv_usec = 0;
	time2.tv_sec = 2678800;
	string = g_strdup(("Months ago"));
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==, string);
	g_free(string);

	time2.tv_usec = 0;
	time2.tv_sec = 32140800;
	g_assert_cmpstr(gebr_calculate_relative_time(&time1, &time2, NULL, NULL),==,"Years ago");
}

static void
test_gebr_utf8_is_asc_alnum(void)
{
	g_assert(gebr_utf8_is_asc_alnum("Grupo") == TRUE);
	g_assert(gebr_utf8_is_asc_alnum("Grupo1") == TRUE);
	g_assert(gebr_utf8_is_asc_alnum("groups") == TRUE);
	g_assert(gebr_utf8_is_asc_alnum("grupoímpar") == FALSE);
	g_assert(gebr_utf8_is_asc_alnum("Ímpar") == FALSE);
	g_assert(gebr_utf8_is_asc_alnum("grupo,2") == FALSE);
}

static void
test_gebr_utf8_strstr(void)
{
	g_assert_cmpstr(gebr_utf8_strstr("Olá, mundo!", "mundo"), ==, "mundo!");
	g_assert_cmpstr(gebr_utf8_strstr("Olá, mundo!", "!"), ==, "!");
	g_assert_cmpstr(gebr_utf8_strstr("Olá, mundo!", "Olá"), ==, "Olá, mundo!");
	g_assert(gebr_utf8_strstr("Olá, mundo!", "xxxxx") == NULL);
}

void
test_gebr_calculate_number_of_processors(void)
{
	gint nprocs[] 	 = {1, 2, 9, 9};
	gint agression[] = {1, 3, 3, 5};
	g_assert_cmpint(gebr_calculate_number_of_processors(nprocs[0], agression[0]), ==, 1);
	g_assert_cmpint(gebr_calculate_number_of_processors(nprocs[1], agression[1]), ==, 1);
	g_assert_cmpint(gebr_calculate_number_of_processors(nprocs[2], agression[2]), ==, 5);
	g_assert_cmpint(gebr_calculate_number_of_processors(nprocs[3], agression[3]), ==, 9);
}

void
test_gebr_double_list_to_list(void)
{
	GList *list1 = NULL;
	GList *list2 = NULL;
	GList *double_list = NULL;
	GList *single_list = NULL;
	gint n = 4;
	
	char text[10][10] = {"A", "B", "C", "D", "E", "F"};

	list1 = g_list_prepend(list1, text[0]);
	list1 = g_list_prepend(list1, text[1]);

	list2 = g_list_prepend(list2, text[2]);
	list2 = g_list_prepend(list2, text[3]);

	double_list = g_list_prepend(double_list, list1);
	double_list = g_list_prepend(double_list, list2);
	single_list = gebr_double_list_to_list(double_list);


	for (gint i = 0; i < n; i++) {
		g_assert_cmpstr(g_list_nth_data(single_list, i),
				==, text[n-i-1]);
	}
}
void
test_gebr_g_string_remove_accents(void){
	gchar *title1 = "eh sem acento";
	gchar *title2 = "Várias açõe’s² ñ'Ñ";
	gchar *title3 = "благодарю";
	g_assert_cmpstr(gebr_g_string_remove_accents(title1), == , "eh sem acento");
	g_assert_cmpstr(gebr_g_string_remove_accents(title2), == , "Varias acoes n'N");
	g_assert_cmpstr(gebr_g_string_remove_accents(title3), == , "");
}


int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/utils/str-escape", test_gebr_str_escape);
	g_test_add_func("/libgebr/utils/str_word_before_pos", test_gebr_str_word_before_pos);
	g_test_add_func("/libgebr/utils/str_remove_trailing_zeros", test_gebr_str_remove_trailing_zeros);
	g_test_add_func("/libgebr/utils/str_canonical_var_name", test_gebr_str_canonical_var_name);
	g_test_add_func("/libgebr/utils/str_replace", test_gebr_str_replace);
	g_test_add_func("/libgebr/utils/calculate_relative_time", test_gebr_calculate_relative_time);
	g_test_add_func("/libgebr/utils/gebr_utf8_is_asc_alnum", test_gebr_utf8_is_asc_alnum);
	g_test_add_func("/libgebr/utils/gebr_utf8_strstr", test_gebr_utf8_strstr);
	g_test_add_func("/libgebr/utils/test_gebr_calculate_number_of_processors", test_gebr_calculate_number_of_processors);
	g_test_add_func("/libgebr/utils/gebr_double_list_to_list", test_gebr_double_list_to_list);
	g_test_add_func("/libgebr/utils/gebr_g_string_remove_accents", test_gebr_g_string_remove_accents);
	
	gint ret = g_test_run();
	return ret;
}
