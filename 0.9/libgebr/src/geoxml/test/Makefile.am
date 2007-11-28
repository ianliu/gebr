# libgeoxml - An interface to describe seismic software in XML
# Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

check_PROGRAMS = test
test_SOURCES = test.c

#FIXME with make distcheck
# TESTS = test
TESTS_ENVIRONMENT = top_builddir=$(top_builddir)
EXTRA_DIST =		\
	sufrac.mnu	\
	sugain.mnu	\
	suximage.mnu

INCLUDES = -I..
AM_CFLAGS = -Wall $(GDOME2_CFLAGS)
test_LDADD = ../geoxml/libgebr_geoxml.la
