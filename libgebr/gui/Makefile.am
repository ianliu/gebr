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
TESTSDIR = tests
endif
SUBDIRS = . pixmaps data icons ckeditor ${TESTSDIR}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gebr-gui.pc

EXTRA_DIST = gebr-gui.pc

libgebr_guiincludedir = $(includedir)/libgebr
libgebr_guiinclude_HEADERS = gui.h

lib_LTLIBRARIES = libgebr_gui.la
libgebr_gui_la_SOURCES = 		\
	gebr-gui-about.c 		\
	gebr-gui-enhanced-entry.c	\
	gebr-gui-file-entry.c		\
	gebr-gui-help-edit-widget.c	\
	gebr-gui-help-edit-window.c	\
	gebr-gui-html-viewer-widget.c	\
	gebr-gui-html-viewer-window.c	\
	gebr-gui-icons.c 		\
	gebr-gui-js.c			\
	gebr-gui-parameter.c		\
	gebr-gui-pixmaps.c 		\
	gebr-gui-program-edit.c		\
	gebr-gui-save-dialog.c		\
	gebr-gui-sequence-edit.c	\
	gebr-gui-tool-button.c		\
	gebr-gui-utils.c 		\
	gebr-gui-validatable-widget.c	\
	gebr-gui-value-sequence-edit.c	\
	sexy-icon-entry.c 		\
	$(NULL)

libgebr_guisubincludedir = $(includedir)/libgebr/gui
libgebr_guisubinclude_HEADERS = 	\
	gebr-gui-about.h		\
	gebr-gui-enhanced-entry.h	\
	gebr-gui-file-entry.h		\
	gebr-gui-help-edit-widget.h	\
	gebr-gui-help-edit-window.h	\
	gebr-gui-html-viewer-widget.h	\
	gebr-gui-html-viewer-window.h	\
	gebr-gui-icons.h		\
	gebr-gui-js.h			\
	gebr-gui-parameter.h		\
	gebr-gui-pixmaps.h		\
	gebr-gui-program-edit.h		\
	gebr-gui-save-dialog.h		\
	gebr-gui-sequence-edit.h	\
	gebr-gui-tool-button.h		\
	gebr-gui-utils.h		\
	gebr-gui-validatable-widget.h	\
	gebr-gui-value-sequence-edit.h	\
	sexy-icon-entry.h		\
	$(NULL)

AM_CPPFLAGS =				\
	$(GUI_CFLAGS)			\
	$(GDOME2_CFLAGS)		\
	$(GEBR_CFLAGS)			\
	$(GEBR_GEOXML_CFLAGS)		\
	@DEBUG_CFLAGS@			\
	$(NULL)

libgebr_gui_la_LIBADD =			\
	$(GUI_LIBS)			\
	$(GDOME2_LIBS)			\
	$(GEBR_LIBS)			\
	$(GEBR_GEOXML_LIBS)		\
	$(NULL)

AM_CFLAGS = $(COMMON_CFLAGS) -fPIC
libgebr_gui_la_LDFLAGS = -version-info @GEBR_VERSION_INFO@
