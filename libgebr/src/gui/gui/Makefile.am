# libgebr - GÍBR Library
# Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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

lib_LTLIBRARIES = libgebr_gui.la
libgebr_gui_la_SOURCES = \
	about.c utils.c gtkfileentry.c pixmaps.c

libgebr_guisubincludedir = $(includedir)/libgebr/gui
libgebr_guisubinclude_HEADERS = \
	about.h utils.h gtkfileentry.h pixmaps.h
noinst_HEADERS = \
	support.h
EXTRA_DIST = ${noinst_HEADERS}

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DPIXMAPS_DIR=\"$(datadir)/libgebr/pixmaps/\"
AM_CFLAGS = -Wall $(GTK_CFLAGS)
libgebr_gui_la_LIBADD = $(INTLLIBS) $(GTK_LIBS)
libgebr_gui_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@
