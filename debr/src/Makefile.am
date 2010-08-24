# Debr - GeBR Designer
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

NULL=

bin_PROGRAMS = debr

debrgladedir = $(datadir)/debr/glade/
debrglade_DATA = menu-dialog-save-ui.glade

debrscriptsdir = $(datadir)/libgebr/ckeditor/
debrscripts_DATA = debr-help-edit-script.js

noinst_LIBRARIES = libdebr.a
libdebr_a_SOURCES = \
	callbacks.c \
	categoryedit.c \
	debr.c \
	enumoptionedit.c \
	help.c \
	interface.c \
	menu.c \
	parameter.c \
	parametergroup.c \
	preferences.c \
	program.c \
	validate.c \
	debr-help-edit-widget.c \
	$(NULL)

debr_SOURCES = main.c
noinst_HEADERS = \
	callbacks.h \
	categoryedit.h \
	debr.h \
	enumoptionedit.h \
	help.h \
	interface.h \
	menu.h \
	parameter.h \
	parametergroup.h \
	preferences.h \
	program.h \
	validate.h \
	debr-help-edit-widget.h \
	$(NULL)

EXTRA_DIST = $(noinst_HEADERS) $(debrglade_DATA) $(debrscripts_DATA)

INCLUDES = 
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ \
	$(GEBR_CFLAGS) $(GEBR_GEOXML_CFLAGS) $(GEBR_GUI_CFLAGS)
debr_LDADD = libdebr.a $(INTLLIBS) \
	$(GEBR_LIBS) $(GEBR_GEOXML_LIBS) $(GEBR_GUI_LIBS)
