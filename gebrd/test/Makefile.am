# GêBR Daemon - Process and control execution of flows
# Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

noinst_PROGRAMS = gebrd-test
gebrd_test_SOURCES =	main.c \
			fakeclient.c
noinst_HEADERS =	fakeclient.h
EXTRA_DIST = $(noinst_HEADERS)

INCLUDES =
AM_CFLAGS = -Wall $(GLIB_CFLAGS) ${LIBGEBR_MISC_CFLAGS} ${LIBGEBR_GEOXML_CFLAGS}
gebrd_test_LDADD = $(GOBJECT_LIBS) ${LIBGEBR_MISC_LIBS} ${LIBGEBR_GEOXML_LIBS}
