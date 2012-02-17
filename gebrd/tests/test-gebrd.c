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

#include "../gebrd.h"
#include "../gebrd-client.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <unistd.h>

#define MAX_AGRESSION 5
#define MIN_AGRESSION 1


gint set_mkdir_renamedir (GString *new_path, GString *old_path, GString *opt)
{
	g_debug("new_path:%s, old_path:%s, opt:%s", new_path->str, old_path->str, opt->str);

	GList *new_paths= parse_comma_separated_string(new_path->str);

	gint option = gebr_comm_protocol_path_str_to_enum(opt->str);
	gint status_id = -1;
	gboolean flag_exists = FALSE;
	gboolean flag_error = FALSE;

	switch (option) {
	case GEBR_COMM_PROTOCOL_PATH_CREATE:
		for (GList *j = new_paths; j; j = j->next) {
			GString *path = j->data;
			if (g_file_test(path->str, G_FILE_TEST_IS_DIR)){
				flag_exists = TRUE;
			}
			else if (*(path->str) && g_mkdir_with_parents(path->str, 0700)) {
				flag_error = TRUE;
				break;
			}
			if (g_access(path->str, W_OK)==-1){
				flag_error = TRUE;
				break;
			}
		}

		if (flag_error)
			status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
		else if (flag_exists)
			status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS;
		else
			status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
		break;
	case GEBR_COMM_PROTOCOL_PATH_RENAME:
		g_debug("Renaming %s to %s", old_path->str, new_path->str);
		if (!g_file_test(new_path->str, G_FILE_TEST_IS_DIR) && g_rename(old_path->str, new_path->str)){
			status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
			break;
		}
		else {
			status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
			break;
		}
		status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS;
		break;
	case GEBR_COMM_PROTOCOL_PATH_DELETE:
		for (GList *j = new_paths; j; j = j->next) {
			GString *path = j->data;
			if (g_rmdir(path->str))
				status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR;
			else
				status_id = GEBR_COMM_PROTOCOL_STATUS_PATH_OK;
		}
		break;
	default:
		g_warn_if_reached();
		break;
	}
	g_list_free(new_paths);
	return status_id;
}


void
test_set_mkdir_renamedir(void)
{
	gchar *filenamenew;
	gchar *filenameold;
	gchar *option;
	GString *new_path;
	GString *old_path;
	GString *opt;
	gint out;

	//CREATING: GEBR_COMM_PROTOCOL_STATUS_PATH_OK
	filenamenew = g_build_filename(g_get_home_dir(), "demonstracao", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "demonstracao", NULL);
	option = g_strdup("create");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_OK);

	//CREATING: GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS
	filenamenew = g_build_filename(g_get_home_dir(), "demonstracao", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "demonstracao", NULL);
	option = g_strdup("create");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS);

	//CREATING: GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR

	//RENAMING: GEBR_COMM_PROTOCOL_STATUS_PATH_OK
	filenamenew = g_build_filename(g_get_home_dir(), "demonst", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "demonstracao", NULL);
	option = g_strdup("rename");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_OK);
	//RENAMING: GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS
	filenamenew = g_build_filename(g_get_home_dir(), "demos", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "New Line", NULL);
	option = g_strdup("rename");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_OK);
	//RENAMING: GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR

	//DELETING: GEBR_COMM_PROTOCOL_STATUS_PATH_OK
	filenamenew = g_build_filename(g_get_home_dir(), "demos", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "demonst", NULL);
	option = g_strdup("delete");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_OK);

	//DELETING: GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS
	filenamenew = g_build_filename(g_get_home_dir(), "demos", NULL);
	filenameold = g_build_filename(g_get_home_dir(), "demonst", NULL);
	option = g_strdup("delete");

	new_path = g_string_new(filenamenew);
	old_path = g_string_new(filenameold);
	opt = g_string_new(option);

	out = set_mkdir_renamedir (new_path, old_path, opt);
	g_assert_cmpint(out, ==, GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS);

	//DELETING: GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR

	g_string_free(new_path, TRUE);
	g_string_free(old_path, TRUE);
	g_string_free(opt, TRUE);
	g_free(filenamenew);
	g_free(filenameold);
	g_free(option);
}


void
test_set_heuristic_aggression_border (void)
{
	GebrdApp *self = gebrd_app_new();

	g_assert_cmpint(gebrd_app_set_heuristic_aggression(self, MIN_AGRESSION), ==, 1);

	g_assert_cmpint(gebrd_app_set_heuristic_aggression(self, MAX_AGRESSION), ==, self->nprocs);
}

void
test_set_heuristic_aggression (void)
{
	GebrdApp *self = gebrd_app_new();

	g_assert_cmpint(gebrd_app_set_heuristic_aggression(self, 2), ==, (self->nprocs - 1) * 1/4 + 1);
}


void
test_parse_comma_separated_string (void)
{
	gchar *in;
	GList *out;

	in = g_strdup_printf("%s", "aa");
	out = parse_comma_separated_string (in);

	g_assert_cmpint(g_list_length(out), ==, 15);
	g_free(in);


	in = g_strdup_printf("%s", "aa,bb");
	out = parse_comma_separated_string (in);

	g_assert_cmpint(g_list_length(out), ==, 2);
	g_free(in);
}


int main(int argc, char * argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/gebrd/heuristic_aggression_border", test_set_heuristic_aggression_border);
	g_test_add_func("/gebrd/gebrd/heuristic_aggression", test_set_heuristic_aggression);
//	g_test_add_func("/gebrd/gebrd/parse_comma_separated_string", test_parse_comma_separated_string);

	g_test_add_func("/gebrd/test-gebrd/set_mkdir_renamedir", test_set_mkdir_renamedir);

	return g_test_run();
}
