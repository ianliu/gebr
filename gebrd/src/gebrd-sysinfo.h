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

#ifndef __GEBRD_SYSINFO_H__
#define __GEBRD_SYSINFO_H__

typedef struct _GebrdCpuInfo GebrdCpuInfo;

/**
 * gebrd_cpu_info_new:
 *
 * Returns: a new GebrdCpuInfo object. Free with gebrd_cpu_info_free().
 */
GebrdCpuInfo *gebrd_cpu_info_new (void);

/**
 * gebrd_cpu_info_get:
 * @self: A #GebrdCpuInfo object created with gebrd_cpu_info_new()
 * @proc_id: Which processor to get the value from
 * @prop: A property name, as seen in `cat /proc/cpuinfo'
 *
 * Returns: the value for @prop or %NULL if @prop was not found.
 */
const gchar *gebrd_cpu_info_get (GebrdCpuInfo *self,
				 guint proc_id,
				 const gchar *prop);

/**
 * gebrd_cpu_info_n_procs:
 * @self: A #GebrdCpuInfo object created with gebrd_cpu_info_new()
 *
 * Returns: the number of processors returned by `cat /proc/cpuinfo'
 */
guint gebrd_cpu_info_n_procs (GebrdCpuInfo *self);

/**
 * gebrd_cpu_info_free:
 *
 * Frees the data structure pointed by @self. Note that the strings returned by
 * gebrd_cpu_info_get() are also freed.
 */
void gebrd_cpu_info_free (GebrdCpuInfo *self);

#endif /* __GEBRD_SYSINFO_H__ */
