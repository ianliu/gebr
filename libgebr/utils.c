/*   libgebr - GeBR Library
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

//remove round warning
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <fcntl.h>
#include <signal.h>
#include <glib/gi18n.h>

#include "utils.h"

#define DEFAULT_NPROCS 1

void
gebr_g_string_replace(GString * string,
		      const gchar * oldtext,
		      const gchar * newtext)
{
	g_return_if_fail(string != NULL);
	g_return_if_fail(oldtext != NULL);

	gchar * text = NULL;

	if (g_strrstr(string->str, oldtext) == NULL)
		return;

	if (newtext == NULL)
		text = g_strdup("");
	else
		text = g_strdup(newtext);

	gchar ** split = g_strsplit(string->str, oldtext, -1);

	gchar * new_string = g_strjoinv(text, split);
	string = g_string_assign(string, new_string);

	g_free(new_string);
	g_free(text);
	g_strfreev(split);
}

/**
 * Same as \ref gebr_g_string_replace but only apply to first
 * reference found.
 */
void gebr_g_string_replace_first_ref(GString * string, const gchar * oldtext, const gchar * newtext)
{
	gchar *position;

	position = string->str;
	if ((position = strstr(position, oldtext)) != NULL) {
		gssize index;

		index = (position - string->str) / sizeof(gchar);
		g_string_erase(string, index, strlen(oldtext));

		if (newtext != NULL)
			g_string_insert(string, index, newtext);
	}
}

gboolean gebr_g_string_starts_with(GString * string, const gchar * val)
{
	return g_str_has_prefix(string->str, val);
}

gboolean gebr_g_string_ends_with(GString * string, const gchar * val)
{
	return g_str_has_suffix(string->str, val);
}

gboolean gebr_append_filename_extension(GString * filename, const gchar * extension)
{
	if (!g_str_has_suffix(filename->str, extension)) {
		g_string_append(filename, extension);
		return TRUE;
	}
	return FALSE;
}

gboolean gebr_path_is_at_home(const gchar * path)
{
	const gchar *home = g_get_home_dir();
	if (home == NULL)
		return FALSE;
	return g_str_has_prefix(path, home);
}

gboolean gebr_path_use_home_variable(GString * path)
{
	if (gebr_path_is_at_home(path->str)) {
		const gchar *home = g_get_home_dir();
		gebr_g_string_replace_first_ref(path, home, "$HOME");
		return TRUE;
	}

	return FALSE;
}

gboolean gebr_path_resolve_home_variable(GString * path)
{
	if (gebr_g_string_starts_with(path, "$HOME")) {
		const gchar *home = g_get_home_dir();
		gebr_g_string_replace_first_ref(path, "$HOME", home);
		return TRUE;
	}

	return FALSE;
}

void gebr_path_set_to(GString * path, gboolean relative)
{
	if (relative)
		gebr_path_use_home_variable(path);
	else
		gebr_path_resolve_home_variable(path);
}

/**
 * Create a file based on \p template.
 *
 * \p template must have a XXXXXX that will be replaced to random
 * generated and unique string that results on a filename that doens't exists.
 * \p template should be the absolute path of the file.
 */
GString *gebr_make_unique_filename(const gchar * template)
{
	GString *path;

	/* assembly file path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s", template);

	/* create a temporary file. */
	close(g_mkstemp(path->str));

	return path;
}

/**
 * Create a temporary directory inside GeBR's temporary diretory.
 * Use \ref gebr_temp_directory_destroy after use to free string memory and
 * delete directory's contents
 */
GString *gebr_temp_directory_create(void)
{
	GString *path;

	/* assembly dir path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/tmp/XXXXXX", g_get_home_dir());

	/* create a temporary file. */
	close(g_mkstemp(path->str));
	unlink(path->str);

	g_mkdir_with_parents(path->str, gebr_home_mode());

	return path;
}

/**
 * Delete dir at \p path and free it.
 */
void gebr_temp_directory_destroy(GString * path)
{
	// FIXME: Should this free 'path' argument?
	//
	if (gebr_system("rm -fr %s", path->str) != 0) {
		// do nothing
	}
	g_string_free(path, TRUE);
}

