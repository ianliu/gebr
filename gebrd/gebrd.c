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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/json/json-glib.h>
#include <libgebr/comm/gebr-comm-protocol.h>

#include "gebrd.h"
#include "gebrd-server.h"
#include "gebrd-client.h"
#include "gebrd-sysinfo.h"

#define GEBRD_CONF_FILE "/etc/gebr/gebrd.conf"

#define MAX_NICE 19
#define MIN_NICE 0
#define DEFAULT_NPROCS 1

GebrdApp *gebrd = NULL;

/* GOBJECT STUFF */
enum {
	PROP_0,
	LAST_PROPERTY
};

enum {
	LAST_SIGNAL
};

G_DEFINE_TYPE(GebrdApp, gebrd_app, G_TYPE_OBJECT)

static void gebrd_app_init(GebrdApp * self)
{
	self->user_data_filename = g_string_new(NULL);
	g_string_printf(self->user_data_filename, "%s/.gebr/gebrd/fs_name.json", g_get_home_dir());
	self->user = GEBRD_USER(json_gobject_from_file(GEBRD_USER_TYPE, self->user_data_filename->str));
	if (self->user == NULL)
		self->user = GEBRD_USER(g_object_new(GEBRD_USER_TYPE, NULL));

	self->run_filename = g_string_new(NULL);
	self->fs_lock = g_string_new(NULL);

	gethostname(self->hostname, 255);
	g_random_set_seed((guint32) time(NULL));
	self->flow = NULL;
	self->line = NULL;
	self->proj = NULL;
	self->validator = NULL;

	GebrdCpuInfo *cpu = gebrd_cpu_info_new();
	self->nprocs = gebrd_cpu_info_n_procs(cpu);
	gebrd_cpu_info_free(cpu);
}

static void gebrd_app_finalize(GObject * object)
{
	GebrdApp *self = (GebrdApp *) object;

	g_string_free(self->user_data_filename, TRUE);
	g_string_free(self->run_filename, TRUE);
	g_string_free(self->fs_lock, TRUE);

	if (self->validator)
		gebr_validator_free(self->validator);

	gebr_geoxml_document_free(gebrd->flow);
	gebr_geoxml_document_free(gebrd->line);
	gebr_geoxml_document_free(gebrd->proj);

	G_OBJECT_CLASS(gebrd_app_parent_class)->finalize(object);
}
static void
gebrd_app_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	//GebrdApp *self = (GebrdApp *) object;
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void
gebrd_app_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	//GebrdApp *self = (GebrdApp *) object;
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void gebrd_app_class_init(GebrdAppClass * klass)
{ 
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebrd_app_finalize;
	
	/* properties */
	//GParamSpec *spec;
	gobject_class->set_property = gebrd_app_set_property;
	gobject_class->get_property = gebrd_app_get_property;
}

GebrdApp *gebrd_app_new(void)
{
	return GEBRD_APP(g_object_new(GEBRD_APP_TYPE, NULL));
}

/*
 * \internal
 * References: 
 * http://stackoverflow.com/questions/899038/getting-the-highest-allocated-file-descriptor
 * http://anoncvs.mindrot.org/index.cgi/openssh/openbsd-compat/bsd-closefrom.c?view=markup
 * http://git.gnome.org/browse/glib/tree/gio/libasyncns/asyncns.c?h=2.21.0#n205
 */
