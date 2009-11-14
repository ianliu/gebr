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

SUBDIRS = . @CONFIG_SUBDIRS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr.pc

lib_LTLIBRARIES = libgebr.la
libgebr_la_SOURCES = libgebr.c \
	date.c log.c utils.c

libgebrincludedir = $(includedir)/libgebr
libgebrinclude_HEADERS = libgebr.h \
	intl.h date.h log.h utils.h
noinst_HEADERS = defines.h

EXTRADIST = ${pkgconfig_DATA} $(noinst_HEADERS)

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GLIB_CFLAGS)
libgebr_la_LIBADD = -lutil $(INTLLIBS) $(GLIB_LIBS)
libgebr_la_LDFLAGS = -version-info @GEBR_GEOXML_VERSION_INFO@