/**
 * Create a file based on \p template.
 *
 * \p template must have a XXXXXX that will be replaced to random
 * generated and unique string that results on a filename that doens't exists.
 * \p template is just a filename, not an absolute path.
 * The returned string then is an absolute path of
 * The string returned is a path to the file on the temporary directory.
 */
GString *gebr_make_temp_filename(const gchar * template)
{
	GString *path;

	/* assembly file path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/tmp/%s", g_get_home_dir(), template);

	/* create a temporary file. */
	close(g_mkstemp(path->str));

	return path;
}

/**
 * GêBR's wrapper to system command.
 * Remember to escape each argument.
 */
gint gebr_system(const gchar *cmd, ...)
{
	gint exit, ret;

	va_list argp;
	va_start(argp, cmd);
	gchar *cmd_parsed = g_strdup_vprintf(cmd, argp);
	va_end(argp);
	ret = system(cmd_parsed);
	exit = WEXITSTATUS(ret);
	g_free(cmd_parsed);

	return exit;
}

/**
 * Return TRUE if the directory at \p dir_path could be loaded and has at least one file.
 * Otherwise return FALSE.
 */
gboolean gebr_dir_has_files(const gchar *dir_path)
{
	GError *error = NULL;
	GDir *dir = g_dir_open(dir_path, 0, &error);
	if (dir == NULL)
		return FALSE;

	gboolean ret = g_dir_read_name(dir) == NULL ? FALSE : TRUE;
	g_dir_close(dir);
	return ret;
}

/**
 * Returns the home's permission mode. Useful for
 * preserving permissions when creating files
 */
int gebr_home_mode(void)
{
	struct stat home_stat;
	const gchar *home;

	home = g_get_home_dir();
	g_stat(home, &home_stat);

	return home_stat.st_mode;
}

static gboolean gebr_make_config_dir(const gchar * dirname)
{
	GString *path;
	gboolean ret = TRUE;

	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/%s", g_get_home_dir(), dirname);
	if (g_file_test(path->str, G_FILE_TEST_IS_DIR) == FALSE)
		if (g_mkdir_with_parents(path->str, gebr_home_mode()))
			ret = FALSE;
	g_string_free(path, TRUE);

	return ret;
}

/**
 * Create all configurations directories for all GeBR programs.
 * Used by GeBR programs before read/write config..
 */
gboolean gebr_create_config_dirs(void)
{
	GString *string = g_string_new(NULL);
	gboolean ret = TRUE;
	const gchar *home = g_get_home_dir();

	/* Test for gebr conf dir and subdirs */
	if (!gebr_make_config_dir(""))
		goto err;
	if (!gebr_make_config_dir("run"))
		goto err;
	if (!gebr_make_config_dir("log"))
		goto err;
	if (!gebr_make_config_dir("tmp"))
		goto err;
	if (!gebr_make_config_dir("debr"))
		goto err;
	if (!gebr_make_config_dir("gebr"))
		goto err;
	if (!gebr_make_config_dir("gebrd"))
		goto err;
	if (!gebr_make_config_dir("gebr/menus"))
		goto err;
	if (!gebr_make_config_dir("gebr/data"))
		goto err;

	/* DEPRECATED: migration from old structure */
	g_string_printf(string, "%s/.gebr/menus", home);
	if (g_file_test(string->str, G_FILE_TEST_IS_DIR) == TRUE &&
	    gebr_dir_has_files(string->str) == TRUE) {
		gint ret = gebr_system("mv %s/.gebr/menus/* %s/.gebr/gebr/menus",
				       home, home);
		if (ret)
			goto err;
	}
	gebr_system("rm -fr %s", string->str);
	g_string_printf(string, "%s/.gebr/gebrdata", home);
	if (g_file_test(string->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == TRUE &&
	    gebr_dir_has_files(string->str) == TRUE) {
		gint ret = gebr_system("mv %s/.gebr/gebrdata/* %s/.gebr/gebr/data",
				       home, home, home);
		if (ret)
			goto err;
	}
	gebr_system("rm -fr %s", string->str);
	g_string_printf(string, "%s/.gebr/menus.idx", home);
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		gint ret = gebr_system("mv %s/.gebr/menus.idx %s/.gebr/gebr", home, home);
		if (ret)
			goto err;
	}
	g_string_printf(string, "%s/.gebr/gebr.conf", home);
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		gint ret = gebr_system("mv %s/.gebr/gebr.conf %s/.gebr/gebr", home, home);
		if (ret)
			goto err;
	}
	g_string_printf(string, "%s/.gebr/debr.conf", home);
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		gint ret = gebr_system("mv %s/.gebr/debr.conf %s/.gebr/debr", home, home);
		if (ret)
			goto err;
	}

	goto out;
 err:	ret = FALSE;
 out:	g_string_free(string, TRUE);

	return ret;
}

