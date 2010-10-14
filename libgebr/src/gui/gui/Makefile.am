# libgebr - GeBR Library # Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

marshalers.h: marshalers.list
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --header >marshalers.h

marshalers.c: marshalers.h
	glib-genmarshal --prefix=_gebr_gui_marshal marshalers.list --body >marshalers.c

lib_LTLIBRARIES = libgebr_gui.la
libgebr_gui_la_SOURCES = 		\
	marshalers.c 			\
	sexy-icon-entry.c 		\
	gebr-gui-about.c 			\
	gebr-gui-utils.c 			\
	gebr-gui-pixmaps.c 			\
	gebr-gui-icons.c 			\
	gebr-gui-file-entry.c			\
	gebr-gui-enhanced-entry.c		\
	gebr-gui-sequence-edit.c		\
	gebr-gui-value-sequence-edit.c		\
	gebr-gui-program-edit.c			\
	gebr-gui-parameter.c			\
	gebr-gui-directory-chooser.c		\
	gebr-gui-save-dialog.c		\
	gebr-gui-help-edit-widget.c	\
	gebr-gui-help-edit-window.c	\
	gebr-gui-html-viewer-widget.c	\
	gebr-gui-html-viewer-window.c	\
	gebr-gui-js.c

libgebr_guisubincludedir = $(includedir)/libgebr/gui
libgebr_guisubinclude_HEADERS = 	\
	marshalers.h			\
	sexy-icon-entry.h		\
	gebr-gui-about.h				\
	gebr-gui-utils.h				\
	gebr-gui-pixmaps.h			\
	gebr-gui-icons.h				\
	gebr-gui-file-entry.h			\
	gebr-gui-enhanced-entry.h		\
	gebr-gui-sequence-edit.h		\
	gebr-gui-value-sequence-edit.h		\
	gebr-gui-program-edit.h			\
	gebr-gui-parameter.h			\
	gebr-gui-directory-chooser.h		\
	gebr-gui-save-dialog.h		\
	gebr-gui-help-edit-widget.h	\
	gebr-gui-help-edit-window.h	\
	gebr-gui-html-viewer-widget.h	\
	gebr-gui-html-viewer-window.h	\
	gebr-gui-js.h

noinst_HEADERS = marshalers.h
EXTRA_DIST = ${noinst_HEADERS} marshalers.list 

INCLUDES = -I$(srcdir)/../.. -I$(srcdir)/../../geoxml
AM_CFLAGS = -Wall -std=gnu99 -fPIC @DEBUG_CFLAGS@ $(GTK_CFLAGS) $(GTHREAD_CFLAGS) $(WEBKIT_CFLAGS)
libgebr_gui_la_LIBADD = ../../libgebr.la ../../geoxml/geoxml/libgebr_geoxml.la \
	$(INTLLIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(WEBKIT_LIBS) 
libgebr_gui_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@

CLEANFILES = marshalers.c marshalers.h
