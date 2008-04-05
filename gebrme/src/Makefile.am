# G�BR ME - G�BR Menu Editor
# Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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

bin_PROGRAMS = gebrme

gebrme_SOURCES = \
	main.c \
	interface.c \
	callbacks.c \
	gebrme.c \
	menu.c \
	summary.c \
	category.c \
	program.c \
	parameters.c \
	groupparameters.c \
	parameter.c \
	enumoptionedit.c \
	help.c \
	preferences.c \
	support.c
noinst_HEADERS = \
	interface.h \
	callbacks.h \
	gebrme.h \
	menu.h \
	summary.h \
	category.h \
	program.h \
	parameters.h \
	groupparameters.h \
	parameter.h \
	enumoptionedit.h \
	help.h \
	preferences.h \
	support.h
EXTRA_DIST = $(noinst_HEADERS)

LIBGEBR_CFLAGS = $(LIBGEBR_GEOXML_CFLAGS) $(LIBGEBR_GUI_CFLAGS) $(LIBGEBR_MISC_CFLAGS)
LIBGEBR_LIBS = $(LIBGEBR_GEOXML_LIBS) $(LIBGEBR_GUI_LIBS) $(LIBGEBR_MISC_LIBS)
INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"
AM_CFLAGS = -Wall -std=gnu99 @DEBUG_CFLAGS@ $(GTK_CFLAGS) ${LIBGEBR_CFLAGS}
gebrme_LDADD = $(INTLLIBS) $(GTK_LIBS) $(LIBGEBR_LIBS)
