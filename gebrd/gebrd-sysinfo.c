#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gebrd-sysinfo.h"

struct _GebrdCpuInfo {
	guint length;
	GList *cores;
};

GebrdCpuInfo *gebrd_cpu_info_new (void)
{
	FILE *fp;
	char *line;
	size_t length;
	ssize_t read;
	GHashTable *tb;
	gboolean create = TRUE;
	GebrdCpuInfo *cpuinfo;

	fp = fopen("/proc/cpuinfo", "r");
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

	return cpuinfo;
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

GebrdMemInfo *gebrd_mem_info_new (void)
{
	FILE *fp;
	char *line;
	size_t length;
	ssize_t read;
	GebrdMemInfo *mem;

	fp = fopen("/proc/meminfo", "r");
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

	return mem;
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