/**
 * Simplified version of g_locale_to_utf8
 */
gchar *gebr_locale_to_utf8(const gchar * string)
{
	gchar *output;
	gsize bytes_read;
	gsize bytes_written;
	GError *error;

	error = NULL;
	output = g_locale_to_utf8(string, -1, &bytes_read, &bytes_written, &error);

	if (error) {
		g_warning ("Failed to convert string into UTF-8: %s", error->message);
		g_clear_error (&error);
	}

	return output;
}

gboolean g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key)
{
	GError *error = NULL;

	return g_key_file_has_key(key_file, group, key, &error);
}

GString *gebr_g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key,
					 const gchar * default_value)
{
	GString *value;
	gchar *tmp;

	value = g_string_new(NULL);
	tmp = g_key_file_get_string(key_file, group, key, NULL);
	g_string_assign(value, (tmp == NULL || ((tmp != NULL) && (strlen(tmp) == 0)))
			? default_value : tmp);

	g_free(tmp);

	return value;
}

gboolean
gebr_g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key, gboolean default_value)
{
	gboolean value;
	GError *error;

	error = NULL;
	value = g_key_file_get_boolean(key_file, group, key, &error);

	if (error != NULL) {
		g_error_free(error);
		return default_value;
	}

	return value;
}

int gebr_g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value)
{
	int value;
	GError *error;

	error = NULL;
	value = g_key_file_get_integer(key_file, group, key, &error);
	if (error != NULL) {
		g_error_free(error);
		return default_value;
	}

	return value;
}

gboolean gebr_g_key_file_has_key(GKeyFile * key_file, const gchar * group, const gchar * key)
{
	GError *error = NULL;
	return g_key_file_has_key(key_file, group, key, &error);
}

gboolean gebr_g_key_file_remove_key(GKeyFile * key_file, const gchar * group, const gchar * key)
{
	GError *error = NULL;
	return g_key_file_remove_key(key_file, group, key, &error);
}

/*
 * Function: gebr_validate_int
 * Validate an int parameter
 *
 * FIXME Deprecated
 */
const gchar *gebr_validate_int(const gchar * text_value, const gchar * min, const gchar * max)
{
	static gchar number[31];
	gdouble value;
	gchar * endptr = NULL;
	size_t len = 0;

	if ((len = strlen(text_value)) == 0)
		return "";

	value = g_ascii_strtod(text_value, &endptr);
	if (endptr - text_value	!= len)
		return "";

	g_ascii_dtostr(number, 30, round(value));

	if (min != NULL && strlen(min) && value < atof(min))
		return min;
	if (max != NULL && strlen(max) && value > atof(max))
		return max;
	return number;
}

/*
 * Function: gebr_validate_float
 * Validate a float parameter
 */
const gchar *gebr_validate_float(const gchar * text_value, const gchar * min, const gchar * max)
{
	static gchar number[31];
	gchar *last;
	gdouble value;
	GString *value_str;

	if (strlen(text_value) == 0)
		return "";

	/* initialization */
	value_str = g_string_new(NULL);

	value = g_ascii_strtod(text_value, &last);
	g_string_assign(value_str, text_value);
	g_string_truncate(value_str, strlen(text_value) - strlen(last));
	strncpy(number, value_str->str, 30);

	/* frees */
	g_string_free(value_str, TRUE);

	if (min != NULL && strlen(min) && value < atof(min))
		return min;
	if (max != NULL && strlen(max) && value > atof(max))
		return max;
	return number;
}

#if !GLIB_CHECK_VERSION(2,16,0)
int g_strcmp0(const char * str1, const char * str2)
{
	if (!str1)
		return -(str1 != str2);
	if (!str2)
		return str1 != str2;
	return strcmp(str1, str2);
}
#endif

