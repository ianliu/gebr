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

marshalers.c:
	glib-genmarshal --prefix=_libgebr_marshal marshalers.list --body >marshalers.c
	glib-genmarshal --prefix=_libgebr_marshal marshalers.list --header >marshalers.h

lib_LTLIBRARIES = libgebr_gui.la
libgebr_gui_la_SOURCES = \
	marshalers.c sexy-icon-entry.c \
	about.c utils.c pixmaps.c icons.c help.c \
	gtkfileentry.c gtkenhancedentry.c \
	gtksequenceedit.c valuesequenceedit.c \
	programedit.c parameter.c gtkdirectorychooser.c

libgebr_guisubincludedir = $(includedir)/libgebr/gui
libgebr_guisubinclude_HEADERS = \
	about.h utils.h pixmaps.h icons.h help.h \
	gtkfileentry.h gtkenhancedentry.h \
	gtksequenceedit.h valuesequenceedit.h \
	programedit.h parameter.h gtkdirectorychooser.h
noinst_HEADERS = marshalers.h sexy-icon-entry.h
EXTRA_DIST = ${noinst_HEADERS} marshalers.list 

INCLUDES = -I../.. -I../../geoxml $(WEBKIT_ENABLED) \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DPIXMAPS_DIR=\"$(datadir)/libgebr/pixmaps/\" \
	-DICONS_DIR=\"$(datadir)/libgebr/icons/\"
AM_CFLAGS = -Wall -std=gnu99 -fPIC @DEBUG_CFLAGS@ $(GTK_CFLAGS) $(GTHREAD_CFLAGS) $(WEBKIT_CFLAGS)
libgebr_gui_la_LIBADD = $(INTLLIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(WEBKIT_LIBS) \
	../../libgebr.la \
	../../geoxml/geoxml/libgebr_geoxml.la
libgebr_gui_la_LDFLAGS = -version-info @GEOXML_VERSION_INFO@