static void close_open_fds(int lowfd, int highfd)
{
//TODO: check for this in configure.ac
//#if defined(HAVE_DIRFD) && defined(HAVE_PROC_PID)
	long fd = lowfd;
#if 1
	char fdpath[PATH_MAX], *endp;
	struct dirent *dent;
	DIR *dirp;
	int len;

	/* Check for a /proc/$$/fd directory. */
	len = snprintf(fdpath, sizeof(fdpath), "/proc/%ld/fd", (long)getpid());
	if (len > 0 && (size_t)len <= sizeof(fdpath) && (dirp = opendir(fdpath))) {
		while (fd <= highfd && (dent = readdir(dirp)) != NULL) {
			fd = strtol(dent->d_name, &endp, 10);
			if (dent->d_name != endp && *endp == '\0' &&
			    fd >= 0 && fd < INT_MAX && fd >= lowfd && fd != dirfd(dirp))
				(void) close((int) fd);
		}
		(void) closedir(dirp);
	}
#else
	long maxfd;
	/*
	 * 	 Fall back on sysconf() or getdtablesize().  We avoid checking
	 * 	 resource limits since it is possible to open a file
	 * 	 descriptor * and then drop the rlimit such that it is below the
	 * 	 open fd.  */
#ifdef HAVE_SYSCONF
	maxfd = sysconf(_SC_OPEN_MAX);
#else
	maxfd = getdtablesize();
#endif /* HAVE_SYSCONF */
	if (maxfd < 0)
		maxfd = OPEN_MAX;
	for (fd = lowfd; fd <= highfd && fd < maxfd; fd++)
		(void) close((int) fd);
#endif /* defined(HAVE_DIRFD) && defined(HAVE_PROC_PID) */
}

void gebrd_init(void)
{
	if (gebrd->options.foreground) {
		if (server_init() == TRUE) {
			gebrd->main_loop = g_main_loop_new(NULL, FALSE);
			g_main_loop_run(gebrd->main_loop);
			g_main_loop_unref(gebrd->main_loop);
		}
	} else {
		if (pipe(gebrd->finished_starting_pipe) == -1)
			g_warning("%s:%d: Error when creating pipe with error code %d",
				  __FILE__, __LINE__, errno);

		if (fork() == 0) {
			/* daemon stuff */
			setsid();
			umask(027);
			close(0);
			close(1);
			close(2);
			int i = open("/dev/null", O_RDWR);	/* open stdin */
			dup(i);	/* stdout */
			dup(i);	/* stderr */
			signal(SIGCHLD, SIG_IGN);
			setpgrp();

			if (server_init()) {
				close_open_fds(0, 3);
				gebrd->main_loop = g_main_loop_new(NULL, FALSE);
				g_main_loop_run(gebrd->main_loop);
				g_main_loop_unref(gebrd->main_loop);
			}
		} else {
			/* wait for server_init sign that it finished */
			gchar buffer[100];
			read(gebrd->finished_starting_pipe[0], buffer, 10);
			fprintf(stdout, "%s", buffer);
		}
	}
}

void gebrd_quit(void)
{
	gebrd_message(GEBR_LOG_END, _("Server quit."));
	g_list_foreach(gebrd->mpi_flavors, (GFunc)gebrd_mpi_config_free, NULL);
	g_list_free(gebrd->mpi_flavors);
	server_quit();
	g_main_loop_quit(gebrd->main_loop);
}

void gebrd_message(GebrLogMessageType type, const gchar * message, ...)
{
	gchar *string;
	const gchar *loglevel;

#ifndef DEBUG
	if (type == GEBR_LOG_DEBUG)
		return;
#endif

	va_list argp;
	va_start(argp, message);
	gchar *tmp = g_strdup_vprintf(message, argp);
	va_end(argp);

	loglevel = g_getenv ("GEBRD_LOG_LEVEL");
	if (g_strcmp0 (loglevel, "verbose") != 0) {
		string = g_strescape (tmp, "\"");
		if (strlen (string) > 120)
			string[120] = '\0';
	} else
		string = g_strdup (tmp);

	gebr_log_add_message(gebrd->log, type, tmp);
	if (gebrd->options.foreground == TRUE) {
		if (type != GEBR_LOG_ERROR)
			fprintf(stdout, "%s\n", string);
		else
			fprintf(stderr, "%s\n", string);
	}

	g_free(tmp);
	g_free(string);
}

guint8 gebrd_get_x11_redirect_display(void)
{
	static guint8 display = 10;

	while (gebr_comm_listen_socket_is_local_port_available(6000 + display) == FALSE) {
		if (display == 255) {
			display = 10;
			return 0;
		}
		display++;
	}

	return display++;
}

GebrCommServerType gebrd_get_server_type(void)
{
	static gboolean got_type = FALSE;
	static GebrCommServerType server_type;

	if (got_type)
		return server_type;

	if (g_find_program_in_path("msub") != NULL &&
	    g_find_program_in_path("mcredctl") != NULL &&
	    g_find_program_in_path("checkjob") != NULL)
		server_type = GEBR_COMM_SERVER_TYPE_MOAB;
	else 
		server_type = GEBR_COMM_SERVER_TYPE_REGULAR;

	got_type = TRUE;
	return server_type;
}

