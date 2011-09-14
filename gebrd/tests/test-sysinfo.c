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

#include "../gebrd-sysinfo.h"

static void
test_cpuinfo_get(void)
{
	GebrdCpuInfo *cpu = gebrd_cpu_info_new_from_file(TEST_DIR"/cpuinfo");
	g_assert_cmpstr(gebrd_cpu_info_get(cpu, 0, "vendor_id"), ==, "GenuineIntel");
	g_assert_cmpstr(gebrd_cpu_info_get(cpu, 1, "siblings"), ==, "4");
	gebrd_cpu_info_free(cpu);
}

static void
test_cpuinfo_n_procs(void)
{
	GebrdCpuInfo *cpu = gebrd_cpu_info_new_from_file(TEST_DIR"/cpuinfo");
	g_assert_cmpint(gebrd_cpu_info_n_procs(cpu), ==, 4);
	gebrd_cpu_info_free(cpu);
}

static void
test_meminfo_get(void)
{
	GebrdMemInfo *mem = gebrd_mem_info_new_from_file(TEST_DIR"/meminfo");
	g_assert_cmpstr(gebrd_mem_info_get(mem, "Cached"), ==, "927372 kB");
	gebrd_mem_info_free(mem);
}


int main(int argc, char * argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/sysinfo/cpu_info_get", test_cpuinfo_get);
	g_test_add_func("/gebrd/sysinfo/cpu_info_n_procs", test_cpuinfo_n_procs);
	g_test_add_func("/gebrd/sysinfo/mem_info_get", test_meminfo_get);

	return g_test_run();
}
