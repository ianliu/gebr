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

#include <zlib.h>
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


gchar *
gebr_g_string_remove_accents(gchar *title)
{
	GString *buffer = g_string_new(NULL);
	gsize len;
	for (gchar *i = title; i[0] != '\0' ; i = g_utf8_next_char(i)) {
		gunichar *decomp = g_unicode_canonical_decomposition(g_utf8_get_char(i) , &len);
		if (decomp[0] <= 255 && g_ascii_isprint(decomp[0]))
			g_string_append_c(buffer, decomp[0]);
		g_free(decomp);
	}
	return g_string_free(buffer, FALSE);
}

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
	size_t len = 0;

	if ((len = strlen(text_value)) == 0)
		return "";

	/* initialization */
	value_str = g_string_new(NULL);
	value = g_ascii_strtod(text_value, &last);

	if (last - text_value != len)
		return "";

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

gchar * gebr_lock_file(const gchar *path, const gchar *content)
{
	if (!content) {
		gchar *str;
		if (!g_file_get_contents(path, &str, NULL, NULL))
			return NULL;
		return g_strstrip(str);
	}

	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = getpid();

	int fd = open(path, O_CREAT | O_WRONLY, gebr_home_mode());

	if (fd == -1) {
		perror("open");
		return NULL;
	}

	fcntl(fd, F_SETLKW, &fl);

	if (!g_file_set_contents(path, content, -1, NULL)) {
		close(fd);
		return NULL;
	}

	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);
	close(fd);

	return NULL;
}