gboolean gebr_realpath_equal (const gchar *path1, const gchar *path2)
{
	struct stat stat1, stat2;

	if (g_stat (path1, &stat1) != 0)
	{
		return FALSE;
	}

	if (g_stat (path2, &stat2) != 0)
	{
		return FALSE;
	}

	return stat1.st_ino == stat2.st_ino;
}

gchar *gebr_str_escape (const gchar *str)
{
	if (!str)
		return NULL;
	gchar c;
	gchar *dest = g_new (gchar, 2*strlen(str) + 1);
	gchar *retval;

	retval = dest;

	c = *(str++);
	while (c) {
		switch(c) {
		case '\a':
			*(dest++) = '\\';
			*(dest++) = 'a';
			break;
		case '\b':
			*(dest++) = '\\';
			*(dest++) = 'b';
			break;
		case '\t':
			*(dest++) = '\\';
			*(dest++) = 't';
			break;
		case '\n':
			*(dest++) = '\\';
			*(dest++) = 'n';
			break;
		case '\v':
			*(dest++) = '\\';
			*(dest++) = 'v';
			break;
		case '\f':
			*(dest++) = '\\';
			*(dest++) = 'f';
			break;
		case '\r':
			*(dest++) = '\\';
			*(dest++) = 'r';
			break;
		case '\\':
			*(dest++) = '\\';
			*(dest++) = '\\';
			break;
			case '\"':
				*(dest++) = '\\';
			*(dest++) = '\"';
			break;
		default:
			*(dest++) = c;
		}
		c = *(str++);
	}

	*dest = '\0'; /* Ensure nul terminator */
	return retval;
}

gchar *gebr_date_get_localized (const gchar *format, const gchar *locale)
{
	gsize written;
	gchar *datestr;
	GDate *date;
	gchar *oldloc;

	datestr = g_new (gchar, 1024);
	oldloc = setlocale(LC_TIME, NULL);
	setlocale(LC_TIME, locale);
	date = g_date_new ();
	g_date_set_time_t (date, time (NULL));
	written = g_date_strftime (datestr, 1024, format, date);
	setlocale (LC_TIME, oldloc);

	if (!written) {
		g_warning ("Unable to write date: buffer was too small!");
		return NULL;
	}

	return datestr;
}

gchar *gebr_id_random_create(gssize bytes)
{
	gchar * id = g_new(gchar, bytes+1);

	for (gint i = 0; i < bytes; ++i) {
		GTimeVal current_time;
		g_get_current_time(&current_time);
		g_random_set_seed(current_time.tv_usec);
	
		//take care not to use separators as space, comma and others!
		gchar c;
		do
			c = (gchar)g_random_int_range(48, 90);
		while (c >= 58 && c <= 64); //rejected range
		id[i] = c;
	}
	id[bytes] = '\0';

	return id;
}

gchar * gebr_lock_file(const gchar *pathname, const gchar *new_lock_content, gboolean symlink)
{
	/* TODO */
	if (symlink)
		return NULL;

	gchar *contents = NULL;

	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = getpid();

	int fd = open(pathname, O_CREAT | O_WRONLY, gebr_home_mode());
	fcntl(fd, F_SETLKW, &fl);

	GError *error = NULL;
	gsize length = 0;
	if (g_file_test(pathname, G_FILE_TEST_IS_REGULAR) &&
	    g_file_get_contents(pathname, &contents, &length, &error) &&
	    length > 0) {
		/* file exists and could be read, make it a null-terminated string */
		gchar * tmp = g_new(gchar, length+1);
		strncpy(tmp, contents, length);
		tmp[length] = '\0';
		g_free(contents);
		contents = tmp;
	} else {
		length = strlen(new_lock_content);
		if (write(fd, new_lock_content, length) > 0)
			contents = g_strdup(new_lock_content);
		else
			contents = NULL;
	}

	close(fd);
	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);

	return contents;
}

gchar *gebr_str_word_before_pos(const gchar *str, gint *pos)
{
	gchar *word;
	gint ini = *pos;
	gint end = *pos;

	while (ini >= 0 && (g_ascii_isalnum(str[ini]) || str[ini] == '_'))
		ini--;

	if (ini == end)
		return NULL;

	// Advance one so ini points to the first valid char
	ini++;
	*pos = ini;
	word = g_strdup(str+ini);
	word[end-ini+1] = '\0';

	return word;
}