void gebrd_config_load(void)
{
	gchar ** groups;
	gchar * config_path;
	GKeyFile * key_file;
	GError * err1, * err2;

	err1 = err2 = NULL;
	gebrd->mpi_flavors = NULL;
	config_path = g_strdup_printf("%s/.gebr/gebrd/gebrd.conf", g_get_home_dir());
	key_file = g_key_file_new();

	/*
	 * Prints the error message of @error.
	 */
	gboolean key_file_exception(GError ** error, const gchar * path) {
		if (*error == NULL)
			return FALSE;

		if ((*error)->domain == G_FILE_ERROR && (*error)->code == G_FILE_ERROR_NOENT)
			return FALSE;

		g_warning("Error reading %s: %s\n", path, (*error)->message);
		g_error_free(*error);
		*error = NULL;

		return TRUE;
	}

	/*
	 * Loads both configuration files in the same GKeyFile structure.
	 * If both fail to load, prints an error message and exit.
	 */
	g_key_file_load_from_file(key_file, config_path, G_KEY_FILE_NONE, &err1);
	g_key_file_load_from_file(key_file, GEBRD_CONF_FILE, G_KEY_FILE_NONE, &err2);

	if (key_file_exception(&err1, config_path)
	    && key_file_exception(&err2, GEBRD_CONF_FILE))
		goto out;

	/*
	 * Iterate over all groups starting with `mpi-', populating the
	 * GList gebrd->mpi_flavors.
	 */
	groups = g_key_file_get_groups(key_file, NULL);
	for (int i = 0; groups[i]; i++) {
		if (!g_str_has_prefix(groups[i], "mpi-"))
			continue;
		GebrdMpiConfig * mpi_config;
		mpi_config = g_new(GebrdMpiConfig, 1);

		mpi_config->name = g_string_new(groups[i] + 4);
		mpi_config->mpirun = gebr_g_key_file_load_string_key(key_file, groups[i], "mpirun", "mpirun");
		mpi_config->libpath = gebr_g_key_file_load_string_key(key_file, groups[i], "libpath", "");
		mpi_config->binpath = gebr_g_key_file_load_string_key(key_file, groups[i], "binpath", "");
		mpi_config->host = gebr_g_key_file_load_string_key(key_file, groups[i], "host", "");
		mpi_config->init_cmd = gebr_g_key_file_load_string_key(key_file, groups[i], "init_command", "");
		mpi_config->end_cmd = gebr_g_key_file_load_string_key(key_file, groups[i], "end_command", "");

		gebrd->mpi_flavors = g_list_prepend(gebrd->mpi_flavors, mpi_config);
	}

out:
	g_free(config_path);
	g_key_file_free(key_file);
}

void gebrd_user_data_save(void)
{
	if (gebrd == NULL)
		return;

	if (!json_gobject_to_file(G_OBJECT(gebrd->user), gebrd->user_data_filename->str))
		g_warning("Could not save user data");
}

const GebrdMpiConfig * gebrd_get_mpi_config_by_name(const gchar * name)
{
	GList * iter;
	for (iter = gebrd->mpi_flavors; iter; iter = iter->next)
		if (g_strcmp0(((GebrdMpiConfig*)iter->data)->name->str, name) == 0)
			return (GebrdMpiConfig*)iter->data;
	return NULL;
}

void
gebrd_clean_proj_line_dicts(void)
{
	gebr_geoxml_document_free(gebrd->line);
	gebr_geoxml_document_free(gebrd->proj);
	gebrd->line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	gebrd->proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());
}

GebrValidator *gebrd_get_validator(GebrdApp *self)
{
	if (!self->validator)
		self->validator = gebr_validator_new(&self->flow, &self->line, &self->proj);

	return self->validator;
}

gint
gebrd_app_set_heuristic_aggression(GebrdApp *self,
				   gint aggressive)
{
	return (self->nprocs - DEFAULT_NPROCS) * (aggressive-1)/4 + DEFAULT_NPROCS;
}
