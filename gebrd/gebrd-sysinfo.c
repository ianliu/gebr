#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>

#include "gebrd-sysinfo.h"

struct _GebrdCpuInfo {
	guint length;
	GList *cores;
};

GebrdCpuInfo *
gebrd_cpu_info_new_from_file (const gchar *file)
{
	FILE *fp;
	char *line;
	size_t length;
	ssize_t read;
	GHashTable *tb = NULL;
	gboolean create = TRUE;
	GebrdCpuInfo *cpuinfo;

	fp = fopen(file, "r");
	if (!fp)
		return NULL;

	cpuinfo = g_new(GebrdCpuInfo, 1);
	cpuinfo->length = 0;
	cpuinfo->cores = NULL;

	line = NULL;
	read = getline(&line, &length, fp);
	for (; read != -1; read = getline(&line, &length, fp)) {
		char *prop;
		char *val;

		if (create) {
			tb = g_hash_table_new_full(g_str_hash,
						   g_str_equal,
						   (GDestroyNotify)g_free,
						   (GDestroyNotify)g_free);
			cpuinfo->cores = g_list_prepend(cpuinfo->cores, tb);
			cpuinfo->length++;
		}

		create = (line[0] == '\n') ? TRUE:FALSE;

		prop = line;
		val = strchr(line, ':');
		if (!val) {
			g_free (line);
			line = NULL;
			continue;
		}
		val[0] = '\0';
		val++;
		g_strstrip (prop);
		g_strstrip (val);
		g_hash_table_insert (tb, g_strdup (prop), g_strdup (val));
		g_free (line);
		line = NULL;
	}

	fclose(fp);

	return cpuinfo;
}

GebrdCpuInfo *
gebrd_cpu_info_new (void)
{
	return gebrd_cpu_info_new_from_file("/proc/cpuinfo");
}

const gchar *gebrd_cpu_info_get (GebrdCpuInfo *self,
				 guint proc_id,
				 const gchar *prop)
{
	g_return_val_if_fail (proc_id >= 0, NULL);
	g_return_val_if_fail (proc_id < self->length, NULL);

	const gchar *value;
	GHashTable *tb = g_list_nth_data (self->cores, proc_id);
	value = g_hash_table_lookup (tb, prop);

	return value;
}

const gchar *
gebrd_cpu_info_get_clock(GebrdCpuInfo *self,
                         guint proc_id,
                         const gchar *prop)
{
	const gchar *filename = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
	if (g_access(filename, R_OK)) {
		gchar *output;
		gchar *cmd = g_strdup_printf("cat %s", filename);

		g_spawn_command_line_sync(cmd, &output, NULL, NULL, NULL);

		g_free(cmd);

		if (output && *output) {
			gdouble clock = atoi(output)/1000;
			g_free(output);
			return g_strdup_printf("%f", clock);
		}
	}

	return gebrd_cpu_info_get(self, 0, "cpu MHz");
}

guint gebrd_cpu_info_n_procs (GebrdCpuInfo *self)
{
	return self->length;
}

void gebrd_cpu_info_free (GebrdCpuInfo *self)
{
	g_list_foreach (self->cores, (GFunc) g_hash_table_unref, NULL);
	g_list_free (self->cores);
	g_free (self);
}

struct _GebrdMemInfo {
	GHashTable *props;
};

GebrdMemInfo *
gebrd_mem_info_new_from_file (const gchar *file)
{
	FILE *fp;
	char *line;
	size_t length;
	ssize_t read;
	GebrdMemInfo *mem;

	fp = fopen(file, "r");
	if (!fp)
		return NULL;

	mem = g_new(GebrdMemInfo, 1);
	mem->props = g_hash_table_new_full(g_str_hash,
					   g_str_equal,
					   (GDestroyNotify)g_free,
					   (GDestroyNotify)g_free);

	line = NULL;
	read = getline(&line, &length, fp);
	for (; read != -1; read = getline(&line, &length, fp)) {
		char *prop;
		char *val;

		prop = line;
		val = strchr(line, ':');
		if (!val) {
			g_free (line);
			line = NULL;
			continue;
		}
		val[0] = '\0';
		val++;
		g_strstrip (prop);
		g_strstrip (val);
		g_hash_table_insert (mem->props, g_strdup (prop), g_strdup (val));
		g_free (line);
		line = NULL;
	}

	fclose(fp);

	return mem;
}

GebrdMemInfo *
gebrd_mem_info_new(void)
{
	return gebrd_mem_info_new_from_file("/proc/meminfo");
}

const gchar *gebrd_mem_info_get (GebrdMemInfo *self,
				 const gchar *prop)
{
	return g_hash_table_lookup (self->props, prop);
}

void gebrd_mem_info_free (GebrdMemInfo *self)
{
	g_hash_table_unref (self->props);
	g_free (self);
}
