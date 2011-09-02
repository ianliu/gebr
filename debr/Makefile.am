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

SUBDIRS = . data po tests

bin_PROGRAMS = debr

debrgladedir = $(datadir)/debr/glade/
debrglade_DATA = menu-dialog-save-ui.glade

noinst_LIBRARIES = libdebr.a
libdebr_a_SOURCES = \
	debr-callbacks.c \
	debr-callbacks.h \
	debr-categoryedit.c \
	debr-categoryedit.h \
	debr-enumoptionedit.c \
	debr-enumoptionedit.h \
	debr-gettext.h	\
	debr-help-edit-widget.c \
	debr-help-edit-widget.h \
	debr-help.c \
	debr-help.h \
	debr-interface.c \
	debr-interface.h \
	debr-menu.c \
	debr-menu.h \
	debr-parameter.c \
	debr-parameter.h \
	debr-parametergroup.c \
	debr-parametergroup.h \
	debr-preferences.c \
	debr-preferences.h \
	debr-program.c \
	debr-program.h \
	debr-validate.c \
	debr-validate.h \
	debr.c \
	debr.h \
	$(NULL)

debr_SOURCES = debr-main.c

EXTRA_DIST = $(noinst_HEADERS) $(debrglade_DATA) $(debrscripts_DATA)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	$(NULL)

debr_LDADD =			\
	$(GUI_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	libdebr.a		\
	$(NULL)
	
