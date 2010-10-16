# GeBR - An environment for seismic processing.
# Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.59])

m4_define(gebr_major_version,  0)
m4_define(gebr_minor_version,  9)
m4_define(gebr_micro_version, 18)
m4_define(gebr_version, gebr_major_version.gebr_minor_version.gebr_micro_version)

AC_INIT([gebr], [gebr_version], [http://code.google.com/p/gebr/issues/list])
AM_INIT_AUTOMAKE

m4_include(configure.ac.decl)

AC_CONFIG_SUBDIRS([libgebr
		   gebrd
		   gebr
		   debr])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