gchar *
gebr_str_remove_trailing_zeros(gchar *str)
{
	gsize i;

	i = strlen(str);
	while (i--)
		if (str[i] == '.' || str[i] != '0')
			break;

	if (i != -1) {
		if (str[i] == '.')
			str[i] = '\0';
		else
			str[i+1] = '\0';
	}

	return str;
}

gboolean
gebr_str_canonical_var_name(const gchar * keyword, gchar ** new_value, GError ** error)
{
	g_return_val_if_fail(keyword != NULL, FALSE);
	g_return_val_if_fail(error == NULL, FALSE);

	/* This pointer must point to something */
	g_return_val_if_fail(new_value != NULL, FALSE);

	gchar *str = g_utf8_strdown(keyword, -1);

	/* g_strcanon modifies the string in place */
	g_strcanon(str, "abcdefghijklmnopqrstuvxwyz1234567890_", ' ');
	g_strstrip(str);
	g_strcanon(str, "abcdefghijklmnopqrstuvxwyz1234567890_", '_');
	if (!g_unichar_isalpha(g_utf8_get_char(str))) {
		*new_value = g_strdup_printf("var_%s", str);
		g_free(str);
	} else
		*new_value = str;

	return TRUE;
}

gchar *
gebr_calculate_relative_time (GTimeVal *time1, GTimeVal *time2)
{
	glong time_diff = time2->tv_sec - time1->tv_sec;
	if ( time_diff < 0) 
		return NULL;
	else if ( time_diff < 5)
		return (g_strdup(_("A moment")));
	else if ( time_diff < 60)
		return (g_strdup_printf(_("%ld seconds"), time_diff));
	else if ( time_diff < 60*2)
		return (g_strdup(_("1 minute")));
	else if ( time_diff < 3600)
		return (g_strdup_printf(_("%ld minutes"), (time_diff)/60));
	else if ( time_diff < 3600*2)
		return (g_strdup_printf(_("1 hour")));
	else if ( time_diff < 86400)
		return (g_strdup_printf(_("%ld hours"), (time_diff)/3600));
	else if ( time_diff < 86400*2)
		return (g_strdup_printf(_("1 day")));
	else if ( time_diff < 604800)
		return (g_strdup_printf(_("%ld days"), (time_diff)/86400));
	else if ( time_diff < 604800*2)
		return (g_strdup_printf(_("1 week")));
	else if ( time_diff < 2678400)
		return (g_strdup_printf(_("%ld weeks"), (time_diff)/604800));
	else if ( time_diff < 2678400*2)
		return (g_strdup_printf(_("1 month")));
	else if ( time_diff < 32140800)
		return (g_strdup_printf(_("%ld months"), (time_diff)/2678400));
	else 
		return (g_strdup(_("More than a year")));
}

gchar *
gebr_calculate_detailed_relative_time(GTimeVal *time1, GTimeVal *time2)
{
	glong time_diff = time2->tv_sec - time1->tv_sec;
	glong h = time_diff/3600;
	glong m = (time_diff - h*3600)/60;
	glong s = (time_diff - h*3600 - m*60);
	glong micro_s = time2->tv_usec - time1->tv_usec;
	glong ms = micro_s / 1000;

	if      (h==1 && m==1 && s==1)
		return g_strdup(_("1 hour, 1 minute and 1 second"));
	else if (h==1 && m==1 && s)
		return g_strdup_printf(_("1 hour, 1 minute and %ld seconds"), s);
	else if (h==1 && m && s==1)
		return g_strdup_printf(_("1 hour, %ld minute and 1 second"), m);
	else if (h==1 && m && s)
		return g_strdup_printf(_("1 hour, %ld minutes and %ld seconds"), m, s);
	else if (h && m && s)
		return g_strdup_printf(_("%ld hours, %ld minutes and %ld seconds"), h, m, s);
	else if (h && m && !s)
		return g_strdup_printf(_("%ld hours and %ld minutes"), h, m);
	else if (!h && m==1 && s==1)
		return g_strdup(_("1 minute and 1 second"));
	else if (!h && m==1 && s)
		return g_strdup_printf(_("1 minute and %ld seconds"), s);
	else if (!h && m && s==1)
		return g_strdup_printf(_("%ld minutes and 1 second"), m);
	else if (!h && m && s)
		return g_strdup_printf(_("%ld minutes and %ld seconds"), m, s);
	else if (h==1 && !m && s)
		return g_strdup_printf(_("1 hour and %ld seconds"), s);
	else if (h && !m && s)
		return g_strdup_printf(_("%ld hours and %ld seconds"), h, s);
	else if (h && !m && !s)
		return g_strdup_printf(_("%ld hours"), h);
	else if (!h && m==1 && !s)
		return g_strdup(_("1 minute"));
	else if (!h && m && !s)
		return g_strdup_printf(_("%ld minutes"), m);
	else if (!h && !m && s>1)
		return g_strdup_printf(_("%ld seconds"), s);
	else if (!h && !m && s)
		return (_("1 second"));
	else if (!h && !m && !s && micro_s < 2000) 
		return _("1 milisecond");
	else
		return g_strdup_printf(_("%ld miliseconds"), ms);
}

