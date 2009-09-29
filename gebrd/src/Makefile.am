# GeBR Daemon - Process and control execution of flows
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

bin_PROGRAMS = gebrd
gebrd_SOURCES = \
	main.c \
	gebrd.c \
	server.c \
	client.c \
	job.c
noinst_HEADERS = \
	gebrd.h \
	server.h \
	client.h \
	job.h \
	macros.h
EXTRA_DIST = $(noinst_HEADERS)

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS) \
	$(LIBGEBR_CFLAGS) $(LIBGEBR_GEOXML_CFLAGS) $(LIBGEBR_COMM_CFLAGS)
gebrd_LDADD = $(INTLLIBS) $(GLIB_LIBS) \
	$(LIBGEBR_LIBS) $(LIBGEBR_GEOXML_LIBS) $(LIBGEBR_COMM_LIBS)
