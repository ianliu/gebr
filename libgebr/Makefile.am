# libgebr - GeBR Library
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
include $(top_srcdir)/Makefile.decl

if ENABLE_TESTS
TESTDIR = tests
endif
if ENABLE_GUI
GUIDIR=gui
endif
SUBDIRS = . json geoxml comm ${GUIDIR} po ${TESTDIR}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr.pc

touch:
	if [ -d $(top_srcdir)/.hg ]; then touch version.c -r $(top_srcdir)/.hg; fi
date.c: touch

marshalers.h: marshalers.list
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --header >marshalers.h
marshalers.c: marshalers.h marshalers.list
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --body >marshalers.c
CLEANFILES = marshalers.c marshalers.h

lib_LTLIBRARIES = libgebr.la
libgebr_la_SOURCES = 	\
	marshalers.c	\
	date.c		\
	gebr-tar.c	\
	libgebr.c	\
	log.c		\
	utils.c		\
	validate.c	\
	version.c	\
	$(NULL)

libgebrincludedir = $(includedir)/libgebr
libgebrinclude_HEADERS =\
	date.h		\
	defines.h	\
	gebr-tar.h	\
	libgebr.h	\
	log.h		\
	utils.h		\
	validate.h	\
	version.h	\
	$(NULL)

noinst_HEADERS = defines.h marshalers.h

EXTRA_DIST = ${pkgconfig_DATA} $(noinst_HEADERS) config.h.in marshalers.list
DISTCLEANFILES = config.h

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC

AM_CPPFLAGS = $(GLIB_CFLAGS)	\
	      -DNANOVERSION=\"$(NANOVERSION)\"\
	      @DEBUG_CFLAGS@

libgebr_la_LIBADD = -lutil $(INTLLIBS) $(GLIB_LIBS)
libgebr_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@

distcheck-hook: all

.PHONY: touch