gboolean
gebr_utf8_is_asc_alnum(const gchar *str)
{
	const gchar *name = str;
	while (name && *name) {
		gunichar c = g_utf8_get_char(name);

		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
			return FALSE;

		name = g_utf8_next_char(name);
	}
	return TRUE;
}

gchar *
gebr_utf8_strstr(const gchar *str,
		 const gchar *search)
{
	g_return_val_if_fail(str != NULL, NULL);
	g_return_val_if_fail(search != NULL, NULL);

	const gchar *i = str;
	const gchar *j = search;
	gunichar a, b;

	while (i && *i) {
		const gchar *k = i;
		a = g_utf8_get_char(k);
		b = g_utf8_get_char(j);

		while (TRUE) {
			if (a != b)
				break;

			k = g_utf8_next_char(k);
			j = g_utf8_next_char(j);

			if (!j || !*j)
				return (gchar*) i;

			if (!k || !*k)
				return NULL;

			a = g_utf8_get_char(k);
			b = g_utf8_get_char(j);
		}

		j = search;
		i = g_utf8_next_char(i);
	}

	return NULL;
}

gint
gebr_calculate_number_of_processors(gint total_nprocs,
                                    gint aggressive)
{
	return (total_nprocs - DEFAULT_NPROCS) * (aggressive-1)/4 + DEFAULT_NPROCS;
}

void
gebr_pairstrfreev(gchar ***strv)
{
	if (!strv)
		return;

	for (int i = 0; strv[i]; i++) {
		g_free(strv[i][0]);
		g_free(strv[i][1]);
		g_free(strv[i]);
	}
	g_free(strv);
}

static gchar *
remove_gvfs_prefix(gchar *path)
{
	gchar *gvfs_dir = g_build_filename(g_get_home_dir(), ".gvfs" G_DIR_SEPARATOR_S, NULL);
	gchar *ret;

	if (g_str_has_prefix(path, gvfs_dir))
		ret = strchr(path + strlen(gvfs_dir), G_DIR_SEPARATOR);
	else
		ret = path;
	g_free(gvfs_dir);

	return ret;
}

gchar *
gebr_relativise_path(const gchar *path_str,
		     gchar ***pvector)
{
	g_return_val_if_fail(path_str != NULL, NULL);

	if (!*path_str)
		return g_strdup("");

	for (int i = 0; pvector[i]; i++)
		if (g_str_has_prefix(path_str, pvector[i][1]))
			return g_strdup(path_str);

	GString *tmp = g_string_new(path_str);
	gebr_path_resolve_home_variable(tmp);
	gchar *tmp2 = g_string_free(tmp, FALSE);
	gchar *path = g_strdup(remove_gvfs_prefix(tmp2));
	g_free(tmp2);

	g_return_val_if_fail(path != NULL, NULL);

	gchar **dirspath = g_strsplit(path, "/", -1);
	gchar **dirtmp;
	gint max_index = 0, max_size = 0;
	gint j = 0;

	// Set size default like BASE size
	gchar **dirdefault = g_strsplit(pvector[0][0], "/" , -1);
	gint k;
	for (k = 0; dirdefault[k]; k++);
	gint size_default = k;
	g_strfreev(dirdefault);

	for (int i = 0; pvector[i]; i++) {
		dirtmp = g_strsplit(pvector[i][0], "/" , -1);
		j = 0;
		while (dirspath[j] != NULL || dirtmp[j] != NULL) {
			if (g_strcmp0(dirspath[j], dirtmp[j]) != 0) {
				if (j > max_size) {
					max_size = j;
					max_index = i;
				}
				break;
			}
			j++;
		}
		if (dirspath[j] == NULL && dirtmp[j] == NULL) {
			max_size = j;
			max_index = i;
			g_strfreev(dirtmp);
			break;
		}
		g_strfreev(dirtmp);
	}
	g_strfreev(dirspath);

	if (max_size < size_default)
		return path;

	GString *rel_path = g_string_new(path);
	g_free(path);

	if (g_str_has_suffix(pvector[max_index][0], "/"))
		rel_path = g_string_erase(rel_path, 0, (strlen(pvector[max_index][0]) - 1));
	else
		rel_path = g_string_erase(rel_path, 0, strlen(pvector[max_index][0]));
	rel_path = g_string_prepend (rel_path, pvector[max_index][1]);

	return g_string_free(rel_path, FALSE);
}

