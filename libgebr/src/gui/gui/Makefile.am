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

marshalers.h:
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --header >marshalers.h

marshalers.c: marshalers.h
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --body >marshalers.c

lib_LTLIBRARIES = libgebr_gui.la
libgebr_gui_la_SOURCES = 		\
	marshalers.c 			\
	sexy-icon-entry.c 		\
	about.c 			\
	utils.c 			\
	pixmaps.c 			\
	icons.c 			\
	help.c 				\
	gtkfileentry.c			\
	gtkenhancedentry.c		\
	gtksequenceedit.c		\
	valuesequenceedit.c		\
	programedit.c			\
	parameter.c			\
	gtkdirectorychooser.c		\
	gebr-gui-save-dialog.c		\
	gebr-gui-help-edit.c		\
	gebr-gui-html-viewer-widget.c	\
	gebr-gui-html-viewer-window.c	\
	gebr-js.c

libgebr_guisubincludedir = $(includedir)/libgebr/gui
libgebr_guisubinclude_HEADERS = 	\
	marshalers.h			\
	sexy-icon-entry.h		\
	about.h				\
	utils.h				\
	pixmaps.h			\
	icons.h				\
	help.h				\
	gtkfileentry.h			\
	gtkenhancedentry.h		\
	gtksequenceedit.h		\
	valuesequenceedit.h		\
	programedit.h			\
	parameter.h			\
	gtkdirectorychooser.h		\
	gebr-gui-save-dialog.h		\
	gebr-gui-html-viewer-widget.h	\
	gebr-gui-html-viewer-window.h	\
	gebr-gui-help-edit.h		\
	gebr-js.h

noinst_HEADERS = marshalers.h
EXTRA_DIST = ${noinst_HEADERS} marshalers.list 

INCLUDES = -I$(srcdir)/../.. -I$(srcdir)/../../geoxml \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DPIXMAPS_DIR=\"$(datadir)/libgebr/pixmaps/\" \
	-DICONS_DIR=\"$(datadir)/libgebr/icons/\" \
	-DCKEDITOR_DIR=\"$(datadir)/libgebr/ckeditor/\"
AM_CFLAGS = -Wall -std=gnu99 -fPIC @DEBUG_CFLAGS@ $(GTK_CFLAGS) $(GTHREAD_CFLAGS) $(WEBKIT_CFLAGS)
libgebr_gui_la_LIBADD = ../../libgebr.la ../../geoxml/geoxml/libgebr_geoxml.la \
	$(INTLLIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(WEBKIT_LIBS) 
libgebr_gui_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@