gchar *gebr_str_word_before_pos(const gchar *str, gint *pos)
{
	gint ini = *pos;
	gint end = *pos;
	gunichar c_curr;

	gchar *tmp = g_utf8_offset_to_pointer(str, *pos);
	for (; ini >= 0; ini--) {
		c_curr = g_utf8_get_char(tmp);
		if (!(g_unichar_isalnum(c_curr) || c_curr == '_'))
			break;
		tmp = g_utf8_prev_char(tmp);
	}

	if (ini == end)
		return NULL;

	// Advance one so ini points to the first valid char
	ini++;
	tmp = g_utf8_next_char(tmp);
	*pos = ini;
	gint len = g_utf8_offset_to_pointer(tmp, end) - g_utf8_offset_to_pointer(tmp, ini - 1);
	gchar *word = g_new0(gchar, len + 1);
	g_utf8_strncpy(word, tmp, end-ini+1);

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

gdouble
gebr_get_lower_bound_for_type(TimesType type)
{
	if (type == TIME_MOMENTS_AGO)
		return -100;
	if (type == TIME_HOURS_AGO)
		return 3600;
	if (type == TIME_DAYS_AGO)
		return 86400;
	if (type == TIME_WEEKS_AGO)
		return 86400*7;
	if (type == TIME_MONTHS_AGO)
		return 2678400;
	if (type == TIME_YEARS_AGO)
		return 32140800;

	return 0;
}

gint
gebr_get_number_of_time_controls()
{
	return TIME_N_TYPES;
}

gchar *
gebr_get_control_text_for_type(TimesType type)
{
	if (type == TIME_MOMENTS_AGO)
		return g_strdup(_("Moments ago"));
	if (type == TIME_HOURS_AGO)
		return g_strdup(_("Hours ago"));
	if (type == TIME_DAYS_AGO)
		return g_strdup(_("Days ago"));
	if (type == TIME_WEEKS_AGO)
		return g_strdup(_("Weeks ago"));
	if (type == TIME_MONTHS_AGO)
		return g_strdup(_("Months ago"));
	if (type == TIME_YEARS_AGO)
		return g_strdup(_("Years ago"));

	return NULL;
}

gchar *
gebr_calculate_relative_time (GTimeVal *time1,
                              GTimeVal *time2,
                              TimesType *_type,
                              gdouble *delta)
{
	TimesType type;
	gdouble time_diff = (time2->tv_sec - time1->tv_sec) + (time2->tv_usec - time1->tv_usec)/1000000.0;

	if ( time_diff < 0)
		return NULL;

	if (delta)
		*delta = time_diff;

	if ( time_diff < 3600)
		type = TIME_MOMENTS_AGO;
	else if ( time_diff < 86400)
		type = TIME_HOURS_AGO;
	else if ( time_diff < 86400*7)
		type = TIME_DAYS_AGO;
	else if ( time_diff < 2678400)
		type = TIME_WEEKS_AGO;
	else if ( time_diff < 32140800)
		type = TIME_MONTHS_AGO;
	else
		type = TIME_YEARS_AGO;

	if (_type)
		*_type = type;

	return gebr_get_control_text_for_type(type);
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

gchar *
gebr_compute_diff_iso_times(const gchar *iso_time1, const gchar *iso_time2)
{
	GTimeVal time1, time2, diff_times;
	g_time_val_from_iso8601(iso_time1, &time1);
	g_time_val_from_iso8601(iso_time2, &time2);
	diff_times.tv_sec = time2.tv_sec - time1.tv_sec;
	diff_times.tv_usec = time2.tv_sec - time1.tv_sec;
	return g_time_val_to_iso8601(&diff_times);
}

gint
gebr_compute_diff_clock_to_me(const gchar *iso_time1)
{
	GTimeVal curr_time, time1;
	g_get_current_time(&curr_time);
	g_time_val_from_iso8601(iso_time1, &time1);
	return time1.tv_sec - curr_time.tv_sec;
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
                                    gdouble aggressive)
{
	return MAX((gint)round(total_nprocs * aggressive/5), 1);
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

gchar *
gebr_remove_path_prefix(const gchar *prefix, const gchar *path)
{
	if (!prefix)
		return g_strdup(path);
	if (g_str_has_prefix(path, prefix))
		return g_strdup(path + strlen(prefix));
	else
		return g_strdup(path);
}

gchar *
gebr_relativise_old_home_path(const gchar *path_string)
{
	GString *path = g_string_new(path_string);
	gebr_path_use_home_variable(path);

	if (g_str_has_prefix(path->str, "$HOME"))
		gebr_g_string_replace_first_ref(path, "$HOME", "<HOME>");

	return g_string_free(path, FALSE);
}

gchar *
gebr_relativise_home_path(const gchar *path_string,
                          const gchar *mount_point,
                          const gchar *home)
{
	gchar ***pvector = g_new0(gchar**, 2);
	pvector[0] = g_new0(gchar*, 2);
	pvector[0][0] = g_strdup(home);
	pvector[0][1] = g_strdup("HOME");

	gchar *folder = gebr_relativise_path(path_string, mount_point, pvector);

	gebr_pairstrfreev(pvector);

	return folder;
}

static gchar *
gebr_relativise_path_recursive(const gchar *path,
                               const gchar *mount_point,
                               gchar ***pvector)
{
	g_return_val_if_fail(path != NULL, NULL);

	gchar **dirspath = g_strsplit(path, "/", -1);
	gchar **dirtmp;
	gint max_index = 0, max_size = 0;
	gint j = 0;

	for (int i = 0; pvector[i]; i++) {
		dirtmp = g_strsplit(pvector[i][0], "/" , -1);
		j = 0;
		while (dirspath[j] && dirtmp[j]) {
			if (g_strcmp0(dirspath[j], dirtmp[j]) != 0)
				break;
			j++;
		}

		if (!dirtmp[j]) {
			if (j > max_size) {
				max_size = j;
				max_index = i;
			}
		}

		g_strfreev(dirtmp);
	}

	g_strfreev(dirspath);

	if (!max_size)
		return g_strdup(path);

	GString *rel_path = g_string_new(path);

	if (g_str_has_suffix(pvector[max_index][0], "/"))
		rel_path = g_string_erase(rel_path, 0, (strlen(pvector[max_index][0]) - 1));
	else
		rel_path = g_string_erase(rel_path, 0, strlen(pvector[max_index][0]));

	g_string_prepend (rel_path, ">");
	g_string_prepend (rel_path, pvector[max_index][1]);
	g_string_prepend (rel_path, "<");

	gchar *new_path = gebr_relativise_path_recursive(rel_path->str, mount_point, pvector);

	g_string_free(rel_path, TRUE);

	return new_path;
}

gchar *
gebr_relativise_path(const gchar *path_string,
                     const gchar *mount_point,
		     gchar ***pvector)
{
	gchar *path_str;

	g_return_val_if_fail(path_string != NULL, NULL);

	if (!*path_string)
		return g_strdup("");

	path_str = gebr_resolve_relative_path(path_string, pvector);
	gchar *path = gebr_remove_path_prefix(mount_point, path_str);

	gchar *rel_path = gebr_relativise_path_recursive(path, mount_point, pvector);

	g_free(path);

	return rel_path;
}

gchar *
gebr_resolve_relative_path(const gchar *path,
                            gchar ***pvector)
{
	g_return_val_if_fail(path != NULL, NULL);
	g_return_val_if_fail(pvector != NULL, NULL);
	
	gint i = 0;
	gchar **dirspath = g_strsplit(path, "/", -1);
	GString *str = g_string_new(path);
	gboolean has_rpath = FALSE;

	while (pvector[i] != NULL ) {
		gchar *tmp = g_strdup_printf("<%s>", pvector[i][1]);
		if (!g_strcmp0(dirspath[0], tmp)) {
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
	
	gchar *new_path = gebr_resolve_relative_path(str->str, pvector);

	g_string_free(str, TRUE);

	return new_path;
}

gchar ***
gebr_generate_paths_with_home(const gchar *home)
{
	gchar ***paths = g_new0(gchar**, 2);

	paths[0] = g_new0(gchar*, 2);
	paths[0][0] = g_strdup(home);
	paths[0][1] = g_strdup("HOME");
	paths[1] = NULL;

	return paths;
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

		gchar *resolved = gebr_resolve_relative_path(paths[i][0], paths);
		gchar *escaped;
		if (uri_prefix && strstr(uri_prefix, "sftp"))
			escaped = g_uri_escape_string(resolved + 1, "/", TRUE);
		else
			escaped = g_uri_escape_string(resolved, "/", TRUE);

		g_string_append_printf(buf, "%s%s %s (GeBR)\n",
		                       uri_prefix? uri_prefix : "", escaped, paths[i][1]);
		g_free(resolved);
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
			gchar *resolved = gebr_resolve_relative_path(paths[j][0], paths);
			gchar *escaped = g_uri_escape_string(resolved, "/", TRUE);
			gchar *suffix = g_strdup_printf("%s %s (GeBR)", escaped, paths[j][1]);

			if (g_str_has_suffix(lines[i], suffix))
				has_path = TRUE;

			g_free(resolved);
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

gboolean
gebr_verify_starting_slash (const gchar *string)
{
	gboolean slash = string[0]=='/' ? TRUE : FALSE;
	return slash;
}

gboolean
gebr_validate_path(const gchar *path,
		   gchar ***pvector,
		   gchar **err_msg)
{
	if (!path || !*path)
		return TRUE;

	for (int i = 0; pvector[i]; i++) {
		gchar *tmp = g_strdup_printf("<%s>", pvector[i][1]);
		if (g_str_has_prefix(path, tmp))
			return TRUE;
	}
	if (*path != '/') {
		if (err_msg) {
			if (*path == '<')
				*err_msg = g_markup_escape_text(_("The specified line path does not exist"), -1);
			else
				*err_msg = g_markup_escape_text(_("The path must either start with < or /"), -1);
		}
		return FALSE;
	}
	else
		return TRUE;
}

gint
gebr_strv_indexof(const gchar **strv, const gchar *value)
{
	for (int i = 0; strv[i]; i++)
		if (g_strcmp0(strv[i], value) == 0)
			return i;
	return -1;
}

gchar *
gebr_key_filename(gboolean public) {
	return g_build_filename(g_get_home_dir(), ".ssh", public ? "gebr.key.pub" : "gebr.key", NULL);
}

gboolean
gebr_generate_key()
{
	gchar *path = gebr_key_filename(FALSE);

	if (g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_free(path);
		return TRUE;
	}

	gchar *std_out;
	gchar *std_error;
	gint exit_status;
	GError *error = NULL;
	GString *cmd_line = g_string_new(NULL);

	g_string_printf(cmd_line, "ssh-keygen -b '2048' -t 'rsa' -N '' -f '%s'", path);

	if (!g_spawn_command_line_sync(cmd_line->str, &std_out, &std_error, &exit_status, &error)) {
		g_debug("Erros %s | %s", std_error, std_out);
		g_debug("GError: %s", error->message);
		return FALSE;
	}

	if (error) {
		g_debug("COMMAND OF GENERATE KEY HAS ERROR: %s", error->message);
		g_error_free(error);
	}

	g_debug("Key 'gebr.key' created! - With exit_status: %d", exit_status);

	g_string_free(cmd_line, TRUE);
	g_free(path);
	g_free(std_error);
	g_free(std_out);

	return TRUE;
}

gboolean
gebr_add_remove_ssh_key(gboolean remove)
{
	gchar *path = gebr_key_filename(FALSE);

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_free(path);
		return FALSE;
	}

	gchar *std_out;
	gchar *std_error;
	gint exit_status;
	GError *error = NULL;
	GString *cmd_line = g_string_new(NULL);

	const gchar *option;
	if (remove)
		option = "-d ";
	else
		option = "";

	g_string_printf(cmd_line, "ssh-add %s%s", option, path);

	if (!g_spawn_command_line_sync(cmd_line->str, &std_out, &std_error, &exit_status, &error)) {
		g_debug("Erros %s | %s", std_error, std_out);
		g_debug("GError: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	if (error) {
		g_debug("COMMAND OF ADD/REMOVE KEY HAS ERROR: %s", error->message);
		g_error_free(error);
	}

	if (remove)
		g_debug("Remove key 'gebr.key' - With exit_status: %d", exit_status);
	else
		g_debug("Add key 'gebr.key'- With exit_status: %d", exit_status);

	g_string_free(cmd_line, TRUE);
	g_free(path);
	g_free(std_error);
	g_free(std_out);

	return TRUE;
}

gchar *
gebr_get_user_from_address(const gchar *address)
{
	if (!address || !*address)
		return NULL;

	gchar **at_split = g_strsplit(address, "@", 2);
	gchar *user;

	if (g_strv_length(at_split) == 2)
		user = g_strdup(at_split[0]);
	else
		user = g_strdup(g_get_user_name());

	g_strfreev(at_split);

	return user;
}

gchar *
gebr_get_host_from_address(const gchar *address)
{
	if (!address || !*address)
		return NULL;

	gchar **at_split = g_strsplit(address, "@", -1);
	gchar **space_split;
	gchar *addr_without_user;

	if (at_split[1]) {	//There's @
		space_split = g_strsplit(at_split[1], " ", -1);
		addr_without_user = g_strdup(space_split[0]);
	} else {		//There's no @
		space_split = g_strsplit(at_split[0], " ", -1);
		addr_without_user = g_strdup(space_split[0]);
	}

	g_strfreev(space_split);
	g_strfreev(at_split);
	
	return addr_without_user;
}

gboolean
gebr_verify_address_without_username(const gchar *address)
{
	gchar *new_addr = gebr_get_host_from_address(address);
	gboolean ret;
	if (g_strcmp0(address, new_addr) != 0) {
		ret = FALSE;
	} else {
		ret = TRUE;
	}
	g_free(new_addr);
	return ret;
}

gboolean
gebr_kill_by_port(gint port)
{
	gchar *cmd_line;
	gboolean ret = FALSE;
	cmd_line = g_strdup_printf("fuser -sk -15 %d/tcp", port);

	if (g_spawn_command_line_sync(cmd_line, NULL, NULL, NULL, NULL))
		ret = TRUE;
	g_free(cmd_line);
	return ret;
}

gchar *
gebr_create_id_with_current_time()
{
	GTimeVal result;

	g_get_current_time(&result);

	return g_strdup_printf("%ld%ld", result.tv_sec, result.tv_usec);
}

GList *
gebr_double_list_to_list(GList *double_list)
{
	GList *single_list = NULL;

	for (GList *i = double_list; i; i = i->next) {
		single_list = g_list_concat(single_list, g_list_copy(i->data));
	
	}
	return single_list;
}

gboolean
gebr_gzfile_get_contents(const gchar *filename,
                         GString *contents,
                         gchar **error)
{
	g_return_val_if_fail(contents != NULL, FALSE);

	gzFile docgz;

	docgz = gzopen(filename, "r");

	if (docgz == NULL) {
		gzclose(docgz);
		return FALSE;
	}

	guint length = 4096;

	gchar buffer[length];
	gint bytes;

	while (1) {
		bytes = gzread(docgz, buffer, length);

		if (bytes < length) {
			if (gzeof(docgz)) {
				g_string_append_len(contents, buffer, bytes);
				break;
			}
			else {
				gint err;
				const gchar *error_string;
				error_string = gzerror(docgz, &err);
				if (err) {
					if (error)
						*error = g_strdup(error_string);

					gzclose(docgz);

					return FALSE;
				}
			}
		}
		g_string_append_len(contents, buffer, bytes);
	}

	g_string_append_c(contents, '\0');
	if (error)
		*error = NULL;

	gzclose(docgz);

	return TRUE;
}

gboolean
gebr_callback_true(void)
{
	return TRUE;
}

void
gebr_string_freeall(GString *string)
{
	g_string_free(string, TRUE);
}

const gchar *
gebr_apply_pattern_on_address(const gchar *addr)
{
	const gchar *address;
	if (g_strcmp0(addr, "127.0.0.1") == 0 ||
	    g_strcmp0(addr, "localhost") == 0)
		address = g_get_host_name();
	else
		address = addr;

	return address;
}

GQueue *
gebr_gqueue_push_tail_avoiding_duplicates(GQueue *queue,
                                          const gchar *data)
{
	if (!g_queue_find_custom(queue, data, (GCompareFunc) g_strcmp0) && *data)
		g_queue_push_tail(queue, g_strdup(data));

	return queue;
}

const gchar*
gebr_paths_get_value_by_key(const gchar ***paths,
                            const gchar *key)
{
	for (gint i = 0; paths && paths[i]; i++) {
		if (g_strcmp0(paths[i][1], key) == 0) {
			return paths[i][0];
		}
	}
	return NULL;
}