gchar *
gebr_resolve_relative_path(const char *path,
			   gchar ***pvector)
{
	g_return_val_if_fail(path != NULL, NULL);
	g_return_val_if_fail(pvector != NULL, NULL);
	
	gint i = 0;
	gchar **dirspath = g_strsplit(path, "/", -1);
	GString *str = g_string_new(path);
	gboolean has_rpath = FALSE;

	while (pvector[i] != NULL ) {
		if (!g_strcmp0(dirspath[0], pvector[i][1])) {
			str = g_string_erase(str, 0, strlen(dirspath[0]));
			str = g_string_prepend(str, pvector[i][0]);
			has_rpath = TRUE;
			break;
		}
		i++;
	}
	g_strfreev(dirspath);
	
	if (!has_rpath)
		return g_strdup(path);
	
	return g_string_free(str, FALSE);
}

void
gebr_gtk_bookmarks_add_paths(const gchar *filename,
                             const gchar *uri_prefix,
                             gchar ***paths)
{
	gchar *contents;
	GString *buf;
	GError *error = NULL;

	if (!g_file_get_contents(filename, &contents, NULL, &error)) {
		if (error->code != G_FILE_ERROR_NOENT) {
			g_error_free(error);
			return;
		} else {
			contents = g_strdup("");
		}
		g_error_free(error);
	}

	buf = g_string_new(NULL);

	for (gint i = 0; paths[i]; i++) {
		if (!*paths[i][0])
			continue;

		gchar *escaped = g_uri_escape_string(paths[i][0], "/", TRUE);
		g_string_append_printf(buf, "%s%s %s (GeBR)\n",
				       uri_prefix, escaped, paths[i][1]);
		g_free(escaped);
	}

	g_string_append(buf, contents);

	if (!g_file_set_contents(filename, buf->str, -1, NULL))
		g_warn_if_reached();

	g_free(contents);
}

void
gebr_gtk_bookmarks_remove_paths(const gchar *filename,
				gchar ***paths)
{
	gchar *content;
	GError *err = NULL;

	if (!g_file_get_contents(filename, &content, NULL, &err)) {
		g_debug("Cannot obtain content of bookmark file: %s", err->message);
		g_error_free(err);
		return;
	}

	gchar **lines = g_strsplit(content, "\n", -1);
	GString *real_bookmarks = g_string_new(NULL);
	gboolean has_path;

	for (gint i = 0; lines[i]; i++) {
		has_path = FALSE;
		for (gint j = 0; paths[j] && !has_path; j++) {
			gchar *escaped = g_uri_escape_string(paths[j][0], "/", TRUE);
			gchar *suffix = g_strdup_printf("%s %s (GeBR)", escaped, paths[j][1]);

			if (g_str_has_suffix(lines[i], suffix))
				has_path = TRUE;

			g_free(escaped);
			g_free(suffix);
		}

		if (!has_path && *lines[i]) {
			g_string_append(real_bookmarks, lines[i]);
			g_string_append_c(real_bookmarks, '\n');
		}
	}

	if (!g_file_set_contents(filename, real_bookmarks->str, -1, &err)) {
		g_debug("Cannot set content of bookmark file: %s", err->message);
		g_error_free(err);
		return;
	}

	g_free(content);
	g_string_free(real_bookmarks, TRUE);
}
